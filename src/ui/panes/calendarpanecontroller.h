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

#ifndef CALENDARPANECONTROLLER_H
#define CALENDARPANECONTROLLER_H

#include "calendarpane.h"
#include "eventdialogflow.h"
#include "taskdialogflow.h"

#include <QObject>

class CollectionService;
class EventReader;
class EventService;
class ReloadCoordinator;
class TaskService;
struct WindowServices;

class CalendarPaneController : public QObject
{
    Q_OBJECT

public:
    struct Deps
    {
        const EventReader &eventReader;
        EventService &eventService;
        TaskService &taskService;
        CollectionService &collectionService;
        ReloadCoordinator &reloads;
        const WindowServices &services;
    };

    explicit CalendarPaneController(CalendarPane *pane, const Deps &deps, QObject *parent = nullptr);

private:
    void newEvent(const QTime &initialStartTime, const QTime &initialEndTime);
    void updateEvent(const EventOccurrence &selectedEvent);
    void rescheduleEvent(const EventOccurrence &currentEvent, const QTime &startTime, const QTime &endTime);
    void quickAddTask(const QString &title);
    void newTask();
    void editTask(const Task &task);
    void deleteTaskWithPrompt(const Task &task);
    void deleteEventWithPrompt(const EventOccurrence &event);
    void setEventCompleted(const EventOccurrence &event, bool completed);
    void saveTaskInlineEdit(int row, const Task &task);
    void findCalendarItem(bool forward,
                          const QString &needle,
                          const QDate &startDate,
                          const EventOccurrence &currentEvent,
                          const Task &currentTask);
    QList<QPair<QString, QString>> writableCalendarOptions() const;
    void reloadCalendar();
    void warnNoWritableCalendar() const;

    CalendarPane *m_pane = nullptr;
    Deps m_deps;
    EventDialogFlow m_eventFlow;
    TaskDialogFlow m_taskFlow;
};

#endif // CALENDARPANECONTROLLER_H
