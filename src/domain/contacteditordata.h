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

#ifndef CONTACTEDITORDATA_H
#define CONTACTEDITORDATA_H

#include <QString>

#define CONTACT_FIELDS(FIELD)                                                                                          \
    FIELD(QString, firstName, )                                                                                        \
    FIELD(QString, middleNames, )                                                                                      \
    FIELD(QString, lastName, )                                                                                         \
    FIELD(QString, email, )                                                                                            \
    FIELD(QString, street, )                                                                                           \
    FIELD(QString, district, )                                                                                         \
    FIELD(QString, city, )                                                                                             \
    FIELD(QString, county, )                                                                                           \
    FIELD(QString, postcode, )                                                                                         \
    FIELD(QString, country, )                                                                                          \
    FIELD(QString, phoneNumber, )                                                                                      \
    FIELD(QString, collectionId, )

struct ContactFields
{
#define DECLARE_CONTACT_FIELD(type, name, defaultValue) type name defaultValue;
    CONTACT_FIELDS(DECLARE_CONTACT_FIELD)
#undef DECLARE_CONTACT_FIELD

    QString validationError() const;
    bool operator==(const ContactFields &other) const
    {
#define COMPARE_CONTACT_FIELD(type, name, defaultValue)                                                                \
    if (name != other.name)                                                                                            \
    {                                                                                                                  \
        return false;                                                                                                  \
    }
        CONTACT_FIELDS(COMPARE_CONTACT_FIELD)
#undef COMPARE_CONTACT_FIELD
        return true;
    }
};

#undef CONTACT_FIELDS

#endif // CONTACTEDITORDATA_H
