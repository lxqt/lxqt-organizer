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

#include "calendareditordata.h"
#include "calendaritem.h"
#include "calendaritemstore.h"
#include "cancellationtoken.h"
#include "collectionreloadsubscription.h"
#include "collectionservice.h"
#include "itemidentity.h"
#include "operationcapability.h"
#include "storageresult.h"

#include <QDate>
#include <QFuture>
#include <QList>
#include <QSet>
#include <QString>

#include <functional>
#include <memory>
#include <optional>

class VdirIo;

struct EventOccurrenceListResult
{
    QList<EventOccurrence> occurrences;
    QList<ReadFailure> readFailures;
};

// @thread any-thread; caches live behind store mutexes and writes use VdirIo.
class EventService
{
public:
    enum class SearchDirection
    {
        Forward,
        Backward
    };

    using EventSaveResult = StorageResult<CalendarItem>;
    using EventUpdateResult = StorageMoveResult<CalendarItem>;
    using EventMoveResult = StorageMoveResult<CalendarItem>;

    explicit EventService(const CollectionService &collections, const VdirIo &vdirIo);
    ~EventService();

    // Read API.
    EventFields defaultEventEditorData(const QDate &date) const;
    EventFields editorDataForOccurrence(const EventOccurrence &event) const;
    QList<CalendarItem> calendarItemSummaries(QList<ReadFailure> *readFailures = nullptr,
                                              const CancellationToken &cancellation = {}) const;
    EventOccurrence eventSeries(const OccurrenceRef &occurrence) const;
    QFuture<EventOccurrenceListResult> eventOccurrencesInDateRangeAsync(
        const QDate &rangeStart, const QDate &rangeEnd, const CancellationToken &cancellation = {}) const;
    std::optional<EventOccurrence>
    searchOccurrences(const QString &needle,
                      const QDate &anchor,
                      SearchDirection direction,
                      const EventOccurrence &current,
                      const std::function<bool(const QString &collectionId)> &isCollectionVisible,
                      const CancellationToken &cancellation = {}) const;
    void invalidateItem(const ItemKey &item) const;

    // Write API.
    EventSaveResult addEvent(const EventFields &event) const;
    EventUpdateResult updateEvent(const EventOccurrence &currentOccurrence, const EventFields &event) const;
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
    QFuture<EventUpdateResult> updateEventAsync(const EventOccurrence &currentOccurrence,
                                                const EventFields &event) const;
    QFuture<EventMoveResult> moveEventAsync(const EventOccurrence &currentOccurrence,
                                            const EventFields &event,
                                            const QString &destinationCollectionId) const;
    QFuture<StorageStatus> deleteEventAsync(const OccurrenceRef &occurrence) const;
    void clearCache() const;
    void clearCacheForCollections(const QSet<QString> &collectionIds) const;
    void retainRepositoriesForCollections(const QList<Collection> &collections) const;

private:
    struct EventMoveWriteResult;
    struct OccurrenceCache;
    void notifyItemWritten(const ItemRef &storage) const;
    EventMoveWriteResult moveEventObject(const EventOccurrence &currentOccurrence,
                                         const EventFields &event,
                                         const QString &destinationCollectionId) const;
    QList<EventOccurrence>
    occurrencesForItem(const CalendarItem &object, const QDate &rangeStart, const QDate &rangeEnd) const;
    EventOccurrenceListResult collectOccurrencesInRangeForCollection(const Collection &collection,
                                                                     const QDate &rangeStart,
                                                                     const QDate &rangeEnd,
                                                                     const CancellationToken &cancellation) const;

    const CollectionService &m_collections;
    CalendarItemStore m_items;
    std::unique_ptr<OccurrenceCache> m_occurrenceCache;
    // Declared last so destruction disconnects the reload handler before the
    // store it captures is torn down.
    CollectionReloadSubscription m_collectionReloadSubscription;
};

#endif // EVENTSERVICE_H
