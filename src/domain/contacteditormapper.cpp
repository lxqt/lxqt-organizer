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

#include "contacteditormapper.h"

#include "contactpatcher.h"
#include "contactutils.h"

#include <KContacts/Address>
#include <KContacts/Addressee>
#include <KContacts/PhoneNumber>

namespace ContactEditorMapper {

ContactFields fromAddressee(const KContacts::Addressee &addressee)
{
    ContactFields data;
    const KContacts::Address preferredAddress = addressee.address(KContacts::Address::Pref);
    const KContacts::PhoneNumber preferredPhone = addressee.phoneNumber(KContacts::PhoneNumber::Pref);
    data.firstName = addressee.givenName();
    data.middleNames = addressee.additionalName();
    data.lastName = addressee.familyName();
    data.email = addressee.preferredEmail();
    data.street = preferredAddress.street();
    data.district = ContactUtils::primaryDistrict(addressee);
    data.city = preferredAddress.locality();
    data.county = preferredAddress.region();
    data.postcode = preferredAddress.postalCode();
    data.country = preferredAddress.country();
    data.phoneNumber = preferredPhone.number();
    return data;
}

ContactFields fromContact(const Contact &contact)
{
    ContactFields data = contact.addressee ? fromAddressee(*contact.addressee) : ContactFields{};
    data.collectionId = contact.ref.collectionId;
    return data;
}

void applyToAddressee(KContacts::Addressee &addressee, const ContactFields &data)
{
    const ContactPatcher::Options changedOptions = ContactPatcher::changedOptionsForAddressee(addressee, data);
    ContactPatcher::applyToAddressee(addressee, data, changedOptions);
}

Contact createContact(const ContactFields &data)
{
    KContacts::Addressee addressee;
    applyToAddressee(addressee, data);
    Contact contact;
    contact.uid = addressee.uid();
    contact.addressee = addresseeSnapshot(addressee);
    return contact;
}

Contact applyToContact(Contact contact, const ContactFields &data)
{
    if (!contact.addressee)
    {
        return contact;
    }
    KContacts::Addressee addressee = *contact.addressee;
    applyToAddressee(addressee, data);
    return Contact{contact.ref, contact.uid, addresseeSnapshot(addressee)};
}

} // namespace ContactEditorMapper
