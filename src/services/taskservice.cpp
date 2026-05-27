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

#include "taskservice.h"

#include "asyncsubmit.h"
#include "calendarmover.h"
#include "calendaritemstore.h"
#include "calendarsnapshot.h"
#include "calendaritemmutator.h"
#include "calendaritemrepository.h"
#include "calendarvalidation.h"
#include "completedfuture.h"
#include "incidencefields.h"
#include "incidenceresolver.h"
#include "itemidentity.h"
#include "moveoutcomelog.h"
#include "storagelog.h"
#include "todopatcher.h"
#include "vdirio.h"

#include <QDateTime>
#include <QDebug>

#include <KCalendarCore/Todo>

#include <utility>

namespace {

using WritableCalendarItem = CalendarItemStore::WritableCalendarItem;

ReadFailure readFailureForCalendarItem(const QString &collectionId,
                                       const CalendarItemRepository::ReadResult &readResult)
{
    ItemRef storage = readResult.object.ref;
    if (storage.collectionId.isEmpty())
    {
        storage.collectionId = collectionId;
    }
    return ReadFailure{storage, readResult.status};
}

TaskService::TaskSaveResult taskSaveFailure(StorageStatus status, const Task &task)
{
    TaskService::TaskSaveResult result;
    result.status = status;
    result.snapshot = task;
    return result;
}

TaskService::TaskMoveResult
taskMoveFailure(StorageStatus status, const Task &task, const QString &destinationCollectionId)
{
    TaskService::TaskMoveResult result;
    result.snapshot = task;
    ItemRef destination;
    destination.collectionId = destinationCollectionId;
    result.move = MoveOutcome::preconditionFailed(status, task.ref, destination);
    return result;
}

TaskSnapshotListResult collectTaskSnapshotsForCollection(const CalendarItemRepository &repository,
                                                         const QString &collectionId,
                                                         const CancellationToken &cancellation)
{
    TaskSnapshotListResult result;
    bool keepGoing = true;
    repository.forEachObject(
        [&](const CalendarItemRepository::ReadResult &readResult) {
            if (!readResult.isOk())
            {
                qCWarning(storageLog) << "Skipping calendar item" << collectionId << readResult.object.ref.href
                                      << storageStatusName(readResult.status);
                result.readFailures.append(readFailureForCalendarItem(collectionId, readResult));
                return true;
            }
            const CalendarItem &object = readResult.object;
            const KCalendarCore::MemoryCalendar::Ptr calendar = CalendarSnapshot::calendarForItem(object);
            if (calendar.isNull() || !CalendarValidation::isSupportedCalendar(calendar))
            {
                qCWarning(storageLog) << "Skipping unsupported calendar item" << collectionId << object.ref.href;
                return true;
            }
            if (calendar->rawTodos().isEmpty())
            {
                return true;
            }
            for (const KCalendarCore::Todo::Ptr &todo : calendar->rawTodos())
            {
                if (cancellation.isCancellationRequested())
                {
                    keepGoing = false;
                    return false;
                }
                const QString uid = todo->uid().isEmpty() ? object.uid : todo->uid();
                result.tasks.append(
                    Task{object.ref, uid, CalendarSnapshot::todoSnapshot(CalendarSnapshot::cloneTodo(todo))});
            }
            return true;
        },
        [&cancellation, &keepGoing]() { return !keepGoing || cancellation.isCancellationRequested(); });
    return result;
}

void sortTasks(QList<Task> *tasks)
{
    std::sort(tasks->begin(), tasks->end(), [](const Task &first, const Task &second) {
        const bool firstDone = first.todo->isCompleted();
        const bool secondDone = second.todo->isCompleted();
        if (firstDone != secondDone)
        {
            return !firstDone;
        }

        const QDateTime firstDue = first.todo->hasDueDate() ? first.todo->dtDue() : QDateTime();
        const QDateTime secondDue = second.todo->hasDueDate() ? second.todo->dtDue() : QDateTime();
        if (firstDue.isValid() != secondDue.isValid())
        {
            return firstDue.isValid();
        }
        if (firstDue.isValid() && firstDue != secondDue)
        {
            return firstDue < secondDue;
        }

        return first.todo->summary().toCaseFolded() < second.todo->summary().toCaseFolded();
    });
}

} // namespace

