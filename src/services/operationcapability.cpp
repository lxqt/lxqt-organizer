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

#include "operationcapability.h"

#include "collectionservice.h"
#include "vdirio.h"

#include <optional>

OperationCapability allowedOperationCapability(const ItemRef &source, const QString &destinationCollectionId)
{
    return OperationCapability{OperationCapabilityStatus::Allowed, source, destinationCollectionId};
}

OperationCapability invalidSelectionCapability(const ItemRef &source, const QString &destinationCollectionId)
{
    return OperationCapability{OperationCapabilityStatus::InvalidSelection, source, destinationCollectionId};
}

OperationCapability capabilityForWritableCollection(const CollectionService &collections,
                                                    CollectionKind kind,
                                                    const QString &collectionId,
                                                    OperationCapabilityStatus readOnlyStatus)
{
    const std::optional<Collection> collection =
        kind == CollectionKind::Calendar
            ? (collectionId.isEmpty() ? collections.defaultCalendar() : collections.calendarForRead(collectionId))
            : (collectionId.isEmpty() ? collections.defaultAddressBook()
                                      : collections.addressBookForRead(collectionId));
    if (!collection)
    {
        return OperationCapability{OperationCapabilityStatus::CollectionMissing, {}, collectionId};
    }
    if (!collection->isWritable())
    {
        return OperationCapability{readOnlyStatus, {}, collection->id};
    }
    if (!VdirPath::fromCollection(*collection).isValid())
    {
        return OperationCapability{OperationCapabilityStatus::StorageUnavailable, {}, collection->id};
    }
    return allowedOperationCapability({}, collection->id);
}

OperationCapability capabilityForWritableCollection(const CollectionService &collections,
                                                    CollectionKind kind,
                                                    const QString &collectionId,
                                                    OperationCapabilityStatus readOnlyStatus,
                                                    const ItemRef &source,
                                                    const QString &destinationCollectionId)
{
    OperationCapability capability = capabilityForWritableCollection(collections, kind, collectionId, readOnlyStatus);
    capability.source = source;
    capability.destinationCollectionId = destinationCollectionId;
    return capability;
}
