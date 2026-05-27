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

#include "incidenceresolver.h"

namespace IncidenceResolver {

bool contains(const KCalendarCore::MemoryCalendar::Ptr &calendar, const QString &uid)
{
    if (uid.isEmpty())
    {
        return false;
    }
    for (const KCalendarCore::Event::Ptr &event : calendar->events())
    {
        if (event->uid() == uid)
        {
            return true;
        }
    }
    for (const KCalendarCore::Todo::Ptr &todo : calendar->rawTodos())
    {
        if (todo->uid() == uid)
        {
            return true;
        }
    }
    return false;
}

IncidenceLocator inferLocator(const KCalendarCore::MemoryCalendar::Ptr &calendar, const QString &preferredUid)
{
    if (contains(calendar, preferredUid))
    {
        return {preferredUid, std::nullopt};
    }

    for (const KCalendarCore::Event::Ptr &event : calendar->events())
    {
        if (!event->uid().isEmpty())
        {
            return {event->uid(), std::nullopt};
        }
    }
    for (const KCalendarCore::Todo::Ptr &todo : calendar->rawTodos())
    {
        if (!todo->uid().isEmpty())
        {
            return {todo->uid(), std::nullopt};
        }
    }
    return {preferredUid, std::nullopt};
}

KCalendarCore::Event::Ptr findMasterEvent(const KCalendarCore::MemoryCalendar::Ptr &calendar,
                                          const IncidenceLocator &locator)
{
    if (!locator.isValid() || !locator.isMasterOnly())
    {
        return {};
    }

    KCalendarCore::Event::Ptr matched;
    for (const KCalendarCore::Event::Ptr &event : calendar->events())
    {
        if (event->uid() != locator.uid || event->hasRecurrenceId())
        {
            continue;
        }
        if (!matched.isNull())
        {
            return {};
        }
        matched = event;
    }
    return matched;
}

KCalendarCore::Event::Ptr findEvent(const KCalendarCore::MemoryCalendar::Ptr &calendar, const IncidenceLocator &locator)
{
    if (!locator.isValid())
    {
        return {};
    }
    if (locator.isMasterOnly())
    {
        return findMasterEvent(calendar, locator);
    }

    KCalendarCore::Event::Ptr matched;
    for (const KCalendarCore::Event::Ptr &event : calendar->events())
    {
        if (event->uid() != locator.uid || !event->hasRecurrenceId() || event->recurrenceId() != *locator.recurrenceId)
        {
            continue;
        }
        if (!matched.isNull())
        {
            return {};
        }
        matched = event;
    }
    return matched;
}

KCalendarCore::Todo::Ptr findTodo(const KCalendarCore::MemoryCalendar::Ptr &calendar, const IncidenceLocator &locator)
{
    if (!locator.isValid())
    {
        return {};
    }

    KCalendarCore::Todo::Ptr matched;
    for (const KCalendarCore::Todo::Ptr &todo : calendar->rawTodos())
    {
        if (todo->uid() != locator.uid)
        {
            continue;
        }
        if (!matched.isNull())
        {
            return {};
        }
        matched = todo;
    }
    return matched;
}

} // namespace IncidenceResolver
