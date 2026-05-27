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

#include "calendarpanecontroller.h"

#include "calendarpaneutils.h"
#include "eventreader.h"
#include "eventservice.h"
#include "calendareditormapper.h"
#include "collectionservice.h"
#include "reloadcoordinator.h"
#include "futurewatch.h"
#include "operationcapabilitymessages.h"
#include "storageerrormessages.h"
#include "taskservice.h"

#include <QMessageBox>
#include <QTimer>

#include <algorithm>
#include <optional>

namespace {

struct CalendarFindMatch
{
    enum class Type
    {
        Event,
        Task
    };
    Type type = Type::Event;
    QDate date;
    QString sortKey;
    EventOccurrence event;
    Task task;
};

struct CalendarFindPosition
{
    CalendarFindMatch::Type type = CalendarFindMatch::Type::Event;
    QDate date;
    QTime time;
    QString sortKey;
};

QString eventSummary(const EventOccurrence &event)
{
    return CalendarSnapshot::eventDisplay(event).summary;
}

QString normalizedNeedle(const QString &needle)
{
    return needle.toCaseFolded();
}

bool textMatches(const QString &text, const QString &needle)
{
    return !needle.isEmpty() && text.toCaseFolded().contains(needle);
}

bool textListMatches(const QStringList &texts, const QString &needle)
{
    return std::any_of(
        texts.cbegin(), texts.cend(), [&needle](const QString &text) { return textMatches(text, needle); });
}

bool eventMatchesText(const EventOccurrence &event, const QString &needle)
{
    const EventDisplay display = CalendarSnapshot::eventDisplay(event);
    return textListMatches({display.summary, display.location, display.description}, needle);
}

bool taskMatchesText(const Task &task, const WindowServices &services, const QString &needle)
{
    if (!CalendarSnapshot::hasTask(task))
    {
        return false;
    }
    const CollectionSummary collection = services.summarizeCollection(CollectionKind::Calendar, task.ref.collectionId);
    const TaskDisplay display = CalendarSnapshot::taskDisplay(task, collection);
    return textListMatches({display.summary, display.description, display.collection.displayName}, needle);
}

QString eventSortKey(const EventOccurrence &event)
{
    return QStringLiteral("%1\n%2\n%3\n%4")
        .arg(event.ref.item.collectionId, event.ref.item.href, event.ref.uid, event.start.toString(Qt::ISODateWithMs));
}

QString taskSortKey(const Task &task)
{
    return QStringLiteral("%1\n%2\n%3").arg(task.ref.collectionId, task.ref.href, task.uid);
}

QTime matchTime(const CalendarFindMatch &match)
{
    return match.type == CalendarFindMatch::Type::Event ? match.event.start.time()
                                                        : CalendarPaneUtils::taskSortTime(match.task);
}

int compareFindOrder(const QDate &leftDate,
                     const QTime &leftTime,
                     CalendarFindMatch::Type leftType,
                     const QString &leftSortKey,
                     const QDate &rightDate,
                     const QTime &rightTime,
                     CalendarFindMatch::Type rightType,
                     const QString &rightSortKey)
{
    if (leftDate != rightDate)
    {
        return leftDate < rightDate ? -1 : 1;
    }
    if (leftTime != rightTime)
    {
        return leftTime < rightTime ? -1 : 1;
    }
    if (leftType == rightType)
    {
        return QString::compare(leftSortKey, rightSortKey, Qt::CaseInsensitive);
    }
    return static_cast<int>(leftType) < static_cast<int>(rightType) ? -1 : 1;
}

int compareFindMatches(const CalendarFindMatch &left, const CalendarFindMatch &right)
{
    return compareFindOrder(
        left.date, matchTime(left), left.type, left.sortKey, right.date, matchTime(right), right.type, right.sortKey);
}

bool isAfterPosition(const CalendarFindMatch &match, const CalendarFindPosition &position)
{
    return compareFindOrder(match.date,
                            matchTime(match),
                            match.type,
                            match.sortKey,
                            position.date,
                            position.time,
                            position.type,
                            position.sortKey) > 0;
}

bool isBeforePosition(const CalendarFindMatch &match, const CalendarFindPosition &position)
{
    return compareFindOrder(match.date,
                            matchTime(match),
                            match.type,
                            match.sortKey,
                            position.date,
                            position.time,
                            position.type,
                            position.sortKey) < 0;
}

std::optional<CalendarFindPosition>
currentFindPosition(const EventOccurrence &currentEvent, const Task &currentTask, const QDate &anchorDate)
{
    if (CalendarSnapshot::hasEventRef(currentEvent))
    {
        return CalendarFindPosition{CalendarFindMatch::Type::Event,
                                    currentEvent.start.date(),
                                    currentEvent.start.time(),
                                    eventSortKey(currentEvent)};
    }
    if (CalendarSnapshot::hasTask(currentTask))
    {
        const QDate dueDate = CalendarSnapshot::taskDueDate(currentTask);
        return CalendarFindPosition{CalendarFindMatch::Type::Task,
                                    dueDate.isValid() ? dueDate : anchorDate,
                                    CalendarPaneUtils::taskSortTime(currentTask),
                                    taskSortKey(currentTask)};
    }
    return std::nullopt;
}

std::optional<CalendarFindMatch> findTaskMatch(CalendarPane *pane,
                                               const WindowServices &services,
                                               bool forward,
                                               const QString &needle,
                                               const QDate &startDate,
                                               const std::optional<CalendarFindPosition> &position)
{
    QList<CalendarFindMatch> matches;
    for (const Task &task : pane->visibleTasks())
    {
        if (!CalendarSnapshot::hasTask(task))
        {
            continue;
        }
        if (taskMatchesText(task, services, needle))
        {
            const QDate dueDate = CalendarSnapshot::taskDueDate(task);
            matches.append(
                {CalendarFindMatch::Type::Task, dueDate.isValid() ? dueDate : startDate, taskSortKey(task), {}, task});
        }
    }
    std::sort(matches.begin(), matches.end(), [](const CalendarFindMatch &first, const CalendarFindMatch &second) {
        return compareFindMatches(first, second) < 0;
    });

    if (forward)
    {
        for (const CalendarFindMatch &match : matches)
        {
            if (position ? isAfterPosition(match, *position) : match.date >= startDate)
            {
                return match;
            }
        }
        return std::nullopt;
    }

    for (auto it = matches.crbegin(); it != matches.crend(); ++it)
    {
        if (position ? isBeforePosition(*it, *position) : it->date <= startDate)
        {
            return *it;
        }
    }
    return std::nullopt;
}

std::optional<CalendarFindMatch> findSameDayEventAroundTask(CalendarPane *pane,
                                                            bool forward,
                                                            const QString &needle,
                                                            const CalendarFindPosition &position)
{
    QList<CalendarFindMatch> matches;
    for (const EventOccurrence &event : pane->sortedDayEvents(position.date))
    {
        CalendarFindMatch match{CalendarFindMatch::Type::Event, event.start.date(), eventSortKey(event), event, {}};
        if (eventMatchesText(event, needle) &&
            (forward ? isAfterPosition(match, position) : isBeforePosition(match, position)))
        {
            matches.append(match);
        }
    }
    if (matches.isEmpty())
    {
        return std::nullopt;
    }
    return forward ? matches.constFirst() : matches.constLast();
}

std::optional<CalendarFindMatch> findEventMatch(CalendarPane *pane,
                                                const EventReader &eventReader,
                                                bool forward,
                                                const QString &needle,
                                                const QDate &startDate,
                                                const EventOccurrence &currentEvent,
                                                const std::optional<CalendarFindPosition> &position)
{
    if (position && position->type == CalendarFindMatch::Type::Task)
    {
        const std::optional<CalendarFindMatch> sameDay = findSameDayEventAroundTask(pane, forward, needle, *position);
        if (sameDay)
        {
            return sameDay;
        }
        const QDate nextDate = forward ? position->date.addDays(1) : position->date.addDays(-1);
        const std::optional<EventOccurrence> match = eventReader.searchOccurrences(
            needle,
            nextDate,
            forward ? EventReader::SearchDirection::Forward : EventReader::SearchDirection::Backward,
            {},
            [](const QString &collectionId) { return !collectionId.isEmpty(); });
        return match ? std::optional<CalendarFindMatch>(CalendarFindMatch{
                           CalendarFindMatch::Type::Event, match->start.date(), eventSortKey(*match), *match, {}})
                     : std::nullopt;
    }

    const std::optional<EventOccurrence> match = eventReader.searchOccurrences(
        needle,
        startDate,
        forward ? EventReader::SearchDirection::Forward : EventReader::SearchDirection::Backward,
        currentEvent,
        [](const QString &collectionId) { return !collectionId.isEmpty(); });
    return match ? std::optional<CalendarFindMatch>(CalendarFindMatch{
                       CalendarFindMatch::Type::Event, match->start.date(), eventSortKey(*match), *match, {}})
                 : std::nullopt;
}

std::optional<CalendarFindMatch> nearestMatch(bool forward,
                                              const std::optional<CalendarFindMatch> &eventMatch,
                                              const std::optional<CalendarFindMatch> &taskMatch)
{
    if (!eventMatch)
    {
        return taskMatch;
    }
    if (!taskMatch)
    {
        return eventMatch;
    }
    const int order = compareFindMatches(*eventMatch, *taskMatch);
    return forward ? (order <= 0 ? eventMatch : taskMatch) : (order >= 0 ? eventMatch : taskMatch);
}

QDate firstDateOfMonth(int year, int month)
{
    return QDate(year, month, 1);
}

QDate lastDateOfMonth(int year, int month)
{
    const QDate first = firstDateOfMonth(year, month);
    return first.isValid() ? first.addMonths(1).addDays(-1) : QDate();
}

} // namespace

