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

#ifndef DAYTASKLISTWIDGET_H
#define DAYTASKLISTWIDGET_H

#include "calendaritem.h"
#include "collectionsummary.h"

#include <QWidget>

namespace Ui {
class DayTaskListWidget;
}

class TaskListModel;
class QSortFilterProxyModel;

class DayTaskListWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DayTaskListWidget(QWidget *parent = nullptr);
    ~DayTaskListWidget() override;

    void setCollectionSummaryProvider(CollectionSummaryProvider provider);
    void setTasks(const QList<Task> &tasks);
    bool replaceTask(int row, const Task &task);
    int rowCount() const;
    QList<Task> tasksInDisplayOrder() const;
    Task selectedTask() const;
    bool hasSelectedTask() const;
    void clearSelection();
    bool selectTask(const Task &task);
    bool toggleSelectedTaskCompleted();
    void scrollToSelectedTask();
    void installTaskViewEventFilter(QObject *filter);
    bool isTaskView(QObject *object) const;
    void setQuickAddIconVisible(bool visible);

Q_SIGNALS:
    void selectionChanged();
    void taskActivated(const Task &task);
    void quickAddRequested(const QString &title);
    void newTaskRequested();
    void taskEditRequested(const Task &task);
    void taskDeleteRequested(const Task &task);
    void inlineEditFinished(int row, const Task &task);

private:
    QModelIndex currentProxyIndex() const;
    Task taskForProxyIndex(const QModelIndex &index) const;
    bool sameTask(const Task &first, const Task &second) const;

    Ui::DayTaskListWidget *ui = nullptr;
    TaskListModel *m_taskListModel = nullptr;
    QSortFilterProxyModel *m_proxyModelTasks = nullptr;
};

#endif // DAYTASKLISTWIDGET_H
