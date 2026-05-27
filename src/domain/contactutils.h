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

#ifndef CONTACTUTILS_H
#define CONTACTUTILS_H

#include <QString>

#include <KContacts/Addressee>

namespace ContactUtils {

void setNameParts(KContacts::Addressee &addressee,
                  const QString &firstName,
                  const QString &middleNames,
                  const QString &lastName);

void setPrimaryEmail(KContacts::Addressee &addressee, const QString &email);
void setPrimaryTelephone(KContacts::Addressee &addressee, const QString &telephone);
void setPrimaryStreet(KContacts::Addressee &addressee, const QString &street);
QString primaryDistrict(const KContacts::Addressee &addressee);
void setPrimaryDistrict(KContacts::Addressee &addressee, const QString &district);
void setPrimaryCity(KContacts::Addressee &addressee, const QString &city);
void setPrimaryCounty(KContacts::Addressee &addressee, const QString &county);
void setPrimaryPostcode(KContacts::Addressee &addressee, const QString &postcode);
void setPrimaryCountry(KContacts::Addressee &addressee, const QString &country);

} // namespace ContactUtils

#endif // CONTACTUTILS_H
