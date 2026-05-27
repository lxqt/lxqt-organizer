/*
 * LXQt Organizer - personal information manager for LXQt.
 * Copyright (C) 2026 Basil Crow
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "eventservice.h"

#include "asyncsubmit.h"
#include "eventoccurrences.h"
#include "calendarmover.h"
#include "calendaritemstore.h"
#include "calendarsnapshot.h"
#include "calendareditormapper.h"
#include "calendaritemmutator.h"
#include "calendaritemrepository.h"
#include "calendarvalidation.h"
#include "collectionreloadsubscription.h"
#include "eventdatetime.h"
#include "eventpatcher.h"
#include "incidenceresolver.h"
#include "moveoutcomelog.h"
#include "storagelog.h"
#include "vdirio.h"

#include <QDateTime>
#include <QDebug>

#include <KCalendarCore/Todo>

#include <utility>

namespace {

using WritableCalendarItem = CalendarItemStore::WritableCalendarItem;

template <class... Ts> struct Overloaded : Ts...
{
    using Ts::operator()...;
};

template <class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

EventService::EventSaveResult eventAddFailure(StorageStatus status, const EventFields &event)
{
    EventService::EventSaveResult result;
    result.status = status;
    result.snapshot.ref.collectionId = event.collectionId;
    return result;
}

EventService::EventMoveResult
eventMoveFailure(StorageStatus status, const ItemRef &sourceStorage, const QString &destinationCollectionId)
{
    ItemRef destinationStorage;
    destinationStorage.collectionId = destinationCollectionId;
    EventService::EventMoveResult result;
    result.snapshot.ref = sourceStorage;
    result.move = MoveOutcome::preconditionFailed(status, sourceStorage, destinationStorage);
    return result;
}

CalendarItem calendarItemForEvent(const KCalendarCore::Event::Ptr &event, const QString &collectionId)
{
    KCalendarCore::Event::Ptr storedEvent =
        event.isNull() ? KCalendarCore::Event::Ptr(new KCalendarCore::Event) : CalendarSnapshot::cloneEvent(event);
    if (!storedEvent->created().isValid())
    {
        storedEvent->setCreated(QDateTime::currentDateTimeUtc());
    }

    CalendarItem object;
    object.ref.collectionId = collectionId;
    object.uid = storedEvent->uid();
    object.payload = CalendarSnapshot::eventSnapshot(CalendarSnapshot::cloneEvent(storedEvent));
    return object;
}

KCalendarCore::Event::Ptr createEvent(const EventFields &draft)
{
    KCalendarCore::Event::Ptr event = EventDateTime::createEvent();
    EventPatcher::applyToEvent(event, draft, EventPatcher::editableOptions());
    return event;
}

QString resolvedEventUid(const KCalendarCore::MemoryCalendar::Ptr &calendar,
                         const QString &preferredUid,
                         const QString &fallbackUid)
{
    QString uid = IncidenceResolver::inferLocator(calendar, preferredUid).uid;
    if (uid.isEmpty())
    {
        uid = IncidenceResolver::inferLocator(calendar, fallbackUid).uid;
    }
    return uid;
}

StorageStatus storageStatusForMoveOutcome(const MoveOutcome &move)
{
    return std::visit(Overloaded{[](const UpdateOutcome &update) { return update.status; },
                                 [](const MoveSuccess &) { return StorageStatus::Ok; },
                                 [](const MoveDestinationCreateFailed &failure) { return failure.status; },
                                 [](const MoveSourceRemoveFailed &failure) { return failure.status; },
                                 [](const MoveRollbackFailed &failure) { return failure.sourceStatus; }},
                      move.outcome);
}

} // namespace

struct EventSaveWriteResult
{
    StorageResult<CalendarItem> result;
    bool wroteStorage = false;
};

struct EventService::EventMoveWriteResult
{
    EventMoveResult result;
    bool wroteStorage = false;
};

// Events mutate incidences inside a calendar file, so this service owns the
// CalendarItemStore directly instead of using ItemService's one-snapshot flow.
class EventService::Impl
{
public:
    explicit Impl(const CollectionService &collections, const VdirIo &scheduler)
        : m_collections(collections)
        , m_items(collections, scheduler)
        , m_collectionReloadSubscription(collections, [this](const QSet<QString> &changedCollectionIds) {
            clearCacheForCollections(changedCollectionIds);
        })
    {}

    void clearCache() const { m_items.clearCache(); }
    void clearCacheForCollections(const QSet<QString> &collectionIds) const
    {
        m_items.clearCacheForCollections(collectionIds);
    }
    void retainRepositoriesForCollections(const QList<Collection> &collections) const
    {
        m_items.retainRepositoriesForCollections(collections);
    }
    const CollectionService &collections() const { return m_collections; }
    const VdirIo &scheduler() const { return m_items.vdirIo(); }
    const CalendarItemStore &store() const { return m_items; }

private:
    const CollectionService &m_collections;
    CalendarItemStore m_items;
    CollectionReloadSubscription m_collectionReloadSubscription;
};

EventService::EventService(const CollectionService &collections, const VdirIo &vdirIo)
    : m_impl(std::make_unique<Impl>(collections, vdirIo))
{}

EventService::~EventService() = default;

void EventService::clearCache() const
{
    m_impl->clearCache();
}

void EventService::clearCacheForCollections(const QSet<QString> &collectionIds) const
{
    m_impl->clearCacheForCollections(collectionIds);
}

void EventService::retainRepositoriesForCollections(const QList<Collection> &collections) const
{
    m_impl->retainRepositoriesForCollections(collections);
}

void EventService::notifyItemWritten(const ItemRef &ref) const
{
    m_impl->collections().notifyItemWritten(ref);
}

EventService::EventSaveResult EventService::addEvent(const EventFields &event) const
{
    CalendarItem object = calendarItemForEvent(createEvent(event), event.collectionId);
    EventSaveResult result = m_impl->store().addObject(object);
    if (result.isOk())
    {
        notifyItemWritten(result.snapshot.ref);
    }
    return result;
}

EventService::EventSaveResult EventService::updateEvent(const EventOccurrence &currentOccurrence,
                                                        const EventFields &event) const
{
    const EventMoveWriteResult saveResult =
        moveEventObject(currentOccurrence, event, currentOccurrence.ref.item.collectionId);
    EventSaveResult result;
    result.status = storageStatusForMoveOutcome(saveResult.result.move);
    result.snapshot = saveResult.result.snapshot;
    if (saveResult.wroteStorage && result.isOk())
    {
        notifyItemWritten(currentOccurrence.ref.item);
        notifyItemWritten(result.snapshot.ref);
    }
    return result;
}

EventService::EventMoveResult EventService::moveEvent(const EventOccurrence &currentOccurrence,
                                                      const EventFields &event,
                                                      const QString &destinationCollectionId) const
{
    const EventMoveWriteResult saveResult = moveEventObject(currentOccurrence, event, destinationCollectionId);
    EventMoveResult result = saveResult.result;
    if (saveResult.wroteStorage && result.isOk())
    {
        notifyItemWritten(currentOccurrence.ref.item);
        notifyItemWritten(result.snapshot.ref);
    }
    return result;
}

namespace {

StorageResult<CalendarItem> replaceSingleEventItem(const CalendarItemStore &calendarItems,
                                                   CalendarItem replacement,
                                                   const ItemRef &currentStorage,
                                                   const QString &uid)
{
    StorageResult<CalendarItem> result;
    result.snapshot = replacement;

    WritableCalendarItem item =
        calendarItems.writableItemForUpdate(currentStorage.collectionId, currentStorage.href, currentStorage.etag, uid);
    if (!item.isValid())
    {
        result.status = item.status;
        return result;
    }

    KCalendarCore::MemoryCalendar::Ptr storedCalendar = CalendarSnapshot::mutableCalendarForItem(item.object);
    if (storedCalendar.isNull())
    {
        result.status = StorageStatus::Unsupported;
        return result;
    }
    const KCalendarCore::Event::Ptr replacementEvent =
        CalendarItemMutator::editableSingleEventClone(CalendarSnapshot::calendarForItem(replacement));
    if (replacementEvent.isNull())
    {
        result.status = StorageStatus::Unsupported;
        return result;
    }
    const KCalendarCore::Event::Ptr storedEvent =
        IncidenceResolver::findMasterEvent(storedCalendar, {item.object.uid, std::nullopt});
    if (storedEvent.isNull())
    {
        result.status = StorageStatus::NotFound;
        return result;
    }
    if (!CalendarItemMutator::ensureEventUid(replacementEvent, item.object.uid))
    {
        result.status = StorageStatus::Conflict;
        return result;
    }
    if (!CalendarItemMutator::replaceEventByUid(storedCalendar, item.object.uid, replacementEvent))
    {
        result.status = StorageStatus::IoError;
        return result;
    }

    item.object = CalendarSnapshot::calendarItem(
        item.object.ref, item.object.uid, storedCalendar, CalendarSnapshot::payloadShape(item.object));
    return calendarItems.replaceWritableItem(item);
}

EventSaveWriteResult updateSingleEventItem(const CalendarItemStore &calendarItems,
                                           const EventFields &event,
                                           const ItemRef &currentStorage,
                                           const QString &uid)
{
    EventSaveWriteResult saveResult;
    saveResult.result.snapshot.ref = currentStorage;

    WritableCalendarItem item =
        calendarItems.writableItemForUpdate(currentStorage.collectionId, currentStorage.href, currentStorage.etag, uid);
    if (!item.isValid())
    {
        saveResult.result.status = item.status;
        return saveResult;
    }
    saveResult.result.snapshot = item.object;

    KCalendarCore::MemoryCalendar::Ptr storedCalendar = CalendarSnapshot::mutableCalendarForItem(item.object);
    if (storedCalendar.isNull())
    {
        saveResult.result.status = StorageStatus::Unsupported;
        return saveResult;
    }

    const QString resolvedUid = resolvedEventUid(storedCalendar, uid, item.object.uid);
    if (resolvedUid.isEmpty())
    {
        saveResult.result.status = StorageStatus::NotFound;
        return saveResult;
    }

    IncidenceLocator targetLocator = event.locator.value_or(IncidenceLocator{resolvedUid, std::nullopt});
    if (targetLocator.uid.isEmpty())
    {
        targetLocator.uid = resolvedUid;
    }
    if (targetLocator.uid != resolvedUid)
    {
        saveResult.result.status = StorageStatus::Conflict;
        return saveResult;
    }

    KCalendarCore::Event::Ptr storedEvent =
        event.locator.has_value() ? IncidenceResolver::findEvent(storedCalendar, targetLocator)
                                  : IncidenceResolver::findMasterEvent(storedCalendar, {resolvedUid, std::nullopt});
    bool insertNewException = false;
    if (storedEvent.isNull() && event.locator.has_value() && targetLocator.recurrenceId.has_value())
    {
        const KCalendarCore::Event::Ptr masterEvent =
            IncidenceResolver::findMasterEvent(storedCalendar, {resolvedUid, std::nullopt});
        storedEvent = CalendarItemMutator::createException(masterEvent, *targetLocator.recurrenceId);
        insertNewException = !storedEvent.isNull();
    }
    if (storedEvent.isNull())
    {
        saveResult.result.status = StorageStatus::NotFound;
        return saveResult;
    }

    const EventPatcher::Options changedOptions = EventPatcher::changedOptionsForEvent(storedEvent, event);
    if (!EventPatcher::hasChanges(changedOptions))
    {
        saveResult.result.status = StorageStatus::Ok;
        return saveResult;
    }

    KCalendarCore::Event::Ptr editedEvent =
        CalendarItemMutator::eventCloneForEdit(storedEvent, targetLocator.recurrenceId);
    EventPatcher::applyToEvent(editedEvent, event, changedOptions);
    if (!CalendarItemMutator::ensureEventUid(editedEvent, resolvedUid))
    {
        saveResult.result.status = StorageStatus::Conflict;
        return saveResult;
    }
    const bool updated =
        CalendarItemMutator::saveEventInstance(storedCalendar, storedEvent, editedEvent, insertNewException);
    if (!updated)
    {
        saveResult.result.status = StorageStatus::IoError;
        return saveResult;
    }

    item.object = CalendarSnapshot::calendarItem(
        item.object.ref, resolvedUid, storedCalendar, CalendarSnapshot::PayloadShape::Auto);
    saveResult.result = calendarItems.replaceWritableItem(item);
    saveResult.wroteStorage = saveResult.result.isOk();
    return saveResult;
}

StorageMoveResult<CalendarItem> moveSingleEventItem(const CalendarItemStore &calendarItems,
                                                    CalendarItem replacement,
                                                    const ItemRef &currentStorage,
                                                    const QString &currentUid,
                                                    const QString &destinationCollectionId)
{
    StorageMoveResult<CalendarItem> result;
    result.snapshot = replacement;

    if (currentStorage.collectionId == destinationCollectionId)
    {
        const ItemRef sourceStorage = currentStorage;
        const StorageResult<CalendarItem> updateResult =
            replaceSingleEventItem(calendarItems, replacement, currentStorage, currentUid);
        result.snapshot = updateResult.snapshot;
        result.move = MoveOutcome::update(UpdateOutcome{
            updateResult.status, sourceStorage, updateResult.isOk() ? updateResult.snapshot.ref : replacement.ref});
        return result;
    }
    const auto preconditionFailed = [&replacement, &currentStorage, &destinationCollectionId](StorageStatus status) {
        StorageMoveResult<CalendarItem> result;
        result.snapshot = replacement;
        ItemRef destination;
        destination.collectionId = destinationCollectionId;
        result.move = MoveOutcome::preconditionFailed(status, currentStorage, destination);
        return result;
    };
    if (!currentStorage.isValid())
    {
        return preconditionFailed(StorageStatus::Unsupported);
    }

    const CalendarItemStore::PrecheckedWrite checkedSource =
        calendarItems.precheckWrite(currentStorage.collectionId, currentStorage.href, currentStorage.etag);
    if (!checkedSource.isValid())
    {
        return preconditionFailed(checkedSource.status);
    }

    const std::optional<Collection> destinationCollection = calendarItems.calendarForWrite(destinationCollectionId);
    if (!destinationCollection)
    {
        return preconditionFailed(calendarItems.calendarWriteFailureStatus(destinationCollectionId));
    }
    if (!calendarItems.isValidForWrite(*destinationCollection))
    {
        return preconditionFailed(StorageStatus::IoError);
    }

    const KCalendarCore::Event::Ptr replacementEvent =
        CalendarItemMutator::editableSingleEventClone(CalendarSnapshot::calendarForItem(replacement));
    if (replacementEvent.isNull())
    {
        return preconditionFailed(StorageStatus::Unsupported);
    }

    const CalendarItemRepository::ReadResult storedItem = calendarItems.readPrechecked(checkedSource);
    if (!storedItem.isOk())
    {
        return preconditionFailed(storedItem.status);
    }
    const KCalendarCore::MemoryCalendar::Ptr storedCalendar =
        CalendarSnapshot::mutableCalendarForItem(storedItem.object);
    if (storedCalendar.isNull())
    {
        return preconditionFailed(StorageStatus::Unsupported);
    }
    if (!storedCalendar->rawTodos().isEmpty() || !EventOccurrences::canEditAsEvent(storedCalendar))
    {
        return preconditionFailed(StorageStatus::Unsupported);
    }
    const QString uid = IncidenceResolver::inferLocator(storedCalendar).uid;
    if (uid.isEmpty())
    {
        return preconditionFailed(StorageStatus::NotFound);
    }
    if (!CalendarItemMutator::ensureEventUid(replacementEvent, uid))
    {
        return preconditionFailed(StorageStatus::Conflict);
    }

    if (!CalendarItemMutator::replaceSingleEvent(storedCalendar, replacementEvent))
    {
        return preconditionFailed(StorageStatus::IoError);
    }

    CalendarItem inserted;
    inserted.ref.collectionId = destinationCollectionId;
    inserted.uid = uid;
    inserted = CalendarSnapshot::calendarItem(
        inserted.ref, inserted.uid, storedCalendar, CalendarSnapshot::PayloadShape::Event);
    result =
        CalendarMover::moveObject<StorageMoveResult<CalendarItem>>(calendarItems,
                                                                   checkedSource,
                                                                   checkedSource.ref,
                                                                   destinationCollection,
                                                                   inserted,
                                                                   replacement,
                                                                   [](const CalendarItem &moved) { return moved; });
    MoveOutcomeLog::logCrossVdirFailures(result.move,
                                         QStringLiteral("Could not remove moved source item"),
                                         QStringLiteral("Could not roll back moved event item"));
    return result;
}

} // namespace

EventService::EventMoveWriteResult EventService::moveEventObject(const EventOccurrence &currentOccurrence,
                                                                 const EventFields &event,
                                                                 const QString &destinationCollectionId) const
{
    const auto updateResultFor = [](StorageStatus status, const ItemRef &source, CalendarItem snapshot = {}) {
        EventService::EventMoveWriteResult saveResult;
        if (!snapshot.ref.isValid())
        {
            snapshot.ref = source;
        }
        saveResult.result.snapshot = snapshot;
        saveResult.result.move = MoveOutcome::update(UpdateOutcome{status, source, snapshot.ref});
        return saveResult;
    };

    if (!CalendarSnapshot::hasEventRef(currentOccurrence))
    {
        return updateResultFor(StorageStatus::Unsupported, currentOccurrence.ref.item);
    }

    const QString targetCollectionId =
        destinationCollectionId.isEmpty() ? currentOccurrence.ref.item.collectionId : destinationCollectionId;
    const bool moving = targetCollectionId != currentOccurrence.ref.item.collectionId;
    if (!moving)
    {
        const EventSaveWriteResult updateResult =
            updateSingleEventItem(m_impl->store(), event, currentOccurrence.ref.item, currentOccurrence.ref.uid);
        EventMoveWriteResult saveResult =
            updateResultFor(updateResult.result.status, currentOccurrence.ref.item, updateResult.result.snapshot);
        saveResult.wroteStorage = updateResult.wroteStorage;
        return saveResult;
    }
    if (event.locator.has_value())
    {
        return updateResultFor(StorageStatus::Unsupported, currentOccurrence.ref.item);
    }

    const WritableCalendarItem item = m_impl->store().writableItemForUpdate(currentOccurrence.ref.item.collectionId,
                                                                            currentOccurrence.ref.item.href,
                                                                            currentOccurrence.ref.item.etag,
                                                                            currentOccurrence.ref.uid);
    if (!item.isValid())
    {
        return updateResultFor(item.status, currentOccurrence.ref.item);
    }
    KCalendarCore::MemoryCalendar::Ptr calendar = CalendarSnapshot::calendarForItem(item.object);
    if (calendar.isNull())
    {
        return updateResultFor(StorageStatus::Unsupported, currentOccurrence.ref.item, item.object);
    }
    const KCalendarCore::Event::Ptr storedEvent =
        IncidenceResolver::findMasterEvent(calendar, {item.object.uid, std::nullopt});
    if (storedEvent.isNull())
    {
        return updateResultFor(StorageStatus::NotFound, currentOccurrence.ref.item, item.object);
    }
    const EventPatcher::Options changedOptions = EventPatcher::changedOptionsForEvent(storedEvent, event);

    KCalendarCore::Event::Ptr editedEvent = CalendarItemMutator::eventCloneForEdit(storedEvent);
    EventPatcher::applyToEvent(editedEvent, event, changedOptions);
    CalendarItem replacement = calendarItemForEvent(editedEvent, currentOccurrence.ref.item.collectionId);

    EventMoveWriteResult saveResult;
    saveResult.result = moveSingleEventItem(
        m_impl->store(), replacement, currentOccurrence.ref.item, currentOccurrence.ref.uid, targetCollectionId);
    saveResult.wroteStorage = saveResult.result.isOk();
    return saveResult;
}

StorageStatus EventService::deleteEvent(const OccurrenceRef &occurrence) const
{
    const CalendarItemStore::PrecheckedWrite checked =
        m_impl->store().precheckWrite(occurrence.item.collectionId, occurrence.item.href, occurrence.item.etag);
    if (!checked.isValid())
    {
        return checked.status;
    }

    const CalendarItemRepository::ReadResult storedItem = m_impl->store().readPrechecked(checked);
    if (!storedItem.isOk())
    {
        return storedItem.status;
    }
    const KCalendarCore::MemoryCalendar::Ptr calendar = CalendarSnapshot::mutableCalendarForItem(storedItem.object);
    if (calendar.isNull())
    {
        qCWarning(storageLog) << "Calendar payload missing after read" << checked.ref.href;
        return StorageStatus::Unsupported;
    }

    const QString uid = IncidenceResolver::inferLocator(calendar, occurrence.uid).uid;
    if (uid.isEmpty())
    {
        qCWarning(storageLog) << "Could not resolve event uid for deletion" << checked.ref.href;
        return StorageStatus::NotFound;
    }

    if (!CalendarItemMutator::removeEventsByUid(calendar, uid))
    {
        qCWarning(storageLog) << "Failed to remove event by resolved uid" << checked.ref.href << uid;
        return StorageStatus::NotFound;
    }
    if (!CalendarItemMutator::hasIncidences(calendar))
    {
        const StorageStatus status = m_impl->store().commitRemove(checked);
        if (status != StorageStatus::Ok)
        {
            qCWarning(storageLog) << "Could not remove vdir item" << checked.ref.href << storageStatusName(status);
        }
        else
        {
            notifyItemWritten(checked.ref);
        }
        return status;
    }

    WritableCalendarItem item;
    item.collection = m_impl->store().calendarForWrite(checked.ref.collectionId);
    item.object.ref = checked.ref;
    item.object = CalendarSnapshot::calendarItem(
        item.object.ref, item.object.uid, calendar, CalendarSnapshot::PayloadShape::Calendar);
    item.object.uid = IncidenceResolver::inferLocator(calendar).uid;
    item.status = item.object.uid.isEmpty() ? StorageStatus::NotFound : StorageStatus::Ok;
    if (item.object.uid.isEmpty())
    {
        qCWarning(storageLog) << "Could not rewrite calendar item after partial delete (uid lost)" << checked.ref.href;
    }
    const StorageResult<CalendarItem> result = m_impl->store().replaceWritableItem(item);
    if (result.isOk())
    {
        notifyItemWritten(checked.ref);
        notifyItemWritten(result.snapshot.ref);
    }
    return result.status;
}

OperationCapability EventService::canCreateEvent(const QString &collectionId) const
{
    return capabilityForWritableCollection(
        m_impl->collections(), CollectionKind::Calendar, collectionId, OperationCapabilityStatus::DestinationReadOnly);
}

OperationCapability EventService::canEditEvent(const EventOccurrence &event) const
{
    if (!CalendarSnapshot::hasEventRef(event) || event.ref.uid.isEmpty())
    {
        return invalidSelectionCapability(event.ref.item);
    }
    return capabilityForWritableCollection(m_impl->collections(),
                                           CollectionKind::Calendar,
                                           event.ref.item.collectionId,
                                           OperationCapabilityStatus::SourceReadOnly,
                                           event.ref.item);
}

OperationCapability EventService::canMoveEvent(const EventOccurrence &event,
                                               const QString &destinationCollectionId) const
{
    OperationCapability sourceCapability = canEditEvent(event);
    if (!sourceCapability.isAllowed())
    {
        return sourceCapability;
    }

    const QString targetCollectionId =
        destinationCollectionId.isEmpty() ? event.ref.item.collectionId : destinationCollectionId;
    if (targetCollectionId == event.ref.item.collectionId)
    {
        sourceCapability.destinationCollectionId = targetCollectionId;
        return sourceCapability;
    }
    if (event.recurrenceId.has_value())
    {
        return OperationCapability{
            OperationCapabilityStatus::UnsupportedRecurringInstance, event.ref.item, targetCollectionId};
    }

    return capabilityForWritableCollection(m_impl->collections(),
                                           CollectionKind::Calendar,
                                           targetCollectionId,
                                           OperationCapabilityStatus::DestinationReadOnly,
                                           event.ref.item,
                                           targetCollectionId);
}

OperationCapability EventService::canDeleteEvent(const OccurrenceRef &event) const
{
    if (!event.isValid())
    {
        return invalidSelectionCapability(event.item);
    }
    return capabilityForWritableCollection(m_impl->collections(),
                                           CollectionKind::Calendar,
                                           event.item.collectionId,
                                           OperationCapabilityStatus::SourceReadOnly,
                                           event.item);
}

OperationCapability EventService::canCompleteEvent(const EventOccurrence &event) const
{
    return canEditEvent(event);
}

QFuture<EventService::EventSaveResult> EventService::addEventAsync(const EventFields &event) const
{
    return AsyncSubmit::submit<EventSaveResult>(
        m_impl->scheduler(),
        QStringLiteral("event add"),
        [this, event] {
            return AsyncSubmit::ResolvedCollection{m_impl->store().calendarForWrite(event.collectionId),
                                                   m_impl->store().calendarWriteFailureStatus(event.collectionId)};
        },
        [this, event] { return addEvent(event); },
        [event](StorageStatus status) { return eventAddFailure(status, event); });
}

QFuture<EventService::EventSaveResult> EventService::updateEventAsync(const EventOccurrence &currentOccurrence,
                                                                      const EventFields &event) const
{
    return AsyncSubmit::submit<EventSaveResult>(
        m_impl->scheduler(),
        QStringLiteral("event update"),
        [this, currentOccurrence] { return m_impl->store().calendarForRead(currentOccurrence.ref.item.collectionId); },
        [this, currentOccurrence, event] { return updateEvent(currentOccurrence, event); },
        [currentOccurrence](StorageStatus status) {
            EventSaveResult result;
            result.status = status;
            result.snapshot.ref = currentOccurrence.ref.item;
            return result;
        });
}

QFuture<EventService::EventMoveResult> EventService::moveEventAsync(const EventOccurrence &currentOccurrence,
                                                                    const EventFields &event,
                                                                    const QString &destinationCollectionId) const
{
    const QString targetCollectionId =
        destinationCollectionId.isEmpty() ? currentOccurrence.ref.item.collectionId : destinationCollectionId;
    return AsyncSubmit::submitMove<EventMoveResult>(
        m_impl->scheduler(),
        QStringLiteral("event move"),
        targetCollectionId,
        [this, currentOccurrence] { return m_impl->store().calendarForRead(currentOccurrence.ref.item.collectionId); },
        [this, targetCollectionId] {
            return AsyncSubmit::ResolvedCollection{m_impl->store().calendarForWrite(targetCollectionId),
                                                   m_impl->store().calendarWriteFailureStatus(targetCollectionId)};
        },
        [this, currentOccurrence, event, destinationCollectionId] {
            return moveEvent(currentOccurrence, event, destinationCollectionId);
        },
        [currentOccurrence, targetCollectionId](StorageStatus status) {
            return eventMoveFailure(status, currentOccurrence.ref.item, targetCollectionId);
        });
}

QFuture<StorageStatus> EventService::deleteEventAsync(const OccurrenceRef &occurrence) const
{
    return AsyncSubmit::submit<StorageStatus>(
        m_impl->scheduler(),
        QStringLiteral("event delete"),
        [this, occurrence] { return m_impl->store().calendarForRead(occurrence.item.collectionId); },
        [this, occurrence] { return deleteEvent(occurrence); },
        [](StorageStatus status) { return status; });
}
