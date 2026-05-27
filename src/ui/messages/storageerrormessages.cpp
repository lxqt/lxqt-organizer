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

#include "storageerrormessages.h"

#include <QMessageBox>
#include <QObject>

namespace {

template <class... Ts> struct Overloaded : Ts...
{
    using Ts::operator()...;
};

template <class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

} // namespace

bool StorageOutcome::isOk() const
{
    return std::visit(Overloaded{[](StorageStatus status) { return status == StorageStatus::Ok; },
                                 [](const MoveOutcome &move) { return move.isOk(); }},
                      result);
}

namespace StorageErrorMessages {

QString storageFailureDetail(StorageStatus status)
{
    switch (status)
    {
    case StorageStatus::Ok:
        return QString();
    case StorageStatus::InvalidHref:
        return QObject::tr("The storage item name is invalid.", "StorageErrorMessages");
    case StorageStatus::NotFound:
        return QObject::tr("The item no longer exists in its collection.", "StorageErrorMessages");
    case StorageStatus::ReadOnly:
        return QObject::tr("The collection is read-only or unavailable.", "StorageErrorMessages");
    case StorageStatus::Conflict:
        return QObject::tr("The file was changed by another process.", "StorageErrorMessages");
    case StorageStatus::ParseError:
        return QObject::tr("The file could not be parsed.", "StorageErrorMessages");
    case StorageStatus::Unsupported:
        return QObject::tr("The item uses an unsupported format.", "StorageErrorMessages");
    case StorageStatus::IoError:
        return QObject::tr("The file could not be read or written.", "StorageErrorMessages");
    }
    return QObject::tr("The file could not be read or written.", "StorageErrorMessages");
}

QString operationFailureMessage(const QString &operation, const QString &item, StorageStatus status)
{
    const QString detail = storageFailureDetail(status);
    if (detail.isEmpty())
    {
        return QObject::tr("Could not %1 %2.", "StorageErrorMessages").arg(operation, item);
    }
    return QObject::tr("Could not %1 %2. %3", "StorageErrorMessages").arg(operation, item, detail);
}

QString moveFailureMessage(const QString &item, const MoveOutcome &result)
{
    return std::visit(
        Overloaded{
            [&](const UpdateOutcome &update) {
                return update.isOk()
                           ? QString()
                           : operationFailureMessage(QObject::tr("save", "StorageErrorMessages"), item, update.status);
            },
            [](const MoveSuccess &) { return QString(); },
            [&](const MoveDestinationCreateFailed &failure) {
                return QObject::tr("Could not save %1 in the destination collection. %2", "StorageErrorMessages")
                    .arg(item, storageFailureDetail(failure.status));
            },
            [&](const MoveSourceRemoveFailed &failure) {
                QString message =
                    QObject::tr("Could not remove the original %1 after copying it to the destination collection. %2",
                                "StorageErrorMessages")
                        .arg(item, storageFailureDetail(failure.status));
                message += QLatin1Char(' ');
                message += QObject::tr("The destination copy was removed.", "StorageErrorMessages");
                return message;
            },
            [&](const MoveRollbackFailed &failure) {
                return QObject::tr(
                           "Could not remove the original %1 after copying it to the destination collection. %2 "
                           "The destination copy could not be removed. %3",
                           "StorageErrorMessages")
                    .arg(
                        item, storageFailureDetail(failure.sourceStatus), storageFailureDetail(failure.rollbackStatus));
            }},
        result.outcome);
}

QString eventSaveFailureMessage(const QString &item, const MoveOutcome &result)
{
    return moveFailureMessage(item, result);
}

QString outcomeMessage(const QString &operation, const QString &item, const StorageOutcome &outcome)
{
    if (outcome.isOk())
    {
        return QString();
    }
    return std::visit(Overloaded{[&](StorageStatus status) { return operationFailureMessage(operation, item, status); },
                                 [&](const MoveOutcome &move) { return moveFailureMessage(item, move); }},
                      outcome.result);
}

bool presentOutcome(QWidget *parent, const QString &operation, const QString &item, const StorageOutcome &outcome)
{
    const QString message = outcomeMessage(operation, item, outcome);
    if (message.isEmpty())
    {
        return true;
    }
    QMessageBox::warning(parent, QObject::tr("Organizer", "StorageErrorMessages"), message);
    return false;
}

} // namespace StorageErrorMessages
