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

#include "calendarpane.h"
#include "ui_calendarpane.h"

#include "calendarpaneutils.h"
#include "tasklistwidget.h"
#include "daytimelinewidget.h"
#include "findbar.h"
#include "monthchromewidget.h"

#include "collectioncatalog.h"
#include "mainwindowservices.h"
#include "preferencescontroller.h"

#include <QAction>
#include <QCoreApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QSplitter>

#include <algorithm>
#include <utility>

using namespace CalendarPaneUtils;

namespace {

QString completionStatusText(bool completed)
{
    return completed ? QCoreApplication::translate("CalendarPane", "Completed")
                     : QCoreApplication::translate("CalendarPane", "Not Completed");
}

QString shortDateTimeStatusText(const QDateTime &dateTime, const QLocale &locale)
{
    return QStringLiteral("%1, %2").arg(locale.toString(dateTime.date(), QLocale::ShortFormat),
                                        locale.toString(dateTime.time(), QLocale::ShortFormat));
}

QString eventTimeStatusText(const EventDisplay &display, const QLocale &locale)
{
    if (display.allDay)
    {
        return QCoreApplication::translate("CalendarPane", "All day");
    }

    if (!display.start.isValid())
    {
        return {};
    }

    if (!display.end.isValid())
    {
        return QCoreApplication::translate("CalendarPane", "Time: %1")
            .arg(locale.toString(display.start.time(), QLocale::ShortFormat));
    }

    if (display.start.date() == display.end.date())
    {
        return QCoreApplication::translate("CalendarPane", "Time: %1 - %2")
            .arg(locale.toString(display.start.time(), QLocale::ShortFormat),
                 locale.toString(display.end.time(), QLocale::ShortFormat));
    }

    return QStringLiteral("%1 - %2").arg(shortDateTimeStatusText(display.start, locale),
                                         shortDateTimeStatusText(display.end, locale));
}

QString taskDueStatusText(const Task &task, const QLocale &locale)
{
    const QDate dueDate = CalendarSnapshot::taskDueDate(task);
    if (!dueDate.isValid())
    {
        return {};
    }
    return QCoreApplication::translate("CalendarPane", "Due: %1").arg(locale.toString(dueDate, QLocale::ShortFormat));
}

} // namespace

