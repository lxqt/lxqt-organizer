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

#include "operationcapabilitymessages.h"

#include <QMessageBox>
#include <QObject>

namespace OperationCapabilityMessages {

QString capabilityFailureDetail(OperationCapabilityStatus status)
{
    switch (status)
    {
    case OperationCapabilityStatus::Allowed:
        return QString();
    case OperationCapabilityStatus::InvalidSelection:
        return QObject::tr("The selected item could not be loaded.", "OperationCapabilityMessages");
    case OperationCapabilityStatus::CollectionMissing:
        return QObject::tr("The collection is no longer available.", "OperationCapabilityMessages");
    case OperationCapabilityStatus::SourceReadOnly:
        return QObject::tr("The source collection is read-only or unavailable.", "OperationCapabilityMessages");
    case OperationCapabilityStatus::DestinationReadOnly:
        return QObject::tr("The destination collection is read-only or unavailable.", "OperationCapabilityMessages");
    case OperationCapabilityStatus::UnsupportedItemShape:
        return QObject::tr("The item uses an unsupported format.", "OperationCapabilityMessages");
    case OperationCapabilityStatus::UnsupportedRecurringSeries:
        return QObject::tr("This recurring series operation is not supported.", "OperationCapabilityMessages");
    case OperationCapabilityStatus::UnsupportedRecurringInstance:
        return QObject::tr("This recurring occurrence operation is not supported.", "OperationCapabilityMessages");
    case OperationCapabilityStatus::ConflictLikely:
        return QObject::tr("The item may have changed since it was loaded.", "OperationCapabilityMessages");
    case OperationCapabilityStatus::StorageUnavailable:
        return QObject::tr("The collection storage path is unavailable.", "OperationCapabilityMessages");
    }
    return QObject::tr("The operation cannot be performed.", "OperationCapabilityMessages");
}

QString
operationCapabilityMessage(const OperationCapability &capability, const QString &operationName, const QString &itemName)
{
    if (capability.isAllowed())
    {
        return QString();
    }

    const QString detail = capabilityFailureDetail(capability.status);
    if (detail.isEmpty())
    {
        return QObject::tr("Could not %1 %2.", "OperationCapabilityMessages").arg(operationName, itemName);
    }
    return QObject::tr("Could not %1 %2. %3", "OperationCapabilityMessages").arg(operationName, itemName, detail);
}

bool presentCapability(QWidget *parent,
                       const OperationCapability &capability,
                       const QString &operationName,
                       const QString &itemName)
{
    const QString message = operationCapabilityMessage(capability, operationName, itemName);
    if (message.isEmpty())
    {
        return true;
    }
    QMessageBox::warning(parent, QObject::tr("Organizer", "OperationCapabilityMessages"), message);
    return false;
}

} // namespace OperationCapabilityMessages
