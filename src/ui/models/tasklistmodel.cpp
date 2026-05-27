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

#include "tasklistmodel.h"

#include <QBrush>
#include <QColor>
#include <QCoreApplication>
#include <QDateTime>
#include <QFont>
#include <QLocale>
#include <QSize>
#include <QStringList>

#include <utility>

namespace {

TaskListRow taskRow(const Task &task, const CollectionSummary &collection)
{
    return {CalendarSnapshot::taskDisplay(task, collection), CalendarSnapshot::cloneTask(task)};
}

bool sameTaskIdentity(const Task &first, const Task &second)
{
    return first.ref.collectionId == second.ref.collectionId && first.ref.href == second.ref.href &&
           first.uid == second.uid;
}

QList<TaskListRow> taskRows(const QList<Task> &tasks, const CollectionSummaryProvider &provider)
{
    QList<TaskListRow> rows;
    rows.reserve(tasks.size());
    for (const Task &task : tasks)
    {
        const CollectionSummary collection = provider ? provider(task.ref.collectionId) : CollectionSummary{{}, {}};
        rows.append(taskRow(task, collection));
    }
    return rows;
}

QString taskTitle(const TaskDisplay &task)
{
    const QString summary = task.summary.trimmed();
    return summary.isEmpty() ? TaskListModel::tr("Untitled") : summary;
}

QString taskDueText(const TaskDisplay &task)
{
    const QDateTime due = task.hasDue ? task.due : QDateTime();
    if (!due.isValid())
    {
        return QString();
    }
    return QLocale::system().toString(due.date(), QLocale::ShortFormat);
}

QVariant colorDecoration(const QString &color)
{
    const QColor parsed(color);
    return parsed.isValid() ? QVariant(parsed) : QVariant();
}

QString collectionTooltipLine(const QString &displayName)
{
    return displayName.isEmpty() ? QString()
                                 : QCoreApplication::translate("TaskListModel", "Collection: %1").arg(displayName);
}

} // namespace

TaskListModel::TaskListModel(QObject *parent)
    : QAbstractTableModel(parent)
{}

void TaskListModel::setCollectionSummaryProvider(CollectionSummaryProvider provider)
{
    m_collectionSummaryProvider = std::move(provider);
    if (!m_tasks.isEmpty())
    {
        Q_EMIT dataChanged(index(0, 0), index(m_tasks.size() - 1, columnCount() - 1));
    }
}

void TaskListModel::setTasks(const QList<Task> &tasks)
{
    beginResetModel();
    m_tasks = taskRows(tasks, m_collectionSummaryProvider);
    endResetModel();
}

Task TaskListModel::getTask(int index) const
{
    return CalendarSnapshot::cloneTask(m_tasks.value(index).edit);
}

bool TaskListModel::replaceTask(int row, const Task &task)
{
    if (row < 0 || row >= m_tasks.size() || !sameTaskIdentity(m_tasks.at(row).edit, task))
    {
        row = -1;
        for (int i = 0; i < m_tasks.size(); ++i)
        {
            if (sameTaskIdentity(m_tasks.at(i).edit, task))
            {
                row = i;
                break;
            }
        }
    }
    if (row < 0)
    {
        return false;
    }

    m_tasks[row] = taskRow(task, collectionSummary(task.ref.collectionId));
    Q_EMIT dataChanged(index(row, 0), index(row, columnCount() - 1));
    return true;
}

CollectionSummary TaskListModel::collectionSummary(const QString &collectionId) const
{
    return m_collectionSummaryProvider ? m_collectionSummaryProvider(collectionId) : CollectionSummary{{}, {}};
}

int TaskListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_tasks.size();
}

int TaskListModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 3;
}

