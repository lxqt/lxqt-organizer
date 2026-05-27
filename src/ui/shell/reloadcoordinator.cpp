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

#include "reloadcoordinator.h"

#include "completedfuture.h"
#include "eventreader.h"
#include "contactservice.h"
#include "futurewatch.h"
#include "taskservice.h"

#include <QDateTime>
#include <QDebug>
#include <QFuture>
#include <QList>
#include <QLoggingCategory>
#include <QTimeZone>

#include <variant>

Q_STATIC_LOGGING_CATEGORY(reloadCoordinatorLog, "lxqt.organizer.ui.shell.reload")

namespace {

bool sameOccurrence(const EventOccurrence &first, const EventOccurrence &second)
{
    return first.ref.item.collectionId == second.ref.item.collectionId && first.ref.item.href == second.ref.item.href &&
           first.ref.uid == second.ref.uid && first.start == second.start;
}

bool sameReadFailure(const ReadFailure &first, const ReadFailure &second)
{
    return first.ref.collectionId == second.ref.collectionId && first.ref.href == second.ref.href &&
           first.status == second.status;
}

void appendUniqueReadFailures(QList<ReadFailure> &target, const QList<ReadFailure> &source)
{
    for (const ReadFailure &failure : source)
    {
        if (!failure.isFailure())
        {
            continue;
        }

        bool found = false;
        for (const ReadFailure &existing : std::as_const(target))
        {
            if (sameReadFailure(existing, failure))
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            target.append(failure);
        }
    }
}

void appendUniqueEventOccurrences(QList<EventOccurrence> &target, const QList<EventOccurrence> &source)
{
    for (const EventOccurrence &event : source)
    {
        bool found = false;
        for (EventOccurrence &existing : target)
        {
            if (sameOccurrence(existing, event))
            {
                if (existing.ref.item.etag != event.ref.item.etag)
                {
                    existing = event;
                }
                found = true;
                break;
            }
        }
        if (!found)
        {
            target.append(event);
        }
    }
}

} // namespace

ReloadCoordinator::ReloadCoordinator(const EventReader &eventReader,
                                     TaskService &taskService,
                                     const ContactService &contactService,
                                     QObject *parent)
    : QObject(parent)
    , m_eventReader(eventReader)
    , m_taskService(taskService)
    , m_contactService(contactService)
{
    m_midnightTimer.setSingleShot(true);
    connect(&m_midnightTimer, &QTimer::timeout, this, [this]() {
        const QDate today = QDate::currentDate();
        if (m_today != today)
        {
            m_today = today;
            Q_EMIT todayAdvanced();
            reloadCalendar();
        }
        const QDateTime now = QDateTime::currentDateTime();
        const QDateTime nextMidnight(QDate::currentDate().addDays(1), QTime(0, 0), QTimeZone::systemTimeZone());
        m_midnightTimer.start(static_cast<int>(qMax<qint64>(1000, now.msecsTo(nextMidnight))));
    });
    const QDateTime now = QDateTime::currentDateTime();
    const QDateTime nextMidnight(QDate::currentDate().addDays(1), QTime(0, 0), QTimeZone::systemTimeZone());
    m_midnightTimer.start(static_cast<int>(qMax<qint64>(1000, now.msecsTo(nextMidnight))));

    connect(&m_calendarReloadWatcher, &QFutureWatcher<CalendarReloadPayload>::finished, this, [this]() {
        const CalendarReloadPayload payload = m_calendarReloadWatcher.result();
        if (!stopping() && payload.generation == m_calendarReloadGeneration)
        {
            Q_EMIT calendarReloaded(payload);
            rollForwardTasks(payload.taskList);
            if (!payload.readFailures.isEmpty())
            {
                Q_EMIT readFailures(payload.readFailures);
            }
        }
        if (!stopping() && m_calendarReloadPending)
        {
            m_calendarReloadPending = false;
            reloadCalendar();
        }
    });

    connect(&m_contactReloadWatcher, &QFutureWatcher<ContactReloadPayload>::finished, this, [this]() {
        const ContactReloadPayload payload = m_contactReloadWatcher.result();
        if (!stopping() && payload.generation == m_contactReloadGeneration)
        {
            Q_EMIT contactsReloaded(payload);
            if (!payload.readFailures.isEmpty())
            {
                Q_EMIT readFailures(payload.readFailures);
            }
        }
        if (!stopping() && m_contactReloadPending)
        {
            m_contactReloadPending = false;
            reloadContacts();
        }
    });
}

ReloadCoordinator::~ReloadCoordinator()
{
    m_stopping.store(true);
    m_calendarReloadPending = false;
    m_contactReloadPending = false;
    m_midnightTimer.stop();
    disconnect(&m_calendarReloadWatcher, nullptr, this, nullptr);
    disconnect(&m_contactReloadWatcher, nullptr, this, nullptr);
    if (m_calendarReloadWatcher.isRunning())
    {
        qCInfo(reloadCoordinatorLog) << "Waiting for calendar reload job during shutdown";
        m_calendarReloadWatcher.waitForFinished();
    }
    if (m_contactReloadWatcher.isRunning())
    {
        qCInfo(reloadCoordinatorLog) << "Waiting for contact reload job during shutdown";
        m_contactReloadWatcher.waitForFinished();
    }
}