CalendarPaneController::CalendarPaneController(CalendarPane *pane, const Deps &deps, QObject *parent)
    : QObject(parent)
    , m_pane(pane)
    , m_deps(deps)
    , m_eventFlow(m_pane, &m_deps.eventReader, m_deps.services)
    , m_taskFlow(m_pane, m_deps.services)
{
    Q_ASSERT(m_pane);
    connect(
        &deps.collectionService, &CollectionService::itemWritten, this, [this](const ItemRef &) { reloadCalendar(); });
    connect(m_pane, &CalendarPane::viewWindowChanged, this, [this](const QDate &) { reloadCalendar(); });
    connect(m_pane, &CalendarPane::newEventRequested, this, &CalendarPaneController::newEvent);
    connect(m_pane, &CalendarPane::eventEditRequested, this, &CalendarPaneController::updateEvent);
    connect(m_pane, &CalendarPane::eventRescheduleRequested, this, &CalendarPaneController::rescheduleEvent);
    connect(m_pane, &CalendarPane::quickTaskAddRequested, this, &CalendarPaneController::quickAddTask);
    connect(m_pane, &CalendarPane::newTaskRequested, this, &CalendarPaneController::newTask);
    connect(m_pane, &CalendarPane::taskEditRequested, this, &CalendarPaneController::editTask);
    connect(m_pane, &CalendarPane::taskDeleteRequested, this, &CalendarPaneController::deleteTaskWithPrompt);
    connect(m_pane, &CalendarPane::eventDeleteRequested, this, &CalendarPaneController::deleteEventWithPrompt);
    connect(m_pane, &CalendarPane::eventCompletionRequested, this, &CalendarPaneController::setEventCompleted);
    connect(m_pane, &CalendarPane::taskInlineEditCommitted, this, &CalendarPaneController::saveTaskInlineEdit);
    connect(m_pane, &CalendarPane::calendarFindRequested, this, &CalendarPaneController::findCalendarItem);
}

