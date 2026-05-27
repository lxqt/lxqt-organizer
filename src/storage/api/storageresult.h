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

#ifndef STORAGERESULT_H
#define STORAGERESULT_H

#include "itemidentity.h"

#include <QString>
#include <QStringList>

#include <type_traits>
#include <utility>
#include <variant>

enum class [[nodiscard]] StorageStatus
{
    Ok,
    InvalidHref,
    NotFound,
    ReadOnly,
    Conflict,
    ParseError,
    Unsupported,
    IoError
};

struct CollectionSaveResult
{
    StorageStatus status = StorageStatus::IoError;
    QStringList warnings;

    bool isOk() const { return status == StorageStatus::Ok; }
};

struct [[nodiscard]] UpdateOutcome
{
    StorageStatus status = StorageStatus::IoError;
    ItemRef sourceStorage;
    ItemRef destinationStorage;

    bool isOk() const { return status == StorageStatus::Ok; }
};

struct [[nodiscard]] MoveSuccess
{
    ItemRef sourceStorage;
    ItemRef destinationStorage;
};

struct [[nodiscard]] MoveDestinationCreateFailed
{
    StorageStatus status = StorageStatus::IoError;
    ItemRef sourceStorage;
    ItemRef destinationStorage;
};

struct [[nodiscard]] MoveReverted
{
    StorageStatus status = StorageStatus::IoError;
    ItemRef sourceStorage;
    ItemRef destinationStorage;
};

struct [[nodiscard]] MoveRollbackFailed
{
    StorageStatus sourceStatus = StorageStatus::IoError;
    StorageStatus rollbackStatus = StorageStatus::IoError;
    ItemRef sourceStorage;
    ItemRef destinationStorage;
};

struct [[nodiscard]] MoveOutcome
{
    using Outcome =
        std::variant<UpdateOutcome, MoveSuccess, MoveDestinationCreateFailed, MoveReverted, MoveRollbackFailed>;

    Outcome outcome = UpdateOutcome{};

    static MoveOutcome preconditionFailed(StorageStatus status, const ItemRef &source, const ItemRef &destination)
    {
        MoveOutcome result;
        result.outcome = UpdateOutcome{status, source, destination};
        return result;
    }

    static MoveOutcome update(UpdateOutcome update)
    {
        MoveOutcome result;
        result.outcome = update;
        return result;
    }

    static MoveOutcome destinationCreateFailed(StorageStatus status, const ItemRef &source, const ItemRef &destination)
    {
        MoveOutcome result;
        result.outcome = MoveDestinationCreateFailed{status, source, destination};
        return result;
    }

    static MoveOutcome crossCollection(const ItemRef &source,
                                       const ItemRef &destination,
                                       StorageStatus sourceRemoveStatus,
                                       StorageStatus rollbackStatus = StorageStatus::Ok)
    {
        MoveOutcome result;
        if (sourceRemoveStatus == StorageStatus::Ok)
        {
            result.outcome = MoveSuccess{source, destination};
        }
        else if (rollbackStatus == StorageStatus::Ok)
        {
            result.outcome = MoveReverted{sourceRemoveStatus, source, destination};
        }
        else
        {
            result.outcome = MoveRollbackFailed{sourceRemoveStatus, rollbackStatus, source, destination};
        }
        return result;
    }

    ItemRef sourceStorage() const
    {
        return std::visit([](const auto &value) { return value.sourceStorage; }, outcome);
    }

    ItemRef destinationStorage() const
    {
        return std::visit([](const auto &value) { return value.destinationStorage; }, outcome);
    }

    bool isOk() const
    {
        if (const auto *value = std::get_if<UpdateOutcome>(&outcome))
        {
            return value->isOk();
        }
        return std::holds_alternative<MoveSuccess>(outcome);
    }
};

struct ReadFailure
{
    ItemRef ref;
    StorageStatus status = StorageStatus::Ok;

    bool isFailure() const { return status != StorageStatus::Ok; }
};

template <typename T> struct [[nodiscard]] ReadResult
{
    using Object = T;

    T object;
    StorageStatus status = StorageStatus::NotFound;

    bool isOk() const { return status == StorageStatus::Ok; }
};

template <typename T> struct [[nodiscard]] StorageResult
{
    StorageStatus status = StorageStatus::IoError;
    // On successful writes, snapshot.ref contains the committed href and
    // fresh etag returned by storage, replacing any pre-write etag.
    T snapshot;

    bool isOk() const { return status == StorageStatus::Ok; }

    static StorageResult ok(const T &snapshot)
    {
        StorageResult result;
        result.status = StorageStatus::Ok;
        result.snapshot = snapshot;
        return result;
    }

    static StorageResult fail(StorageStatus status, const T &snapshot = {})
    {
        StorageResult result;
        result.status = status;
        result.snapshot = snapshot;
        return result;
    }
};

template <typename T> struct [[nodiscard]] StorageMoveResult
{
    MoveOutcome move;
    T snapshot;

    bool isOk() const { return move.isOk(); }

    static StorageMoveResult ok(const T &snapshot, MoveOutcome move)
    {
        StorageMoveResult result;
        result.move = move;
        result.snapshot = snapshot;
        return result;
    }

    static StorageMoveResult fail(MoveOutcome move, const T &snapshot = {})
    {
        StorageMoveResult result;
        result.move = move;
        result.snapshot = snapshot;
        return result;
    }
};

template <typename T, typename F>
auto map(const StorageResult<T> &result, F &&function) -> StorageResult<std::invoke_result_t<F, const T &>>
{
    using Mapped = std::invoke_result_t<F, const T &>;
    if (!result.isOk())
    {
        return StorageResult<Mapped>::fail(result.status);
    }
    return StorageResult<Mapped>::ok(std::forward<F>(function)(result.snapshot));
}

template <typename T, typename F>
auto andThen(const StorageResult<T> &result, F &&function) -> std::invoke_result_t<F, const T &>
{
    using Chained = std::invoke_result_t<F, const T &>;
    if (!result.isOk())
    {
        return Chained::fail(result.status);
    }
    return std::forward<F>(function)(result.snapshot);
}

inline const char *storageStatusName(StorageStatus status)
{
    switch (status)
    {
    case StorageStatus::Ok:
        return "ok";
    case StorageStatus::InvalidHref:
        return "invalid href";
    case StorageStatus::NotFound:
        return "not found";
    case StorageStatus::ReadOnly:
        return "read-only";
    case StorageStatus::Conflict:
        return "conflict";
    case StorageStatus::ParseError:
        return "parse error";
    case StorageStatus::Unsupported:
        return "unsupported";
    case StorageStatus::IoError:
        return "I/O error";
    }
    return "unknown";
}

inline QString itemRefDescription(const ItemRef &storage)
{
    const QString collection =
        storage.collectionId.isEmpty() ? QStringLiteral("<unknown collection>") : storage.collectionId;
    const QString href = storage.href.isEmpty() ? QStringLiteral("<unknown href>") : storage.href;
    return collection + QLatin1Char('/') + href;
}

#endif // STORAGERESULT_H
