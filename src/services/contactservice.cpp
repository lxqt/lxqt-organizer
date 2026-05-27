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

#include "completedfuture.h"
#include "contactrepository.h"
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

} // namespace

ItemRef ContactServiceTraits::ref(const Contact &contact)
{
    return contact.ref;
}

bool ContactServiceTraits::isValidForWrite(const Contact &contact)
{
    return contact.addressee && !contact.addressee->isEmpty();
}

bool ContactServiceTraits::isUnchanged(const Contact &contact, const Contact &stored)
{
    return contact.addressee && stored.addressee && *contact.addressee == *stored.addressee;
}

Contact ContactServiceTraits::objectForAdd(const Contact &contact, const QString &collectionId)
{
    Contact object;
    object.ref.collectionId = collectionId;
    object.uid = contact.uid;
    object.addressee = contact.addressee;
    return object;
}

Contact ContactServiceTraits::objectForUpdate(const Contact &contact, const ItemRef &ref)
{
    Contact object;
    object.ref = ref;
    object.uid = contact.uid;
    object.addressee = contact.addressee;
    return object;
}

Contact ContactServiceTraits::objectForMove(const Contact &contact, const QString &destinationCollectionId)
{
    return objectForAdd(contact, destinationCollectionId);
}

Contact ContactServiceTraits::snapshotFromStored(const Contact &stored, const Contact &)
{
    return Contact{stored.ref, stored.uid, stored.addressee};
}

QString ContactServiceTraits::addLabel()
{
    return QStringLiteral("contact add");
}

QString ContactServiceTraits::updateLabel()
{
    return QStringLiteral("contact update");
}

QString ContactServiceTraits::sameCollectionMoveLabel()
{
    return QStringLiteral("contact same-collection move");
}

QString ContactServiceTraits::crossCollectionMoveLabel()
{
    return QStringLiteral("contact cross-collection move");
}

QString ContactServiceTraits::removeLabel()
{
    return QStringLiteral("contact remove");
}

QString ContactServiceTraits::sourceRemoveFailureMessage()
{
    return QStringLiteral("Could not remove moved contact item");
}

QString ContactServiceTraits::rollbackFailureMessage()
{
    return QStringLiteral("Could not roll back moved contact item");
}

void ContactServiceTraits::logAddFailure(StorageStatus status)
{
    qCWarning(storageLog) << "Could not add contact item" << storageStatusName(status);
}

void ContactServiceTraits::logUpdateFailure(StorageStatus status, const ItemRef &ref)
{
    qCWarning(storageLog) << "Could not update contact item" << ref.href << storageStatusName(status);
}

void ContactServiceTraits::logRemoveFailure(StorageStatus status, const ItemRef &ref)
{
    qCWarning(storageLog) << "Could not remove contact item" << ref.href << storageStatusName(status);
}

ContactService::ContactService(const CollectionService &collections, const VdirIo &vdirIo)
    : Base(collections, vdirIo)
{}

ContactService::~ContactService() = default;

ContactService::ContactSaveResult ContactService::addContact(const Contact &contact, const QString &collectionId) const
{
    return addObject(contact, collectionId);
}

ContactService::ContactSaveResult ContactService::updateContact(const Contact &contact) const
{
    return updateObject(contact);
}

ContactService::ContactMoveResult ContactService::moveContact(const Contact &contact,
                                                              const QString &destinationCollectionId) const
{
    return moveObject(contact, destinationCollectionId);
}

OperationCapability ContactService::canCreateContact(const QString &collectionId) const
{
    return capabilityForWritableCollection(
        collections(), CollectionKind::AddressBook, collectionId, OperationCapabilityStatus::DestinationReadOnly);
}

OperationCapability ContactService::canEditContact(const Contact &contact) const
{
    if (!contact.ref.isValid() || !ContactServiceTraits::isValidForWrite(contact) || contactDisplay(contact).isEmpty)
    {
        return invalidSelectionCapability(contact.ref);
    }
    return capabilityForWritableCollection(collections(),
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

    return capabilityForWritableCollection(collections(),
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
    const VdirIo &vdirIo = scheduler();
    for (const Collection &collection : collections().addressBookList())
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
    return collections().addressBookList();
}

StorageStatus ContactService::deleteContact(const Contact &contact) const
{
    return deleteContact(contact.ref);
}

StorageStatus ContactService::deleteContact(const ItemRef &storage) const
{
    return removeObject(storage);
}

StorageStatus ContactService::removeContact(const Contact &contact) const
{
    return deleteContact(contact);
}

StorageStatus ContactService::removeContact(const ItemRef &storage) const
{
    return deleteContact(storage);
}

QFuture<ContactService::ContactSaveResult> ContactService::addContactAsync(const Contact &contact,
                                                                           const QString &collectionId) const
{
    return addObjectAsync(contact, collectionId);
}

QFuture<ContactService::ContactSaveResult> ContactService::updateContactAsync(const Contact &contact) const
{
    return updateObjectAsync(contact);
}

QFuture<ContactService::ContactMoveResult>
ContactService::moveContactAsync(const Contact &contact, const QString &destinationCollectionId) const
{
    return moveObjectAsync(contact, destinationCollectionId);
}

QFuture<StorageStatus> ContactService::deleteContactAsync(const ItemRef &storage) const
{
    return removeObjectAsync(storage);
}

QFuture<StorageStatus> ContactService::deleteContactAsync(const Contact &contact) const
{
    return deleteContactAsync(contact.ref);
}

QFuture<StorageStatus> ContactService::removeContactAsync(const ItemRef &storage) const
{
    return deleteContactAsync(storage);
}

QFuture<StorageStatus> ContactService::removeContactAsync(const Contact &contact) const
{
    return deleteContactAsync(contact);
}

void ContactService::clearCache() const
{
    Base::clearCache();
}

void ContactService::clearCacheForCollections(const QSet<QString> &collectionIds) const
{
    Base::clearCacheForCollections(collectionIds);
}

void ContactService::retainRepositoriesForCollections(const QList<Collection> &collections) const
{
    Base::retainRepositoriesForCollections(collections);
}

void ContactService::invalidateDownstreamCaches(const ItemRef &ref) const
{
    collections().notifyItemWritten(ref);
}
