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

#ifndef CONTACTITEMSTORE_H
#define CONTACTITEMSTORE_H

#include "collectionresolver.h"
#include "contactrepository.h"
#include "itemstore.h"
#include "storageresult.h"

#include <QSet>

#include <memory>
#include <optional>

class VdirIo;

// @thread any-thread-with-mutex; cache state is mutex-protected by ItemStore.
class ContactItemStore
{
public:
    using PrecheckedWrite = ItemStore<ContactRepository, CollectionKind::AddressBook>::PrecheckedWrite;

    explicit ContactItemStore(const CollectionResolver &collections, const VdirIo &vdirIo);

    std::optional<Collection> collectionForRead(const QString &collectionId) const;
    std::optional<Collection> collectionForWrite(const QString &collectionId) const;
    std::optional<Collection> addressBookForRead(const QString &collectionId) const;
    std::optional<Collection> addressBookForWrite(const QString &collectionId) const;
    std::shared_ptr<ContactRepository> repositoryForCollection(const Collection &collection) const;
    std::shared_ptr<ContactRepository> repositoryForCollection(const StorageCollectionRef &collection) const;
    const VdirIo &vdirIo() const;
    StorageStatus writeFailureStatus(const QString &collectionId) const;
    StorageStatus addressBookWriteFailureStatus(const QString &collectionId) const;

    PrecheckedWrite precheckWrite(const QString &collectionId, const QString &href, const QString &etag) const;
    StorageStatus commitRemove(const PrecheckedWrite &checked) const;
    MoveOutcome commitCrossCollectionMove(const PrecheckedWrite &source,
                                          const std::optional<StorageCollectionRef> &destinationCollection,
                                          const ItemRef &inserted) const;
    MoveOutcome commitCrossCollectionMove(const PrecheckedWrite &source,
                                          const std::optional<Collection> &destinationCollection,
                                          const ItemRef &inserted) const;

    void clearCache() const;
    void clearCacheForCollections(const QSet<QString> &collectionIds) const;
    void retainRepositoriesForCollections(const QList<Collection> &collections) const;

private:
    ItemStore<ContactRepository, CollectionKind::AddressBook> m_items;
};

#endif // CONTACTITEMSTORE_H
