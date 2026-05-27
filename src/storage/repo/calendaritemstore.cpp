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
    return m_items.repositoryForCollection(collection);
}

std::shared_ptr<CalendarItemRepository>
CalendarItemStore::repositoryForCollection(const StorageCollectionRef &collection) const
{
    return m_items.repositoryForCollection(collection);
}

bool CalendarItemStore::isValidForWrite(const Collection &collection) const
{
    return m_items.isValidForWrite(collection);
}

CalendarItemStore::PrecheckedWrite
CalendarItemStore::precheckWrite(const QString &collectionId, const QString &href, const QString &etag) const
{
    return m_items.precheckWrite(collectionId, href, etag);
}

CalendarItemRepository::ReadResult CalendarItemStore::readPrechecked(const PrecheckedWrite &checked) const
{
    return m_items.readPrechecked(checked);
}

CalendarItemStore::WritableCalendarItem CalendarItemStore::writableItemForUpdate(const QString &collectionId,
                                                                                 const QString &href,
                                                                                 const QString &etag,
                                                                                 const QString &fallbackUid) const
{
    return m_items.writableItemForUpdate(collectionId, href, etag, fallbackUid);
}

StorageResult<CalendarItem> CalendarItemStore::addObject(CalendarItem object) const
{
    return m_items.addObject(std::move(object));
}

StorageResult<CalendarItem> CalendarItemStore::replaceObject(CalendarItem object) const
{
    return m_items.replaceObject(std::move(object));
}

StorageResult<CalendarItem> CalendarItemStore::replaceWritableItem(WritableCalendarItem item) const
{
    return m_items.replaceWritableItem(std::move(item));
}

StorageStatus CalendarItemStore::commitRemove(const PrecheckedWrite &checked) const
{
    return m_items.commitRemove(checked);
}

StorageStatus CalendarItemStore::removeWritableItem(const WritableCalendarItem &item) const
{
    return m_items.removeWritableItem(item);
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
    return m_items.commitCrossCollectionMove(
        source,
        destinationCollection
            ? std::optional<StorageCollectionRef>(storageCollectionRefForCollection(*destinationCollection))
            : std::nullopt,
        inserted);
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
