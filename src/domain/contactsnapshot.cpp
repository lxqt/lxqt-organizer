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

#include "contactsnapshot.h"

#include "contactutils.h"

#include <KContacts/Address>
#include <KContacts/Addressee>
#include <KContacts/PhoneNumber>

Contact::AddresseePtr addresseeSnapshot(const KContacts::Addressee &addressee)
{
    return Contact::AddresseePtr(QSharedPointer<const KContacts::Addressee>::create(addressee));
}

ContactDisplay contactDisplay(const Contact &contact)
{
    return contactDisplay(contact, {});
}

ContactDisplay contactDisplay(const Contact &contact, const CollectionSummary &collection)
{
    ContactDisplay display;
    display.collection = collection;
    const KContacts::Addressee &addressee = *contact.addressee;
    const KContacts::Address preferredAddress = addressee.address(KContacts::Address::Pref);
    const KContacts::PhoneNumber preferredPhone = addressee.phoneNumber(KContacts::PhoneNumber::Pref);
    display.isEmpty = addressee.isEmpty();
    display.givenName = addressee.givenName();
    display.additionalName = addressee.additionalName();
    display.familyName = addressee.familyName();
    display.preferredEmail = addressee.preferredEmail();
    display.street = preferredAddress.street();
    display.district = ContactUtils::primaryDistrict(addressee);
    display.city = preferredAddress.locality();
    display.county = preferredAddress.region();
    display.postcode = preferredAddress.postalCode();
    display.country = preferredAddress.country();
    display.phoneNumber = preferredPhone.number();
    return display;
}
