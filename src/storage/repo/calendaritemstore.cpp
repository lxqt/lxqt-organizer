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

#include "calendaritemstore.h"

#include "calendarvalidation.h"
#include "incidenceresolver.h"
#include "storagelog.h"

#include <QDebug>

#include <KCalendarCore/Todo>

CalendarItemStore::CalendarItemStore(const CollectionResolver &collections, const VdirIo &vdirIo)
    : m_items(collections, vdirIo)
{}

std::optional<Collection> CalendarItemStore::calendarForRead(const QString &collectionId) const
{
    return m_items.collectionForRead(collectionId);
}

std::optional<Collection> CalendarItemStore::collectionForRead(const QString &collectionId) const
{
    return calendarForRead(collectionId);
}

std::optional<Collection> CalendarItemStore::calendarForWrite(const QString &collectionId) const
{
    return m_items.collectionForWrite(collectionId);
}

std::optional<Collection> CalendarItemStore::collectionForWrite(const QString &collectionId) const
{
    return calendarForWrite(collectionId);
}

const VdirIo &CalendarItemStore::vdirIo() const
{
    return m_items.vdirIo();
}

StorageStatus CalendarItemStore::calendarWriteFailureStatus(const QString &collectionId) const
{
    return m_items.writeFailureStatus(collectionId);
}

StorageStatus CalendarItemStore::writeFailureStatus(const QString &collectionId) const
{
    return calendarWriteFailureStatus(collectionId);
}

std::shared_ptr<CalendarItemRepository> CalendarItemStore::repositoryForCollection(const Collection &collection) const
{
    return m_items.repositoryForCollection(storageCollectionRefForCollection(collection));
}

std::shared_ptr<CalendarItemRepository>
CalendarItemStore::repositoryForCollection(const StorageCollectionRef &collection) const
{
    return m_items.repositoryForCollection(collection);
}

bool CalendarItemStore::isValidForWrite(const Collection &collection) const
{
    return repositoryForCollection(collection)->isValid();
}

CalendarItemStore::PrecheckedWrite
CalendarItemStore::precheckWrite(const QString &collectionId, const QString &href, const QString &etag) const
{
    return m_items.precheckWrite(collectionId, href, etag);
}

CalendarItemRepository::ReadResult CalendarItemStore::readPrechecked(const PrecheckedWrite &checked) const
{
    if (!checked.isValid())
    {
        CalendarItemRepository::ReadResult result;
        result.status = checked.status == StorageStatus::Ok ? StorageStatus::IoError : checked.status;
        return result;
    }

    return repositoryForCollection(*checked.collection)->readCurrentObject(checked.ref.href, checked.ref.etag);
}

CalendarItemStore::WritableCalendarItem CalendarItemStore::writableItemForUpdate(const QString &collectionId,
                                                                                 const QString &href,
                                                                                 const QString &etag,
                                                                                 const QString &fallbackUid) const
{
    const PrecheckedWrite checked = precheckWrite(collectionId, href, etag);
    if (!checked.isValid())
    {
        WritableCalendarItem item;
        item.status = checked.status;
        return item;
    }

    const CalendarItemRepository::ReadResult readResult = readPrechecked(checked);
    if (!readResult.isOk())
    {
        WritableCalendarItem item;
        item.status = readResult.status;
        return item;
    }

    const CalendarItem stored = readResult.object;
    const KCalendarCore::MemoryCalendar::Ptr storedCalendar = CalendarSnapshot::calendarForItem(stored);
    if (storedCalendar.isNull())
    {
        WritableCalendarItem item;
        item.status = StorageStatus::Unsupported;
        return item;
    }
    const QString uid = IncidenceResolver::inferLocator(storedCalendar, fallbackUid).uid;
    if (uid.isEmpty())
    {
        WritableCalendarItem item;
        item.status = StorageStatus::NotFound;
        return item;
    }

    WritableCalendarItem item;
    item.collection = calendarForWrite(collectionId);
    item.object = stored;
    item.object.ref = checked.ref;
    item.object.uid = uid;
    item.status = StorageStatus::Ok;
    return item;
}

