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

#include "contactitemstore.h"

ContactItemStore::ContactItemStore(const CollectionResolver &collections, const VdirIo &vdirIo)
    : m_items(collections, vdirIo)
{}

std::optional<Collection> ContactItemStore::collectionForRead(const QString &collectionId) const
{
    return m_items.collectionForRead(collectionId);
}

std::optional<Collection> ContactItemStore::collectionForWrite(const QString &collectionId) const
{
    return m_items.collectionForWrite(collectionId);
}

std::optional<Collection> ContactItemStore::addressBookForRead(const QString &collectionId) const
{
    return collectionForRead(collectionId);
}

std::optional<Collection> ContactItemStore::addressBookForWrite(const QString &collectionId) const
{
    return collectionForWrite(collectionId);
}

std::shared_ptr<ContactRepository> ContactItemStore::repositoryForCollection(const Collection &collection) const
{
    return m_items.repositoryForCollection(storageCollectionRefForCollection(collection));
}

std::shared_ptr<ContactRepository>
ContactItemStore::repositoryForCollection(const StorageCollectionRef &collection) const
{
    return m_items.repositoryForCollection(collection);
}

const VdirIo &ContactItemStore::vdirIo() const
{
    return m_items.vdirIo();
}

StorageStatus ContactItemStore::writeFailureStatus(const QString &collectionId) const
{
    return m_items.writeFailureStatus(collectionId);
}

StorageStatus ContactItemStore::addressBookWriteFailureStatus(const QString &collectionId) const
{
    return writeFailureStatus(collectionId);
}

ContactItemStore::PrecheckedWrite
ContactItemStore::precheckWrite(const QString &collectionId, const QString &href, const QString &etag) const
{
    return m_items.precheckWrite(collectionId, href, etag);
}

StorageStatus ContactItemStore::commitRemove(const PrecheckedWrite &checked) const
{
    return m_items.commitRemove(checked);
}

MoveOutcome
ContactItemStore::commitCrossCollectionMove(const PrecheckedWrite &source,
                                            const std::optional<StorageCollectionRef> &destinationCollection,
                                            const ItemRef &inserted) const
{
    return m_items.commitCrossCollectionMove(source, destinationCollection, inserted);
}

MoveOutcome ContactItemStore::commitCrossCollectionMove(const PrecheckedWrite &source,
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

void ContactItemStore::clearCache() const
{
    m_items.clearCache();
}

void ContactItemStore::clearCacheForCollections(const QSet<QString> &collectionIds) const
{
    m_items.clearCacheForCollections(collectionIds);
}

void ContactItemStore::retainRepositoriesForCollections(const QList<Collection> &collections) const
{
    m_items.retainRepositoriesForCollections(collections);
}
