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

#ifndef RELOADCOORDINATOR_H
#define RELOADCOORDINATOR_H

#include "cancellationtoken.h"
#include "eventoccurrences.h"
#include "calendaritem.h"
#include "contactsnapshot.h"
#include "storageresult.h"

#include <QDate>
#include <QFutureWatcher>
#include <QObject>
#include <QTimer>

#include <atomic>

class EventService;
class ContactService;
class CollectionService;
class TaskService;

class ReloadCoordinator : public QObject
{
    Q_OBJECT

public:
    struct CalendarReloadPayload
    {
        int generation = 0;
        QList<EventOccurrence> eventList;
        QList<Task> taskList;
        QList<ReadFailure> readFailures;
    };

    struct ContactReloadPayload
    {
        int generation = 0;
        QList<Contact> contactList;
        QList<ReadFailure> readFailures;
    };

    ReloadCoordinator(const CollectionService &collectionService,
                      const EventService &eventService,
                      TaskService &taskService,
                      const ContactService &contactService,
                      QObject *parent = nullptr);
    ~ReloadCoordinator() override;

    void setCalendarWindow(const QDate &visibleStart, const QDate &visibleEnd, const QDate &today);
    void reloadCalendar();
    void reloadContacts();
    void reloadAll();

Q_SIGNALS:
    void calendarReloaded(const ReloadCoordinator::CalendarReloadPayload &payload);
    void contactsReloaded(const ReloadCoordinator::ContactReloadPayload &payload);
    void readFailures(const QList<ReadFailure> &readFailures);
    void todayAdvanced();

private:
    void rollForwardTasks(const QList<Task> &tasks);
    bool stopping() const;

    const CollectionService &m_collectionService;
    const EventService &m_eventService;
    TaskService &m_taskService;
    const ContactService &m_contactService;
    QFutureWatcher<CalendarReloadPayload> m_calendarReloadWatcher;
    QFutureWatcher<ContactReloadPayload> m_contactReloadWatcher;
    QTimer m_midnightTimer;
    QDate m_visibleStart;
    QDate m_visibleEnd;
    QDate m_today;
    bool m_calendarReloadPending = false;
    bool m_contactReloadPending = false;
    int m_calendarReloadGeneration = 0;
    int m_contactReloadGeneration = 0;
    std::atomic_bool m_stopping = false;
};

#endif // RELOADCOORDINATOR_H
