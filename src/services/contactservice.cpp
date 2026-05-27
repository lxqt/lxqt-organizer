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

#include "contactservice.h"

#include "asyncsubmit.h"
#include "completedfuture.h"
#include "contactrepository.h"
#include "moveoutcomelog.h"
#include "storagecollectionref.h"
#include "storagelog.h"
#include "vdirio.h"

#include <QDebug>

namespace {

ReadFailure readFailureForContactItem(const QString &collectionId, const ContactRepository::ReadResult &readResult)
{
    ItemRef storage = readResult.object.ref;
    if (storage.collectionId.isEmpty())
    {
        storage.collectionId = collectionId;
    }
    return ReadFailure{storage, readResult.status};
}

ContactSnapshotListResult collectContactSnapshotsForCollection(const ContactRepository &repository,
                                                               const QString &collectionId,
                                                               const CancellationToken &cancellation)
{
    ContactSnapshotListResult result;
    bool keepGoing = true;
    repository.forEachObject(
        [&](const ContactRepository::ReadResult &readResult) {
            if (!readResult.isOk())
            {
                qCWarning(storageLog) << "Skipping contact item" << collectionId << readResult.object.ref.href
                                      << storageStatusName(readResult.status);
                result.readFailures.append(readFailureForContactItem(collectionId, readResult));
                return true;
            }
            if (cancellation.isCancellationRequested())
            {
                keepGoing = false;
                return false;
            }
            const Contact &object = readResult.object;
            result.contacts.append(Contact{object.ref, object.uid, object.addressee});
            return true;
        },
        [&cancellation, &keepGoing]() { return !keepGoing || cancellation.isCancellationRequested(); });
    return result;
}

bool isValidContactForWrite(const Contact &contact)
{
    return contact.addressee && !contact.addressee->isEmpty();
}

bool isUnchangedContact(const Contact &contact, const Contact &stored)
{
    return contact.addressee && stored.addressee && *contact.addressee == *stored.addressee;
}

Contact contactForAdd(const Contact &contact, const QString &collectionId)
{
    Contact object;
    object.ref.collectionId = collectionId;
    object.uid = contact.uid;
    object.addressee = contact.addressee;
    return object;
}

Contact contactForUpdate(const Contact &contact, const ItemRef &ref)
{
    Contact object;
    object.ref = ref;
    object.uid = contact.uid;
    object.addressee = contact.addressee;
    return object;
}

Contact snapshotFromStoredContact(const Contact &stored)
{
    return Contact{stored.ref, stored.uid, stored.addressee};
}

ContactService::ContactSaveResult saveFailure(StorageStatus status, const Contact &snapshot)
{
    ContactService::ContactSaveResult result;
    result.status = status;
    result.snapshot = snapshot;
    return result;
}

ContactService::ContactSaveResult saveIoFailure()
{
    ContactService::ContactSaveResult result;
    result.status = StorageStatus::IoError;
    return result;
}

ContactService::ContactMoveResult
moveFailure(StorageStatus status, const Contact &snapshot, const QString &destinationCollectionId)
{
    ContactService::ContactMoveResult result;
    result.snapshot = snapshot;
    ItemRef destination;
    destination.collectionId = destinationCollectionId;
    result.move = MoveOutcome::preconditionFailed(status, snapshot.ref, destination);
    return result;
}

} // namespace

ContactService::ContactService(const CollectionService &collections, const VdirIo &vdirIo)
    : m_collections(collections)
    , m_items(collections, vdirIo)
    , m_collectionReloadSubscription(collections, [this](const QSet<QString> &changedCollectionIds) {
        clearCacheForCollections(changedCollectionIds);
    })
{}

ContactService::~ContactService() = default;

std::shared_ptr<ContactRepository> ContactService::repositoryFor(const QString &collectionId) const
{
    const std::optional<Collection> collection = m_items.collectionForRead(collectionId);
    if (!collection)
    {
        return {};
    }
    return repositoryFor(*collection);
}

std::shared_ptr<ContactRepository> ContactService::repositoryFor(const Collection &collection) const
{
    return m_items.repositoryForCollection(storageCollectionRefForCollection(collection));
}

void ContactService::invalidateDownstreamCaches(const ItemRef &ref) const
{
    m_collections.notifyItemWritten(ref);
}