void CalendarPaneController::newEvent(const QTime &initialStartTime, const QTime &initialEndTime)
{
    const QList<QPair<QString, QString>> collectionOptions = writableCalendarOptions();
    if (collectionOptions.isEmpty())
    {
        warnNoWritableCalendar();
        return;
    }

    const auto result = m_eventFlow.create(m_pane->selectedDate(), collectionOptions, initialStartTime, initialEndTime);
    if (!result)
    {
        return;
    }

    const EventFields eventData = result->fields;
    m_pane->setSelectedDate(eventData.date);
    const OperationCapability capability = m_deps.eventService.canCreateEvent(eventData.collectionId);
    if (!OperationCapabilityMessages::presentCapability(m_pane, capability, tr("save"), tr("event")))
    {
        return;
    }
    FutureWatcher::watch(
        this, m_deps.eventService.addEventAsync(eventData), [this](const EventService::EventSaveResult &result) {
            if (!StorageErrorMessages::presentOutcome(m_pane, tr("save"), tr("event"), {result.status}))
            {
                return;
            }
            reloadCalendar();
        });
}

void CalendarPaneController::updateEvent(const EventOccurrence &selectedEvent)
{
    EventOccurrence currentEvent = selectedEvent;
    const OperationCapability editCapability = m_deps.eventService.canEditEvent(currentEvent);
    if (!OperationCapabilityMessages::presentCapability(m_pane, editCapability, tr("edit"), tr("event")))
    {
        return;
    }

    const QList<QPair<QString, QString>> collectionOptions = writableCalendarOptions();
    const auto result = m_eventFlow.edit(currentEvent, collectionOptions);
    if (!result)
    {
        return;
    }

    const EventFields eventData = result->fields;
    m_pane->setSelectedDate(eventData.date);
    const QString destinationCollectionId = result->destinationCollectionId;
    const bool moving = destinationCollectionId != currentEvent.ref.item.collectionId;
    const OperationCapability saveCapability =
        moving ? m_deps.eventService.canMoveEvent(currentEvent, destinationCollectionId)
               : m_deps.eventService.canEditEvent(currentEvent);
    if (!OperationCapabilityMessages::presentCapability(m_pane, saveCapability, tr("save"), tr("event")))
    {
        return;
    }

    if (moving)
    {
        FutureWatcher::watch(
            this,
            m_deps.eventService.moveEventAsync(currentEvent, eventData, destinationCollectionId),
            [this](const EventService::EventMoveResult &moveResult) {
                if (!StorageErrorMessages::presentOutcome(m_pane, tr("save"), tr("event"), {moveResult.move}))
                {
                    return;
                }
                reloadCalendar();
            });
        return;
    }

    FutureWatcher::watch(
        this,
        m_deps.eventService.updateEventAsync(currentEvent, eventData),
        [this](const EventService::EventSaveResult &updateResult) {
            if (!StorageErrorMessages::presentOutcome(m_pane, tr("save"), tr("event"), {updateResult.status}))
            {
                return;
            }
            reloadCalendar();
        });
}

