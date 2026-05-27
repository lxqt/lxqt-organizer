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

#ifndef VDIRIO_H
#define VDIRIO_H

#include "collectiontypes.h"

#include <QFuture>
#include <QList>
#include <QPromise>
#include <QString>
#include <QtGlobal>

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

class VdirPath
{
public:
    static VdirPath fromCanonicalPath(QString path) { return VdirPath(std::move(path)); }

    static VdirPath fromCollection(const Collection &collection) { return fromCanonicalPath(collection.canonicalPath); }

    bool isValid() const { return !m_path.isEmpty(); }
    const QString &path() const { return m_path; }

private:
    explicit VdirPath(QString path)
        : m_path(std::move(path))
    {}

    QString m_path;
};

class VdirIo
{
public:
    virtual ~VdirIo() = default;

    VdirIo(const VdirIo &) = delete;
    VdirIo &operator=(const VdirIo &) = delete;

    template <typename Callable>
    auto submit(const VdirPath &path,
                const QString &label,
                std::invoke_result_t<std::decay_t<Callable>> failureValue,
                Callable &&callable) const -> QFuture<std::invoke_result_t<std::decay_t<Callable>>>;

    template <typename Callable>
    auto submit(const VdirPath &path, const QString &label, Callable &&callable) const
        -> QFuture<std::invoke_result_t<std::decay_t<Callable>>>;

    template <typename Callable>
    auto submitComposite(const QList<VdirPath> &paths,
                         const QString &label,
                         std::invoke_result_t<std::decay_t<Callable>> failureValue,
                         Callable &&callable) const -> QFuture<std::invoke_result_t<std::decay_t<Callable>>>;

    template <typename Callable>
    auto runSync(const VdirPath &path,
                 const QString &label,
                 std::invoke_result_t<std::decay_t<Callable>> failureValue,
                 Callable &&callable) const -> std::invoke_result_t<std::decay_t<Callable>>;

    template <typename Callable>
    auto runSync(const VdirPath &path, const QString &label, Callable &&callable) const
        -> std::invoke_result_t<std::decay_t<Callable>>;

    virtual bool isWorkerThread() const = 0;
    virtual bool isWorkerThreadForPath(const VdirPath &path) const = 0;

protected:
    VdirIo() = default;

    virtual bool submitJob(const VdirPath &path, const QString &label, std::function<void()> job) const = 0;
    virtual bool
    submitCompositeJob(const QList<VdirPath> &paths, const QString &label, std::function<void()> job) const = 0;
};

template <typename Callable>
auto VdirIo::submit(const VdirPath &path,
                    const QString &label,
                    std::invoke_result_t<std::decay_t<Callable>> failureValue,
                    Callable &&callable) const -> QFuture<std::invoke_result_t<std::decay_t<Callable>>>
{
    using Result = std::invoke_result_t<std::decay_t<Callable>>;

    auto promise = std::make_shared<QPromise<Result>>();
    QFuture<Result> future = promise->future();
    promise->start();

    auto task = std::make_shared<std::decay_t<Callable>>(std::forward<Callable>(callable));
    auto job = [promise, task]() mutable {
        if constexpr (std::is_void_v<Result>)
        {
            (*task)();
            promise->finish();
        }
        else
        {
            promise->addResult((*task)());
            promise->finish();
        }
    };

    if (!submitJob(path, label, std::move(job)))
    {
        promise->addResult(std::move(failureValue));
        promise->finish();
    }
    return future;
}

template <typename Callable>
auto VdirIo::submit(const VdirPath &path, const QString &label, Callable &&callable) const
    -> QFuture<std::invoke_result_t<std::decay_t<Callable>>>
{
    static_assert(
        std::is_void_v<std::invoke_result_t<std::decay_t<Callable>>>,
        "non-void submit calls must provide an explicit failure value; pass failureValue as the third argument");
    using Result = std::invoke_result_t<std::decay_t<Callable>>;

    auto promise = std::make_shared<QPromise<Result>>();
    QFuture<Result> future = promise->future();
    promise->start();

    auto task = std::make_shared<std::decay_t<Callable>>(std::forward<Callable>(callable));
    auto job = [promise, task]() mutable {
        (*task)();
        promise->finish();
    };

    if (!submitJob(path, label, std::move(job)))
    {
        promise->finish();
    }
    return future;
}

template <typename Callable>
auto VdirIo::submitComposite(const QList<VdirPath> &paths,
                             const QString &label,
                             std::invoke_result_t<std::decay_t<Callable>> failureValue,
                             Callable &&callable) const -> QFuture<std::invoke_result_t<std::decay_t<Callable>>>
{
    using Result = std::invoke_result_t<std::decay_t<Callable>>;

    auto promise = std::make_shared<QPromise<Result>>();
    QFuture<Result> future = promise->future();
    promise->start();

    auto task = std::make_shared<std::decay_t<Callable>>(std::forward<Callable>(callable));
    auto job = [promise, task]() mutable {
        if constexpr (std::is_void_v<Result>)
        {
            (*task)();
            promise->finish();
        }
        else
        {
            promise->addResult((*task)());
            promise->finish();
        }
    };

    if (!submitCompositeJob(paths, label, std::move(job)))
    {
        promise->addResult(std::move(failureValue));
        promise->finish();
    }
    return future;
}

template <typename Callable>
auto VdirIo::runSync(const VdirPath &path,
                     const QString &label,
                     std::invoke_result_t<std::decay_t<Callable>> failureValue,
                     Callable &&callable) const -> std::invoke_result_t<std::decay_t<Callable>>
{
    using Result = std::invoke_result_t<std::decay_t<Callable>>;

    if (isWorkerThreadForPath(path))
    {
        if constexpr (std::is_void_v<Result>)
        {
            std::forward<Callable>(callable)();
            return;
        }
        else
        {
            return std::forward<Callable>(callable)();
        }
    }

    if (isWorkerThread())
    {
        Q_ASSERT_X(false, "VdirIo::runSync", "cross-vdir synchronous call from a different vdir worker can deadlock");
        return failureValue;
    }

    QFuture<Result> future = submit(path, label, std::move(failureValue), std::forward<Callable>(callable));
    future.waitForFinished();
    if constexpr (std::is_void_v<Result>)
    {
        return;
    }
    else
    {
        return future.result();
    }
}

template <typename Callable>
auto VdirIo::runSync(const VdirPath &path, const QString &label, Callable &&callable) const
    -> std::invoke_result_t<std::decay_t<Callable>>
{
    static_assert(
        std::is_void_v<std::invoke_result_t<std::decay_t<Callable>>>,
        "non-void runSync calls must provide an explicit failure value; pass failureValue as the third argument");

    if (isWorkerThreadForPath(path))
    {
        std::forward<Callable>(callable)();
        return;
    }

    if (isWorkerThread())
    {
        Q_ASSERT_X(false, "VdirIo::runSync", "cross-vdir synchronous call from a different vdir worker can deadlock");
        return;
    }

    QFuture<void> future = submit(path, label, std::forward<Callable>(callable));
    future.waitForFinished();
}

#endif // VDIRIO_H
