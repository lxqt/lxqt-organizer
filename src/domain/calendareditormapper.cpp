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

#include "calendareditormapper.h"

#include "eventdatetime.h"
#include "eventpatcher.h"
#include "incidencefields.h"
#include "todopatcher.h"

#include <QTimeZone>
#include <QUrl>

namespace CalendarEditorMapper {

namespace {

QString timeZoneIdForDateTime(const QDateTime &dateTime)
{
    if (!dateTime.isValid())
    {
        return {};
    }
    if (dateTime.timeSpec() == Qt::UTC)
    {
        return QStringLiteral("UTC");
    }
    const QTimeZone timeZone = dateTime.timeZone();
    return timeZone.isValid() ? QString::fromUtf8(timeZone.id()) : QString();
}

} // namespace

EventFields defaultEvent(const QDate &date)
{
    const QDate eventDate = date.isValid() ? date : QDate::currentDate();
    const QTime now = QTime::currentTime();

    EventFields data;
    data.date = eventDate;
    data.startTime = now < QTime(23, 0) ? now : QTime(23, 0);
    data.endTime = now.addSecs(60 * 60);
    if (data.endTime <= data.startTime)
    {
        data.endTime = QTime(23, 59);
    }
    return data;
}

EventFields fromEventOccurrence(const EventOccurrence &event)
{
    EventFields data;
    const EventDisplay display = CalendarSnapshot::eventDisplay(event);
    if (!CalendarSnapshot::hasEventRef(event))
    {
        return data;
    }

    data.title = display.summary;
    data.location = display.location;
    data.description = display.description;
    data.url = display.url.toString();
    data.attachmentUri = display.attachmentUri;
    data.timeZoneId = display.timeZoneId;
    data.displayTimeZoneId = timeZoneIdForDateTime(display.start);
    if (data.displayTimeZoneId.isEmpty())
    {
        data.displayTimeZoneId = timeZoneIdForDateTime(display.end);
    }
    data.collectionId = event.ref.item.collectionId;
    if (event.recurrenceId.has_value())
    {
        data.locator = IncidenceLocator{event.ref.uid, event.recurrenceId};
    }
    data.highlightMode = display.highlightMode;
    data.priority = display.priority;
    data.completed = display.completed;

    const QDateTime start = display.start;
    const QDateTime end = display.end;
    data.date = start.date().isValid() ? start.date() : event.start.date();
    data.startTime = start.time().isValid() ? start.time() : event.start.time();
    data.endTime = end.time().isValid() ? end.time() : event.end.time();
    data.allDay = display.allDay;
    return data;
}

KCalendarCore::Event::Ptr createEvent(const EventFields &data)
{
    KCalendarCore::Event::Ptr event = EventDateTime::createEvent();
    EventPatcher::applyToEvent(event, data, EventPatcher::editableOptions());
    return event;
}

TaskFields fromTask(const Task &task, const QDate &defaultDueDate)
{
    TaskFields data;
    const CalendarSnapshot::MutableTodoPtr todo = CalendarSnapshot::cloneTodo(task.todo);
    data.title = todo->summary();
    data.description = todo->description();
    data.collectionId = task.ref.collectionId;
    data.highlightMode = CalendarUtils::highlightMode(todo.dynamicCast<KCalendarCore::Incidence>());
    data.hasDue = todo->hasDueDate();

    const QDateTime dueDateTime = data.hasDue ? todo->dtDue() : QDateTime();
    QDate dueDate = data.hasDue ? dueDateTime.date() : defaultDueDate;
    if (!dueDate.isValid())
    {
        dueDate = QDate::currentDate();
    }
    const QTime dueTime = data.hasDue && dueDateTime.time().isValid() ? dueDateTime.time() : QTime(9, 0);
    data.due = QDateTime(dueDate, dueTime);

    data.priority = todo->priority();
    data.completed = todo->isCompleted();
    data.rollForward = CalendarUtils::isRollForwardEnabled(todo);
    return data;
}

KCalendarCore::Todo::Ptr createTodo(const TaskFields &data)
{
    KCalendarCore::Todo::Ptr todo(new KCalendarCore::Todo);
    todo->setCreated(QDateTime::currentDateTimeUtc());
    TodoPatcher::applyToTodo(todo, data, TodoPatcher::editableOptions());
    return todo;
}

Task createTask(const TaskFields &data)
{
    const KCalendarCore::Todo::Ptr todo = createTodo(data);
    Task task;
    task.uid = todo->uid();
    task.todo = CalendarSnapshot::todoSnapshot(todo);
    return task;
}

Task applyToTask(Task task, const TaskFields &data)
{
    CalendarSnapshot::MutableTodoPtr todo = CalendarSnapshot::cloneTodo(task.todo);
    TodoPatcher::applyToTodo(todo, data, TodoPatcher::editableOptions());
    task.todo = CalendarSnapshot::todoSnapshot(todo);
    return task;
}

} // namespace CalendarEditorMapper
