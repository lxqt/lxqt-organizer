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

#ifndef CONTACTREPOSITORY_H
#define CONTACTREPOSITORY_H

#include "contactsnapshot.h"
#include "storagecollectionref.h"
#include "storageresult.h"
#include "vcardcodec.h"
#include "vdiritemrepository.h"

#include <QList>
#include <QString>

#include <functional>

#include <KContacts/Addressee>

// @thread vdir-worker; repository state is owned by the path scheduler.
class ContactRepository
{
public:
    using ReadResult = ::ReadResult<Contact>;

    explicit ContactRepository(const StorageCollectionRef &collection, const VdirIo &vdirIo);

    StorageCollectionRef collection() const;
    void updateCollection(const StorageCollectionRef &collection);
    bool isValid() const;
    StorageStatus checkCurrentItem(const QString &href, const QString &expectedEtag, ItemRef *storage = nullptr) const;
    ReadResult readCurrentObject(const QString &href, const QString &expectedEtag) const;
    void forEachObject(const std::function<bool(const ReadResult &)> &visitor,
                       const std::function<bool()> &stopRequested = {}) const;
    QList<ReadResult> readObjects() const;
    StorageStatus remove(const ItemRef &storage) const;
    StorageResult<Contact> addObject(Contact object) const;
    StorageResult<Contact> replaceObject(Contact object) const;

private:
    QString hrefForUid(const QString &uid) const;
    StorageStatus create(const QString &href, const QString &text, QString *etag = nullptr) const;
    StorageStatus
    replace(const QString &href, const QString &text, const QString &expectedEtag, QString *newEtag = nullptr) const;
    StorageStatus createContact(const QString &href,
                                const KContacts::Addressee &addressee,
                                QString *etag,
                                KContacts::Addressee *parsedAddressee) const;
    StorageStatus replaceContact(const QString &href,
                                 const KContacts::Addressee &addressee,
                                 const QString &expectedEtag,
                                 QString *newEtag,
                                 KContacts::Addressee *parsedAddressee) const;
    VdirItemRepository m_items;
};

#endif // CONTACTREPOSITORY_H
