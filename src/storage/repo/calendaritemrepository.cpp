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

#include "calendaritemrepository.h"

#include "icalcodec.h"
#include "calendarvalidation.h"
#include "incidenceresolver.h"
#include "itemidentity.h"
#include "vdiritemrepository.h"

#include <KCalendarCore/Todo>

namespace {

struct SerializedCalendar
{
    StorageStatus status = StorageStatus::IoError;
    QString text;
    KCalendarCore::MemoryCalendar::Ptr calendar;
};

KCalendarCore::MemoryCalendar::Ptr
calendarCopyWithUid(const KCalendarCore::MemoryCalendar::Ptr &source, const QString &uid, bool replaceExistingUid)
{
    // VJOURNAL is not modeled by the app and would round-trip lossy.
    if (!source->journals().isEmpty())
    {
        return {};
    }

    KCalendarCore::MemoryCalendar::Ptr calendar =
        KCalendarCore::MemoryCalendar::Ptr(new KCalendarCore::MemoryCalendar(source->timeZone()));
    for (const KCalendarCore::Event::Ptr &event : source->events())
    {
        KCalendarCore::Event::Ptr copy(event->clone());
        if (copy->uid().isEmpty() || replaceExistingUid)
        {
            copy->setUid(uid);
        }
        if (!calendar->addEvent(copy))
        {
            return {};
        }
    }
    for (const KCalendarCore::Todo::Ptr &todo : source->rawTodos())
    {
        KCalendarCore::Todo::Ptr copy(todo->clone());
        if (copy->uid().isEmpty() || replaceExistingUid)
        {
            copy->setUid(uid);
        }
        if (!calendar->addTodo(copy))
        {
            return {};
        }
    }
    return calendar;
}

QString firstCalendarUid(const KCalendarCore::MemoryCalendar::Ptr &calendar, const QString &fallback)
{
    QString uid = fallback;
    for (const KCalendarCore::Event::Ptr &event : calendar->events())
    {
        if (uid.isEmpty() && !event->uid().isEmpty())
        {
            uid = event->uid();
            break;
        }
    }
    for (const KCalendarCore::Todo::Ptr &todo : calendar->rawTodos())
    {
        if (uid.isEmpty() && !todo->uid().isEmpty())
        {
            uid = todo->uid();
            break;
        }
    }
    return uid;
}

QString safeReplacementUid(const KCalendarCore::MemoryCalendar::Ptr &calendar,
                           const QString &preferredUid,
                           bool *replaceEmptyIncidenceUids)
{
    if (replaceEmptyIncidenceUids)
    {
        *replaceEmptyIncidenceUids = false;
    }
    if (!preferredUid.isEmpty())
    {
        return preferredUid;
    }
    if (calendar.isNull())
    {
        return {};
    }

    const KCalendarCore::Event::List events = calendar->events();
    const KCalendarCore::Todo::List todos = calendar->rawTodos();
    if (events.size() + todos.size() != 1)
    {
        return {};
    }

    QString uid;
    if (!events.isEmpty())
    {
        uid = events.constFirst()->uid();
    }
    else
    {
        uid = todos.constFirst()->uid();
    }
    if (!uid.isEmpty())
    {
        return uid;
    }

    if (replaceEmptyIncidenceUids)
    {
        *replaceEmptyIncidenceUids = true;
    }
    return randomItemUid();
}

SerializedCalendar serializeCalendar(const KCalendarCore::MemoryCalendar::Ptr &calendar)
{
    SerializedCalendar result;
    const QString serialized = ICalCodec::serialize(calendar);
    if (serialized.isEmpty())
    {
        result.status = StorageStatus::ParseError;
        return result;
    }

    result.status = StorageStatus::Ok;
    result.text = serialized;
    result.calendar = calendar;
    return result;
}

} // namespace

CalendarItemRepository::CalendarItemRepository(const StorageCollectionRef &collection, const VdirIo &vdirIo)
    : m_items(collection, vdirIo)
{}

StorageCollectionRef CalendarItemRepository::collection() const
{
    return m_items.collection();
}

void CalendarItemRepository::updateCollection(const StorageCollectionRef &collection)
{
    m_items.updateCollection(collection);
}

bool CalendarItemRepository::isValid() const
{
    return m_items.isValid();
}