void ReloadCoordinator::setCalendarWindow(const QDate &visibleStart, const QDate &visibleEnd, const QDate &today)
{
    const bool dayChanged = m_today.isValid() && today.isValid() && m_today != today;
    m_visibleStart = visibleStart;
    m_visibleEnd = visibleEnd;
    m_today = today;
    if (dayChanged)
    {
        Q_EMIT todayAdvanced();
    }
}

void ReloadCoordinator::reloadCalendar()
{
    if (stopping())
    {
        return;
    }
    ++m_calendarReloadGeneration;
    if (m_calendarReloadWatcher.isRunning())
    {
        m_calendarReloadPending = true;
        return;
    }

    const int generation = m_calendarReloadGeneration;
    const QDate visibleStart = m_visibleStart;
    const QDate visibleEnd = m_visibleEnd;
    const QDate today = m_today.isValid() ? m_today : QDate::currentDate();
    const CancellationToken cancellation(m_stopping);

    const QFuture<EventOccurrenceListResult> visibleRangeFuture =
        (visibleStart.isValid() && visibleEnd.isValid())
            ? m_eventReader.eventOccurrencesInDateRangeAsync(visibleStart, visibleEnd, cancellation)
            : CalendarIoUtils::completedFuture(EventOccurrenceListResult{});
    const QFuture<EventOccurrenceListResult> todayRangeFuture =
        m_eventReader.eventOccurrencesInDateRangeAsync(today, today.addDays(1), cancellation);
    const QFuture<TaskSnapshotListResult> taskFuture = m_taskService.taskSnapshotsAsync(cancellation);

    using CalendarReloadVariant = std::variant<QFuture<EventOccurrenceListResult>,
                                               QFuture<EventOccurrenceListResult>,
                                               QFuture<TaskSnapshotListResult>>;
    QFuture<CalendarReloadPayload> payloadFuture =
        QtFuture::whenAll(visibleRangeFuture, todayRangeFuture, taskFuture)
            .then(QtFuture::Launch::Sync, [generation](const QList<CalendarReloadVariant> &done) {
                CalendarReloadPayload payload;
                payload.generation = generation;
                EventOccurrenceListResult visibleResult = std::get<0>(done.at(0)).result();
                EventOccurrenceListResult todayResult = std::get<1>(done.at(1)).result();
                TaskSnapshotListResult taskResult = std::get<2>(done.at(2)).result();
                appendUniqueEventOccurrences(payload.eventList, visibleResult.occurrences);
                appendUniqueReadFailures(payload.readFailures, visibleResult.readFailures);
                appendUniqueEventOccurrences(payload.eventList, todayResult.occurrences);
                appendUniqueReadFailures(payload.readFailures, todayResult.readFailures);
                payload.taskList = std::move(taskResult.tasks);
                appendUniqueReadFailures(payload.readFailures, taskResult.readFailures);
                return payload;
            });
    m_calendarReloadWatcher.setFuture(payloadFuture);
}

void ReloadCoordinator::reloadContacts()
{
    if (stopping())
    {
        return;
    }
    ++m_contactReloadGeneration;
    if (m_contactReloadWatcher.isRunning())
    {
        m_contactReloadPending = true;
        return;
    }

    const int generation = m_contactReloadGeneration;
    const CancellationToken cancellation(m_stopping);
    QFuture<ContactReloadPayload> payloadFuture =
        m_contactService.contactSnapshotsAsync(cancellation)
            .then(QtFuture::Launch::Sync, [generation](ContactSnapshotListResult result) {
                ContactReloadPayload payload;
                payload.generation = generation;
                payload.contactList = std::move(result.contacts);
                payload.readFailures = std::move(result.readFailures);
                return payload;
            });
    m_contactReloadWatcher.setFuture(payloadFuture);
}

void ReloadCoordinator::reloadAll()
{
    reloadCalendar();
    reloadContacts();
}

void ReloadCoordinator::rollForwardTasks(const QList<Task> &tasks)
{
    if (stopping())
    {
        return;
    }
    const QDate today = QDate::currentDate();
    for (const QFuture<TaskService::TaskSaveResult> &future : m_taskService.rollForwardOverdueTasksAsync(tasks, today))
    {
        FutureWatcher::watch(this, future, [this](const TaskService::TaskSaveResult &saveResult) {
            if (saveResult.isOk())
            {
                reloadCalendar();
            }
            else
            {
                qCWarning(reloadCoordinatorLog)
                    << "Auto roll-forward of overdue task failed" << storageStatusName(saveResult.status);
            }
        });
    }
}

bool ReloadCoordinator::stopping() const
{
    return m_stopping.load();
}