void CalendarPaneController::rescheduleEvent(const EventOccurrence &currentEvent,
                                             const QTime &startTime,
                                             const QTime &endTime)
{
    if (!startTime.isValid() || !endTime.isValid() || startTime >= endTime)
    {
        return;
    }
    const OperationCapability capability = m_deps.eventService.canEditEvent(currentEvent);
    if (!OperationCapabilityMessages::presentCapability(m_pane, capability, tr("save"), tr("event")))
    {
        return;
    }

    EventFields eventData = m_deps.eventReader.editorDataForOccurrence(currentEvent);
    eventData.date = currentEvent.start.date();
    eventData.startTime = startTime;
    eventData.endTime = endTime;
    FutureWatcher::watch(
        this,
        m_deps.eventService.updateEventAsync(currentEvent, eventData),
        [this](const EventService::EventSaveResult &updateResult) {
            if (!StorageErrorMessages::presentOutcome(m_pane, tr("save"), tr("event"), {updateResult.status}))
            {
                return;
            }
            reloadCalendar();
        });
}

void CalendarPaneController::quickAddTask(const QString &title)
{
    const QString trimmedTitle = title.trimmed();
    if (trimmedTitle.isEmpty())
    {
        return;
    }
    const QList<QPair<QString, QString>> collectionOptions = writableCalendarOptions();
    if (collectionOptions.isEmpty())
    {
        warnNoWritableCalendar();
        return;
    }

    TaskFields taskData;
    taskData.title = trimmedTitle;
    Task task = CalendarEditorMapper::createTask(taskData);
    const OperationCapability capability = m_deps.taskService.canCreateTask(collectionOptions.first().second);
    if (!OperationCapabilityMessages::presentCapability(m_pane, capability, tr("save"), tr("task")))
    {
        return;
    }

    FutureWatcher::watch(
        this,
        m_deps.taskService.addTaskAsync(task, collectionOptions.first().second),
        [this](const TaskService::TaskSaveResult &saveResult) {
            if (!StorageErrorMessages::presentOutcome(m_pane, tr("save"), tr("task"), {saveResult.status}))
            {
                return;
            }
            reloadCalendar();
        });
}