StorageStatus
CalendarItemRepository::checkCurrentItem(const QString &href, const QString &expectedEtag, ItemRef *storage) const
{
    return m_items.checkCurrentItem(href, expectedEtag, storage);
}

CalendarItemRepository::ReadResult CalendarItemRepository::readCurrentObject(const QString &href,
                                                                             const QString &expectedEtag) const
{
    ReadResult result;
    result.object.ref = ItemRef{m_items.collection().id, href, expectedEtag};

    const ::ReadResult<VdirItemRepository::ItemPayload> current = m_items.readCurrentItem(href, expectedEtag);
    result.object.ref = current.object.ref;
    if (!current.isOk())
    {
        result.status = current.status;
        return result;
    }

    const std::optional<KCalendarCore::MemoryCalendar::Ptr> parsedCalendar = ICalCodec::parse(current.object.content);
    if (!parsedCalendar)
    {
        result.status = StorageStatus::ParseError;
        return result;
    }
    if (!CalendarValidation::isSupportedCalendar(*parsedCalendar))
    {
        result.status = StorageStatus::Unsupported;
        return result;
    }

    const QString uid = IncidenceResolver::inferLocator(*parsedCalendar).uid;
    result.object =
        CalendarSnapshot::calendarItem(current.object.ref, uid, *parsedCalendar, CalendarSnapshot::PayloadShape::Auto);
    result.status = StorageStatus::Ok;
    return result;
}

void CalendarItemRepository::forEachObject(const std::function<bool(const ReadResult &)> &visitor,
                                           const std::function<bool()> &stopRequested) const
{
    if (!visitor)
    {
        return;
    }
    if (!isValid())
    {
        ReadResult readResult;
        readResult.status = StorageStatus::IoError;
        visitor(readResult);
        return;
    }

    // Hold the path worker's FIFO slot for the full traversal so a concurrent
    // write to the same vdir cannot interleave between items and tear the snapshot.
    m_items.runOnWorker(QStringLiteral("calendar forEachObject"), [&] {
        for (const VdirItemRepository::ItemState &state : m_items.itemStates())
        {
            if (stopRequested && stopRequested())
            {
                break;
            }
            if (!visitor(readCurrentObject(state.href, state.etag)))
            {
                break;
            }
        }
    });
}

QList<CalendarItemRepository::ReadResult> CalendarItemRepository::readObjects() const
{
    QList<ReadResult> result;
    // Scheduler shutdown skips the traversal, so empty can also mean teardown.
    forEachObject([&result](const ReadResult &readResult) {
        result.append(readResult);
        return true;
    });
    return result;
}

QString CalendarItemRepository::hrefForUid(const QString &uid) const
{
    return m_items.hrefForUid(uid);
}

StorageStatus CalendarItemRepository::create(const QString &href, const QString &text, QString *etag) const
{
    return m_items.create(href, text, etag);
}

StorageStatus CalendarItemRepository::replace(const QString &href,
                                              const QString &text,
                                              const QString &expectedEtag,
                                              QString *newEtag) const
{
    return m_items.replace(href, text, expectedEtag, newEtag);
}

StorageStatus CalendarItemRepository::remove(const ItemRef &storage) const
{
    return m_items.remove(storage);
}

StorageResult<CalendarItem> CalendarItemRepository::createCalendar(const QString &href, CalendarItem object) const
{
    StorageResult<CalendarItem> result;
    result.snapshot = object;

    const SerializedCalendar serialized = serializeCalendar(CalendarSnapshot::calendarForItem(object));
    if (serialized.status != StorageStatus::Ok)
    {
        result.status = serialized.status;
        return result;
    }

    QString etag;
    const StorageStatus createStatus = create(href, serialized.text, &etag);
    if (createStatus != StorageStatus::Ok)
    {
        result.status = createStatus;
        return result;
    }

    result.status = StorageStatus::Ok;
    const ItemRef ref{m_items.collection().id, href, etag};
    const QString uid = IncidenceResolver::inferLocator(serialized.calendar, object.uid).uid;
    result.snapshot =
        CalendarSnapshot::calendarItem(ref, uid, serialized.calendar, CalendarSnapshot::PayloadShape::Auto);
    return result;
}

