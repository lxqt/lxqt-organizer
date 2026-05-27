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

#ifndef EVENTREADER_H
#define EVENTREADER_H

#include "calendareditordata.h"
#include "calendaritem.h"
#include "cancellationtoken.h"
#include "collectionreloadsubscription.h"
#include "collectionservice.h"
#include "storageresult.h"

#include <QDate>
#include <QFuture>
#include <QHash>
#include <QList>
#include <QMutex>
#include <QSet>

#include <functional>
#include <memory>
#include <optional>

class VdirIo;
class CalendarItemStore;

struct EventOccurrenceListResult
{
    QList<EventOccurrence> occurrences;
    QList<ReadFailure> readFailures;
};

// @thread any-thread-with-mutex; occurrence cache mutations are mutex-protected.
class EventReader
{
public:
    enum class SearchDirection
    {
        Forward,
        Backward
    };

    explicit EventReader(const CollectionService &collections, const VdirIo &vdirIo);
    ~EventReader();

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
    void clearCache() const;
    void clearCacheForCollections(const QSet<QString> &collectionIds) const;
    void invalidateItem(const ItemKey &item) const;

private:
    static constexpr qsizetype MaxOccurrenceCacheEntries = 256;

    struct OccurrenceCacheKey
    {
        ItemKey item;
        QString etag;
        QString uid;
        QDate rangeStart;
        QDate rangeEnd;

        bool operator==(const OccurrenceCacheKey &other) const
        {
            return item == other.item && etag == other.etag && uid == other.uid && rangeStart == other.rangeStart &&
                   rangeEnd == other.rangeEnd;
        }

        friend size_t qHash(const OccurrenceCacheKey &key, size_t seed = 0)
        {
            seed = qHash(key.item, seed);
            seed = qHash(key.etag, seed);
            seed = qHash(key.uid, seed);
            seed = qHash(key.rangeStart, seed);
            return qHash(key.rangeEnd, seed);
        }
    };

    QList<EventOccurrence>
    occurrencesForItem(const CalendarItem &object, const QDate &rangeStart, const QDate &rangeEnd) const;
    EventOccurrenceListResult collectOccurrencesInRangeForCollection(const Collection &collection,
                                                                     const QDate &rangeStart,
                                                                     const QDate &rangeEnd,
                                                                     const CancellationToken &cancellation) const;

    const CollectionService &m_collections;
    std::unique_ptr<CalendarItemStore> m_calendarItems;
    // Occurrence expansion is memoized from const read methods and guarded so
    // views can share the reader without racing the cache.
    mutable QHash<OccurrenceCacheKey, QList<EventOccurrence>> m_occurrenceCache;
    mutable QList<OccurrenceCacheKey> m_occurrenceCacheLru;
    // Leaf cache mutex: must not call out while held.
    mutable QMutex m_occurrenceCacheMutex;
    CollectionReloadSubscription m_collectionReloadSubscription;
};

#endif // EVENTREADER_H
