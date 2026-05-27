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

#ifndef INCIDENCELOCATOR_H
#define INCIDENCELOCATOR_H

#include <QDateTime>
#include <QString>

#include <optional>

struct IncidenceLocator
{
    QString uid;
    // Empty means the master series instance with no recurrence-id.
    // Set means a specific override.
    std::optional<QDateTime> recurrenceId;

    bool isMasterOnly() const { return !recurrenceId.has_value(); }
    bool isValid() const { return !uid.isEmpty(); }
    bool operator==(const IncidenceLocator &other) const
    {
        return uid == other.uid && recurrenceId == other.recurrenceId;
    }
};

#endif // INCIDENCELOCATOR_H