TaskService::TaskService(const CollectionService &collections, const VdirIo &vdirIo)
    : m_collections(collections)
    , m_items(collections, vdirIo)
    , m_collectionReloadSubscription(collections, [this](const QSet<QString> &changedCollectionIds) {
        clearCacheForCollections(changedCollectionIds);
    })
{}

TaskService::~TaskService() = default;

void TaskService::clearCache() const
{
    m_items.clearCache();
}

void TaskService::clearCacheForCollections(const QSet<QString> &collectionIds) const
{
    m_items.clearCacheForCollections(collectionIds);
}

void TaskService::retainRepositoriesForCollections(const QList<Collection> &collections) const
{
    m_items.retainRepositoriesForCollections(collections);
}

void TaskService::notifyItemWritten(const ItemRef &ref) const
{
    m_collections.notifyItemWritten(ref);
}

bool TaskService::rollForwardTaskTo(Task *task, const QDate &today)
{
    if (!task || task->todo.isNull() || task->todo->isCompleted())
    {
        return false;
    }
    const KCalendarCore::Todo::Ptr currentTodo = CalendarSnapshot::cloneTodo(task->todo);
    if (!CalendarUtils::isRollForwardEnabled(currentTodo))
    {
        return false;
    }
    const QDateTime due = task->todo->dtDue();
    if (!due.isValid() || due.date() >= today)
    {
        return false;
    }

    KCalendarCore::Todo::Ptr updatedTodo = CalendarSnapshot::cloneTodo(task->todo);
    if (!CalendarUtils::originalDueDate(updatedTodo).isValid())
    {
        CalendarUtils::setOriginalDueDate(updatedTodo, due.date());
    }
    QDateTime newDue = due;
    newDue.setDate(today);
    updatedTodo->setDtDue(newDue);
    task->todo = CalendarSnapshot::todoSnapshot(updatedTodo);
    return true;
}

QFuture<TaskSnapshotListResult> TaskService::taskSnapshotsAsync(const CancellationToken &cancellation) const
{
    QList<QFuture<TaskSnapshotListResult>> perCollectionFutures;
    const VdirIo &scheduler = m_items.vdirIo();
    for (const Collection &collection : m_collections.calendarList())
    {
        if (cancellation.isCancellationRequested())
        {
            break;
        }
        const std::shared_ptr<CalendarItemRepository> repository = m_items.repositoryForCollection(collection);
        if (!repository->isValid())
        {
            continue;
        }
        const VdirPath path = VdirPath::fromCollection(collection);
        if (!path.isValid())
        {
            continue;
        }
        const QString collectionId = collection.id;
        perCollectionFutures.append(scheduler.submit(
            path, QStringLiteral("task snapshots"), TaskSnapshotListResult{}, [repository, collectionId, cancellation] {
                return collectTaskSnapshotsForCollection(*repository, collectionId, cancellation);
            }));
    }
    if (perCollectionFutures.isEmpty())
    {
        return CalendarIoUtils::completedFuture(TaskSnapshotListResult{});
    }
    return QtFuture::whenAll(perCollectionFutures.begin(), perCollectionFutures.end())
        .then(QtFuture::Launch::Sync, [](const QList<QFuture<TaskSnapshotListResult>> &done) {
            TaskSnapshotListResult aggregate;
            for (const QFuture<TaskSnapshotListResult> &future : done)
            {
                TaskSnapshotListResult perCollection = future.result();
                aggregate.tasks.append(std::move(perCollection.tasks));
                aggregate.readFailures.append(std::move(perCollection.readFailures));
            }
            sortTasks(&aggregate.tasks);
            return aggregate;
        });
}

