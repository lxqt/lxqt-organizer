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

#ifndef WINDOWSERVICES_H
#define WINDOWSERVICES_H

#include "collectionsummary.h"
#include "collectiontypes.h"

#include <QLocale>
#include <QString>

#include <functional>

struct WindowServices
{
    std::function<QLocale()> currentLocale;
    std::function<CollectionSummary(CollectionKind, const QString &)> collectionSummary;

    QLocale locale() const { return currentLocale ? currentLocale() : QLocale::system(); }

    CollectionSummary summarizeCollection(CollectionKind kind, const QString &collectionId) const
    {
        return collectionSummary ? collectionSummary(kind, collectionId) : CollectionSummary{collectionId, {}};
    }
};

#endif // WINDOWSERVICES_H
