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

#ifndef STORAGEERRORMESSAGES_H
#define STORAGEERRORMESSAGES_H

#include "storageresult.h"

#include <QString>

#include <variant>

class QWidget;

struct [[nodiscard]] StorageOutcome
{
    std::variant<StorageStatus, MoveOutcome> result = StorageStatus::IoError;

    bool isOk() const;
};

namespace StorageErrorMessages {

QString storageFailureDetail(StorageStatus status);
QString operationFailureMessage(const QString &operation, const QString &item, StorageStatus status);
QString moveFailureMessage(const QString &item, const MoveOutcome &result);
QString eventSaveFailureMessage(const QString &item, const MoveOutcome &result);
QString outcomeMessage(const QString &operation, const QString &item, const StorageOutcome &outcome);
bool presentOutcome(QWidget *parent, const QString &operation, const QString &item, const StorageOutcome &outcome);

} // namespace StorageErrorMessages

#endif // STORAGEERRORMESSAGES_H