QVariant TaskListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_tasks.size())
    {
        return {};
    }

    const TaskDisplay &task = m_tasks.at(index.row()).display;
    const bool completed = task.completed;

    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch (index.column())
        {
        case 0:
            return QString();
        case 1:
            return taskTitle(task);
        case 2:
            return QString();
        default:
            return {};
        }
    }

    if (role == Qt::UserRole)
    {
        switch (index.column())
        {
        case 0:
            return completed;
        case 1:
            return taskTitle(task).toCaseFolded();
        case 2:
            return task.collection.displayName.toCaseFolded();
        default:
            return {};
        }
    }

    if (role == Qt::CheckStateRole && index.column() == 0)
    {
        return completed ? Qt::Checked : Qt::Unchecked;
    }

    if (role == Qt::DecorationRole && index.column() == 2)
    {
        return colorDecoration(task.collection.color);
    }

    if (role == Qt::ForegroundRole && completed)
    {
        return QBrush(QColor(Qt::gray));
    }

    if (role == Qt::FontRole && completed && index.column() != 0)
    {
        QFont font;
        font.setStrikeOut(true);
        return font;
    }

    if (role == Qt::TextAlignmentRole && index.column() == 0)
    {
        return Qt::AlignCenter;
    }

    if (role == Qt::SizeHintRole && index.column() == 0)
    {
        return QSize(34, 0);
    }

    if (role == Qt::ToolTipRole)
    {
        QStringList details;
        details.append(tr("Task: %1").arg(taskTitle(task)));
        if (!taskDueText(task).isEmpty())
        {
            details.append(tr("Due: %1").arg(taskDueText(task)));
        }
        const QString collectionLine = collectionTooltipLine(task.collection.displayName);
        if (!collectionLine.isEmpty())
        {
            details.append(collectionLine);
        }
        if (!task.description.trimmed().isEmpty())
        {
            details.append(task.description.trimmed());
        }
        return details.join(QLatin1Char('\n'));
    }

    return {};
}

bool TaskListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_tasks.size())
    {
        return false;
    }
    Task task = CalendarSnapshot::cloneTask(m_tasks[index.row()].edit);
    if (!CalendarSnapshot::hasTask(task))
    {
        return false;
    }

    if (index.column() == 0 && role == Qt::CheckStateRole)
    {
        const bool completed = value.toInt() == Qt::Checked;
        if (CalendarSnapshot::taskDisplay(task).completed == completed)
        {
            return false;
        }
        const Task updatedTask = CalendarSnapshot::taskWithCompleted(task, value.toInt() == Qt::Checked);
        m_tasks[index.row()] = taskRow(updatedTask, collectionSummary(updatedTask.ref.collectionId));
        Q_EMIT dataChanged(index.siblingAtColumn(0),
                           index.siblingAtColumn(columnCount() - 1),
                           {Qt::CheckStateRole,
                            Qt::DisplayRole,
                            Qt::EditRole,
                            Qt::UserRole,
                            Qt::ForegroundRole,
                            Qt::FontRole,
                            Qt::ToolTipRole});
        return true;
    }

    if (index.column() == 1 && role == Qt::EditRole)
    {
        const QString summary = value.toString().trimmed();
        if (summary.isEmpty())
        {
            return false;
        }
        if (CalendarSnapshot::taskDisplay(task).summary.trimmed() == summary)
        {
            return false;
        }
        const Task updatedTask = CalendarSnapshot::taskWithSummary(task, summary);
        m_tasks[index.row()] = taskRow(updatedTask, collectionSummary(updatedTask.ref.collectionId));
        Q_EMIT dataChanged(index.siblingAtColumn(0),
                           index.siblingAtColumn(columnCount() - 1),
                           {Qt::DisplayRole, Qt::EditRole, Qt::UserRole, Qt::ToolTipRole});
        return true;
    }

    return false;
}

Qt::ItemFlags TaskListModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags itemFlags = QAbstractTableModel::flags(index);
    if (index.isValid() && index.column() == 0)
    {
        itemFlags |= Qt::ItemIsUserCheckable;
    }
    if (index.isValid() && index.column() == 1)
    {
        itemFlags |= Qt::ItemIsEditable;
    }
    return itemFlags;
}

QVariant TaskListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
    {
        return {};
    }

    switch (section)
    {
    case 0:
        return tr("Completed");
    case 1:
        return tr("Task");
    case 2:
        return tr("Collection");
    default:
        return {};
    }
}