CalendarPane::CalendarPane(const Deps &deps, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CalendarPane)
    , m_deps(deps)
{
    ui->setupUi(this);
    createActions();

    ui->monthChrome->installCalendarEventFilter(this);
    ui->calendarSplitter->setChildrenCollapsible(false);
    ui->calendarSplitter->setStretchFactor(0, 0);
    ui->calendarSplitter->setStretchFactor(1, 1);
    ui->monthChrome->setLocale(m_deps.preferences.locale());
    ui->taskList->installTaskViewEventFilter(this);
    ui->taskList->setCollectionSummaryProvider([services = &m_deps.services](const QString &collectionId) {
        return summarizeCollection(*services->catalogSnapshot(), CollectionKind::Calendar, collectionId);
    });
    ui->dayTimeline->installEventFilter(this);

    connect(ui->monthChrome, &MonthChromeWidget::previousMonthRequested, this, &CalendarPane::gotoPreviousMonth);
    connect(ui->monthChrome, &MonthChromeWidget::todayRequested, this, &CalendarPane::gotoToday);
    connect(ui->monthChrome, &MonthChromeWidget::nextMonthRequested, this, &CalendarPane::gotoNextMonth);
    connect(ui->monthChrome, &MonthChromeWidget::dateSelected, this, [this](const QDate &date) {
        setSelectedDate(date);
        Q_EMIT viewWindowChanged(m_selectedDate);
    });
    connect(ui->monthChrome, &MonthChromeWidget::dateActivated, this, [this](const QDate &date) {
        setSelectedDate(date);
        Q_EMIT viewWindowChanged(m_selectedDate);
        newEvent();
    });
    connect(ui->monthChrome, &MonthChromeWidget::newEventRequested, this, [this](const QDate &date) {
        setSelectedDate(date);
        Q_EMIT viewWindowChanged(m_selectedDate);
        newEvent();
    });
    connect(ui->monthChrome, &MonthChromeWidget::newTaskRequested, this, [this](const QDate &date) {
        setSelectedDate(date);
        Q_EMIT viewWindowChanged(m_selectedDate);
        newTask();
    });
    connect(ui->taskList, &TaskListWidget::selectionChanged, this, [this]() {
        if (ui->taskList->hasSelectedTask())
        {
            ui->dayTimeline->clearSelection();
        }
        updateActionState();
    });
    connect(ui->taskList, &TaskListWidget::taskActivated, this, &CalendarPane::editTask);
    connect(ui->taskList, &TaskListWidget::quickAddRequested, this, &CalendarPane::quickAddTask);
    connect(ui->taskList, &TaskListWidget::newTaskRequested, this, &CalendarPane::newTask);
    connect(ui->taskList, &TaskListWidget::taskEditRequested, this, &CalendarPane::editTask);
    connect(ui->taskList, &TaskListWidget::taskDeleteRequested, this, &CalendarPane::taskDeleteRequested);
    connect(ui->taskList, &TaskListWidget::inlineEditFinished, this, &CalendarPane::taskInlineEditCommitted);
    connect(ui->dayTimeline, &DayTimelineWidget::eventActivated, this, &CalendarPane::updateEvent);
    connect(ui->dayTimeline, &DayTimelineWidget::eventSelected, this, [this](const EventOccurrence &) {
        ui->taskList->clearSelection();
        updateActionState();
    });
    connect(ui->dayTimeline, &DayTimelineWidget::eventTimeChangeRequested, this, &CalendarPane::rescheduleEvent);
    connect(ui->dayTimeline, &DayTimelineWidget::eventCreationRequested, this, &CalendarPane::newEvent);
    connect(
        ui->dayTimeline,
        &DayTimelineWidget::contextMenuRequested,
        this,
        [this](
            const EventOccurrence &occurrence, const QTime &startTime, const QTime &endTime, const QPoint &globalPos) {
            showTimelineContextMenu(
                this,
                occurrence,
                startTime,
                endTime,
                globalPos,
                [this](const QTime &start, const QTime &end) { newEvent(start, end); },
                [this](const EventOccurrence &event) { updateEvent(event); },
                [this](const EventOccurrence &event, bool completed) { setEventCompleted(event, completed); },
                [this](const EventOccurrence &event) { deleteEventWithPrompt(event); });
        });
    connect(ui->findBar, &FindBar::findRequested, this, [this](const QString &needle, bool forward) {
        const QDate startDate = m_selectedDate.isValid() ? m_selectedDate : QDate::currentDate();
        Q_EMIT calendarFindRequested(forward, needle, startDate, ui->dayTimeline->selectedEvent(), selectedTask());
    });
    connect(&m_deps.reloads,
            &ReloadCoordinator::calendarReloaded,
            this,
            &CalendarPane::handleCalendarReloaded,
            Qt::UniqueConnection);
    connect(&m_deps.reloads,
            &ReloadCoordinator::todayAdvanced,
            this,
            &CalendarPane::handleTodayAdvanced,
            Qt::UniqueConnection);
}

CalendarPane::~CalendarPane()
{
    delete ui;
}

void CalendarPane::createActions()
{
    const CalendarPaneActions actions = createCalendarPaneActions(this);
    m_actionNewEvent = actions.newEvent;
    m_actionNewTask = actions.newTask;
    m_actionEditEvent = actions.editItem;
    m_actionDeleteEvent = actions.deleteItem;
    m_actionFind = actions.find;
    m_actionFindNext = actions.findNext;
    m_actionFindPrev = actions.findPrevious;

    connect(m_actionNewEvent, &QAction::triggered, this, [this]() { newEvent(); });
    connect(m_actionNewTask, &QAction::triggered, this, &CalendarPane::newTask);
    connect(m_actionEditEvent, &QAction::triggered, this, &CalendarPane::editSelectedCalendarItem);
    connect(m_actionDeleteEvent, &QAction::triggered, this, &CalendarPane::deleteSelectedCalendarItem);
    connect(m_actionFind, &QAction::triggered, this, &CalendarPane::showFindBar);
    connect(m_actionFindNext, &QAction::triggered, this, [this]() { findCalendarItem(true); });
    connect(m_actionFindPrev, &QAction::triggered, this, [this]() { findCalendarItem(false); });
    updateActionState();
}

