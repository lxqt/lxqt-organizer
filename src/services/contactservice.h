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

#ifndef CONTACTSERVICE_H
#define CONTACTSERVICE_H

#include "cancellationtoken.h"
#include "collectionservice.h"
#include "contactitemstore.h"
#include "contactsnapshot.h"
#include "itemservice.h"
#include "itemidentity.h"
#include "operationcapability.h"
#include "storageresult.h"

#include <QList>
#include <QFuture>
#include <QSet>
#include <QString>

#include <functional>
#include <memory>

class ContactRepository;
class VdirIo;

struct ContactSnapshotListResult
{
    QList<Contact> contacts;
    QList<ReadFailure> readFailures;
};

struct ContactServiceTraits
{
    using Store = ContactItemStore;
    static constexpr CollectionKind kind = CollectionKind::AddressBook;

    static ItemRef ref(const Contact &contact);
    static bool isValidForWrite(const Contact &contact);
    static bool isUnchanged(const Contact &contact, const Contact &stored);
    static Contact objectForAdd(const Contact &contact, const QString &collectionId);
    static Contact objectForUpdate(const Contact &contact, const ItemRef &ref);
    static Contact objectForMove(const Contact &contact, const QString &destinationCollectionId);
    static Contact snapshotFromStored(const Contact &stored, const Contact &fallback);
    static QString addLabel();
    static QString updateLabel();
    static QString sameCollectionMoveLabel();
    static QString crossCollectionMoveLabel();
    static QString removeLabel();
    static QString sourceRemoveFailureMessage();
    static QString rollbackFailureMessage();
    static void logAddFailure(StorageStatus status);
    static void logUpdateFailure(StorageStatus status, const ItemRef &ref);
    static void logRemoveFailure(StorageStatus status, const ItemRef &ref);
};

// @thread any-thread; caches live behind store mutexes and writes use VdirIo.
class ContactService : private ItemService<Contact, ContactRepository, ContactServiceTraits>
{
    using Base = ItemService<Contact, ContactRepository, ContactServiceTraits>;

public:
    using ContactSaveResult = Base::SaveResult;
    using ContactMoveResult = Base::MoveResult;

    explicit ContactService(const CollectionService &collections, const VdirIo &vdirIo);
    ~ContactService();

    ContactSaveResult addContact(const Contact &contact, const QString &collectionId = QString()) const;
    ContactSaveResult updateContact(const Contact &contact) const;
    ContactMoveResult moveContact(const Contact &contact, const QString &destinationCollectionId) const;
    StorageStatus deleteContact(const ItemRef &storage) const;
    StorageStatus deleteContact(const Contact &contact) const;
    OperationCapability canCreateContact(const QString &collectionId) const;
    OperationCapability canEditContact(const Contact &contact) const;
    OperationCapability canMoveContact(const Contact &contact, const QString &destinationCollectionId) const;
    OperationCapability canDeleteContact(const Contact &contact) const;
    QFuture<ContactSnapshotListResult> contactSnapshotsAsync(const CancellationToken &cancellation = {}) const;
    QList<Collection> addressBookCollections() const;
    StorageStatus removeContact(const ItemRef &storage) const;
    StorageStatus removeContact(const Contact &contact) const;
    // Async methods submit storage work directly to VdirIo.
    QFuture<ContactSaveResult> addContactAsync(const Contact &contact, const QString &collectionId = QString()) const;
    QFuture<ContactSaveResult> updateContactAsync(const Contact &contact) const;
    QFuture<ContactMoveResult> moveContactAsync(const Contact &contact, const QString &destinationCollectionId) const;
    QFuture<StorageStatus> deleteContactAsync(const ItemRef &storage) const;
    QFuture<StorageStatus> deleteContactAsync(const Contact &contact) const;
    QFuture<StorageStatus> removeContactAsync(const ItemRef &storage) const;
    QFuture<StorageStatus> removeContactAsync(const Contact &contact) const;
    void clearCache() const;
    void clearCacheForCollections(const QSet<QString> &collectionIds) const;
    void retainRepositoriesForCollections(const QList<Collection> &collections) const;

private:
    void invalidateDownstreamCaches(const ItemRef &ref) const override;
};

#endif // CONTACTSERVICE_H
