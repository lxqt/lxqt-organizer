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

#ifndef CALENDARPANE_H
#define CALENDARPANE_H

#include "calendaritem.h"
#include "collectionsummary.h"
#include "iactivepane.h"
#include "paneshelldeps.h"
#include "reloadcoordinator.h"
#include "storageresult.h"

#include <QDate>
#include <QList>
#include <QLocale>
#include <QTime>
#include <QWidget>

namespace Ui {
class CalendarPane;
}

class DayTimelineWidget;
class EventFindBar;
class QAction;
class QEvent;
class QSplitter;
class PreferencesController;

class CalendarPane : public QWidget, public IActivePane
{
    Q_OBJECT

public:
    struct Deps : PaneShellDeps
    {};

    explicit CalendarPane(const Deps &deps, QWidget *parent = nullptr);
    ~CalendarPane() override;

    QString displayName() const override;
    CollectionKind collectionKind() const override;
    QList<QAction *> contextActions() const override;
    QString selectionStatusText() const override;
    int visibleItemCount() const override;

    QSplitter *calendarSplitter() const;
    void setCalendarFontSize(int fontSize);
    void setTimelineCollectionSummaryProvider(CollectionSummaryProvider provider);
    void setTimelineLocale(const QLocale &locale);
    void setDayViewTimeRange(int startHour, int finishHour);
    void replaceInlineEditedTask(int row, const Task &task);
    void clearCalendarFindStatus();

    void setCalendarData(const QList<EventOccurrence> &events, const QList<Task> &tasks);
    void removeEventByKey(const ItemKey &key);
    void removeEvent(const OccurrenceRef &event);
    void setSelectedDate(const QDate &date);
    QDate selectedDate() const;
    void renderMonthView();
    void newEvent(const QTime &startTime = QTime(), const QTime &endTime = QTime());
    void updateEvent(const EventOccurrence &event);
    void rescheduleEvent(const EventOccurrence &event, const QTime &startTime, const QTime &endTime);
    void quickAddTask(const QString &title);
    void newTask();
    void editTask(const Task &task);
    bool deleteSelectedTaskWithPrompt();
    bool deleteEventWithPrompt(const EventOccurrence &event);
    bool setEventCompleted(const EventOccurrence &event, bool completed);
    bool hasSelectedCalendarItem() const;
    void clearSelectedCalendarItem();
    bool editSelectedCalendarItem();
    bool toggleSelectedCalendarItemCompleted();
    bool deleteSelectedCalendarItem();
    bool selectAdjacentCalendarItem(int delta);
    bool moveSelectedDateBy(int days);
    void gotoNextMonth();
    void gotoPreviousMonth();
    void gotoToday();
    void setTaskPaneVisible(bool visible);
    bool taskPaneVisible() const;
    void showFindBar();
    void hideFindBar();
    bool findCalendarItem(bool forward);
    void showCalendarFindNoMatch();
    void showFoundCalendarEvent(const EventOccurrence &event);
    void showFoundCalendarTask(const Task &task);
    QList<EventOccurrence> sortedDayEvents(const QDate &date) const;
    QList<Task> visibleTasks() const;
    Task selectedTask() const;
    void refreshTaskAndTimelineModels();

Q_SIGNALS:
    void actionStateChanged();
    void itemCountChanged();
    void selectedDateChanged(const QDate &date);
    void viewWindowChanged(const QDate &selectedDate);
    void readFailures(const QList<ReadFailure> &failures);
    void newEventRequested(const QTime &startTime, const QTime &endTime);
    void eventEditRequested(const EventOccurrence &event);
    void eventRescheduleRequested(const EventOccurrence &event, const QTime &startTime, const QTime &endTime);
    void quickTaskAddRequested(const QString &title);
    void newTaskRequested();
    void taskEditRequested(const Task &task);
    void taskDeleteRequested(const Task &task);
    void eventDeleteRequested(const EventOccurrence &event);
    void eventCompletionRequested(const EventOccurrence &event, bool completed);
    void taskInlineEditCommitted(int row, const Task &task);
    void calendarFindRequested(bool forward,
                               const QString &needle,
                               const QDate &startDate,
                               const EventOccurrence &currentEvent,
                               const Task &currentTask);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void createActions();
    void handleCalendarReloaded(const ReloadCoordinator::CalendarReloadPayload &payload);
    void handleTodayAdvanced();
    void updateActionState();
    QString selectedEventStatusText(const EventOccurrence &event) const;
    QString selectedTaskStatusText(const Task &task) const;

    Ui::CalendarPane *ui = nullptr;
    Deps m_deps;
    QAction *m_actionNewEvent = nullptr;
    QAction *m_actionNewTask = nullptr;
    QAction *m_actionDeleteEvent = nullptr;
    QAction *m_actionEditEvent = nullptr;
    QAction *m_actionFind = nullptr;
    QAction *m_actionFindNext = nullptr;
    QAction *m_actionFindPrev = nullptr;
    QList<EventOccurrence> m_events;
    QList<Task> m_tasks;
    QDate m_selectedDate;
    int m_visibleItemCount = 0;
    bool m_taskPaneVisible = true;
};

#endif // CALENDARPANE_H
