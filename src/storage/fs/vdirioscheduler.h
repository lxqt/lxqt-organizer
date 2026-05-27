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

#ifndef VDIRIOSCHEDULER_H
#define VDIRIOSCHEDULER_H

#include "storagelog.h"
#include "vdirio.h"

#include <QHash>
#include <QList>
#include <QMetaObject>
#include <QMutex>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QThread>

#include <functional>
#include <memory>
#include <optional>

// @thread any-thread; public entry points are mutex-protected.
// Path-keyed storage scheduler for vdir repositories.
//
// The scheduler serializes work only inside this process. External writers such
// as vdirsyncer, khal, khard, or another organizer process are handled by vdir
// atomic file writes plus etag/conflict checks in the storage layer, not by this
// queue. Keep this class as vdir storage infrastructure; it is not a general app
// executor: callers must supply a VdirPath produced from canonical collection
// paths, not arbitrary work-queue keys.
//
// Scaling note: one idle worker thread per vdir is acceptable for the expected
// desktop case up to roughly 64 collections. If that stops holding, replace the
// worker map with a bounded thread pool plus a key->FIFO queue map so each vdir
// path still has in-order execution while independent paths share workers.
class VdirIoScheduler : public VdirIo
{
public:
    VdirIoScheduler();
    ~VdirIoScheduler();

    VdirIoScheduler(const VdirIoScheduler &) = delete;
    VdirIoScheduler &operator=(const VdirIoScheduler &) = delete;

    void shutdown();
    bool isWorkerThread() const override;
    bool isWorkerThreadForPath(const VdirPath &path) const override;

private:
    struct Worker;

    bool submitJob(const VdirPath &path, const QString &label, std::function<void()> job) const override;
    bool
    submitCompositeJob(const QList<VdirPath> &paths, const QString &label, std::function<void()> job) const override;
    std::shared_ptr<Worker> workerForPathLocked(const QString &path) const;
    static std::optional<QStringList> normalizedCompositePaths(const QList<VdirPath> &paths, const QString &label);

    mutable QMutex m_mutex;
    mutable QHash<QString, std::shared_ptr<Worker>> m_workers;
    mutable bool m_shuttingDown = false;
};

struct VdirIoScheduler::Worker
{
    QString path;
    QThread *thread = nullptr;
    QPointer<QObject> receiver;
    bool quitPosted = false;

    ~Worker();
    void requestQuit();
    void wait(unsigned long timeoutMsecs);
};

#endif // VDIRIOSCHEDULER_H
