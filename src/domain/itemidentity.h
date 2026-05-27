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

#ifndef ITEMIDENTITY_H
#define ITEMIDENTITY_H

#include <QHash>
#include <QMetaType>
#include <QString>

struct ItemKey
{
    QString collectionId;
    QString href;

    bool isValid() const { return !collectionId.isEmpty() && !href.isEmpty(); }
};

struct ItemRef
{
    QString collectionId;
    QString href;
    QString etag;

    bool isValid() const { return !collectionId.isEmpty() && !href.isEmpty() && !etag.isEmpty(); }

    ItemKey key() const { return ItemKey{collectionId, href}; }
};

inline bool operator==(const ItemRef &left, const ItemRef &right)
{
    return left.collectionId == right.collectionId && left.href == right.href && left.etag == right.etag;
}

inline bool operator==(const ItemKey &left, const ItemKey &right)
{
    return left.collectionId == right.collectionId && left.href == right.href;
}

inline size_t qHash(const ItemKey &key, size_t seed = 0)
{
    seed = qHash(key.collectionId.toUtf8(), seed);
    return qHash(key.href.toUtf8(), seed);
}

QString randomItemUid();

Q_DECLARE_METATYPE(ItemRef)

#endif // ITEMIDENTITY_H
