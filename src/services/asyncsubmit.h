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

#ifndef ASYNCSUBMIT_H
#define ASYNCSUBMIT_H

#include "completedfuture.h"
#include "storageresult.h"
#include "vdirio.h"

#include <QFuture>
#include <QString>

#include <optional>
#include <type_traits>
#include <utility>

namespace AsyncSubmit {

struct ResolvedCollection
{
    std::optional<Collection> collection;
    StorageStatus failureStatus = StorageStatus::NotFound;
};

inline ResolvedCollection resolvedCollection(ResolvedCollection resolved)
{
    return resolved;
}

inline ResolvedCollection resolvedCollection(std::optional<Collection> collection)
{
    return ResolvedCollection{std::move(collection), StorageStatus::NotFound};
}

template <typename Result, typename ResolveFn, typename SyncFn, typename FailFn>
QFuture<Result> submit(
    const VdirIo &scheduler, const QString &label, ResolveFn &&resolveCollection, SyncFn &&syncCall, FailFn &&failureFn)
{
    static_assert(std::is_same_v<Result, std::invoke_result_t<std::decay_t<SyncFn>>>,
                  "AsyncSubmit::submit Result must match the sync callable return type");

    const ResolvedCollection resolved = AsyncSubmit::resolvedCollection(std::forward<ResolveFn>(resolveCollection)());
    if (!resolved.collection.has_value())
    {
        return CalendarIoUtils::completedFuture<Result>(std::forward<FailFn>(failureFn)(resolved.failureStatus));
    }

    const VdirPath path = VdirPath::fromCollection(*resolved.collection);
    if (!path.isValid())
    {
        return CalendarIoUtils::completedFuture<Result>(std::forward<FailFn>(failureFn)(StorageStatus::IoError));
    }

    return scheduler.submit(
        path, label, std::forward<FailFn>(failureFn)(StorageStatus::IoError), std::forward<SyncFn>(syncCall));
}

template <typename Result, typename ResolveSourceFn, typename ResolveDestFn, typename SyncFn, typename FailFn>
QFuture<Result> submitMove(const VdirIo &scheduler,
                           const QString &label,
                           const QString &destinationCollectionId,
                           ResolveSourceFn &&resolveSource,
                           ResolveDestFn &&resolveDestination,
                           SyncFn &&syncCall,
                           FailFn &&failureFn)
{
    static_assert(std::is_same_v<Result, std::invoke_result_t<std::decay_t<SyncFn>>>,
                  "AsyncSubmit::submitMove Result must match the sync callable return type");

    const ResolvedCollection source = AsyncSubmit::resolvedCollection(std::forward<ResolveSourceFn>(resolveSource)());
    if (!source.collection.has_value())
    {
        return CalendarIoUtils::completedFuture<Result>(std::forward<FailFn>(failureFn)(source.failureStatus));
    }

    const VdirPath sourcePath = VdirPath::fromCollection(*source.collection);
    if (!sourcePath.isValid())
    {
        return CalendarIoUtils::completedFuture<Result>(std::forward<FailFn>(failureFn)(StorageStatus::IoError));
    }

    if (source.collection->id == destinationCollectionId)
    {
        return scheduler.submit(
            sourcePath, label, std::forward<FailFn>(failureFn)(StorageStatus::IoError), std::forward<SyncFn>(syncCall));
    }

    const ResolvedCollection destination =
        AsyncSubmit::resolvedCollection(std::forward<ResolveDestFn>(resolveDestination)());
    if (!destination.collection.has_value())
    {
        return CalendarIoUtils::completedFuture<Result>(std::forward<FailFn>(failureFn)(destination.failureStatus));
    }

    const VdirPath destinationPath = VdirPath::fromCollection(*destination.collection);
    if (!destinationPath.isValid())
    {
        return CalendarIoUtils::completedFuture<Result>(std::forward<FailFn>(failureFn)(StorageStatus::IoError));
    }

    return scheduler.submitComposite({sourcePath, destinationPath},
                                     label,
                                     std::forward<FailFn>(failureFn)(StorageStatus::IoError),
                                     std::forward<SyncFn>(syncCall));
}

} // namespace AsyncSubmit

#endif // ASYNCSUBMIT_H
