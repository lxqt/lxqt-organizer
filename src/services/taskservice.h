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

#ifndef TASKSERVICE_H
#define TASKSERVICE_H

#include "calendaritem.h"
#include "cancellationtoken.h"
#include "collectionservice.h"
#include "itemidentity.h"
#include "operationcapability.h"
#include "storageresult.h"

#include <QList>
#include <QFuture>
#include <QSet>
#include <QString>

#include <functional>
#include <memory>

class VdirIo;

struct TaskSnapshotListResult
{
    QList<Task> tasks;
    QList<ReadFailure> readFailures;
};

// @thread any-thread; caches live behind store mutexes and writes use VdirIo.
class TaskService
{
public:
    using TaskSaveResult = StorageResult<Task>;
    using TaskMoveResult = StorageMoveResult<Task>;

    explicit TaskService(const CollectionService &collections, const VdirIo &vdirIo);
    ~TaskService();

    QFuture<TaskSnapshotListResult> taskSnapshotsAsync(const CancellationToken &cancellation = {}) const;
    TaskSaveResult addTask(const Task &task, const QString &collectionId = QString()) const;
    TaskSaveResult updateTask(const Task &task) const;
    TaskMoveResult moveTask(const Task &task, const QString &destinationCollectionId) const;
    StorageStatus deleteTask(const Task &task) const;
    StorageStatus deleteTask(const ItemRef &storage, const QString &uid) const;
    OperationCapability canCreateTask(const QString &collectionId) const;
    OperationCapability canEditTask(const Task &task) const;
    OperationCapability canMoveTask(const Task &task, const QString &destinationCollectionId) const;
    OperationCapability canDeleteTask(const Task &task) const;
    // Async methods submit storage work directly to VdirIo.
    QFuture<TaskSaveResult> addTaskAsync(const Task &task, const QString &collectionId = QString()) const;
    QFuture<TaskSaveResult> updateTaskAsync(const Task &task) const;
    QFuture<TaskMoveResult> moveTaskAsync(const Task &task, const QString &destinationCollectionId) const;
    QFuture<StorageStatus> deleteTaskAsync(const Task &task) const;
    QFuture<StorageStatus> deleteTaskAsync(const ItemRef &storage, const QString &uid) const;
    QList<QFuture<TaskSaveResult>> rollForwardOverdueTasksAsync(const QList<Task> &tasks, const QDate &today) const;
    void clearCache() const;
    void clearCacheForCollections(const QSet<QString> &collectionIds) const;
    void retainRepositoriesForCollections(const QList<Collection> &collections) const;
    static bool rollForwardTaskTo(Task *task, const QDate &today);

private:
    class Impl;
    void notifyItemWritten(const ItemRef &storage) const;
    std::unique_ptr<Impl> m_impl;
};

#endif // TASKSERVICE_H
