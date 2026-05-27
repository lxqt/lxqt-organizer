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

#include "tasklistwidget.h"
#include "ui_tasklistwidget.h"

#include "tasklistmodel.h"

#include <QAbstractItemView>
#include <QFrame>
#include <QHeaderView>
#include <QIcon>
#include <QItemSelectionModel>
#include <QLineEdit>
#include <QMenu>
#include <QSortFilterProxyModel>
#include <QTableView>

TaskListWidget::TaskListWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TaskListWidget)
    , m_taskListModel(new TaskListModel(this))
    , m_proxyModelTasks(new QSortFilterProxyModel(this))
{
    ui->setupUi(this);

    m_proxyModelTasks->setSourceModel(m_taskListModel);
    m_proxyModelTasks->setSortRole(Qt::UserRole);

    ui->lineEditQuickTask->addAction(QIcon::fromTheme(QStringLiteral("list-add")), QLineEdit::LeadingPosition);
    ui->taskView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed |
                                  QAbstractItemView::SelectedClicked);
    ui->taskView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->taskView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->taskView->setAlternatingRowColors(false);
    ui->taskView->setTextElideMode(Qt::ElideRight);
    ui->taskView->setShowGrid(false);
    ui->taskView->setFrameShape(QFrame::NoFrame);
    ui->taskView->verticalHeader()->setVisible(false);
    ui->taskView->verticalHeader()->setDefaultSectionSize(fontMetrics().height() + 12);
    ui->taskView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->taskView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->taskView->setModel(m_proxyModelTasks);
    ui->taskView->setSortingEnabled(false);

    QHeaderView *agendaHeader = ui->taskView->horizontalHeader();
    agendaHeader->setVisible(false);
    agendaHeader->setHighlightSections(false);
    agendaHeader->setSectionsClickable(false);
    agendaHeader->setStretchLastSection(false);
    agendaHeader->setSectionResizeMode(QHeaderView::Fixed);
    agendaHeader->setSectionResizeMode(0, QHeaderView::Fixed);
    agendaHeader->setSectionResizeMode(1, QHeaderView::Stretch);
    agendaHeader->setSectionResizeMode(2, QHeaderView::Fixed);
    ui->taskView->setColumnWidth(0, 38);
    ui->taskView->setColumnWidth(2, 28);

    connect(ui->taskView->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex &) {
        Q_EMIT selectionChanged();
    });
    connect(ui->taskView, &QTableView::doubleClicked, this, [this](const QModelIndex &) {
        const Task task = selectedTask();
        if (CalendarSnapshot::hasTask(task))
        {
            Q_EMIT taskActivated(task);
        }
    });
    connect(m_taskListModel,
            &TaskListModel::dataChanged,
            this,
            [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles) {
                const bool inlineEdit = roles.contains(Qt::CheckStateRole) || roles.contains(Qt::EditRole);
                if (!inlineEdit || !topLeft.isValid() || !bottomRight.isValid())
                {
                    return;
                }
                for (int row = topLeft.row(); row <= bottomRight.row(); ++row)
                {
                    const Task task = m_taskListModel->getTask(row);
                    if (CalendarSnapshot::hasTask(task))
                    {
                        Q_EMIT inlineEditFinished(row, task);
                    }
                }
            });
    connect(ui->lineEditQuickTask, &QLineEdit::returnPressed, this, [this]() {
        const QString title = ui->lineEditQuickTask->text().trimmed();
        if (title.isEmpty())
        {
            return;
        }
        Q_EMIT quickAddRequested(title);
        ui->lineEditQuickTask->clear();
    });

    ui->taskView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->taskView, &QTableView::customContextMenuRequested, this, [this](const QPoint &pos) {
        const QModelIndex index = ui->taskView->indexAt(pos);
        if (index.isValid())
        {
            ui->taskView->setCurrentIndex(index);
            Q_EMIT selectionChanged();
        }

        QMenu menu(this);
        QAction *newAction = menu.addAction(QIcon::fromTheme(QStringLiteral("view-calendar-tasks")), tr("New Task"));
        menu.addSeparator();
        QAction *editAction = menu.addAction(tr("Edit Task"));
        const bool taskCompleted =
            index.isValid() && index.siblingAtColumn(0).data(Qt::CheckStateRole).toInt() == Qt::Checked;
        QAction *completedAction =
            menu.addAction(taskCompleted ? tr("Mark as Not Completed") : tr("Mark as Completed"));
        menu.addSeparator();
        QAction *deleteAction = menu.addAction(QIcon::fromTheme(QStringLiteral("edit-delete")), tr("Delete Task"));
        editAction->setEnabled(index.isValid());
        completedAction->setEnabled(index.isValid());
        deleteAction->setEnabled(index.isValid());

        QAction *selectedAction = menu.exec(ui->taskView->viewport()->mapToGlobal(pos));
        if (selectedAction == newAction)
        {
            Q_EMIT newTaskRequested();
        }
        else if (selectedAction == editAction && index.isValid())
        {
            Q_EMIT taskEditRequested(selectedTask());
        }
        else if (selectedAction == completedAction && index.isValid())
        {
            const QModelIndex sourceIndex = m_proxyModelTasks->mapToSource(index);
            m_taskListModel->setData(m_taskListModel->index(sourceIndex.row(), 0),
                                     taskCompleted ? Qt::Unchecked : Qt::Checked,
                                     Qt::CheckStateRole);
        }
        else if (selectedAction == deleteAction && index.isValid())
        {
            Q_EMIT taskDeleteRequested(selectedTask());
        }
    });
}

