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
#include "collectionreloadsubscription.h"
#include "collectionservice.h"
#include "contactsnapshot.h"
#include "contactstore.h"
#include "itemidentity.h"
#include "operationcapability.h"
#include "storageresult.h"

#include <QFuture>
#include <QList>
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

// @thread any-thread; caches live behind store mutexes and writes use VdirIo.
class ContactService
{
public:
    using ContactSaveResult = StorageResult<Contact>;
    using ContactMoveResult = StorageMoveResult<Contact>;

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
    // Async methods submit storage work directly to VdirIo.
    QFuture<ContactSaveResult> addContactAsync(const Contact &contact, const QString &collectionId = QString()) const;
    QFuture<ContactSaveResult> updateContactAsync(const Contact &contact) const;
    QFuture<ContactMoveResult> moveContactAsync(const Contact &contact, const QString &destinationCollectionId) const;
    QFuture<StorageStatus> deleteContactAsync(const ItemRef &storage) const;
    QFuture<StorageStatus> deleteContactAsync(const Contact &contact) const;
    void clearCache() const;
    void clearCacheForCollections(const QSet<QString> &collectionIds) const;
    void retainRepositoriesForCollections(const QList<Collection> &collections) const;

private:
    std::shared_ptr<ContactRepository> repositoryFor(const QString &collectionId) const;
    std::shared_ptr<ContactRepository> repositoryFor(const Collection &collection) const;
    void invalidateDownstreamCaches(const ItemRef &ref) const;

    const CollectionService &m_collections;
    ContactStore m_items;
    // Declared last so destruction disconnects the reload handler before the
    // store it captures is torn down.
    CollectionReloadSubscription m_collectionReloadSubscription;
};

#endif // CONTACTSERVICE_H
