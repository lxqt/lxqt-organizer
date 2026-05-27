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

#ifndef EVENTSERVICE_H
#define EVENTSERVICE_H

#include "calendaritem.h"
#include "calendareditordata.h"
#include "collectionservice.h"
#include "itemidentity.h"
#include "operationcapability.h"
#include "storageresult.h"

#include <QList>
#include <QFuture>
#include <QSet>
#include <QString>

#include <memory>

class VdirIo;

// @thread any-thread; caches live behind store mutexes and writes use VdirIo.
class EventService
{
public:
    using EventSaveResult = StorageResult<CalendarItem>;
    using EventMoveResult = StorageMoveResult<CalendarItem>;

    explicit EventService(const CollectionService &collections, const VdirIo &vdirIo);
    ~EventService();

    EventSaveResult addEvent(const EventFields &event) const;
    EventSaveResult updateEvent(const EventOccurrence &currentOccurrence, const EventFields &event) const;
    EventMoveResult moveEvent(const EventOccurrence &currentOccurrence,
                              const EventFields &event,
                              const QString &destinationCollectionId) const;
    StorageStatus deleteEvent(const OccurrenceRef &occurrence) const;
    OperationCapability canCreateEvent(const QString &collectionId) const;
    OperationCapability canEditEvent(const EventOccurrence &event) const;
    OperationCapability canMoveEvent(const EventOccurrence &event, const QString &destinationCollectionId) const;
    OperationCapability canDeleteEvent(const OccurrenceRef &event) const;
    OperationCapability canCompleteEvent(const EventOccurrence &event) const;
    // Async methods submit storage work directly to VdirIo.
    QFuture<EventSaveResult> addEventAsync(const EventFields &event) const;
    QFuture<EventSaveResult> updateEventAsync(const EventOccurrence &currentOccurrence, const EventFields &event) const;
    QFuture<EventMoveResult> moveEventAsync(const EventOccurrence &currentOccurrence,
                                            const EventFields &event,
                                            const QString &destinationCollectionId) const;
    QFuture<StorageStatus> deleteEventAsync(const OccurrenceRef &occurrence) const;
    void clearCache() const;
    void clearCacheForCollections(const QSet<QString> &collectionIds) const;
    void retainRepositoriesForCollections(const QList<Collection> &collections) const;

private:
    class Impl;
    struct EventMoveWriteResult;
    void notifyItemWritten(const ItemRef &storage) const;
    EventMoveWriteResult moveEventObject(const EventOccurrence &currentOccurrence,
                                         const EventFields &event,
                                         const QString &destinationCollectionId) const;
    std::unique_ptr<Impl> m_impl;
};

#endif // EVENTSERVICE_H
