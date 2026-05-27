/*
 * LXQt Organizer - personal information manager for LXQt.
 * Author: Alan Crispin aka. crispina <crispinalan@gmail.com>
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

#ifndef TASKLISTMODEL_H
#define TASKLISTMODEL_H

#include "calendaritem.h"
#include "collectionsummary.h"

#include <QAbstractTableModel>
#include <QList>

struct TaskListRow
{
    TaskDisplay display;
    Task edit;
};

class TaskListModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit TaskListModel(QObject *parent = nullptr);

    void setCollectionSummaryProvider(CollectionSummaryProvider provider);
    void setTasks(const QList<Task> &tasks);
    Task getTask(int index) const;
    bool replaceTask(int row, const Task &task);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

private:
    CollectionSummary collectionSummary(const QString &collectionId) const;

    QList<TaskListRow> m_tasks;
    CollectionSummaryProvider m_collectionSummaryProvider;
};

#endif // TASKLISTMODEL_H