StorageResult<CalendarItem> CalendarItemStore::addObject(CalendarItem object) const
{
    StorageResult<CalendarItem> result;
    result.snapshot = object;

    const std::optional<Collection> collection = calendarForWrite(object.ref.collectionId);
    if (!CalendarSnapshot::hasCalendarItemPayload(object))
    {
        result.status = StorageStatus::Unsupported;
        return result;
    }
    if (!collection)
    {
        result.status = calendarWriteFailureStatus(object.ref.collectionId);
        return result;
    }

    const std::shared_ptr<CalendarItemRepository> repository = repositoryForCollection(*collection);
    if (!repository->isValid())
    {
        result.status = StorageStatus::IoError;
        return result;
    }

    result = repository->addObject(object);
    if (!result.isOk())
    {
        qCWarning(storageLog) << "Could not add calendar item" << storageStatusName(result.status);
    }
    return result;
}

StorageResult<CalendarItem> CalendarItemStore::replaceObject(CalendarItem object) const
{
    StorageResult<CalendarItem> result;
    result.snapshot = object;

    const std::optional<Collection> collection = calendarForWrite(object.ref.collectionId);
    if (object.ref.href.isEmpty() || !CalendarSnapshot::hasCalendarItemPayload(object))
    {
        result.status = StorageStatus::Unsupported;
        return result;
    }
    if (!collection)
    {
        result.status = calendarWriteFailureStatus(object.ref.collectionId);
        return result;
    }

    const std::shared_ptr<CalendarItemRepository> repository = repositoryForCollection(*collection);
    if (!repository->isValid())
    {
        result.status = StorageStatus::IoError;
        return result;
    }

    result = repository->replaceObject(object);
    if (!result.isOk())
    {
        qCWarning(storageLog) << "Could not update calendar item" << object.ref.href
                              << storageStatusName(result.status);
    }
    return result;
}

StorageResult<CalendarItem> CalendarItemStore::replaceWritableItem(WritableCalendarItem item) const
{
    StorageResult<CalendarItem> result;
    result.snapshot = item.object;

    if (!item.isValid())
    {
        result.status = item.status;
        return result;
    }

    return replaceObject(item.object);
}

StorageStatus CalendarItemStore::commitRemove(const PrecheckedWrite &checked) const
{
    if (!checked.isValid())
    {
        return checked.status;
    }

    return m_items.commitRemove(checked);
}

StorageStatus CalendarItemStore::removeWritableItem(const WritableCalendarItem &item) const
{
    if (!item.isValid())
    {
        return item.status;
    }

    return repositoryForCollection(*item.collection)->remove(item.object.ref);
}

StorageStatus CalendarItemStore::rollbackInsertedItem(const std::optional<Collection> &collection,
                                                      const ItemRef &inserted) const
{
    if (!collection)
    {
        return StorageStatus::NotFound;
    }

    return m_items.rollbackInsertedItem(storageCollectionRefForCollection(*collection), inserted);
}

MoveOutcome CalendarItemStore::commitCrossCollectionMove(const PrecheckedWrite &source,
                                                         const std::optional<Collection> &destinationCollection,
                                                         const ItemRef &inserted) const
{
    return m_items.commitCrossCollectionMove(
        source,
        destinationCollection
            ? std::optional<StorageCollectionRef>(storageCollectionRefForCollection(*destinationCollection))
            : std::nullopt,
        inserted);
}

MoveOutcome CalendarItemStore::commitCrossCollectionMove(const WritableCalendarItem &source,
                                                         const std::optional<Collection> &destinationCollection,
                                                         const ItemRef &inserted) const
{
    PrecheckedWrite checked;
    checked.collection =
        source.collection ? std::optional<StorageCollectionRef>(storageCollectionRefForCollection(*source.collection))
                          : std::nullopt;
    checked.ref = source.object.ref;
    checked.status = source.status;
    return commitCrossCollectionMove(checked, destinationCollection, inserted);
}

void CalendarItemStore::clearCache() const
{
    m_items.clearCache();
}

void CalendarItemStore::clearCacheForCollections(const QSet<QString> &collectionIds) const
{
    m_items.clearCacheForCollections(collectionIds);
}

void CalendarItemStore::retainRepositoriesForCollections(const QList<Collection> &collections) const
{
    m_items.retainRepositoriesForCollections(collections);
}
