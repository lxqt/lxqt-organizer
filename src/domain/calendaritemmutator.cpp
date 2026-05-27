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

#include "calendaritemmutator.h"

#include "eventoccurrences.h"
#include "incidenceresolver.h"

#include <KCalendarCore/Calendar>

namespace CalendarItemMutator {

bool hasIncidences(const KCalendarCore::MemoryCalendar::Ptr &calendar)
{
    return !calendar->events().isEmpty() || !calendar->rawTodos().isEmpty();
}

KCalendarCore::Event::Ptr editableSingleEventClone(const KCalendarCore::MemoryCalendar::Ptr &calendar)
{
    if (calendar.isNull())
    {
        return {};
    }
    const KCalendarCore::Event::Ptr event = EventOccurrences::editableSingleEvent(calendar);
    return event.isNull() ? KCalendarCore::Event::Ptr() : KCalendarCore::Event::Ptr(event->clone());
}

KCalendarCore::Event::Ptr eventCloneForEdit(const KCalendarCore::Event::Ptr &event,
                                            const std::optional<QDateTime> &recurrenceId)
{
    if (event.isNull())
    {
        return {};
    }

    KCalendarCore::Event::Ptr clone(event->clone());
    if (recurrenceId.has_value() && recurrenceId->isValid() && !clone->hasRecurrenceId())
    {
        clone->setRecurrenceId(*recurrenceId);
    }
    return clone;
}

KCalendarCore::Event::Ptr createException(const KCalendarCore::Event::Ptr &master, const QDateTime &recurrenceId)
{
    if (master.isNull() || !recurrenceId.isValid())
    {
        return {};
    }

    const KCalendarCore::Incidence::Ptr exception =
        KCalendarCore::Calendar::createException(master.dynamicCast<KCalendarCore::Incidence>(), recurrenceId);
    return exception.dynamicCast<KCalendarCore::Event>();
}

bool insertEvent(const KCalendarCore::MemoryCalendar::Ptr &calendar, const KCalendarCore::Event::Ptr &event)
{
    return !calendar.isNull() && !event.isNull() && calendar->addEvent(event);
}

bool ensureEventUid(const KCalendarCore::Event::Ptr &event, const QString &uid)
{
    if (event.isNull() || uid.isEmpty())
    {
        return false;
    }
    if (event->uid().isEmpty())
    {
        event->setUid(uid);
    }
    return event->uid() == uid;
}

bool ensureTodoUid(const KCalendarCore::Todo::Ptr &todo, const QString &uid)
{
    if (todo.isNull() || uid.isEmpty())
    {
        return false;
    }
    if (todo->uid().isEmpty())
    {
        todo->setUid(uid);
    }
    return todo->uid() == uid;
}

bool replaceEventByUid(const KCalendarCore::MemoryCalendar::Ptr &calendar,
                       const QString &uid,
                       const KCalendarCore::Event::Ptr &replacement)
{
    const KCalendarCore::Event::Ptr storedEvent = IncidenceResolver::findMasterEvent(calendar, {uid, std::nullopt});
    return !storedEvent.isNull() && calendar->deleteEvent(storedEvent) && calendar->addEvent(replacement);
}

bool replaceEventInstance(const KCalendarCore::MemoryCalendar::Ptr &calendar,
                          const KCalendarCore::Event::Ptr &storedEvent,
                          const KCalendarCore::Event::Ptr &replacement)
{
    return !calendar.isNull() && !storedEvent.isNull() && !replacement.isNull() && calendar->deleteEvent(storedEvent) &&
           calendar->addEvent(replacement);
}

bool saveEventInstance(const KCalendarCore::MemoryCalendar::Ptr &calendar,
                       const KCalendarCore::Event::Ptr &storedEvent,
                       const KCalendarCore::Event::Ptr &replacement,
                       bool insertNewException)
{
    return insertNewException ? insertEvent(calendar, replacement)
                              : replaceEventInstance(calendar, storedEvent, replacement);
}

bool replaceSingleEvent(const KCalendarCore::MemoryCalendar::Ptr &calendar,
                        const KCalendarCore::Event::Ptr &replacement)
{
    const KCalendarCore::Event::List events = calendar->events();
    return events.size() == 1 && calendar->deleteEvent(events.constFirst()) && calendar->addEvent(replacement);
}

bool replaceTodoByUid(const KCalendarCore::MemoryCalendar::Ptr &calendar,
                      const QString &uid,
                      const KCalendarCore::Todo::Ptr &replacement)
{
    const KCalendarCore::Todo::Ptr storedTodo = IncidenceResolver::findTodo(calendar, {uid, std::nullopt});
    return !storedTodo.isNull() && calendar->deleteTodo(storedTodo) && calendar->addTodo(replacement);
}

bool replaceSingleTodo(const KCalendarCore::MemoryCalendar::Ptr &calendar, const KCalendarCore::Todo::Ptr &replacement)
{
    const KCalendarCore::Todo::List todos = calendar->rawTodos();
    return todos.size() == 1 && calendar->deleteTodo(todos.constFirst()) && calendar->addTodo(replacement);
}

bool removeEventsByUid(const KCalendarCore::MemoryCalendar::Ptr &calendar, const QString &uid)
{
    if (uid.isEmpty())
    {
        return false;
    }

    bool removed = false;
    const KCalendarCore::Event::List events = calendar->events();
    for (const KCalendarCore::Event::Ptr &event : events)
    {
        if (event->uid() == uid)
        {
            removed = calendar->deleteEvent(event) || removed;
        }
    }
    return removed;
}

bool removeTodoByUid(const KCalendarCore::MemoryCalendar::Ptr &calendar, const QString &uid)
{
    const KCalendarCore::Todo::Ptr todo = IncidenceResolver::findTodo(calendar, {uid, std::nullopt});
    return !todo.isNull() && calendar->deleteTodo(todo);
}

} // namespace CalendarItemMutator
