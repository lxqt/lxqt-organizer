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

#include "calendaritem.h"

#include <QTimeZone>

#include <KCalendarCore/Event>
#include <KCalendarCore/MemoryCalendar>
#include <KCalendarCore/Todo>

namespace CalendarSnapshot {

bool hasCalendarItemPayload(const CalendarItem &item)
{
    return !std::holds_alternative<std::monostate>(item.payload);
}

PayloadShape payloadShape(const CalendarItem &item)
{
    if (std::holds_alternative<CalendarPtr>(item.payload))
    {
        return PayloadShape::Calendar;
    }
    if (std::holds_alternative<EventPtr>(item.payload))
    {
        return PayloadShape::Event;
    }
    if (std::holds_alternative<TodoPtr>(item.payload))
    {
        return PayloadShape::Todo;
    }
    return PayloadShape::Auto;
}

MutableCalendarPtr calendarForItem(const CalendarItem &item)
{
    if (!hasCalendarItemPayload(item))
    {
        return {};
    }
    const PayloadShape shape = payloadShape(item);
    if (shape == PayloadShape::Calendar)
    {
        return cloneCalendar(std::get<CalendarPtr>(item.payload));
    }

    MutableCalendarPtr calendar(new KCalendarCore::MemoryCalendar(QTimeZone::systemTimeZone()));
    if (shape == PayloadShape::Event)
    {
        if (!calendar->addEvent(cloneEvent(std::get<EventPtr>(item.payload))))
        {
            return {};
        }
    }
    if (shape == PayloadShape::Todo)
    {
        if (!calendar->addTodo(cloneTodo(std::get<TodoPtr>(item.payload))))
        {
            return {};
        }
    }
    return calendar->events().isEmpty() && calendar->rawTodos().isEmpty() ? MutableCalendarPtr() : calendar;
}

MutableCalendarPtr mutableCalendarForItem(const CalendarItem &item)
{
    return calendarForItem(item);
}

CalendarItem
calendarItem(const ItemRef &storage, const QString &uid, const MutableCalendarPtr &calendar, PayloadShape shape)
{
    CalendarItem item;
    item.ref = storage;
    item.uid = uid;
    if (!calendar->journals().isEmpty())
    {
        item.payload = calendarSnapshot(calendar);
        return item;
    }

    const KCalendarCore::Event::List events = calendar->events();
    const KCalendarCore::Todo::List todos = calendar->rawTodos();
    if (events.isEmpty() && todos.isEmpty())
    {
        return item;
    }
    if (shape == PayloadShape::Calendar)
    {
        item.payload = calendarSnapshot(calendar);
        return item;
    }
    if (shape == PayloadShape::Event)
    {
        if (events.size() == 1 && todos.isEmpty())
        {
            item.payload = eventSnapshot(cloneEvent(events.constFirst()));
        }
        return item;
    }
    if (shape == PayloadShape::Todo)
    {
        if (todos.size() == 1 && events.isEmpty())
        {
            item.payload = todoSnapshot(cloneTodo(todos.constFirst()));
        }
        return item;
    }
    if (events.size() == 1 && todos.isEmpty())
    {
        item.payload = eventSnapshot(cloneEvent(events.constFirst()));
        return item;
    }
    if (todos.size() == 1 && events.isEmpty())
    {
        item.payload = todoSnapshot(cloneTodo(todos.constFirst()));
        return item;
    }

    item.payload = calendarSnapshot(calendar);
    return item;
}

CalendarItemDisplay calendarItemDisplay(const CalendarItem &item)
{
    CalendarItemDisplay display;

    const MutableCalendarPtr calendar = calendarForItem(item);
    if (!calendar.isNull())
    {
        for (const MutableEventPtr &event : calendar->events())
        {
            display.searchableText.append(event->summary());
            display.searchableText.append(event->location());
            display.searchableText.append(event->description());
        }
    }
    return display;
}

bool hasEventRef(const EventOccurrence &event)
{
    return event.ref.item.isValid();
}

EventOccurrence cloneEventOccurrence(const EventOccurrence &event)
{
    EventOccurrence copy = event;
    return copy;
}

QList<EventOccurrence> cloneEventOccurrences(const QList<EventOccurrence> &events)
{
    QList<EventOccurrence> copies;
    copies.reserve(events.size());
    for (const EventOccurrence &event : events)
    {
        copies.append(cloneEventOccurrence(event));
    }
    return copies;
}

bool hasTask(const Task &task)
{
    return !task.todo.isNull();
}

QDate taskDueDate(const Task &task)
{
    if (!task.todo->hasDueDate() || !task.todo->dtDue().isValid())
    {
        return QDate();
    }
    return task.todo->dtDue().date();
}

QTime taskDueTime(const Task &task)
{
    if (!task.todo->hasDueDate())
    {
        return QTime();
    }
    return task.todo->dtDue(true).time();
}

Task cloneTask(const Task &task)
{
    Task copy = task;
    return copy;
}

Task taskWithCompleted(const Task &task, bool completed)
{
    Task copy = cloneTask(task);
    const MutableTodoPtr todo = cloneTodo(copy.todo);
    todo->setCompleted(completed);
    copy.todo = todoSnapshot(todo);
    return copy;
}

Task taskWithSummary(const Task &task, const QString &summary)
{
    Task copy = cloneTask(task);
    const MutableTodoPtr todo = cloneTodo(copy.todo);
    todo->setSummary(summary);
    copy.todo = todoSnapshot(todo);
    return copy;
}

QList<Task> cloneTasks(const QList<Task> &tasks)
{
    QList<Task> copies;
    copies.reserve(tasks.size());
    for (const Task &task : tasks)
    {
        copies.append(cloneTask(task));
    }
    return copies;
}

} // namespace CalendarSnapshot
