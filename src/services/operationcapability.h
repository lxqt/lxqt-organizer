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

#ifndef OPERATIONCAPABILITY_H
#define OPERATIONCAPABILITY_H

#include "collectiontypes.h"
#include "itemidentity.h"

#include <QString>

class CollectionService;

enum class OperationCapabilityStatus
{
    Allowed,
    InvalidSelection,
    CollectionMissing,
    SourceReadOnly,
    DestinationReadOnly,
    UnsupportedItemShape,
    UnsupportedRecurringSeries,
    UnsupportedRecurringInstance,
    ConflictLikely,
    StorageUnavailable,
};

struct [[nodiscard]] OperationCapability
{
    OperationCapabilityStatus status = OperationCapabilityStatus::StorageUnavailable;
    ItemRef source;
    QString destinationCollectionId;

    bool isAllowed() const { return status == OperationCapabilityStatus::Allowed; }
};

OperationCapability allowedOperationCapability(const ItemRef &source = {}, const QString &destinationCollectionId = {});

OperationCapability invalidSelectionCapability(const ItemRef &source = {}, const QString &destinationCollectionId = {});

OperationCapability capabilityForWritableCollection(const CollectionService &collections,
                                                    CollectionKind kind,
                                                    const QString &collectionId,
                                                    OperationCapabilityStatus readOnlyStatus);

OperationCapability capabilityForWritableCollection(const CollectionService &collections,
                                                    CollectionKind kind,
                                                    const QString &collectionId,
                                                    OperationCapabilityStatus readOnlyStatus,
                                                    const ItemRef &source,
                                                    const QString &destinationCollectionId = {});

#endif // OPERATIONCAPABILITY_H
