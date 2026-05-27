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

#include "vdirioscheduler.h"

#include "storagelog.h"

#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QMetaObject>
#include <QObject>
#include <QThreadPool>
#include <QtGlobal>

#include <algorithm>
#include <utility>

namespace {
constexpr unsigned long ShutdownWaitMsecs = 30000;
}

VdirIoScheduler::VdirIoScheduler() = default;

VdirIoScheduler::~VdirIoScheduler()
{
    shutdown();
    Q_ASSERT(m_shuttingDown || m_workers.isEmpty());
}

VdirIoScheduler::Worker::~Worker()
{
    requestQuit();
    wait(ShutdownWaitMsecs);
}

void VdirIoScheduler::Worker::requestQuit()
{
    if (!thread || quitPosted)
    {
        return;
    }
    quitPosted = true;

    QThread *workerThread = thread;
    QObject *target = receiver.data();
    if (!target)
    {
        workerThread->quit();
        return;
    }

    QMetaObject::invokeMethod(
        target,
        [target, workerThread]() {
            target->deleteLater();
            workerThread->quit();
        },
        Qt::QueuedConnection);
}

void VdirIoScheduler::Worker::wait(unsigned long timeoutMsecs)
{
    if (!thread)
    {
        receiver = nullptr;
        return;
    }

    if (!thread->wait(timeoutMsecs))
    {
        qCWarning(storageLog) << "Timed out waiting for vdir IO worker shutdown; terminating" << path;
        thread->terminate();
        thread->wait();
    }
    receiver = nullptr;
    delete thread;
    thread = nullptr;
}

bool VdirIoScheduler::submitJob(const VdirPath &path, const QString &label, std::function<void()> job) const
{
    std::shared_ptr<Worker> worker = workerForPath(path.path());
    if (!worker || worker->receiver.isNull())
    {
        return false;
    }

    auto queuedJob = [worker, job = std::move(job), label]() mutable {
        qCDebug(storageLog) << "Running vdir IO job" << label << worker->path;
        Q_ASSERT(QThread::currentThread() == worker->thread);
        job();
    };

    QMetaObject::invokeMethod(worker->receiver, std::move(queuedJob), Qt::QueuedConnection);
    return true;
}

bool VdirIoScheduler::submitCompositeJob(const QList<VdirPath> &paths,
                                         const QString &label,
                                         std::function<void()> job) const
{
    {
        QMutexLocker locker(&m_mutex);
        if (m_shuttingDown)
        {
            return false;
        }
    }

    const std::optional<QStringList> scheduledPaths = normalizedCompositePaths(paths, label);
    if (!scheduledPaths)
    {
        return false;
    }

    QThreadPool::globalInstance()->start([this, label, scheduledPaths, job = std::move(job)]() mutable {
        qCDebug(storageLog) << "Running composite vdir IO job" << label << *scheduledPaths;
        Q_ASSERT_X(!isWorkerThread(),
                   "VdirIoScheduler::submitComposite",
                   "composite orchestration must not run on a vdir worker");
        job();
    });
    return true;
}

std::optional<QStringList> VdirIoScheduler::normalizedCompositePaths(const QList<VdirPath> &paths, const QString &label)
{
    if (paths.isEmpty())
    {
        qCWarning(storageLog) << "VdirIoScheduler composite job has no paths" << label;
        return std::nullopt;
    }

    QStringList normalizedPaths;
    normalizedPaths.reserve(paths.size());
    for (const VdirPath &path : paths)
    {
        const QString key = VdirPath::normalizedPath(path.path());
        if (key.isEmpty())
        {
            qCWarning(storageLog) << "VdirIoScheduler composite job has an invalid path" << label;
            return std::nullopt;
        }
        normalizedPaths.append(key);
    }

    std::sort(normalizedPaths.begin(), normalizedPaths.end());
    QStringList uniquePaths;
    uniquePaths.reserve(normalizedPaths.size());
    for (const QString &path : std::as_const(normalizedPaths))
    {
        if (!uniquePaths.isEmpty() && uniquePaths.constLast() == path)
        {
            qCWarning(storageLog) << "VdirIoScheduler composite job received a duplicate path" << label << path;
            continue;
        }
        uniquePaths.append(path);
    }
    return uniquePaths;
}

std::shared_ptr<VdirIoScheduler::Worker> VdirIoScheduler::workerForPath(const QString &path) const
{
    const QString key = VdirPath::normalizedPath(path);
    if (key.isEmpty())
    {
        qCWarning(storageLog) << "VdirIoScheduler cannot schedule IO for an empty path";
        return {};
    }

    QMutexLocker locker(&m_mutex);
    if (m_shuttingDown)
    {
        qCWarning(storageLog) << "VdirIoScheduler cannot submit vdir IO after shutdown";
        return {};
    }

    const auto cached = m_workers.constFind(key);
    if (cached != m_workers.constEnd())
    {
        return *cached;
    }

    auto worker = std::make_shared<Worker>();
    worker->path = key;
    worker->thread = new QThread;
    worker->receiver = new QObject;
    worker->receiver->moveToThread(worker->thread);
    worker->thread->setObjectName(QStringLiteral("vdir-io:%1").arg(QFileInfo(key).fileName()));
    worker->thread->start();
    m_workers.insert(key, worker);
    return worker;
}

void VdirIoScheduler::shutdown()
{
    QHash<QString, std::shared_ptr<Worker>> workers;
    {
        QMutexLocker locker(&m_mutex);
        if (m_shuttingDown && m_workers.isEmpty())
        {
            return;
        }
        m_shuttingDown = true;
        workers.swap(m_workers);
    }
    for (const std::shared_ptr<Worker> &worker : std::as_const(workers))
    {
        worker->requestQuit();
    }
    for (const std::shared_ptr<Worker> &worker : std::as_const(workers))
    {
        worker->wait(ShutdownWaitMsecs);
    }
    workers.clear();
}

bool VdirIoScheduler::isWorkerThread() const
{
    const QThread *current = QThread::currentThread();
    QMutexLocker locker(&m_mutex);
    for (auto it = m_workers.cbegin(); it != m_workers.cend(); ++it)
    {
        if (it.value() && it.value()->thread == current)
        {
            return true;
        }
    }
    return false;
}

bool VdirIoScheduler::isWorkerThreadForPath(const VdirPath &path) const
{
    const QString key = VdirPath::normalizedPath(path.path());
    if (key.isEmpty())
    {
        return false;
    }

    const QThread *current = QThread::currentThread();
    QMutexLocker locker(&m_mutex);
    const auto worker = m_workers.constFind(key);
    return worker != m_workers.constEnd() && worker.value() && worker.value()->thread == current;
}
