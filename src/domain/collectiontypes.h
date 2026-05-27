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

#ifndef COLLECTIONTYPES_H
#define COLLECTIONTYPES_H

#include <QString>

enum class CollectionKind
{
    Calendar,
    AddressBook,
};

enum class CollectionAccess
{
    ReadWrite,
    ReadOnly,
    Invalid,
};

#define COLLECTION_FIELDS(FIELD)                                                                                       \
    FIELD(QString, id, )                                                                                               \
    FIELD(CollectionKind, kind, = CollectionKind::Calendar)                                                            \
    FIELD(QString, path, )                                                                                             \
    FIELD(QString, canonicalPath, )                                                                                    \
    FIELD(QString, displayName, )                                                                                      \
    FIELD(QString, color, )                                                                                            \
    FIELD(CollectionAccess, access, = CollectionAccess::Invalid)                                                       \
    FIELD(QString, status, )

struct Collection
{
#define DECLARE_COLLECTION_FIELD(type, name, defaultValue) type name defaultValue;
    COLLECTION_FIELDS(DECLARE_COLLECTION_FIELD)
#undef DECLARE_COLLECTION_FIELD

    bool isValid() const { return access != CollectionAccess::Invalid; }
    bool isWritable() const { return access == CollectionAccess::ReadWrite; }

    bool operator==(const Collection &other) const
    {
#define COMPARE_COLLECTION_FIELD(type, name, defaultValue)                                                             \
    if (name != other.name)                                                                                            \
    {                                                                                                                  \
        return false;                                                                                                  \
    }
        COLLECTION_FIELDS(COMPARE_COLLECTION_FIELD)
#undef COMPARE_COLLECTION_FIELD
        return true;
    }

    bool operator!=(const Collection &other) const { return !(*this == other); }
};

#undef COLLECTION_FIELDS

#endif // COLLECTIONTYPES_H
