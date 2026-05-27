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

#include "contactutils.h"

#include <QStringList>

#include <KContacts/Address>
#include <KContacts/Email>
#include <KContacts/PhoneNumber>

namespace {

QString addresseeCustom(const KContacts::Addressee &addressee, const QString &name)
{
    return addressee.custom(QStringLiteral("organizer"), name);
}

int preferredOrFirstEmailIndex(const KContacts::Email::List &emails)
{
    for (qsizetype i = 0; i < emails.size(); ++i)
    {
        if (emails.at(i).isPreferred())
        {
            return static_cast<int>(i);
        }
    }
    return emails.isEmpty() ? -1 : 0;
}

int preferredOrFirstPhoneIndex(const KContacts::PhoneNumber::List &phones)
{
    for (qsizetype i = 0; i < phones.size(); ++i)
    {
        if (phones.at(i).type().testFlag(KContacts::PhoneNumber::Pref))
        {
            return static_cast<int>(i);
        }
    }
    return phones.isEmpty() ? -1 : 0;
}

int preferredOrFirstAddressIndex(const KContacts::Address::List &addresses)
{
    for (qsizetype i = 0; i < addresses.size(); ++i)
    {
        if (addresses.at(i).type().testFlag(KContacts::Address::Pref))
        {
            return static_cast<int>(i);
        }
    }
    return addresses.isEmpty() ? -1 : 0;
}

void keepOnlyPreferredEmail(KContacts::Email::List &emails, int preferredIndex)
{
    for (qsizetype i = 0; i < emails.size(); ++i)
    {
        if (i != preferredIndex && emails[i].isPreferred())
        {
            emails[i].setPreferred(false);
        }
    }
}

void keepOnlyPreferredPhone(KContacts::PhoneNumber::List &phones, int preferredIndex)
{
    for (qsizetype i = 0; i < phones.size(); ++i)
    {
        if (i == preferredIndex || !phones[i].type().testFlag(KContacts::PhoneNumber::Pref))
        {
            continue;
        }
        KContacts::PhoneNumber::Type type = phones[i].type();
        type.setFlag(KContacts::PhoneNumber::Pref, false);
        phones[i].setType(type);
    }
}

void keepOnlyPreferredAddress(KContacts::Address::List &addresses, int preferredIndex)
{
    for (qsizetype i = 0; i < addresses.size(); ++i)
    {
        if (i == preferredIndex || !addresses[i].type().testFlag(KContacts::Address::Pref))
        {
            continue;
        }
        KContacts::Address::Type type = addresses[i].type();
        type.setFlag(KContacts::Address::Pref, false);
        addresses[i].setType(type);
    }
}

void applyPrimaryEmail(KContacts::Addressee &addressee, const QString &value)
{
    KContacts::Email::List emails = addressee.emailList();
    const int index = preferredOrFirstEmailIndex(emails);
    if (value.isEmpty())
    {
        if (index >= 0)
        {
            emails.removeAt(index);
        }
    }
    else
    {
        KContacts::Email email = index >= 0 ? emails.at(index) : KContacts::Email();
        email.setEmail(value);
        email.setPreferred(true);
        if (email.type() == 0)
        {
            email.setType(KContacts::Email::Home);
        }
        if (index >= 0)
        {
            emails[index] = email;
            keepOnlyPreferredEmail(emails, index);
        }
        else
        {
            emails.append(email);
            keepOnlyPreferredEmail(emails, emails.size() - 1);
        }
    }
    addressee.setEmailList(emails);
}

void applyPrimaryPhone(KContacts::Addressee &addressee, const QString &value)
{
    KContacts::PhoneNumber::List phones = addressee.phoneNumbers();
    const int index = preferredOrFirstPhoneIndex(phones);
    if (value.isEmpty())
    {
        if (index >= 0)
        {
            phones.removeAt(index);
        }
    }
    else
    {
        KContacts::PhoneNumber phone = index >= 0 ? phones.at(index) : KContacts::PhoneNumber();
        phone.setNumber(value);
        phone.setType(phone.type() == 0 ? KContacts::PhoneNumber::Home | KContacts::PhoneNumber::Pref
                                        : phone.type() | KContacts::PhoneNumber::Pref);
        if (index >= 0)
        {
            phones[index] = phone;
            keepOnlyPreferredPhone(phones, index);
        }
        else
        {
            phones.append(phone);
            keepOnlyPreferredPhone(phones, phones.size() - 1);
        }
    }
    addressee.setPhoneNumbers(phones);
}

KContacts::Address preferredOrNewAddress(KContacts::Addressee &addressee, int *index)
{
    const KContacts::Address::List addresses = addressee.addresses();
    *index = preferredOrFirstAddressIndex(addresses);
    KContacts::Address address =
        *index >= 0 ? addresses.at(*index) : KContacts::Address(KContacts::Address::Home | KContacts::Address::Pref);
    address.setType(address.type() == 0 ? KContacts::Address::Home | KContacts::Address::Pref
                                        : address.type() | KContacts::Address::Pref);
    return address;
}

void storeAddress(KContacts::Addressee &addressee, const KContacts::Address &address, int index)
{
    KContacts::Address::List addresses = addressee.addresses();
    if (index >= 0)
    {
        addresses[index] = address;
        keepOnlyPreferredAddress(addresses, index);
    }
    else
    {
        addresses.append(address);
        keepOnlyPreferredAddress(addresses, addresses.size() - 1);
    }
    addressee.setAddresses(addresses);
}

} // namespace

