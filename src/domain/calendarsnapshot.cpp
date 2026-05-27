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

#include "calendarsnapshot.h"

#include <KCalendarCore/Event>
#include <KCalendarCore/MemoryCalendar>
#include <KCalendarCore/Todo>

namespace CalendarSnapshot {

MutableEventPtr cloneEvent(const MutableEventPtr &event)
{
    return MutableEventPtr(event->clone());
}

MutableEventPtr cloneEvent(const EventPtr &event)
{
    return MutableEventPtr(event->clone());
}

MutableTodoPtr cloneTodo(const MutableTodoPtr &todo)
{
    return MutableTodoPtr(todo->clone());
}

MutableTodoPtr cloneTodo(const TodoPtr &todo)
{
    return MutableTodoPtr(todo->clone());
}

MutableCalendarPtr cloneCalendar(const MutableCalendarPtr &calendar)
{
    MutableCalendarPtr copy(new KCalendarCore::MemoryCalendar(calendar->timeZone()));
    for (const MutableEventPtr &event : calendar->events())
    {
        if (!copy->addEvent(cloneEvent(event)))
        {
            return {};
        }
    }
    for (const MutableTodoPtr &todo : calendar->rawTodos())
    {
        if (!copy->addTodo(cloneTodo(todo)))
        {
            return {};
        }
    }
    return copy;
}

MutableCalendarPtr cloneCalendar(const CalendarPtr &calendar)
{
    MutableCalendarPtr copy(new KCalendarCore::MemoryCalendar(calendar->timeZone()));
    for (const MutableEventPtr &event : calendar->events())
    {
        if (!copy->addEvent(cloneEvent(event)))
        {
            return {};
        }
    }
    for (const MutableTodoPtr &todo : calendar->rawTodos())
    {
        if (!copy->addTodo(cloneTodo(todo)))
        {
            return {};
        }
    }
    return copy;
}

CalendarPtr calendarSnapshot(const MutableCalendarPtr &calendar)
{
    return CalendarPtr(qSharedPointerConstCast<const KCalendarCore::MemoryCalendar>(cloneCalendar(calendar)));
}

EventPtr eventSnapshot(const MutableEventPtr &event)
{
    return EventPtr(qSharedPointerConstCast<const KCalendarCore::Event>(cloneEvent(event)));
}

TodoPtr todoSnapshot(const MutableTodoPtr &todo)
{
    return TodoPtr(qSharedPointerConstCast<const KCalendarCore::Todo>(cloneTodo(todo)));
}

} // namespace CalendarSnapshot
