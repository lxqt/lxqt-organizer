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

#include "calendarvalidation.h"

#include "calendareditordata.h"

#include <QCoreApplication>

#include <KCalendarCore/Event>
#include <KCalendarCore/Todo>

QString EventFields::validationError() const
{
    if (title.trimmed().isEmpty())
    {
        return QCoreApplication::translate("EventFields", "Must enter a title");
    }
    if (!allDay && (!startTime.isValid() || !endTime.isValid() || startTime >= endTime))
    {
        return QCoreApplication::translate("EventFields", "End time must be after start time");
    }
    return QString();
}

QString TaskFields::validationError() const
{
    if (title.trimmed().isEmpty())
    {
        return QCoreApplication::translate("TaskFields", "Must enter a title");
    }
    return QString();
}

namespace CalendarValidation {

bool isSupportedCalendar(const KCalendarCore::MemoryCalendar::Ptr &calendar)
{
    if (!calendar->journals().isEmpty())
    {
        return false;
    }

    const KCalendarCore::Event::List events = calendar->events();
    const KCalendarCore::Todo::List todos = calendar->rawTodos();
    if (events.isEmpty() && todos.isEmpty())
    {
        return false;
    }

    for (const KCalendarCore::Todo::Ptr &todo : todos)
    {
        if (todo->uid().isEmpty())
        {
            return false;
        }
    }

    for (const KCalendarCore::Event::Ptr &event : events)
    {
        if (event->uid().isEmpty())
        {
            return false;
        }
    }

    return true;
}

} // namespace CalendarValidation