TaskService::TaskSaveResult TaskService::addTask(const Task &task, const QString &collectionId) const
{
    TaskSaveResult result;
    result.snapshot = task;
    KCalendarCore::Todo::Ptr storedTodo =
        task.todo.isNull() ? KCalendarCore::Todo::Ptr(new KCalendarCore::Todo) : CalendarSnapshot::cloneTodo(task.todo);

    if (!storedTodo->created().isValid())
    {
        storedTodo->setCreated(QDateTime::currentDateTimeUtc());
    }

    const QString originalUid = storedTodo->uid();
    const bool organizerAuthoredUid = originalUid.isEmpty();
    const int attempts = organizerAuthoredUid ? 2 : 1;
    StorageStatus lastStatus = StorageStatus::IoError;
    for (int attempt = 0; attempt < attempts; ++attempt)
    {
        const QString uid = organizerAuthoredUid ? randomItemUid() : originalUid;
        if (uid.isEmpty())
        {
            result.status = StorageStatus::Unsupported;
            return result;
        }
        if (!CalendarItemMutator::ensureTodoUid(storedTodo, uid))
        {
            result.status = StorageStatus::Unsupported;
            return result;
        }

        CalendarItem object;
        object.ref.collectionId = collectionId;
        object.uid = uid;
        object.payload = CalendarSnapshot::todoSnapshot(CalendarSnapshot::cloneTodo(storedTodo));
        const StorageResult<CalendarItem> createResult = m_items.addObject(object);
        if (createResult.isOk())
        {
            object = createResult.snapshot;
            notifyItemWritten(object.ref);
            Task savedTask = task;
            savedTask.ref = object.ref;
            savedTask.uid = object.uid;
            const KCalendarCore::MemoryCalendar::Ptr savedCalendar = CalendarSnapshot::calendarForItem(object);
            if (!savedCalendar.isNull() && !savedCalendar->rawTodos().isEmpty())
            {
                savedTask.todo = CalendarSnapshot::todoSnapshot(savedCalendar->rawTodos().constFirst());
            }
            result.status = StorageStatus::Ok;
            result.snapshot = savedTask;
            return result;
        }
        lastStatus = createResult.status;
    }
    result.status = lastStatus;
    return result;
}

TaskService::TaskSaveResult TaskService::updateTask(const Task &task) const
{
    TaskSaveResult result;
    result.snapshot = task;
    if (task.todo.isNull())
    {
        result.status = StorageStatus::Unsupported;
        return result;
    }

    WritableCalendarItem item =
        m_items.writableItemForUpdate(task.ref.collectionId, task.ref.href, task.ref.etag, task.uid);
    if (!item.isValid())
    {
        result.status = item.status;
        return result;
    }
    KCalendarCore::MemoryCalendar::Ptr calendar = CalendarSnapshot::mutableCalendarForItem(item.object);
    if (calendar.isNull() || calendar->rawTodos().isEmpty())
    {
        result.status = StorageStatus::Unsupported;
        return result;
    }

    const KCalendarCore::Todo::Ptr storedTodo = IncidenceResolver::findTodo(calendar, {item.object.uid, std::nullopt});
    if (storedTodo.isNull())
    {
        result.status = StorageStatus::NotFound;
        return result;
    }
    const KCalendarCore::Todo::Ptr editedTodo = CalendarSnapshot::cloneTodo(task.todo);
    if (!CalendarItemMutator::ensureTodoUid(editedTodo, item.object.uid))
    {
        result.status = StorageStatus::Conflict;
        return result;
    }

    const TodoPatcher::Options changedOptions = TodoPatcher::changedOptionsForTodoEdit(storedTodo, editedTodo);
    if (!TodoPatcher::hasChanges(changedOptions))
    {
        result.status = StorageStatus::Ok;
        result.snapshot.ref = item.object.ref;
        result.snapshot.uid = item.object.uid;
        result.snapshot.todo = CalendarSnapshot::todoSnapshot(storedTodo);
        return result;
    }

    KCalendarCore::Todo::Ptr updatedTodo(storedTodo->clone());
    TodoPatcher::applyTodoEdit(updatedTodo, editedTodo, changedOptions);

    if (!CalendarItemMutator::replaceTodoByUid(calendar, item.object.uid, updatedTodo))
    {
        result.status = StorageStatus::IoError;
        return result;
    }
    item.object = CalendarSnapshot::calendarItem(
        item.object.ref, item.object.uid, calendar, CalendarSnapshot::PayloadShape::Todo);
    const StorageResult<CalendarItem> updateResult = m_items.replaceWritableItem(item);
    if (!updateResult.isOk())
    {
        result.status = updateResult.status;
        return result;
    }
    item.object = updateResult.snapshot;

    Task savedTask = task;
    savedTask.ref = item.object.ref;
    savedTask.uid = item.object.uid;
    const KCalendarCore::MemoryCalendar::Ptr savedCalendar = CalendarSnapshot::calendarForItem(item.object);
    if (!savedCalendar.isNull())
    {
        const KCalendarCore::Todo::Ptr savedTodo =
            IncidenceResolver::findTodo(savedCalendar, {item.object.uid, std::nullopt});
        if (!savedTodo.isNull())
        {
            savedTask.todo = CalendarSnapshot::todoSnapshot(savedTodo);
        }
    }
    result.status = StorageStatus::Ok;
    result.snapshot = savedTask;
    notifyItemWritten(task.ref);
    notifyItemWritten(result.snapshot.ref);
    return result;
}

