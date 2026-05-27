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

#include "calendardisplaysnapshot.h"

#include "calendaritem.h"
#include "eventtimezone.h"
#include "incidencefields.h"

#include <KCalendarCore/Event>
#include <KCalendarCore/Todo>

namespace CalendarSnapshot {

EventDisplay eventDisplayFromEvent(const MutableEventPtr &event,
                                   const CollectionSummary &collection,
                                   const QDateTime &start,
                                   const QDateTime &end)
{
    EventDisplay display;
    display.collection = collection;
    display.start = start;
    display.end = end;
    display.summary = event->summary();
    display.location = event->location();
    display.description = event->description();
    display.timeZoneId = EventTimeZone::timeZoneId(event);
    display.url = event->url();
    display.attachmentUri = CalendarUtils::attachmentUri(event);
    display.priority = event->priority();
    display.highlightMode = CalendarUtils::highlightMode(event.dynamicCast<KCalendarCore::Incidence>());
    display.allDay = event->allDay();
    display.completed = CalendarUtils::isEventCompleted(event);
    return display;
}

EventDisplay eventDisplay(const EventOccurrence &event)
{
    return event.display;
}

EventDisplay eventDisplay(const EventOccurrence &event, const CollectionSummary &collection)
{
    EventDisplay display = event.display;
    display.collection = collection;
    return display;
}

QList<EventDisplay> eventDisplays(const QList<EventOccurrence> &events)
{
    QList<EventDisplay> displays;
    displays.reserve(events.size());
    for (const EventOccurrence &event : events)
    {
        displays.append(eventDisplay(event));
    }
    return displays;
}

TaskDisplay taskDisplay(const Task &task)
{
    return taskDisplay(task, {});
}

TaskDisplay taskDisplay(const Task &task, const CollectionSummary &collection)
{
    TaskDisplay display;
    display.collection = collection;

    const MutableTodoPtr todo = cloneTodo(task.todo);
    display.summary = todo->summary();
    display.description = todo->description();
    display.hasDue = todo->hasDueDate();
    display.due = display.hasDue ? todo->dtDue() : QDateTime();
    display.priority = todo->priority();
    display.highlightMode = CalendarUtils::highlightMode(todo.dynamicCast<KCalendarCore::Incidence>());
    display.completed = todo->isCompleted();
    return display;
}

QList<TaskDisplay> taskDisplays(const QList<Task> &tasks)
{
    QList<TaskDisplay> displays;
    displays.reserve(tasks.size());
    for (const Task &task : tasks)
    {
        displays.append(taskDisplay(task));
    }
    return displays;
}

} // namespace CalendarSnapshot
