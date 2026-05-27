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

#ifndef MOVEOUTCOMELOG_H
#define MOVEOUTCOMELOG_H

#include "storagelog.h"
#include "storageresult.h"

#include <QDebug>
#include <QString>

#include <variant>

namespace MoveOutcomeLog {

inline void logCrossVdirFailures(const MoveOutcome &move,
                                 const QString &removeFailureMessage,
                                 const QString &rollbackFailureMessage)
{
    if (const auto *sourceRemove = std::get_if<MoveSourceRemoveFailed>(&move.outcome))
    {
        qCWarning(storageLog) << removeFailureMessage << itemRefDescription(move.sourceStorage())
                              << storageStatusName(sourceRemove->status);
    }
    if (const auto *rollback = std::get_if<MoveRollbackFailed>(&move.outcome))
    {
        qCWarning(storageLog) << removeFailureMessage << itemRefDescription(move.sourceStorage())
                              << storageStatusName(rollback->sourceStatus);
        qCWarning(storageLog) << rollbackFailureMessage << itemRefDescription(move.destinationStorage())
                              << storageStatusName(rollback->rollbackStatus);
    }
}

} // namespace MoveOutcomeLog

#endif // MOVEOUTCOMELOG_H