TaskService::TaskMoveResult TaskService::moveTask(const Task &task, const QString &destinationCollectionId) const
{
    const QString targetCollectionId =
        destinationCollectionId.isEmpty() ? task.ref.collectionId : destinationCollectionId;
    if (task.ref.collectionId == targetCollectionId)
    {
        const ItemRef sourceStorage = task.ref;
        const TaskSaveResult updateResult = updateTask(task);
        TaskMoveResult result;
        result.snapshot = updateResult.snapshot;
        result.move = MoveOutcome::update(UpdateOutcome{
            updateResult.status, sourceStorage, updateResult.isOk() ? updateResult.snapshot.ref : task.ref});
        return result;
    }
    const auto preconditionFailed = [&task, &targetCollectionId](StorageStatus status) {
        TaskMoveResult result;
        result.snapshot = task;
        ItemRef destination;
        destination.collectionId = targetCollectionId;
        result.move = MoveOutcome::preconditionFailed(status, task.ref, destination);
        return result;
    };
    if (task.todo.isNull())
    {
        return preconditionFailed(StorageStatus::Unsupported);
    }

    const WritableCalendarItem source =
        m_items.writableItemForUpdate(task.ref.collectionId, task.ref.href, task.ref.etag, task.uid);
    if (!source.isValid())
    {
        return preconditionFailed(source.status);
    }

    const std::optional<Collection> destinationCollection = m_items.calendarForWrite(targetCollectionId);
    if (!destinationCollection)
    {
        return preconditionFailed(m_items.calendarWriteFailureStatus(targetCollectionId));
    }
    if (!m_items.isValidForWrite(*destinationCollection))
    {
        return preconditionFailed(StorageStatus::IoError);
    }

    KCalendarCore::MemoryCalendar::Ptr sourceCalendar = CalendarSnapshot::mutableCalendarForItem(source.object);
    if (sourceCalendar.isNull() || !CalendarValidation::isSupportedCalendar(sourceCalendar) ||
        sourceCalendar->rawTodos().isEmpty())
    {
        return preconditionFailed(StorageStatus::Unsupported);
    }
    const QString uid = IncidenceResolver::inferLocator(sourceCalendar, task.uid).uid;
    if (uid.isEmpty())
    {
        return preconditionFailed(StorageStatus::NotFound);
    }
    KCalendarCore::Todo::Ptr editedTodo = CalendarSnapshot::cloneTodo(task.todo);
    if (!CalendarItemMutator::ensureTodoUid(editedTodo, uid))
    {
        return preconditionFailed(StorageStatus::Conflict);
    }

    if (!CalendarItemMutator::replaceSingleTodo(sourceCalendar, editedTodo))
    {
        return preconditionFailed(StorageStatus::IoError);
    }

    CalendarItem inserted;
    inserted.ref.collectionId = targetCollectionId;
    inserted.uid = editedTodo->uid();
    inserted = CalendarSnapshot::calendarItem(
        inserted.ref, inserted.uid, sourceCalendar, CalendarSnapshot::PayloadShape::Todo);
    TaskMoveResult result = CalendarMover::moveObject<TaskMoveResult>(
        m_items,
        source,
        source.object.ref,
        destinationCollection,
        inserted,
        task,
        [&task](const CalendarItem &inserted) {
            Task movedTask = task;
            movedTask.ref = inserted.ref;
            movedTask.uid = inserted.uid;
            const KCalendarCore::MemoryCalendar::Ptr insertedCalendar = CalendarSnapshot::calendarForItem(inserted);
            if (!insertedCalendar.isNull() && !insertedCalendar->rawTodos().isEmpty())
            {
                movedTask.todo = CalendarSnapshot::todoSnapshot(insertedCalendar->rawTodos().constFirst());
            }
            return movedTask;
        });
    MoveOutcomeLog::logCrossVdirFailures(result.move,
                                         QStringLiteral("Could not remove moved task item"),
                                         QStringLiteral("Could not roll back moved task item"));
    if (result.isOk())
    {
        notifyItemWritten(task.ref);
        notifyItemWritten(result.snapshot.ref);
    }
    return result;
}