ContactService::ContactSaveResult ContactService::addContact(const Contact &contact, const QString &collectionId) const
{
    ContactSaveResult result;
    result.snapshot = contact;
    Contact object = contactForAdd(contact, collectionId);

    const std::optional<Collection> collection = m_items.collectionForWrite(collectionId);
    if (!isValidContactForWrite(object))
    {
        result.status = StorageStatus::Unsupported;
        return result;
    }
    if (!collection)
    {
        result.status = m_items.writeFailureStatus(collectionId);
        return result;
    }

    const std::shared_ptr<ContactRepository> repository = repositoryFor(*collection);
    if (!repository->isValid())
    {
        result.status = StorageStatus::IoError;
        return result;
    }

    const ContactSaveResult createResult = repository->addObject(object);
    if (!createResult.isOk())
    {
        qCWarning(storageLog) << "Could not add contact item" << storageStatusName(createResult.status);
        result.status = createResult.status;
        return result;
    }

    result.status = StorageStatus::Ok;
    result.snapshot = snapshotFromStoredContact(createResult.snapshot);
    invalidateDownstreamCaches(result.snapshot.ref);
    return result;
}

ContactService::ContactSaveResult ContactService::updateContact(const Contact &contact) const
{
    ContactSaveResult result;
    result.snapshot = contact;

    const ItemRef ref = contact.ref;
    const ContactStore::PrecheckedWrite checked = m_items.precheckWrite(ref.collectionId, ref.href, ref.etag);
    if (!checked.isValid())
    {
        result.status = checked.status;
        return result;
    }
    if (!isValidContactForWrite(contact))
    {
        result.status = StorageStatus::Unsupported;
        return result;
    }

    const std::shared_ptr<ContactRepository> repository = m_items.repositoryForCollection(*checked.collection);
    const ContactRepository::ReadResult current = repository->readCurrentObject(checked.ref.href, checked.ref.etag);
    if (!current.isOk())
    {
        result.status = current.status;
        return result;
    }
    if (isUnchangedContact(contact, current.object))
    {
        result.status = StorageStatus::Ok;
        result.snapshot = snapshotFromStoredContact(current.object);
        return result;
    }

    const Contact object = contactForUpdate(contact, checked.ref);
    const ContactSaveResult updateResult = repository->replaceObject(object);
    if (!updateResult.isOk())
    {
        qCWarning(storageLog) << "Could not update contact item" << checked.ref.href
                              << storageStatusName(updateResult.status);
        result.status = updateResult.status;
        return result;
    }

    result.status = StorageStatus::Ok;
    result.snapshot = snapshotFromStoredContact(updateResult.snapshot);
    invalidateDownstreamCaches(ref);
    invalidateDownstreamCaches(result.snapshot.ref);
    return result;
}

ContactService::ContactMoveResult ContactService::moveContact(const Contact &contact,
                                                              const QString &destinationCollectionId) const
{
    const ItemRef sourceRef = contact.ref;
    const QString targetCollectionId =
        destinationCollectionId.isEmpty() ? sourceRef.collectionId : destinationCollectionId;
    if (sourceRef.collectionId == targetCollectionId)
    {
        const ContactSaveResult updateResult = updateContact(contact);
        ContactMoveResult result;
        result.snapshot = updateResult.snapshot;
        result.move = MoveOutcome::update(
            UpdateOutcome{updateResult.status, sourceRef, updateResult.isOk() ? updateResult.snapshot.ref : sourceRef});
        return result;
    }

    const auto preconditionFailed = [&contact, &sourceRef, &targetCollectionId](StorageStatus status) {
        ContactMoveResult result;
        result.snapshot = contact;
        ItemRef destination;
        destination.collectionId = targetCollectionId;
        result.move = MoveOutcome::preconditionFailed(status, sourceRef, destination);
        return result;
    };

    if (!isValidContactForWrite(contact))
    {
        return preconditionFailed(StorageStatus::Unsupported);
    }

    const ContactStore::PrecheckedWrite checkedSource =
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
    const std::shared_ptr<ContactRepository> destinationRepository = repositoryFor(*destinationCollection);
    if (!destinationRepository->isValid())
    {
        return preconditionFailed(StorageStatus::IoError);
    }

    const Contact insertedObject = contactForAdd(contact, targetCollectionId);
    const ContactSaveResult createResult = destinationRepository->addObject(insertedObject);
    if (!createResult.isOk())
    {
        ContactMoveResult result;
        result.snapshot = contact;
        result.move =
            MoveOutcome::destinationCreateFailed(createResult.status, checkedSource.ref, createResult.snapshot.ref);
        return result;
    }

    ContactMoveResult result;
    result.snapshot = contact;
    result.move = m_items.commitCrossCollectionMove(checkedSource, destinationCollection, createResult.snapshot.ref);
    MoveOutcomeLog::logCrossVdirFailures(result.move,
                                         QStringLiteral("Could not remove moved contact item"),
                                         QStringLiteral("Could not roll back moved contact item"));
    if (result.move.isOk())
    {
        result.snapshot = snapshotFromStoredContact(createResult.snapshot);
        invalidateDownstreamCaches(sourceRef);
        invalidateDownstreamCaches(result.snapshot.ref);
    }
    return result;
}

