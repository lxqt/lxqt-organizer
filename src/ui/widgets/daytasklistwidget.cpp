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

#include "daytasklistwidget.h"
#include "ui_daytasklistwidget.h"

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

DayTaskListWidget::DayTaskListWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DayTaskListWidget)
    , m_taskListModel(new TaskListModel(this))
    , m_proxyModelTasks(new QSortFilterProxyModel(this))
{
    ui->setupUi(this);

    m_proxyModelTasks->setSourceModel(m_taskListModel);
    m_proxyModelTasks->setSortRole(Qt::UserRole);

    ui->lineEditQuickTask->addAction(QIcon::fromTheme(QStringLiteral("list-add")), QLineEdit::LeadingPosition);
    ui->listViewDay->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed |
                                     QAbstractItemView::SelectedClicked);
    ui->listViewDay->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->listViewDay->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->listViewDay->setAlternatingRowColors(false);
    ui->listViewDay->setTextElideMode(Qt::ElideRight);
    ui->listViewDay->setShowGrid(false);
    ui->listViewDay->setFrameShape(QFrame::NoFrame);
    ui->listViewDay->verticalHeader()->setVisible(false);
    ui->listViewDay->verticalHeader()->setDefaultSectionSize(fontMetrics().height() + 12);
    ui->listViewDay->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->listViewDay->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->listViewDay->setModel(m_proxyModelTasks);
    ui->listViewDay->setSortingEnabled(false);

    QHeaderView *agendaHeader = ui->listViewDay->horizontalHeader();
    agendaHeader->setVisible(false);
    agendaHeader->setHighlightSections(false);
    agendaHeader->setSectionsClickable(false);
    agendaHeader->setStretchLastSection(false);
    agendaHeader->setSectionResizeMode(QHeaderView::Fixed);
    agendaHeader->setSectionResizeMode(0, QHeaderView::Fixed);
    agendaHeader->setSectionResizeMode(1, QHeaderView::Stretch);
    agendaHeader->setSectionResizeMode(2, QHeaderView::Fixed);
    ui->listViewDay->setColumnWidth(0, 38);
    ui->listViewDay->setColumnWidth(2, 28);

    connect(ui->listViewDay->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex &) {
        Q_EMIT selectionChanged();
    });
    connect(ui->listViewDay, &QTableView::doubleClicked, this, [this](const QModelIndex &) {
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

    ui->listViewDay->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->listViewDay, &QTableView::customContextMenuRequested, this, [this](const QPoint &pos) {
        const QModelIndex index = ui->listViewDay->indexAt(pos);
        if (index.isValid())
        {
            ui->listViewDay->setCurrentIndex(index);
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

        QAction *selectedAction = menu.exec(ui->listViewDay->viewport()->mapToGlobal(pos));
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

DayTaskListWidget::~DayTaskListWidget()
{
    delete ui;
}

void DayTaskListWidget::setCollectionSummaryProvider(CollectionSummaryProvider provider)
{
    m_taskListModel->setCollectionSummaryProvider(std::move(provider));
}

void DayTaskListWidget::setTasks(const QList<Task> &tasks)
{
    m_taskListModel->setTasks(tasks);
}

bool DayTaskListWidget::replaceTask(int row, const Task &task)
{
    return m_taskListModel->replaceTask(row, task);
}

int DayTaskListWidget::rowCount() const
{
    return m_proxyModelTasks->rowCount();
}

QList<Task> DayTaskListWidget::tasksInDisplayOrder() const
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

Task DayTaskListWidget::selectedTask() const
{
    return taskForProxyIndex(currentProxyIndex());
}

bool DayTaskListWidget::hasSelectedTask() const
{
    return CalendarSnapshot::hasTask(selectedTask());
}

void DayTaskListWidget::clearSelection()
{
    ui->listViewDay->clearSelection();
}

bool DayTaskListWidget::selectTask(const Task &task)
{
    for (int row = 0; row < m_proxyModelTasks->rowCount(); ++row)
    {
        const QModelIndex proxyIndex = m_proxyModelTasks->index(row, 1);
        if (sameTask(taskForProxyIndex(proxyIndex), task))
        {
            QItemSelectionModel *selectionModel = ui->listViewDay->selectionModel();
            if (!selectionModel)
            {
                return false;
            }
            selectionModel->setCurrentIndex(proxyIndex,
                                            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            ui->listViewDay->scrollTo(proxyIndex, QAbstractItemView::EnsureVisible);
            return true;
        }
    }
    return false;
}

bool DayTaskListWidget::toggleSelectedTaskCompleted()
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

void DayTaskListWidget::scrollToSelectedTask()
{
    const QModelIndex index = currentProxyIndex();
    if (index.isValid())
    {
        ui->listViewDay->scrollTo(index, QAbstractItemView::EnsureVisible);
    }
}

void DayTaskListWidget::installTaskViewEventFilter(QObject *filter)
{
    ui->listViewDay->installEventFilter(filter);
}

bool DayTaskListWidget::isTaskView(QObject *object) const
{
    return object == ui->listViewDay;
}

void DayTaskListWidget::setQuickAddIconVisible(bool visible)
{
    Q_UNUSED(visible);
}

QModelIndex DayTaskListWidget::currentProxyIndex() const
{
    if (!ui->listViewDay->selectionModel())
    {
        return {};
    }

    const QModelIndex index = ui->listViewDay->currentIndex();
    if (!index.isValid() || !ui->listViewDay->selectionModel()->isSelected(index))
    {
        return {};
    }
    return index;
}

Task DayTaskListWidget::taskForProxyIndex(const QModelIndex &index) const
{
    if (!index.isValid())
    {
        return {};
    }
    const QModelIndex sourceIndex = m_proxyModelTasks->mapToSource(index);
    return sourceIndex.isValid() ? m_taskListModel->getTask(sourceIndex.row()) : Task();
}

bool DayTaskListWidget::sameTask(const Task &first, const Task &second) const
{
    return first.ref.collectionId == second.ref.collectionId && first.ref.href == second.ref.href &&
           first.uid == second.uid;
}
