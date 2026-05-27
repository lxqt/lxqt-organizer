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

#ifndef CALENDARPANEUTILS_H
#define CALENDARPANEUTILS_H

#include "calendaritem.h"

#include <QDate>
#include <QTime>
#include <QUrl>
#include <functional>

class QWidget;
class QAction;
class QObject;

namespace CalendarPaneUtils {

bool eventStartsBefore(const EventOccurrence &first, const EventOccurrence &second);
QDate firstDateOfMonth(int year, int month);
QDate lastDateOfMonth(int year, int month);
bool sameOccurrence(const EventOccurrence &first, const EventOccurrence &second);
bool sameItemKey(const ItemRef &ref, const ItemKey &key);
void appendUniqueEventOccurrences(QList<EventOccurrence> &target, const QList<EventOccurrence> &source);
bool removeEventsByKey(QList<EventOccurrence> &events, const ItemKey &key);
bool removeEventRef(QList<EventOccurrence> &events, const OccurrenceRef &event);
bool sameTask(const Task &first, const Task &second);
bool taskOccursOn(const Task &task, const QDate &date);
QTime taskSortTime(const Task &task);
QString priorityStatusText(int priority);
QUrl openableUrlFromText(const QString &text, bool preferLocalFile);

struct CalendarPaneActions
{
    QAction *newEvent = nullptr;
    QAction *newTask = nullptr;
    QAction *editItem = nullptr;
    QAction *deleteItem = nullptr;
    QAction *find = nullptr;
    QAction *findNext = nullptr;
    QAction *findPrevious = nullptr;
};

CalendarPaneActions createCalendarPaneActions(QObject *owner);

void showTimelineContextMenu(QWidget *parent,
                             const EventOccurrence &occurrence,
                             const QTime &startTime,
                             const QTime &endTime,
                             const QPoint &globalPos,
                             const std::function<void(const QTime &, const QTime &)> &newEvent,
                             const std::function<void(const EventOccurrence &)> &editEvent,
                             const std::function<void(const EventOccurrence &, bool)> &setEventCompleted,
                             const std::function<void(const EventOccurrence &)> &deleteEvent);
bool selectAdjacentCalendarItem(int delta,
                                const QDate &date,
                                const QList<EventOccurrence> &dayEvents,
                                const QList<Task> &tasks,
                                const EventOccurrence &selectedEvent,
                                const Task &selectedTask,
                                const std::function<bool(const EventOccurrence &)> &selectEvent,
                                const std::function<bool(const Task &)> &selectTask);

} // namespace CalendarPaneUtils

#endif // CALENDARPANEUTILS_H