void CalendarPaneController::newTask()
{
    const QList<QPair<QString, QString>> collectionOptions = writableCalendarOptions();
    if (collectionOptions.isEmpty())
    {
        warnNoWritableCalendar();
        return;
    }

    const auto result = m_taskFlow.create(m_pane->selectedDate(), collectionOptions);
    if (!result)
    {
        return;
    }
    const OperationCapability capability = m_deps.taskService.canCreateTask(result->destinationCollectionId);
    if (!OperationCapabilityMessages::presentCapability(m_pane, capability, tr("save"), tr("task")))
    {
        return;
    }

    FutureWatcher::watch(
        this,
        m_deps.taskService.addTaskAsync(result->task, result->destinationCollectionId),
        [this](const TaskService::TaskSaveResult &saveResult) {
            if (!StorageErrorMessages::presentOutcome(m_pane, tr("save"), tr("task"), {saveResult.status}))
            {
                return;
            }
            reloadCalendar();
        });
}

void CalendarPaneController::editTask(const Task &task)
{
    const OperationCapability editCapability = m_deps.taskService.canEditTask(task);
    if (!OperationCapabilityMessages::presentCapability(m_pane, editCapability, tr("edit"), tr("task")))
    {
        return;
    }

    const QList<QPair<QString, QString>> collectionOptions = writableCalendarOptions();
    const auto result = m_taskFlow.edit(task, m_pane->selectedDate(), collectionOptions);
    if (!result)
    {
        return;
    }

    const Task editedTask = result->task;
    const QString collectionId = result->destinationCollectionId;
    const bool moving = !collectionId.isEmpty() && collectionId != editedTask.ref.collectionId;
    const OperationCapability saveCapability =
        moving ? m_deps.taskService.canMoveTask(editedTask, collectionId) : m_deps.taskService.canEditTask(editedTask);
    if (!OperationCapabilityMessages::presentCapability(m_pane, saveCapability, tr("save"), tr("task")))
    {
        return;
    }

    if (moving)
    {
        FutureWatcher::watch(
            this,
            m_deps.taskService.moveTaskAsync(editedTask, collectionId),
            [this](const TaskService::TaskMoveResult &moveResult) {
                if (!StorageErrorMessages::presentOutcome(m_pane, tr("move"), tr("task"), {moveResult.move}))
                {
                    return;
                }
                reloadCalendar();
            });
        return;
    }
    FutureWatcher::watch(
        this, m_deps.taskService.updateTaskAsync(editedTask), [this](const TaskService::TaskSaveResult &updateResult) {
            if (!StorageErrorMessages::presentOutcome(m_pane, tr("save"), tr("task"), {updateResult.status}))
            {
                return;
            }
            reloadCalendar();
        });
}

void CalendarPaneController::deleteTaskWithPrompt(const Task &task)
{
    const OperationCapability capability = m_deps.taskService.canDeleteTask(task);
    if (!OperationCapabilityMessages::presentCapability(m_pane, capability, tr("delete"), tr("task")))
    {
        return;
    }

    QString summary = CalendarSnapshot::taskDisplay(task).summary;
    if (summary.isEmpty())
    {
        summary = tr("selected task");
    }
    const int answer = QMessageBox::warning(m_pane,
                                            tr("Organizer"),
                                            tr("Delete task \"%1\"?").arg(summary),
                                            QMessageBox::Yes | QMessageBox::No,
                                            QMessageBox::No);
    if (answer != QMessageBox::Yes)
    {
        return;
    }

    FutureWatcher::watch(this, m_deps.taskService.deleteTaskAsync(task), [this](StorageStatus deleteStatus) {
        if (!StorageErrorMessages::presentOutcome(m_pane, tr("delete"), tr("task"), {deleteStatus}))
        {
            return;
        }
        reloadCalendar();
    });
}

