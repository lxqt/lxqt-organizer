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

#ifndef CALENDARITEM_H
#define CALENDARITEM_H

#include "calendardisplaysnapshot.h"
#include "calendarsnapshot.h"
#include "itemidentity.h"

#include <QDate>
#include <QDateTime>
#include <QList>
#include <QString>
#include <QTime>

#include <optional>

struct CalendarItem
{
    ItemRef ref;
    QString uid;
    CalendarSnapshot::CalendarPayload payload;
};

struct OccurrenceRef
{
    ItemRef item;
    QString uid;

    bool isValid() const { return item.isValid() && !uid.isEmpty(); }
};

struct EventOccurrence
{
    OccurrenceRef ref;
    EventDisplay display;
    QDateTime start;
    QDateTime end;
    std::optional<QDateTime> recurrenceId;
};

struct Task
{
    ItemRef ref;
    QString uid;
    // Storage/service-domain payload. UI rendering should use
    // TaskDisplay, and edits should flow through TaskFields or the
    // CalendarSnapshot task helper functions below.
    CalendarSnapshot::TodoPtr todo;
};

namespace CalendarSnapshot {

bool hasCalendarItemPayload(const CalendarItem &item);
PayloadShape payloadShape(const CalendarItem &item);
MutableCalendarPtr calendarForItem(const CalendarItem &item);
MutableCalendarPtr mutableCalendarForItem(const CalendarItem &item);
CalendarItem
calendarItem(const ItemRef &storage, const QString &uid, const MutableCalendarPtr &calendar, PayloadShape shape);
CalendarItemDisplay calendarItemDisplay(const CalendarItem &item);

bool hasEventRef(const EventOccurrence &event);
EventOccurrence cloneEventOccurrence(const EventOccurrence &event);
QList<EventOccurrence> cloneEventOccurrences(const QList<EventOccurrence> &events);

bool hasTask(const Task &task);
QDate taskDueDate(const Task &task);
QTime taskDueTime(const Task &task);
Task cloneTask(const Task &task);
QList<Task> cloneTasks(const QList<Task> &tasks);
Task taskWithCompleted(const Task &task, bool completed);
Task taskWithSummary(const Task &task, const QString &summary);

} // namespace CalendarSnapshot

#endif // CALENDARITEM_H