bool CalendarPane::eventFilter(QObject *watched, QEvent *event)
{
    const bool watchedCalendarWidget = ui->monthChrome->isCalendarWidget(watched);
    const bool watchedTaskView = ui->taskList->isTaskView(watched);
    if ((watched != ui->dayTimeline && !watchedTaskView && !watchedCalendarWidget) || !event ||
        event->type() != QEvent::KeyPress || !isVisible())
    {
        return QWidget::eventFilter(watched, event);
    }

    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
    const int key = keyEvent->key();
    const Qt::KeyboardModifiers modifiers = keyEvent->modifiers();
    if (modifiers == Qt::ControlModifier && key == Qt::Key_Up)
    {
        return selectAdjacentCalendarItem(-1);
    }
    if (modifiers == Qt::ControlModifier && key == Qt::Key_Down)
    {
        return selectAdjacentCalendarItem(1);
    }
    if (modifiers != Qt::NoModifier)
    {
        return QWidget::eventFilter(watched, event);
    }

    switch (key)
    {
    case Qt::Key_Escape:
        if (!hasSelectedCalendarItem())
        {
            return QWidget::eventFilter(watched, event);
        }
        clearSelectedCalendarItem();
        return true;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        return editSelectedCalendarItem();
    case Qt::Key_Space:
        return toggleSelectedCalendarItemCompleted();
    case Qt::Key_Delete:
        return deleteSelectedCalendarItem();
    case Qt::Key_Left:
        return !hasSelectedCalendarItem() && moveSelectedDateBy(-1);
    case Qt::Key_Right:
        return !hasSelectedCalendarItem() && moveSelectedDateBy(1);
    case Qt::Key_Up:
        return !hasSelectedCalendarItem() && moveSelectedDateBy(-7);
    case Qt::Key_Down:
        return !hasSelectedCalendarItem() && moveSelectedDateBy(7);
    default:
        return QWidget::eventFilter(watched, event);
    }
}

QString CalendarPane::displayName() const
{
    return tr("Calendar");
}
CollectionKind CalendarPane::collectionKind() const
{
    return CollectionKind::Calendar;
}
int CalendarPane::visibleItemCount() const
{
    return m_visibleItemCount;
}
QSplitter *CalendarPane::calendarSplitter() const
{
    return ui->calendarSplitter;
}
QDate CalendarPane::selectedDate() const
{
    return m_selectedDate;
}
bool CalendarPane::taskPaneVisible() const
{
    return m_taskPaneVisible;
}
Task CalendarPane::selectedTask() const
{
    return ui->taskList->selectedTask();
}

QList<QAction *> CalendarPane::contextActions() const
{
    return {m_actionNewEvent,
            m_actionNewTask,
            m_actionEditEvent,
            m_actionDeleteEvent,
            m_actionFind,
            m_actionFindNext,
            m_actionFindPrev};
}

QString CalendarPane::selectionStatusText() const
{
    const EventOccurrence event = ui->dayTimeline->selectedEvent();
    if (CalendarSnapshot::hasEventRef(event) && !event.ref.item.href.isEmpty())
    {
        return selectedEventStatusText(event);
    }
    const Task task = selectedTask();
    if (CalendarSnapshot::hasTask(task))
    {
        return selectedTaskStatusText(task);
    }
    return tr("%1 event(s), %2 task(s)").arg(sortedDayEvents(m_selectedDate).size()).arg(ui->taskList->rowCount());
}

void CalendarPane::setCalendarFontSize(int fontSize)
{
    ui->monthChrome->setCalendarFontSize(fontSize);
}

void CalendarPane::setTimelineCollectionSummaryProvider(CollectionSummaryProvider provider)
{
    ui->dayTimeline->setCollectionSummaryProvider(std::move(provider));
}

