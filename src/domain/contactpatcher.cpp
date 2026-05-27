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

#include "contactpatcher.h"

#include "contactutils.h"

#include <KContacts/Address>
#include <KContacts/Addressee>
#include <KContacts/PhoneNumber>

namespace ContactPatcher {

namespace {

Options noOptions()
{
    Options options;
    forEachOption(options, [](bool &option) { option = false; });
    return options;
}

} // namespace

Options editableOptions()
{
    Options options;
    forEachOption(options, [](bool &option) { option = true; });
    return options;
}

Options changedOptionsForAddressee(const KContacts::Addressee &addressee, const ContactFields &patch)
{
    Options options = noOptions();

    const KContacts::Address preferredAddress = addressee.address(KContacts::Address::Pref);
    const KContacts::PhoneNumber preferredPhone = addressee.phoneNumber(KContacts::PhoneNumber::Pref);

    options.name = patch.firstName != addressee.givenName() || patch.middleNames != addressee.additionalName() ||
                   patch.lastName != addressee.familyName();
    options.email = patch.email != addressee.preferredEmail();
    options.street = patch.street != preferredAddress.street();
    options.district = patch.district != ContactUtils::primaryDistrict(addressee);
    options.city = patch.city != preferredAddress.locality();
    options.county = patch.county != preferredAddress.region();
    options.postcode = patch.postcode != preferredAddress.postalCode();
    options.country = patch.country != preferredAddress.country();
    options.phoneNumber = patch.phoneNumber != preferredPhone.number();
    return options;
}

bool hasChanges(const Options &options)
{
    bool changed = false;
    forEachOption(options, [&changed](bool option) { changed = changed || option; });
    return changed;
}

void applyToAddressee(KContacts::Addressee &addressee, const ContactFields &patch, Options options)
{
    if (options.name)
    {
        ContactUtils::setNameParts(addressee, patch.firstName, patch.middleNames, patch.lastName);
    }
    if (options.email)
    {
        ContactUtils::setPrimaryEmail(addressee, patch.email);
    }
    if (options.street)
    {
        ContactUtils::setPrimaryStreet(addressee, patch.street);
    }
    if (options.district)
    {
        ContactUtils::setPrimaryDistrict(addressee, patch.district);
    }
    if (options.city)
    {
        ContactUtils::setPrimaryCity(addressee, patch.city);
    }
    if (options.county)
    {
        ContactUtils::setPrimaryCounty(addressee, patch.county);
    }
    if (options.postcode)
    {
        ContactUtils::setPrimaryPostcode(addressee, patch.postcode);
    }
    if (options.country)
    {
        ContactUtils::setPrimaryCountry(addressee, patch.country);
    }
    if (options.phoneNumber)
    {
        ContactUtils::setPrimaryTelephone(addressee, patch.phoneNumber);
    }
}

} // namespace ContactPatcher
