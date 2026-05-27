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

#include "contactstore.h"

ContactStore::ContactStore(const CollectionResolver &collections, const VdirIo &vdirIo)
    : m_items(collections, vdirIo)
{}

std::optional<Collection> ContactStore::collectionForRead(const QString &collectionId) const
{
    return m_items.collectionForRead(collectionId);
}

std::optional<Collection> ContactStore::collectionForWrite(const QString &collectionId) const
{
    return m_items.collectionForWrite(collectionId);
}

std::shared_ptr<ContactRepository> ContactStore::repositoryForCollection(const Collection &collection) const
{
    return m_items.repositoryForCollection(collection);
}

std::shared_ptr<ContactRepository> ContactStore::repositoryForCollection(const StorageCollectionRef &collection) const
{
    return m_items.repositoryForCollection(collection);
}

bool ContactStore::isValidForWrite(const Collection &collection) const
{
    return m_items.isValidForWrite(collection);
}

const VdirIo &ContactStore::vdirIo() const
{
    return m_items.vdirIo();
}

StorageStatus ContactStore::writeFailureStatus(const QString &collectionId) const
{
    return m_items.writeFailureStatus(collectionId);
}

ContactStore::PrecheckedWrite
ContactStore::precheckWrite(const QString &collectionId, const QString &href, const QString &etag) const
{
    return m_items.precheckWrite(collectionId, href, etag);
}

ContactRepository::ReadResult ContactStore::readPrechecked(const PrecheckedWrite &checked) const
{
    return m_items.readPrechecked(checked);
}

ContactStore::WritableContactItem ContactStore::writableItemForUpdate(const QString &collectionId,
                                                                      const QString &href,
                                                                      const QString &etag,
                                                                      const QString &fallbackUid) const
{
    return m_items.writableItemForUpdate(collectionId, href, etag, fallbackUid);
}

StorageResult<Contact> ContactStore::addObject(Contact object) const
{
    return m_items.addObject(std::move(object));
}

StorageResult<Contact> ContactStore::replaceObject(Contact object) const
{
    return m_items.replaceObject(std::move(object));
}

StorageResult<Contact> ContactStore::replaceWritableItem(WritableContactItem item) const
{
    return m_items.replaceWritableItem(std::move(item));
}

StorageStatus ContactStore::commitRemove(const PrecheckedWrite &checked) const
{
    return m_items.commitRemove(checked);
}

StorageStatus ContactStore::removeWritableItem(const WritableContactItem &item) const
{
    return m_items.removeWritableItem(item);
}

MoveOutcome ContactStore::commitCrossCollectionMove(const PrecheckedWrite &source,
                                                    const std::optional<StorageCollectionRef> &destinationCollection,
                                                    const ItemRef &inserted) const
{
    return m_items.commitCrossCollectionMove(source, destinationCollection, inserted);
}

MoveOutcome ContactStore::commitCrossCollectionMove(const PrecheckedWrite &source,
                                                    const std::optional<Collection> &destinationCollection,
                                                    const ItemRef &inserted) const
{
    return commitCrossCollectionMove(
        source,
        destinationCollection
            ? std::optional<StorageCollectionRef>(storageCollectionRefForCollection(*destinationCollection))
            : std::nullopt,
        inserted);
}

MoveOutcome ContactStore::commitCrossCollectionMove(const WritableContactItem &source,
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

void ContactStore::clearCache() const
{
    m_items.clearCache();
}

void ContactStore::clearCacheForCollections(const QSet<QString> &collectionIds) const
{
    m_items.clearCacheForCollections(collectionIds);
}

void ContactStore::retainRepositoriesForCollections(const QList<Collection> &collections) const
{
    m_items.retainRepositoriesForCollections(collections);
}