void CalendarPane::setTimelineLocale(const QLocale &locale)
{
    ui->dayTimeline->setLocale(locale);
}

void CalendarPane::setDayViewTimeRange(int startHour, int finishHour)
{
    ui->dayTimeline->setDayViewTimeRange(startHour, finishHour);
}

void CalendarPane::replaceInlineEditedTask(int row, const Task &task)
{
    ui->taskList->replaceTask(row, task);
}

void CalendarPane::clearCalendarFindStatus()
{
    ui->findBar->clearStatus();
}

void CalendarPane::setCalendarData(const QList<EventOccurrence> &events, const QList<Task> &tasks)
{
    m_events = events;
    m_tasks = tasks;
    refreshTaskAndTimelineModels();
    ui->monthChrome->setCalendarData(m_events, m_tasks);
    renderMonthView();
}

void CalendarPane::removeEventByKey(const ItemKey &key)
{
    if (key.isValid() && removeEventsByKey(m_events, key))
    {
        refreshTaskAndTimelineModels();
        ui->monthChrome->setCalendarData(m_events, m_tasks);
    }
}

void CalendarPane::removeEvent(const OccurrenceRef &event)
{
    if (event.isValid() && removeEventRef(m_events, event))
    {
        refreshTaskAndTimelineModels();
        ui->monthChrome->setCalendarData(m_events, m_tasks);
    }
}

void CalendarPane::setSelectedDate(const QDate &date)
{
    if (!date.isValid())
    {
        return;
    }
    m_selectedDate = date;
    ui->dayTimeline->setDate(date);
    ui->monthChrome->setSelectedDate(date);
    refreshTaskAndTimelineModels();
    Q_EMIT selectedDateChanged(date);
}

void CalendarPane::renderMonthView()
{
    ui->monthChrome->setLocale(m_deps.preferences.locale());
    ui->monthChrome->renderMonthView();
    if (m_selectedDate.isValid())
    {
        ui->dayTimeline->setDate(m_selectedDate);
    }
}

void CalendarPane::newEvent(const QTime &startTime, const QTime &endTime)
{
    Q_EMIT newEventRequested(startTime, endTime);
}
void CalendarPane::updateEvent(const EventOccurrence &event)
{
    Q_EMIT eventEditRequested(event);
}
void CalendarPane::rescheduleEvent(const EventOccurrence &event, const QTime &startTime, const QTime &endTime)
{
    Q_EMIT eventRescheduleRequested(event, startTime, endTime);
}
void CalendarPane::quickAddTask(const QString &title)
{
    Q_EMIT quickTaskAddRequested(title);
}
void CalendarPane::newTask()
{
    Q_EMIT newTaskRequested();
}
void CalendarPane::editTask(const Task &task)
{
    Q_EMIT taskEditRequested(task);
}

bool CalendarPane::deleteSelectedTaskWithPrompt()
{
    const Task task = selectedTask();
    Q_EMIT taskDeleteRequested(task);
    return task.ref.isValid() && CalendarSnapshot::hasTask(task);
}

bool CalendarPane::deleteEventWithPrompt(const EventOccurrence &event)
{
    Q_EMIT eventDeleteRequested(event);
    return !event.ref.item.href.isEmpty() && CalendarSnapshot::hasEventRef(event);
}

bool CalendarPane::setEventCompleted(const EventOccurrence &event, bool completed)
{
    Q_EMIT eventCompletionRequested(event, completed);
    return !event.ref.item.href.isEmpty() && CalendarSnapshot::hasEventRef(event);
}

bool CalendarPane::hasSelectedCalendarItem() const
{
    const EventOccurrence event = ui->dayTimeline->selectedEvent();
    return (!event.ref.item.href.isEmpty() && CalendarSnapshot::hasEventRef(event)) || ui->taskList->hasSelectedTask();
}

void CalendarPane::clearSelectedCalendarItem()
{
    ui->dayTimeline->clearSelection();
    ui->taskList->clearSelection();
    updateActionState();
}