StorageStatus TaskService::deleteTask(const Task &task) const
{
    return deleteTask(task.ref, task.uid);
}

StorageStatus TaskService::deleteTask(const ItemRef &storage, const QString &uid) const
{
    WritableCalendarItem item = m_items.writableItemForUpdate(storage.collectionId, storage.href, storage.etag, uid);
    if (!item.isValid())
    {
        return item.status;
    }
    KCalendarCore::MemoryCalendar::Ptr calendar = CalendarSnapshot::mutableCalendarForItem(item.object);
    if (calendar.isNull() || calendar->rawTodos().isEmpty())
    {
        qCWarning(storageLog) << "Calendar payload missing or empty after read" << storage.href;
        return StorageStatus::Unsupported;
    }

    if (!CalendarItemMutator::removeTodoByUid(calendar, item.object.uid))
    {
        qCWarning(storageLog) << "Failed to remove todo by uid" << storage.href << item.object.uid;
        return StorageStatus::NotFound;
    }
    if (!CalendarItemMutator::hasIncidences(calendar))
    {
        const StorageStatus removeStatus = m_items.removeWritableItem(item);
        if (removeStatus != StorageStatus::Ok)
        {
            qCWarning(storageLog) << "Could not remove task item" << item.object.ref.href
                                  << storageStatusName(removeStatus);
            return removeStatus;
        }
        notifyItemWritten(item.object.ref);
        return StorageStatus::Ok;
    }

    item.object.uid = IncidenceResolver::inferLocator(calendar).uid;
    item.object = CalendarSnapshot::calendarItem(
        item.object.ref, item.object.uid, calendar, CalendarSnapshot::PayloadShape::Todo);
    const StorageResult<CalendarItem> result = m_items.replaceWritableItem(item);
    if (result.isOk())
    {
        notifyItemWritten(storage);
        notifyItemWritten(result.snapshot.ref);
    }
    return result.status;
}

OperationCapability TaskService::canCreateTask(const QString &collectionId) const
{
    return capabilityForWritableCollection(
        m_collections, CollectionKind::Calendar, collectionId, OperationCapabilityStatus::DestinationReadOnly);
}

OperationCapability TaskService::canEditTask(const Task &task) const
{
    if (!task.ref.isValid() || task.uid.isEmpty() || !CalendarSnapshot::hasTask(task))
    {
        return invalidSelectionCapability(task.ref);
    }
    return capabilityForWritableCollection(m_collections,
                                           CollectionKind::Calendar,
                                           task.ref.collectionId,
                                           OperationCapabilityStatus::SourceReadOnly,
                                           task.ref);
}