StorageResult<CalendarItem> CalendarItemRepository::addObject(CalendarItem object) const
{
    StorageResult<CalendarItem> result;
    result.snapshot = object;

    if (!isValid())
    {
        result.status = StorageStatus::IoError;
        return result;
    }
    if (!CalendarSnapshot::hasCalendarItemPayload(object))
    {
        result.status = StorageStatus::Unsupported;
        return result;
    }

    const QString originalUid = firstCalendarUid(CalendarSnapshot::calendarForItem(object), object.uid);
    const bool generatedUid = originalUid.isEmpty() && object.ref.href.isEmpty();
    if (!generatedUid && originalUid.isEmpty())
    {
        result.status = StorageStatus::Unsupported;
        return result;
    }

    const int attempts = generatedUid ? 2 : 1;

    StorageResult<CalendarItem> createResult;
    for (int attempt = 0; attempt < attempts; ++attempt)
    {
        const QString uid = generatedUid ? randomItemUid() : originalUid;
        if (uid.isEmpty())
        {
            result.status = StorageStatus::Unsupported;
            return result;
        }

        const KCalendarCore::MemoryCalendar::Ptr calendar =
            calendarCopyWithUid(CalendarSnapshot::calendarForItem(object), uid, generatedUid);
        if (calendar.isNull())
        {
            result.status = StorageStatus::Unsupported;
            return result;
        }
        CalendarItem candidate = object;
        candidate.uid = uid;
        candidate = CalendarSnapshot::calendarItem(
            candidate.ref, candidate.uid, calendar, CalendarSnapshot::PayloadShape::Auto);
        const QString href = object.ref.href.isEmpty() ? hrefForUid(uid) : object.ref.href;
        createResult = createCalendar(href, candidate);
        if (createResult.status == StorageStatus::Ok)
        {
            break;
        }
        if (createResult.status != StorageStatus::Conflict)
        {
            return createResult;
        }
    }
    if (createResult.status != StorageStatus::Ok)
    {
        createResult.status = StorageStatus::Conflict;
    }
    return createResult;
}

StorageResult<CalendarItem>
CalendarItemRepository::replaceCalendar(const QString &href, CalendarItem object, const QString &expectedEtag) const
{
    StorageResult<CalendarItem> result;
    result.snapshot = object;

    const SerializedCalendar serialized = serializeCalendar(CalendarSnapshot::calendarForItem(object));
    if (serialized.status != StorageStatus::Ok)
    {
        result.status = serialized.status;
        return result;
    }

    QString newEtag;
    const StorageStatus replaceStatus = replace(href, serialized.text, expectedEtag, &newEtag);
    if (replaceStatus != StorageStatus::Ok)
    {
        result.status = replaceStatus;
        return result;
    }

    result.status = StorageStatus::Ok;
    const ItemRef ref{m_items.collection().id, href, newEtag};
    const QString uid = IncidenceResolver::inferLocator(serialized.calendar, object.uid).uid;
    result.snapshot =
        CalendarSnapshot::calendarItem(ref, uid, serialized.calendar, CalendarSnapshot::PayloadShape::Auto);
    return result;
}

StorageResult<CalendarItem> CalendarItemRepository::replaceObject(CalendarItem object) const
{
    StorageResult<CalendarItem> result;
    result.snapshot = object;

    if (!isValid())
    {
        result.status = StorageStatus::IoError;
        return result;
    }
    if (object.ref.href.isEmpty() || !CalendarSnapshot::hasCalendarItemPayload(object))
    {
        result.status = StorageStatus::Unsupported;
        return result;
    }

    const KCalendarCore::MemoryCalendar::Ptr sourceCalendar = CalendarSnapshot::calendarForItem(object);
    if (sourceCalendar.isNull())
    {
        result.status = StorageStatus::Unsupported;
        return result;
    }
    bool replaceEmptyUids = false;
    const QString uid = safeReplacementUid(sourceCalendar, object.uid, &replaceEmptyUids);
    if (uid.isEmpty())
    {
        result.status = StorageStatus::Unsupported;
        return result;
    }

    const KCalendarCore::MemoryCalendar::Ptr calendar = calendarCopyWithUid(sourceCalendar, uid, replaceEmptyUids);
    if (calendar.isNull())
    {
        result.status = StorageStatus::Unsupported;
        return result;
    }
    object.uid = uid;
    object = CalendarSnapshot::calendarItem(object.ref, object.uid, calendar, CalendarSnapshot::PayloadShape::Auto);

    return replaceCalendar(object.ref.href, object, object.ref.etag);
}