bool CalendarPane::editSelectedCalendarItem()
{
    const EventOccurrence event = ui->dayTimeline->selectedEvent();
    if (!event.ref.item.href.isEmpty() && CalendarSnapshot::hasEventRef(event))
    {
        updateEvent(event);
        return true;
    }
    const Task task = selectedTask();
    if (CalendarSnapshot::hasTask(task))
    {
        editTask(task);
        return true;
    }
    return false;
}

bool CalendarPane::toggleSelectedCalendarItemCompleted()
{
    const EventOccurrence event = ui->dayTimeline->selectedEvent();
    if (!event.ref.item.href.isEmpty() && CalendarSnapshot::hasEventRef(event))
    {
        return setEventCompleted(event, !CalendarSnapshot::eventDisplay(event).completed);
    }
    return ui->taskList->toggleSelectedTaskCompleted();
}

bool CalendarPane::deleteSelectedCalendarItem()
{
    const EventOccurrence event = ui->dayTimeline->selectedEvent();
    return (!event.ref.item.href.isEmpty() && CalendarSnapshot::hasEventRef(event)) ? deleteEventWithPrompt(event)
                                                                                    : deleteSelectedTaskWithPrompt();
}

bool CalendarPane::selectAdjacentCalendarItem(int delta)
{
    return CalendarPaneUtils::selectAdjacentCalendarItem(
        delta,
        m_selectedDate,
        sortedDayEvents(m_selectedDate),
        ui->taskList->tasksInDisplayOrder(),
        ui->dayTimeline->selectedEvent(),
        selectedTask(),
        [this](const EventOccurrence &event) { return ui->dayTimeline->selectEvent(event); },
        [this](const Task &task) { return ui->taskList->selectTask(task); });
}

bool CalendarPane::moveSelectedDateBy(int days)
{
    const QDate targetDate = m_selectedDate.addDays(days);
    if (!targetDate.isValid())
    {
        return false;
    }
    setSelectedDate(targetDate);
    Q_EMIT viewWindowChanged(m_selectedDate);
    return true;
}

void CalendarPane::gotoNextMonth()
{
    setSelectedDate(QDate(m_selectedDate.year(), m_selectedDate.month(), 1).addMonths(1));
    Q_EMIT viewWindowChanged(m_selectedDate);
}

void CalendarPane::gotoPreviousMonth()
{
    setSelectedDate(QDate(m_selectedDate.year(), m_selectedDate.month(), 1).addMonths(-1));
    Q_EMIT viewWindowChanged(m_selectedDate);
}

void CalendarPane::gotoToday()
{
    setSelectedDate(QDate::currentDate());
    Q_EMIT viewWindowChanged(m_selectedDate);
}

void CalendarPane::setTaskPaneVisible(bool visible)
{
    m_taskPaneVisible = visible;
    ui->taskList->setVisible(visible);
    ui->findBar->setVisible(visible && ui->findBar->isFindActive());
}

void CalendarPane::showFindBar()
{
    ui->findBar->showFind();
    ui->findBar->setVisible(m_taskPaneVisible);
}

void CalendarPane::hideFindBar()
{
    ui->findBar->hideFind();
}

bool CalendarPane::findCalendarItem(bool forward)
{
    if (!ui->findBar->isFindActive())
    {
        showFindBar();
    }
    return ui->findBar->requestFind(forward);
}

void CalendarPane::showCalendarFindNoMatch()
{
    ui->findBar->showNoMatch();
}

void CalendarPane::showFoundCalendarEvent(const EventOccurrence &event)
{
    const QDate startDate = m_selectedDate.isValid() ? m_selectedDate : QDate::currentDate();
    const QDate hitDate = event.start.date();
    if (hitDate != startDate)
    {
        setSelectedDate(hitDate);
        Q_EMIT viewWindowChanged(m_selectedDate);
    }
    ui->dayTimeline->selectEvent(event);
    ui->taskList->clearSelection();
    ui->findBar->clearStatus();
    updateActionState();
}