StorageStatus ContactService::deleteContact(const ItemRef &storage) const
{
    const ContactStore::PrecheckedWrite checked =
        m_items.precheckWrite(storage.collectionId, storage.href, storage.etag);
    if (!checked.isValid())
    {
        return checked.status;
    }

    const StorageStatus removeStatus = m_items.commitRemove(checked);
    if (removeStatus != StorageStatus::Ok)
    {
        qCWarning(storageLog) << "Could not remove contact item" << checked.ref.href << storageStatusName(removeStatus);
        return removeStatus;
    }
    invalidateDownstreamCaches(checked.ref);
    return StorageStatus::Ok;
}

StorageStatus ContactService::deleteContact(const Contact &contact) const
{
    return deleteContact(contact.ref);
}

QFuture<ContactService::ContactSaveResult> ContactService::addContactAsync(const Contact &contact,
                                                                           const QString &collectionId) const
{
    if (!isValidContactForWrite(contact))
    {
        return CalendarIoUtils::completedFuture(saveFailure(StorageStatus::Unsupported, contact));
    }
    const std::optional<Collection> collection = m_items.collectionForWrite(collectionId);
    if (!collection)
    {
        return CalendarIoUtils::completedFuture(saveFailure(m_items.writeFailureStatus(collectionId), contact));
    }
    const VdirPath path = VdirPath::fromCollection(*collection);
    if (!path.isValid())
    {
        return CalendarIoUtils::completedFuture(saveFailure(StorageStatus::IoError, contact));
    }
    return m_items.vdirIo().submit(path, QStringLiteral("contact add"), saveIoFailure(), [this, contact, collectionId] {
        return addContact(contact, collectionId);
    });
}

QFuture<ContactService::ContactSaveResult> ContactService::updateContactAsync(const Contact &contact) const
{
    const ItemRef ref = contact.ref;
    const std::optional<Collection> collection = m_items.collectionForRead(ref.collectionId);
    if (!collection)
    {
        return CalendarIoUtils::completedFuture(saveFailure(StorageStatus::NotFound, contact));
    }
    const VdirPath path = VdirPath::fromCollection(*collection);
    if (!path.isValid())
    {
        return CalendarIoUtils::completedFuture(saveFailure(StorageStatus::IoError, contact));
    }
    return m_items.vdirIo().submit(
        path, QStringLiteral("contact update"), saveIoFailure(), [this, contact] { return updateContact(contact); });
}

QFuture<ContactService::ContactMoveResult>
ContactService::moveContactAsync(const Contact &contact, const QString &destinationCollectionId) const
{
    const ItemRef ref = contact.ref;
    const QString targetCollectionId = destinationCollectionId.isEmpty() ? ref.collectionId : destinationCollectionId;
    return AsyncSubmit::submitMove<ContactMoveResult>(
        m_items.vdirIo(),
        QStringLiteral("contact move"),
        targetCollectionId,
        [this, ref] { return m_items.collectionForRead(ref.collectionId); },
        [this, targetCollectionId] {
            return AsyncSubmit::ResolvedCollection{m_items.collectionForWrite(targetCollectionId),
                                                   m_items.writeFailureStatus(targetCollectionId)};
        },
        [this, contact, targetCollectionId] { return moveContact(contact, targetCollectionId); },
        [contact, targetCollectionId](StorageStatus status) {
            return moveFailure(status, contact, targetCollectionId);
        });
}