void CalendarPaneController::deleteEventWithPrompt(const EventOccurrence &event)
{
    const OperationCapability capability = m_deps.eventService.canDeleteEvent(event.ref);
    if (!OperationCapabilityMessages::presentCapability(m_pane, capability, tr("delete"), tr("event")))
    {
        return;
    }

    QString summary = eventSummary(event);
    if (summary.isEmpty())
    {
        summary = tr("selected event");
    }
    const QString message = tr("Delete event \"%1\"?").arg(summary);
    const int answer =
        QMessageBox::warning(m_pane, tr("Organizer"), message, QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes)
    {
        return;
    }

    auto future = m_deps.eventService.deleteEventAsync(event.ref);
    FutureWatcher::watch(this, future, [this, ref = event.ref](StorageStatus deleteStatus) {
        if (!StorageErrorMessages::presentOutcome(m_pane, tr("delete"), tr("event"), {deleteStatus}))
        {
            return;
        }
        m_pane->removeEvent(ref);
    });
}

void CalendarPaneController::setEventCompleted(const EventOccurrence &event, bool completed)
{
    const OperationCapability capability = m_deps.eventService.canCompleteEvent(event);
    if (!OperationCapabilityMessages::presentCapability(m_pane, capability, tr("save"), tr("event")))
    {
        return;
    }

    EventFields eventData = m_deps.eventReader.editorDataForOccurrence(event);
    eventData.completed = completed;
    FutureWatcher::watch(
        this,
        m_deps.eventService.updateEventAsync(event, eventData),
        [this](const EventService::EventSaveResult &updateResult) {
            if (!StorageErrorMessages::presentOutcome(m_pane, tr("save"), tr("event"), {updateResult.status}))
            {
                return;
            }
            reloadCalendar();
        });
}

void CalendarPaneController::saveTaskInlineEdit(int row, const Task &task)
{
    const OperationCapability capability = m_deps.taskService.canEditTask(task);
    if (!OperationCapabilityMessages::presentCapability(m_pane, capability, tr("save"), tr("task")))
    {
        return;
    }
    FutureWatcher::watch(
        this, m_deps.taskService.updateTaskAsync(task), [this, row](const TaskService::TaskSaveResult &saveResult) {
            if (!StorageErrorMessages::presentOutcome(m_pane, tr("save"), tr("task"), {saveResult.status}))
            {
                return;
            }
            if (m_pane)
            {
                m_pane->replaceInlineEditedTask(row, saveResult.snapshot);
            }
            QTimer::singleShot(0, &m_deps.reloads, &ReloadCoordinator::reloadCalendar);
        });
}

void CalendarPaneController::findCalendarItem(bool forward,
                                              const QString &needle,
                                              const QDate &startDate,
                                              const EventOccurrence &currentEvent,
                                              const Task &currentTask)
{
    const QString foldedNeedle = normalizedNeedle(needle);
    const std::optional<CalendarFindPosition> position = currentFindPosition(currentEvent, currentTask, startDate);
    const std::optional<CalendarFindMatch> eventMatch =
        findEventMatch(m_pane, m_deps.eventReader, forward, foldedNeedle, startDate, currentEvent, position);
    const std::optional<CalendarFindMatch> taskMatch =
        findTaskMatch(m_pane, m_deps.services, forward, foldedNeedle, startDate, position);
    const std::optional<CalendarFindMatch> match = nearestMatch(forward, eventMatch, taskMatch);
    if (!match)
    {
        m_pane->showCalendarFindNoMatch();
        return;
    }

    if (match->type == CalendarFindMatch::Type::Event)
    {
        m_pane->showFoundCalendarEvent(match->event);
        return;
    }
    m_pane->showFoundCalendarTask(match->task);
}

QList<QPair<QString, QString>> CalendarPaneController::writableCalendarOptions() const
{
    const std::optional<Collection> collection = m_deps.collectionService.defaultCalendar();
    if (!collection)
    {
        return {};
    }
    return {qMakePair(collection->displayName, collection->id)};
}

void CalendarPaneController::reloadCalendar()
{
    const QDate date = m_pane->selectedDate().isValid() ? m_pane->selectedDate() : QDate::currentDate();
    m_deps.reloads.setCalendarWindow(
        firstDateOfMonth(date.year(), date.month()), lastDateOfMonth(date.year(), date.month()), QDate::currentDate());
    m_deps.reloads.reloadCalendar();
}

void CalendarPaneController::warnNoWritableCalendar() const
{
    QMessageBox::warning(m_pane, tr("Organizer"), tr("No writable calendar collection is available."));
}