void CalendarPane::showFoundCalendarTask(const Task &task)
{
    const QDate dueDate = CalendarSnapshot::taskDueDate(task);
    if (dueDate.isValid() && dueDate != (m_selectedDate.isValid() ? m_selectedDate : QDate::currentDate()))
    {
        setSelectedDate(dueDate);
        Q_EMIT viewWindowChanged(m_selectedDate);
    }
    ui->dayTimeline->clearSelection();
    ui->taskList->selectTask(task);
    ui->findBar->clearStatus();
    updateActionState();
}

QList<EventOccurrence> CalendarPane::sortedDayEvents(const QDate &date) const
{
    QList<EventOccurrence> dayList;
    for (const EventOccurrence &occurrence : m_events)
    {
        if (!occurrence.ref.item.collectionId.isEmpty() && EventOccurrences::overlapsDate(occurrence, date))
        {
            dayList.append(occurrence);
        }
    }
    std::sort(dayList.begin(), dayList.end(), [](const EventOccurrence &first, const EventOccurrence &second) {
        return eventStartsBefore(first, second);
    });
    return dayList;
}

QList<Task> CalendarPane::visibleTasks() const
{
    QList<Task> result;
    for (const Task &task : m_tasks)
    {
        if (!task.ref.collectionId.isEmpty())
        {
            result.append(task);
        }
    }
    return result;
}

void CalendarPane::refreshTaskAndTimelineModels()
{
    ui->taskList->setTasks(visibleTasks());
    ui->dayTimeline->setEvents(sortedDayEvents(m_selectedDate));
    m_visibleItemCount = sortedDayEvents(m_selectedDate).size() + ui->taskList->rowCount();
    Q_EMIT itemCountChanged();
    updateActionState();
}

void CalendarPane::handleCalendarReloaded(const ReloadCoordinator::CalendarReloadPayload &payload)
{
    setCalendarData(payload.eventList, payload.taskList);
    Q_EMIT itemCountChanged();
    if (!payload.readFailures.isEmpty())
    {
        Q_EMIT readFailures(payload.readFailures);
    }
}

void CalendarPane::handleTodayAdvanced()
{
    const QDate today = QDate::currentDate();
    if (m_selectedDate == today.addDays(-1))
    {
        setSelectedDate(today);
    }
    else
    {
        renderMonthView();
    }
}

void CalendarPane::updateActionState()
{
    const bool hasSelection = hasSelectedCalendarItem();
    m_actionEditEvent->setEnabled(hasSelection);
    m_actionDeleteEvent->setEnabled(hasSelection);
    Q_EMIT actionStateChanged();
}

QString CalendarPane::selectedEventStatusText(const EventOccurrence &event) const
{
    const QLocale currentLocale = m_deps.preferences.locale();
    const EventDisplay display = CalendarSnapshot::eventDisplay(event);
    const CollectionSummary collection =
        summarizeCollection(*m_deps.services.catalogSnapshot(), CollectionKind::Calendar, event.ref.item.collectionId);
    QStringList parts;
    parts.append(tr("Calendar: %1").arg(collection.displayName));
    const QString timeText = eventTimeStatusText(display, currentLocale);
    if (!timeText.isEmpty())
    {
        parts.append(timeText);
    }
    parts.append(completionStatusText(display.completed));
    parts.append(priorityStatusText(display.priority));
    return parts.join(QStringLiteral(" | "));
}

QString CalendarPane::selectedTaskStatusText(const Task &task) const
{
    const QLocale currentLocale = m_deps.preferences.locale();
    const CollectionSummary collection =
        summarizeCollection(*m_deps.services.catalogSnapshot(), CollectionKind::Calendar, task.ref.collectionId);
    QStringList parts;
    parts.append(tr("Calendar: %1").arg(collection.displayName));
    const QString dueText = taskDueStatusText(task, currentLocale);
    if (!dueText.isEmpty())
    {
        parts.append(dueText);
    }
    const TaskDisplay display = CalendarSnapshot::taskDisplay(task);
    parts.append(completionStatusText(display.completed));
    parts.append(priorityStatusText(display.priority));
    return parts.join(QStringLiteral(" | "));
}
