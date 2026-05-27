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

#ifndef COLLECTIONCATALOG_H
#define COLLECTIONCATALOG_H

#include "collectionsummary.h"
#include "collectiontypes.h"

#include <QList>
#include <QString>

#include <optional>

class CollectionCatalog
{
public:
    static bool load(CollectionCatalog *catalog);

    QList<Collection> calendarList() const;
    QList<Collection> addressBookList() const;

    std::optional<Collection> calendar(const QString &id) const;
    std::optional<Collection> addressBook(const QString &id) const;
    std::optional<Collection> defaultCalendar() const;
    std::optional<Collection> defaultAddressBook() const;

private:
    std::optional<Collection> defaultCollection(CollectionKind kind) const;

    Collection m_calendar;
    Collection m_addressBook;
};

CollectionSummary
summarizeCollection(const CollectionCatalog &catalog, CollectionKind kind, const QString &collectionId);

#endif // COLLECTIONCATALOG_H
