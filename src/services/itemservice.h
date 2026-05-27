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

#ifndef ITEMSERVICE_H
#define ITEMSERVICE_H

#include "collectionservice.h"
#include "collectionreloadsubscription.h"
#include "completedfuture.h"
#include "itemstore.h"
#include "moveoutcomelog.h"
#include "storagecollectionref.h"
#include "storageresult.h"
#include "vdirio.h"

#include <QFuture>
#include <QList>
#include <QSet>
#include <QString>

#include <memory>
#include <optional>

template <typename Snapshot, typename Repo, typename Traits> class ItemService
{
public:
    using SaveResult = StorageResult<Snapshot>;
    using MoveResult = StorageMoveResult<Snapshot>;
    using Store = typename Traits::Store;
    using PrecheckedWrite = typename Store::PrecheckedWrite;

    explicit ItemService(const CollectionService &collections, const VdirIo &vdirIo)
        : m_collections(collections)
        , m_items(collections, vdirIo)
        , m_collectionReloadSubscription(collections, [this](const QSet<QString> &changedCollectionIds) {
            clearCacheForCollections(changedCollectionIds);
        })
    {}

    SaveResult addObject(const Snapshot &snapshot, const QString &collectionId) const
    {
        SaveResult result;
        result.snapshot = snapshot;
        Snapshot object = Traits::objectForAdd(snapshot, collectionId);

        const std::optional<Collection> collection = m_items.collectionForWrite(collectionId);
        if (!Traits::isValidForWrite(object))
        {
            result.status = StorageStatus::Unsupported;
            return result;
        }
        if (!collection)
        {
            result.status = m_items.writeFailureStatus(collectionId);
            return result;
        }

        const std::shared_ptr<Repo> repository = repositoryFor(*collection);
        if (!repository->isValid())
        {
            result.status = StorageStatus::IoError;
            return result;
        }

        const SaveResult createResult = repository->addObject(object);
        if (!createResult.isOk())
        {
            Traits::logAddFailure(createResult.status);
            result.status = createResult.status;
            return result;
        }

        result.status = StorageStatus::Ok;
        result.snapshot = Traits::snapshotFromStored(createResult.snapshot, snapshot);
        invalidateDownstreamCaches(result.snapshot.ref);
        return result;
    }

    SaveResult updateObject(const Snapshot &snapshot) const
    {
        SaveResult result;
        result.snapshot = snapshot;

        const ItemRef ref = Traits::ref(snapshot);
        const PrecheckedWrite checked = m_items.precheckWrite(ref.collectionId, ref.href, ref.etag);
        if (!checked.isValid())
        {
            result.status = checked.status;
            return result;
        }
        if (!Traits::isValidForWrite(snapshot))
        {
            result.status = StorageStatus::Unsupported;
            return result;
        }

        const std::shared_ptr<Repo> repository = repositoryFor(*checked.collection);
        const typename Repo::ReadResult current = repository->readCurrentObject(checked.ref.href, checked.ref.etag);
        if (!current.isOk())
        {
            result.status = current.status;
            return result;
        }
        if (Traits::isUnchanged(snapshot, current.object))
        {
            result.status = StorageStatus::Ok;
            result.snapshot = Traits::snapshotFromStored(current.object, snapshot);
            return result;
        }

        const Snapshot object = Traits::objectForUpdate(snapshot, checked.ref);
        const SaveResult updateResult = repository->replaceObject(object);
        if (!updateResult.isOk())
        {
            Traits::logUpdateFailure(updateResult.status, checked.ref);
            result.status = updateResult.status;
            return result;
        }

        result.status = StorageStatus::Ok;
        result.snapshot = Traits::snapshotFromStored(updateResult.snapshot, snapshot);
        invalidateDownstreamCaches(ref);
        invalidateDownstreamCaches(result.snapshot.ref);
        return result;
    }

    MoveResult moveObject(const Snapshot &snapshot, const QString &destinationCollectionId) const
    {
        const ItemRef sourceRef = Traits::ref(snapshot);
        const QString targetCollectionId =
            destinationCollectionId.isEmpty() ? sourceRef.collectionId : destinationCollectionId;
        if (sourceRef.collectionId == targetCollectionId)
        {
            const SaveResult updateResult = updateObject(snapshot);
            MoveResult result;
            result.snapshot = updateResult.snapshot;
            result.move = MoveOutcome::update(UpdateOutcome{
                updateResult.status, sourceRef, updateResult.isOk() ? updateResult.snapshot.ref : sourceRef});
            return result;
        }

        const auto preconditionFailed = [&snapshot, &sourceRef, &targetCollectionId](StorageStatus status) {
            MoveResult result;
            result.snapshot = snapshot;
            ItemRef destination;
            destination.collectionId = targetCollectionId;
            result.move = MoveOutcome::preconditionFailed(status, sourceRef, destination);
            return result;
        };

        if (!Traits::isValidForWrite(snapshot))
        {
            return preconditionFailed(StorageStatus::Unsupported);
        }

        const PrecheckedWrite checkedSource =
            m_items.precheckWrite(sourceRef.collectionId, sourceRef.href, sourceRef.etag);
        if (!checkedSource.isValid())
        {
            return preconditionFailed(checkedSource.status);
        }

        const std::optional<Collection> destinationCollection = m_items.collectionForWrite(targetCollectionId);
        if (!destinationCollection)
        {
            return preconditionFailed(m_items.writeFailureStatus(targetCollectionId));
        }
        const std::shared_ptr<Repo> destinationRepository = repositoryFor(*destinationCollection);
        if (!destinationRepository->isValid())
        {
            return preconditionFailed(StorageStatus::IoError);
        }

        const Snapshot insertedObject = Traits::objectForMove(snapshot, targetCollectionId);
        const SaveResult createResult = destinationRepository->addObject(insertedObject);
        if (!createResult.isOk())
        {
            MoveResult result;
            result.snapshot = snapshot;
            result.move =
                MoveOutcome::destinationCreateFailed(createResult.status, checkedSource.ref, createResult.snapshot.ref);
            return result;
        }

        MoveResult result;
        result.snapshot = snapshot;
        result.move =
            m_items.commitCrossCollectionMove(checkedSource, destinationCollection, createResult.snapshot.ref);
        MoveOutcomeLog::logCrossVdirFailures(
            result.move, Traits::sourceRemoveFailureMessage(), Traits::rollbackFailureMessage());
        if (result.move.isOk())
        {
            result.snapshot = Traits::snapshotFromStored(createResult.snapshot, snapshot);
            invalidateDownstreamCaches(sourceRef);
            invalidateDownstreamCaches(result.snapshot.ref);
        }
        return result;
    }

    StorageStatus removeObject(const ItemRef &ref) const
    {
        const PrecheckedWrite checked = m_items.precheckWrite(ref.collectionId, ref.href, ref.etag);
        if (!checked.isValid())
        {
            return checked.status;
        }

        const StorageStatus removeStatus = m_items.commitRemove(checked);
        if (removeStatus != StorageStatus::Ok)
        {
            Traits::logRemoveFailure(removeStatus, checked.ref);
            return removeStatus;
        }
        invalidateDownstreamCaches(checked.ref);
        return StorageStatus::Ok;
    }

    QFuture<SaveResult> addObjectAsync(const Snapshot &snapshot, const QString &collectionId) const
    {
        if (!Traits::isValidForWrite(snapshot))
        {
            return CalendarIoUtils::completedFuture(saveFailure(StorageStatus::Unsupported, snapshot));
        }
        const std::optional<Collection> collection = m_items.collectionForWrite(collectionId);
        if (!collection)
        {
            return CalendarIoUtils::completedFuture(saveFailure(m_items.writeFailureStatus(collectionId), snapshot));
        }
        const VdirPath path = VdirPath::fromCollection(*collection);
        if (!path.isValid())
        {
            return CalendarIoUtils::completedFuture(saveFailure(StorageStatus::IoError, snapshot));
        }
        return m_items.vdirIo().submit(path, Traits::addLabel(), saveIoFailure(), [this, snapshot, collectionId] {
            return addObject(snapshot, collectionId);
        });
    }

    QFuture<SaveResult> updateObjectAsync(const Snapshot &snapshot) const
    {
        const ItemRef ref = Traits::ref(snapshot);
        const std::optional<Collection> collection = m_items.collectionForRead(ref.collectionId);
        if (!collection)
        {
            return CalendarIoUtils::completedFuture(saveFailure(StorageStatus::NotFound, snapshot));
        }
        const VdirPath path = VdirPath::fromCollection(*collection);
        if (!path.isValid())
        {
            return CalendarIoUtils::completedFuture(saveFailure(StorageStatus::IoError, snapshot));
        }
        return m_items.vdirIo().submit(
            path, Traits::updateLabel(), saveIoFailure(), [this, snapshot] { return updateObject(snapshot); });
    }

    QFuture<MoveResult> moveObjectAsync(const Snapshot &snapshot, const QString &destinationCollectionId) const
    {
        const ItemRef ref = Traits::ref(snapshot);
        const QString targetCollectionId =
            destinationCollectionId.isEmpty() ? ref.collectionId : destinationCollectionId;
        const std::optional<Collection> source = m_items.collectionForRead(ref.collectionId);
        if (!source)
        {
            return CalendarIoUtils::completedFuture(moveFailure(StorageStatus::NotFound, snapshot, targetCollectionId));
        }

        if (ref.collectionId == targetCollectionId)
        {
            const VdirPath path = VdirPath::fromCollection(*source);
            if (!path.isValid())
            {
                return CalendarIoUtils::completedFuture(
                    moveFailure(StorageStatus::IoError, snapshot, targetCollectionId));
            }
            return m_items.vdirIo().submit(
                path, Traits::sameCollectionMoveLabel(), moveIoFailure(), [this, snapshot, targetCollectionId] {
                    return moveObject(snapshot, targetCollectionId);
                });
        }

        const std::optional<Collection> destination = m_items.collectionForWrite(targetCollectionId);
        if (!destination)
        {
            return CalendarIoUtils::completedFuture(
                moveFailure(m_items.writeFailureStatus(targetCollectionId), snapshot, targetCollectionId));
        }
        const VdirPath sourcePath = VdirPath::fromCollection(*source);
        const VdirPath destinationPath = VdirPath::fromCollection(*destination);
        return m_items.vdirIo().submitComposite(
            {sourcePath, destinationPath},
            Traits::crossCollectionMoveLabel(),
            moveIoFailure(),
            [this, snapshot, targetCollectionId] { return moveObject(snapshot, targetCollectionId); });
    }

    QFuture<StorageStatus> removeObjectAsync(const ItemRef &ref) const
    {
        const std::optional<Collection> collection = m_items.collectionForRead(ref.collectionId);
        if (!collection)
        {
            return CalendarIoUtils::completedFuture(StorageStatus::NotFound);
        }
        const VdirPath path = VdirPath::fromCollection(*collection);
        if (!path.isValid())
        {
            return CalendarIoUtils::completedFuture(StorageStatus::IoError);
        }
        return m_items.vdirIo().submit(
            path, Traits::removeLabel(), StorageStatus::IoError, [this, ref] { return removeObject(ref); });
    }

    void clearCache() const { m_items.clearCache(); }

    void clearCacheForCollections(const QSet<QString> &collectionIds) const
    {
        m_items.clearCacheForCollections(collectionIds);
    }

    void retainRepositoriesForCollections(const QList<Collection> &collections) const
    {
        m_items.retainRepositoriesForCollections(collections);
    }

protected:
    virtual ~ItemService() = default;
    virtual void invalidateDownstreamCaches(const ItemRef &) const {}

    const CollectionService &collections() const { return m_collections; }
    const VdirIo &scheduler() const { return m_items.vdirIo(); }

    std::shared_ptr<Repo> repositoryFor(const QString &collectionId) const
    {
        const std::optional<Collection> collection = m_items.collectionForRead(collectionId);
        if (!collection)
        {
            return {};
        }
        return repositoryFor(*collection);
    }

    std::shared_ptr<Repo> repositoryFor(const Collection &collection) const
    {
        return m_items.repositoryForCollection(storageCollectionRefForCollection(collection));
    }

    std::shared_ptr<Repo> repositoryFor(const StorageCollectionRef &collection) const
    {
        return m_items.repositoryForCollection(collection);
    }

    Store &store() { return m_items; }
    const Store &store() const { return m_items; }

private:
    static SaveResult saveFailure(StorageStatus status, const Snapshot &snapshot)
    {
        SaveResult result;
        result.status = status;
        result.snapshot = snapshot;
        return result;
    }

    static SaveResult saveIoFailure()
    {
        SaveResult result;
        result.status = StorageStatus::IoError;
        return result;
    }

    static MoveResult
    moveFailure(StorageStatus status, const Snapshot &snapshot, const QString &destinationCollectionId)
    {
        MoveResult result;
        result.snapshot = snapshot;
        ItemRef destination;
        destination.collectionId = destinationCollectionId;
        result.move = MoveOutcome::preconditionFailed(status, Traits::ref(snapshot), destination);
        return result;
    }

    static MoveResult moveIoFailure()
    {
        MoveResult result;
        result.move = MoveOutcome::preconditionFailed(StorageStatus::IoError, {}, {});
        return result;
    }

    const CollectionService &m_collections;
    Store m_items;
    CollectionReloadSubscription m_collectionReloadSubscription;
};

#endif // ITEMSERVICE_H