QFuture<StorageStatus> ContactService::deleteContactAsync(const ItemRef &storage) const
{
    const std::optional<Collection> collection = m_items.collectionForRead(storage.collectionId);
    if (!collection)
    {
        return CalendarIoUtils::completedFuture(StorageStatus::NotFound);
    }
    const VdirPath path = VdirPath::fromCollection(*collection);
    if (!path.isValid())
    {
        return CalendarIoUtils::completedFuture(StorageStatus::IoError);
    }
    return m_items.vdirIo().submit(path, QStringLiteral("contact remove"), StorageStatus::IoError, [this, storage] {
        return deleteContact(storage);
    });
}

QFuture<StorageStatus> ContactService::deleteContactAsync(const Contact &contact) const
{
    return deleteContactAsync(contact.ref);
}

void ContactService::clearCache() const
{
    m_items.clearCache();
}

void ContactService::clearCacheForCollections(const QSet<QString> &collectionIds) const
{
    m_items.clearCacheForCollections(collectionIds);
}

void ContactService::retainRepositoriesForCollections(const QList<Collection> &collections) const
{
    m_items.retainRepositoriesForCollections(collections);
}

OperationCapability ContactService::canCreateContact(const QString &collectionId) const
{
    return capabilityForWritableCollection(
        m_collections, CollectionKind::AddressBook, collectionId, OperationCapabilityStatus::DestinationReadOnly);
}

OperationCapability ContactService::canEditContact(const Contact &contact) const
{
    if (!contact.ref.isValid() || !isValidContactForWrite(contact) || contactDisplay(contact).isEmpty)
    {
        return invalidSelectionCapability(contact.ref);
    }
    return capabilityForWritableCollection(m_collections,
                                           CollectionKind::AddressBook,
                                           contact.ref.collectionId,
                                           OperationCapabilityStatus::SourceReadOnly,
                                           contact.ref);
}

OperationCapability ContactService::canMoveContact(const Contact &contact, const QString &destinationCollectionId) const
{
    OperationCapability sourceCapability = canEditContact(contact);
    if (!sourceCapability.isAllowed())
    {
        return sourceCapability;
    }

    const QString targetCollectionId =
        destinationCollectionId.isEmpty() ? contact.ref.collectionId : destinationCollectionId;
    if (targetCollectionId == contact.ref.collectionId)
    {
        sourceCapability.destinationCollectionId = targetCollectionId;
        return sourceCapability;
    }

    return capabilityForWritableCollection(m_collections,
                                           CollectionKind::AddressBook,
                                           targetCollectionId,
                                           OperationCapabilityStatus::DestinationReadOnly,
                                           contact.ref,
                                           targetCollectionId);
}

OperationCapability ContactService::canDeleteContact(const Contact &contact) const
{
    return canEditContact(contact);
}

QFuture<ContactSnapshotListResult> ContactService::contactSnapshotsAsync(const CancellationToken &cancellation) const
{
    QList<QFuture<ContactSnapshotListResult>> perCollectionFutures;
    const VdirIo &vdirIo = m_items.vdirIo();
    for (const Collection &collection : m_collections.addressBookList())
    {
        if (cancellation.isCancellationRequested())
        {
            break;
        }
        const std::shared_ptr<ContactRepository> repository = repositoryFor(collection);
        if (!repository->isValid())
        {
            continue;
        }
        const VdirPath path = VdirPath::fromCollection(collection);
        if (!path.isValid())
        {
            continue;
        }
        const QString collectionId = collection.id;
        perCollectionFutures.append(vdirIo.submit(path,
                                                  QStringLiteral("contact snapshots"),
                                                  ContactSnapshotListResult{},
                                                  [repository, collectionId, cancellation] {
                                                      return collectContactSnapshotsForCollection(
                                                          *repository, collectionId, cancellation);
                                                  }));
    }
    if (perCollectionFutures.isEmpty())
    {
        return CalendarIoUtils::completedFuture(ContactSnapshotListResult{});
    }
    return QtFuture::whenAll(perCollectionFutures.begin(), perCollectionFutures.end())
        .then(QtFuture::Launch::Sync, [](const QList<QFuture<ContactSnapshotListResult>> &done) {
            ContactSnapshotListResult aggregate;
            for (const QFuture<ContactSnapshotListResult> &future : done)
            {
                ContactSnapshotListResult perCollection = future.result();
                aggregate.contacts.append(std::move(perCollection.contacts));
                aggregate.readFailures.append(std::move(perCollection.readFailures));
            }
            return aggregate;
        });
}

QList<Collection> ContactService::addressBookCollections() const
{
    return m_collections.addressBookList();
}
