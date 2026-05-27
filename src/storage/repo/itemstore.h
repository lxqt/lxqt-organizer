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
#include "storagelog.h"
#include "vdirio.h"

#include <QDebug>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QSet>

#include <memory>
#include <optional>

// Kind-specific helpers used by the templated WritableItem and ItemStore.
// Specialize in the corresponding Store header so every TU that touches a
// WritableItem<T> sees the same definition (avoids ODR pitfalls).
template <typename T> struct ItemTraits;

template <typename T> struct WritableItem
{
    std::optional<Collection> collection;
    T object;
    StorageStatus status = StorageStatus::IoError;

    bool isValid() const
    {
        return status == StorageStatus::Ok && collection.has_value() && object.ref.isValid() &&
               ItemTraits<T>::hasPayload(object) && !object.uid.isEmpty();
    }
};

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
    using Item = typename Repository::ReadResult::Object;

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

    std::shared_ptr<Repository> repositoryForCollection(const Collection &collection) const
    {
        return repositoryForCollection(storageCollectionRefForCollection(collection));
    }

    bool isValidForWrite(const Collection &collection) const { return repositoryForCollection(collection)->isValid(); }

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

    typename Repository::ReadResult readPrechecked(const PrecheckedWrite &checked) const
    {
        if (!checked.isValid())
        {
            typename Repository::ReadResult result;
            result.status = checked.status == StorageStatus::Ok ? StorageStatus::IoError : checked.status;
            return result;
        }

        return repositoryForCollection(*checked.collection)->readCurrentObject(checked.ref.href, checked.ref.etag);
    }

    WritableItem<Item> writableItemForUpdate(const QString &collectionId,
                                             const QString &href,
                                             const QString &etag,
                                             const QString &fallbackUid) const
    {
        const PrecheckedWrite checked = precheckWrite(collectionId, href, etag);
        if (!checked.isValid())
        {
            WritableItem<Item> item;
            item.status = checked.status;
            return item;
        }

        const typename Repository::ReadResult readResult = readPrechecked(checked);
        if (!readResult.isOk())
        {
            WritableItem<Item> item;
            item.status = readResult.status;
            return item;
        }

        const Item stored = readResult.object;
        const QString uid = ItemTraits<Item>::resolveUid(stored, fallbackUid);
        if (uid.isEmpty())
        {
            WritableItem<Item> item;
            item.status = ItemTraits<Item>::missingUidStatus(stored);
            return item;
        }

        WritableItem<Item> item;
        item.collection = collectionForWrite(collectionId);
        item.object = stored;
        item.object.ref = checked.ref;
        item.object.uid = uid;
        item.status = StorageStatus::Ok;
        return item;
    }

    StorageResult<Item> addObject(Item object) const
    {
        StorageResult<Item> result;
        result.snapshot = object;

        if (!ItemTraits<Item>::hasPayload(object))
        {
            result.status = StorageStatus::Unsupported;
            return result;
        }
        const std::optional<Collection> collection = collectionForWrite(object.ref.collectionId);
        if (!collection)
        {
            result.status = writeFailureStatus(object.ref.collectionId);
            return result;
        }

        const std::shared_ptr<Repository> repository = repositoryForCollection(*collection);
        if (!repository->isValid())
        {
            result.status = StorageStatus::IoError;
            return result;
        }

        result = repository->addObject(object);
        if (!result.isOk())
        {
            qCWarning(storageLog) << "Could not add" << ItemTraits<Item>::logLabel()
                                  << storageStatusName(result.status);
        }
        return result;
    }

    StorageResult<Item> replaceObject(Item object) const
    {
        StorageResult<Item> result;
        result.snapshot = object;

        if (object.ref.href.isEmpty() || !ItemTraits<Item>::hasPayload(object))
        {
            result.status = StorageStatus::Unsupported;
            return result;
        }
        const std::optional<Collection> collection = collectionForWrite(object.ref.collectionId);
        if (!collection)
        {
            result.status = writeFailureStatus(object.ref.collectionId);
            return result;
        }

        const std::shared_ptr<Repository> repository = repositoryForCollection(*collection);
        if (!repository->isValid())
        {
            result.status = StorageStatus::IoError;
            return result;
        }

        result = repository->replaceObject(object);
        if (!result.isOk())
        {
            qCWarning(storageLog) << "Could not update" << ItemTraits<Item>::logLabel() << object.ref.href
                                  << storageStatusName(result.status);
        }
        return result;
    }

    StorageResult<Item> replaceWritableItem(WritableItem<Item> item) const
    {
        StorageResult<Item> result;
        result.snapshot = item.object;

        if (!item.isValid())
        {
            result.status = item.status;
            return result;
        }

        return replaceObject(item.object);
    }

    StorageStatus commitRemove(const PrecheckedWrite &checked) const
    {
        if (!checked.isValid())
        {
            return checked.status;
        }

        return repositoryForCollection(*checked.collection)->remove(checked.ref);
    }

    StorageStatus removeWritableItem(const WritableItem<Item> &item) const
    {
        if (!item.isValid())
        {
            return item.status;
        }

        return repositoryForCollection(*item.collection)->remove(item.object.ref);
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
            source.ref, inserted, sourceRemoveStatus, rollbackInsertedItem(destinationCollection, inserted));
    }

    MoveOutcome commitCrossCollectionMove(const WritableItem<Item> &source,
                                          const std::optional<StorageCollectionRef> &destinationCollection,
                                          const ItemRef &inserted) const
    {
        PrecheckedWrite checked;
        checked.collection =
            source.collection
                ? std::optional<StorageCollectionRef>(storageCollectionRefForCollection(*source.collection))
                : std::nullopt;
        checked.ref = source.object.ref;
        checked.status = source.status;
        return commitCrossCollectionMove(checked, destinationCollection, inserted);
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