TaskListWidget::~TaskListWidget()
{
    delete ui;
}

void TaskListWidget::setCollectionSummaryProvider(CollectionSummaryProvider provider)
{
    m_taskListModel->setCollectionSummaryProvider(std::move(provider));
}

void TaskListWidget::setTasks(const QList<Task> &tasks)
{
    m_taskListModel->setTasks(tasks);
}

bool TaskListWidget::replaceTask(int row, const Task &task)
{
    return m_taskListModel->replaceTask(row, task);
}

int TaskListWidget::rowCount() const
{
    return m_proxyModelTasks->rowCount();
}

QList<Task> TaskListWidget::tasksInDisplayOrder() const
{
    QList<Task> tasks;
    for (int row = 0; row < m_proxyModelTasks->rowCount(); ++row)
    {
        const QModelIndex proxyIndex = m_proxyModelTasks->index(row, 1);
        const Task task = taskForProxyIndex(proxyIndex);
        if (CalendarSnapshot::hasTask(task))
        {
            tasks.append(task);
        }
    }
    return tasks;
}

Task TaskListWidget::selectedTask() const
{
    return taskForProxyIndex(currentProxyIndex());
}

bool TaskListWidget::hasSelectedTask() const
{
    return CalendarSnapshot::hasTask(selectedTask());
}

void TaskListWidget::clearSelection()
{
    ui->taskView->clearSelection();
}

bool TaskListWidget::selectTask(const Task &task)
{
    for (int row = 0; row < m_proxyModelTasks->rowCount(); ++row)
    {
        const QModelIndex proxyIndex = m_proxyModelTasks->index(row, 1);
        if (sameTask(taskForProxyIndex(proxyIndex), task))
        {
            QItemSelectionModel *selectionModel = ui->taskView->selectionModel();
            if (!selectionModel)
            {
                return false;
            }
            selectionModel->setCurrentIndex(proxyIndex,
                                            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            ui->taskView->scrollTo(proxyIndex, QAbstractItemView::EnsureVisible);
            return true;
        }
    }
    return false;
}

bool TaskListWidget::toggleSelectedTaskCompleted()
{
    const QModelIndex index = currentProxyIndex();
    if (!index.isValid())
    {
        return false;
    }

    const QModelIndex sourceIndex = m_proxyModelTasks->mapToSource(index);
    if (!sourceIndex.isValid())
    {
        return false;
    }

    const bool completed = m_taskListModel->index(sourceIndex.row(), 0).data(Qt::CheckStateRole).toInt() == Qt::Checked;
    return m_taskListModel->setData(
        m_taskListModel->index(sourceIndex.row(), 0), completed ? Qt::Unchecked : Qt::Checked, Qt::CheckStateRole);
}

void TaskListWidget::scrollToSelectedTask()
{
    const QModelIndex index = currentProxyIndex();
    if (index.isValid())
    {
        ui->taskView->scrollTo(index, QAbstractItemView::EnsureVisible);
    }
}

void TaskListWidget::installTaskViewEventFilter(QObject *filter)
{
    ui->taskView->installEventFilter(filter);
}

bool TaskListWidget::isTaskView(QObject *object) const
{
    return object == ui->taskView;
}

void TaskListWidget::setQuickAddIconVisible(bool visible)
{
    Q_UNUSED(visible);
}

QModelIndex TaskListWidget::currentProxyIndex() const
{
    if (!ui->taskView->selectionModel())
    {
        return {};
    }

    const QModelIndex index = ui->taskView->currentIndex();
    if (!index.isValid() || !ui->taskView->selectionModel()->isSelected(index))
    {
        return {};
    }
    return index;
}

Task TaskListWidget::taskForProxyIndex(const QModelIndex &index) const
{
    if (!index.isValid())
    {
        return {};
    }
    const QModelIndex sourceIndex = m_proxyModelTasks->mapToSource(index);
    return sourceIndex.isValid() ? m_taskListModel->getTask(sourceIndex.row()) : Task();
}

bool TaskListWidget::sameTask(const Task &first, const Task &second) const
{
    return first.ref.collectionId == second.ref.collectionId && first.ref.href == second.ref.href &&
           first.uid == second.uid;
}
