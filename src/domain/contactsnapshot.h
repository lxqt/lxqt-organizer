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

#ifndef CONTACTSNAPSHOT_H
#define CONTACTSNAPSHOT_H

#include "collectionsummary.h"
#include "itemidentity.h"

#include <QSharedPointer>
#include <QString>

namespace KContacts {
class Addressee;
}

struct Contact
{
    using AddresseePtr = QSharedPointer<const KContacts::Addressee>;

    ItemRef ref;
    QString uid;
    AddresseePtr addressee;
};

Contact::AddresseePtr addresseeSnapshot(const KContacts::Addressee &addressee);

struct ContactDisplay
{
    CollectionSummary collection;
    QString givenName;
    QString additionalName;
    QString familyName;
    QString preferredEmail;
    QString street;
    QString district;
    QString city;
    QString county;
    QString postcode;
    QString country;
    QString phoneNumber;
    bool isEmpty = true;
};

ContactDisplay contactDisplay(const Contact &contact);
ContactDisplay contactDisplay(const Contact &contact, const CollectionSummary &collection);

#endif // CONTACTSNAPSHOT_H
