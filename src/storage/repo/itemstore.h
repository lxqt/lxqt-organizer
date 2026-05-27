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

#ifndef ITEMSTORE_H
#define ITEMSTORE_H

#include "collectionresolver.h"
#include "storagecollectionref.h"
#include "storageresult.h"
#include "vdirio.h"

#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QSet>

#include <memory>
#include <optional>

// @thread any-thread-with-mutex; repository cache mutations are mutex-protected,
// repository calls are serialized by VdirIo.
template <typename Repository, CollectionKind Kind> class ItemStore
{
    struct CacheKey
    {
        QString collectionId;
        QString canonicalPath;

        bool operator==(const CacheKey &other) const
        {
            return collectionId == other.collectionId && canonicalPath == other.canonicalPath;
        }

        friend size_t qHash(const CacheKey &key, size_t seed = 0)
        {
            seed = qHash(key.collectionId, seed);
            return qHash(key.canonicalPath, seed);
        }
    };

    static CacheKey cacheKeyForCollection(const StorageCollectionRef &collection)
    {
        return {collection.id, collection.canonicalPath};
    }

public:
    struct PrecheckedWrite
    {
        std::optional<StorageCollectionRef> collection;
        ItemRef ref;
        StorageStatus status = StorageStatus::IoError;

        bool isValid() const { return status == StorageStatus::Ok && collection.has_value() && ref.isValid(); }
    };

    explicit ItemStore(const CollectionResolver &collections, const VdirIo &vdirIo)
        : m_collections(collections)
        , m_vdirIo(vdirIo)
    {}

    std::optional<Collection> collectionForRead(const QString &collectionId) const
    {
        if constexpr (Kind == CollectionKind::Calendar)
        {
            return m_collections.calendarForRead(collectionId);
        }
        else
        {
            return m_collections.addressBookForRead(collectionId);
        }
    }

    std::optional<Collection> collectionForWrite(const QString &collectionId) const
    {
        if constexpr (Kind == CollectionKind::Calendar)
        {
            return m_collections.calendarForWrite(collectionId);
        }
        else
        {
            return m_collections.addressBookForWrite(collectionId);
        }
    }

    StorageStatus writeFailureStatus(const QString &collectionId) const
    {
        if constexpr (Kind == CollectionKind::Calendar)
        {
            return m_collections.calendarWriteFailureStatus(collectionId);
        }
        else
        {
            return m_collections.addressBookWriteFailureStatus(collectionId);
        }
    }

    const VdirIo &vdirIo() const { return m_vdirIo; }

    std::shared_ptr<Repository> repositoryForCollection(const StorageCollectionRef &collection) const
    {
        const CacheKey key = cacheKeyForCollection(collection);
        QMutexLocker locker(&m_repositoriesMutex);
        // Keep repositories shared so workers that started before a catalog
        // reload can finish against the vdir/cache instance they validated.
        const auto cached = m_repositories.constFind(key);
        if (cached != m_repositories.constEnd())
        {
            (*cached)->updateCollection(collection);
            return *cached;
        }

        const std::shared_ptr<Repository> repository = std::make_shared<Repository>(collection, m_vdirIo);
        m_repositories.insert(key, repository);
        return repository;
    }

    PrecheckedWrite precheckWrite(const QString &collectionId, const QString &href, const QString &etag) const
    {
        PrecheckedWrite checked;
        if (href.isEmpty())
        {
            checked.status = StorageStatus::Unsupported;
            return checked;
        }
        if (etag.isEmpty())
        {
            checked.status = StorageStatus::Conflict;
            return checked;
        }

        const std::optional<Collection> readCollection = collectionForRead(collectionId);
        if (!readCollection)
        {
            checked.status = StorageStatus::NotFound;
            return checked;
        }

        const std::shared_ptr<Repository> readRepository =
            repositoryForCollection(storageCollectionRefForCollection(*readCollection));
        if (!readRepository->isValid())
        {
            checked.status = StorageStatus::IoError;
            return checked;
        }

        ItemRef currentStorage;
        const StorageStatus checkStatus = readRepository->checkCurrentItem(href, etag, &currentStorage);
        if (checkStatus != StorageStatus::Ok)
        {
            checked.status = checkStatus;
            return checked;
        }

        const std::optional<Collection> writeCollection = collectionForWrite(collectionId);
        if (!writeCollection)
        {
            checked.status = writeFailureStatus(collectionId);
            return checked;
        }
        StorageCollectionRef writeRef = storageCollectionRefForCollection(*writeCollection);
        if (!repositoryForCollection(writeRef)->isValid())
        {
            checked.status = StorageStatus::IoError;
            return checked;
        }

        checked.collection = writeRef;
        checked.ref = ItemRef{writeRef.id, currentStorage.href, currentStorage.etag};
        checked.status = StorageStatus::Ok;
        return checked;
    }

    StorageStatus commitRemove(const PrecheckedWrite &checked) const
    {
        if (!checked.isValid())
        {
            return checked.status;
        }

        return repositoryForCollection(*checked.collection)->remove(checked.ref);
    }

    StorageStatus rollbackInsertedItem(const std::optional<StorageCollectionRef> &collection,
                                       const ItemRef &inserted) const
    {
        if (!collection)
        {
            return StorageStatus::NotFound;
        }

        return repositoryForCollection(*collection)->remove(inserted);
    }

    MoveOutcome commitCrossCollectionMove(const PrecheckedWrite &source,
                                          const std::optional<StorageCollectionRef> &destinationCollection,
                                          const ItemRef &inserted) const
    {
        const StorageStatus sourceRemoveStatus = commitRemove(source);
        if (sourceRemoveStatus == StorageStatus::Ok)
        {
            return MoveOutcome::crossCollection(source.ref, inserted, sourceRemoveStatus);
        }

        return MoveOutcome::crossCollection(
            source.ref, inserted, sourceRemoveStatus, true, rollbackInsertedItem(destinationCollection, inserted));
    }

    void clearCache() const
    {
        QMutexLocker locker(&m_repositoriesMutex);
        m_repositories.clear();
    }

    void clearCacheForCollections(const QSet<QString> &collectionIds) const
    {
        if (collectionIds.isEmpty())
        {
            return;
        }

        QMutexLocker locker(&m_repositoriesMutex);
        for (auto it = m_repositories.begin(); it != m_repositories.end();)
        {
            if (collectionIds.contains(it.key().collectionId))
            {
                it = m_repositories.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void retainRepositoriesForCollections(const QList<Collection> &collections) const
    {
        QHash<CacheKey, StorageCollectionRef> current;
        for (const Collection &collection : collections)
        {
            const StorageCollectionRef ref = storageCollectionRefForCollection(collection);
            current.insert(cacheKeyForCollection(ref), ref);
        }

        QMutexLocker locker(&m_repositoriesMutex);
        for (auto it = m_repositories.begin(); it != m_repositories.end();)
        {
            const auto collectionIt = current.constFind(it.key());
            if (collectionIt == current.constEnd())
            {
                it = m_repositories.erase(it);
            }
            else
            {
                (*it)->updateCollection(collectionIt.value());
                ++it;
            }
        }
    }

private:
    const CollectionResolver &m_collections;
    const VdirIo &m_vdirIo;
    // Leaf cache mutex: must not call out while held.
    mutable QHash<CacheKey, std::shared_ptr<Repository>> m_repositories;
    mutable QMutex m_repositoriesMutex;
};

#endif // ITEMSTORE_H
