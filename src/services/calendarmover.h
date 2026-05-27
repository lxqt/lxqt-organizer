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

#ifndef CALENDARMOVER_H
#define CALENDARMOVER_H

#include "calendaritemstore.h"

#include <optional>
#include <utility>

namespace CalendarMover {

template <typename MoveResult, typename Snapshot, typename SourceItem, typename SuccessMapper>
MoveResult moveObject(const CalendarItemStore &calendarItems,
                      const SourceItem &source,
                      const ItemRef &sourceStorage,
                      const std::optional<Collection> &destinationCollection,
                      CalendarItem inserted,
                      const Snapshot &fallbackSnapshot,
                      SuccessMapper &&successMapper)
{
    const StorageResult<CalendarItem> createResult = calendarItems.addObject(inserted);
    if (!createResult.isOk())
    {
        MoveResult result;
        result.snapshot = fallbackSnapshot;
        result.move =
            MoveOutcome::destinationCreateFailed(createResult.status, sourceStorage, createResult.snapshot.ref);
        return result;
    }
    inserted = createResult.snapshot;

    MoveResult result;
    result.snapshot = fallbackSnapshot;
    result.move = calendarItems.commitCrossCollectionMove(source, destinationCollection, inserted.ref);
    if (result.move.isOk())
    {
        result.snapshot = std::forward<SuccessMapper>(successMapper)(inserted);
    }
    return result;
}

} // namespace CalendarMover

#endif // CALENDARMOVER_H