OperationCapability TaskService::canMoveTask(const Task &task, const QString &destinationCollectionId) const
{
    OperationCapability sourceCapability = canEditTask(task);
    if (!sourceCapability.isAllowed())
    {
        return sourceCapability;
    }

    const QString targetCollectionId =
        destinationCollectionId.isEmpty() ? task.ref.collectionId : destinationCollectionId;
    if (targetCollectionId == task.ref.collectionId)
    {
        sourceCapability.destinationCollectionId = targetCollectionId;
        return sourceCapability;
    }

    return capabilityForWritableCollection(m_collections,
                                           CollectionKind::Calendar,
                                           targetCollectionId,
                                           OperationCapabilityStatus::DestinationReadOnly,
                                           task.ref,
                                           targetCollectionId);
}

OperationCapability TaskService::canDeleteTask(const Task &task) const
{
    return canEditTask(task);
}

QFuture<TaskService::TaskSaveResult> TaskService::addTaskAsync(const Task &task, const QString &collectionId) const
{
    return AsyncSubmit::submit<TaskSaveResult>(
        m_items.vdirIo(),
        QStringLiteral("task add"),
        [this, collectionId] {
            return AsyncSubmit::ResolvedCollection{m_collections.calendarForWrite(collectionId),
                                                   m_items.calendarWriteFailureStatus(collectionId)};
        },
        [this, task, collectionId] { return addTask(task, collectionId); },
        [task](StorageStatus status) { return taskSaveFailure(status, task); });
}

QFuture<TaskService::TaskSaveResult> TaskService::updateTaskAsync(const Task &task) const
{
    return AsyncSubmit::submit<TaskSaveResult>(
        m_items.vdirIo(),
        QStringLiteral("task update"),
        [this, task] { return m_collections.calendarForRead(task.ref.collectionId); },
        [this, task] { return updateTask(task); },
        [task](StorageStatus status) { return taskSaveFailure(status, task); });
}

QFuture<TaskService::TaskMoveResult> TaskService::moveTaskAsync(const Task &task,
                                                                const QString &destinationCollectionId) const
{
    const QString targetCollectionId =
        destinationCollectionId.isEmpty() ? task.ref.collectionId : destinationCollectionId;
    return AsyncSubmit::submitMove<TaskMoveResult>(
        m_items.vdirIo(),
        QStringLiteral("task move"),
        targetCollectionId,
        [this, task] { return m_collections.calendarForRead(task.ref.collectionId); },
        [this, targetCollectionId] {
            return AsyncSubmit::ResolvedCollection{m_collections.calendarForWrite(targetCollectionId),
                                                   m_items.calendarWriteFailureStatus(targetCollectionId)};
        },
        [this, task, targetCollectionId] { return moveTask(task, targetCollectionId); },
        [task, targetCollectionId](StorageStatus status) { return taskMoveFailure(status, task, targetCollectionId); });
}

QFuture<StorageStatus> TaskService::deleteTaskAsync(const Task &task) const
{
    return deleteTaskAsync(task.ref, task.uid);
}

QFuture<StorageStatus> TaskService::deleteTaskAsync(const ItemRef &storage, const QString &uid) const
{
    return AsyncSubmit::submit<StorageStatus>(
        m_items.vdirIo(),
        QStringLiteral("task delete"),
        [this, storage] { return m_collections.calendarForRead(storage.collectionId); },
        [this, storage, uid] { return deleteTask(storage, uid); },
        [](StorageStatus status) { return status; });
}

QList<QFuture<TaskService::TaskSaveResult>> TaskService::rollForwardOverdueTasksAsync(const QList<Task> &tasks,
                                                                                      const QDate &today) const
{
    QList<QFuture<TaskSaveResult>> futures;
    for (const Task &snapshot : tasks)
    {
        Task updatedTask = snapshot;
        if (!rollForwardTaskTo(&updatedTask, today) || !m_collections.isCalendarWritable(snapshot.ref.collectionId))
        {
            continue;
        }
        futures.append(updateTaskAsync(updatedTask));
    }
    return futures;
}