namespace ContactUtils {

void setNameParts(KContacts::Addressee &addressee,
                  const QString &firstName,
                  const QString &middleNames,
                  const QString &lastName)
{
    addressee.setFormattedName(QStringList{firstName, middleNames, lastName}.join(QLatin1Char(' ')).simplified());
    addressee.setGivenName(firstName);
    addressee.setAdditionalName(middleNames);
    addressee.setFamilyName(lastName);
}

void setPrimaryEmail(KContacts::Addressee &addressee, const QString &email)
{
    applyPrimaryEmail(addressee, email);
}

void setPrimaryTelephone(KContacts::Addressee &addressee, const QString &telephone)
{
    applyPrimaryPhone(addressee, telephone);
}

void setPrimaryStreet(KContacts::Addressee &addressee, const QString &street)
{
    int index = -1;
    KContacts::Address address = preferredOrNewAddress(addressee, &index);
    address.setStreet(street);
    storeAddress(addressee, address, index);
}

QString primaryDistrict(const KContacts::Addressee &addressee)
{
    return addresseeCustom(addressee, QStringLiteral("DISTRICT"));
}

void setPrimaryDistrict(KContacts::Addressee &addressee, const QString &district)
{
    if (district.isEmpty())
    {
        addressee.removeCustom(QStringLiteral("organizer"), QStringLiteral("DISTRICT"));
    }
    else
    {
        addressee.insertCustom(QStringLiteral("organizer"), QStringLiteral("DISTRICT"), district);
    }
}

void setPrimaryCity(KContacts::Addressee &addressee, const QString &city)
{
    int index = -1;
    KContacts::Address address = preferredOrNewAddress(addressee, &index);
    address.setLocality(city);
    storeAddress(addressee, address, index);
}

void setPrimaryCounty(KContacts::Addressee &addressee, const QString &county)
{
    int index = -1;
    KContacts::Address address = preferredOrNewAddress(addressee, &index);
    address.setRegion(county);
    storeAddress(addressee, address, index);
}

void setPrimaryPostcode(KContacts::Addressee &addressee, const QString &postcode)
{
    int index = -1;
    KContacts::Address address = preferredOrNewAddress(addressee, &index);
    address.setPostalCode(postcode);
    storeAddress(addressee, address, index);
}

void setPrimaryCountry(KContacts::Addressee &addressee, const QString &country)
{
    int index = -1;
    KContacts::Address address = preferredOrNewAddress(addressee, &index);
    address.setCountry(country);
    storeAddress(addressee, address, index);
}

} // namespace ContactUtils
