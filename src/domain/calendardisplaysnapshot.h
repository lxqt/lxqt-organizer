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

#ifndef CALENDARDISPLAYSNAPSHOT_H
#define CALENDARDISPLAYSNAPSHOT_H

#include "calendarsnapshot.h"
#include "collectionsummary.h"
#include "eventdatetime.h"
#include "itemidentity.h"

#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>
#include <QUrl>

struct EventDisplay
{
    CollectionSummary collection;
    QString summary;
    QString location;
    QString description;
    QString timeZoneId;
    QUrl url;
    QString attachmentUri;
    QDateTime start;
    QDateTime end;
    int priority = 5;
    CalendarUtils::HighlightMode highlightMode = CalendarUtils::HighlightMode::Always;
    bool allDay = false;
    bool completed = false;
};

struct CalendarItemDisplay
{
    QStringList searchableText;
};

struct TaskDisplay
{
    CollectionSummary collection;
    QString summary;
    QString description;
    QDateTime due;
    int priority = 5;
    CalendarUtils::HighlightMode highlightMode = CalendarUtils::HighlightMode::Always;
    bool hasDue = false;
    bool completed = false;
};

struct EventOccurrence;
struct Task;

namespace CalendarSnapshot {

EventDisplay eventDisplayFromEvent(const MutableEventPtr &event,
                                   const CollectionSummary &collection,
                                   const QDateTime &start,
                                   const QDateTime &end);
EventDisplay eventDisplay(const EventOccurrence &event);
EventDisplay eventDisplay(const EventOccurrence &event, const CollectionSummary &collection);
QList<EventDisplay> eventDisplays(const QList<EventOccurrence> &events);
TaskDisplay taskDisplay(const Task &task);
TaskDisplay taskDisplay(const Task &task, const CollectionSummary &collection);
QList<TaskDisplay> taskDisplays(const QList<Task> &tasks);

} // namespace CalendarSnapshot

#endif // CALENDARDISPLAYSNAPSHOT_H
