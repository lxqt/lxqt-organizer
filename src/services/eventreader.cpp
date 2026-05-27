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

#include "eventreader.h"

#include "calendaritemstore.h"
#include "completedfuture.h"
#include "eventoccurrences.h"
#include "calendareditormapper.h"
#include "calendaritemrepository.h"
#include "calendarvalidation.h"
#include "incidenceresolver.h"
#include "storagelog.h"
#include "vdirio.h"
#include "vdiritemrepository.h"

#include <QDebug>
#include <QFuture>
#include <QMutexLocker>
#include <QObject>

#include <KCalendarCore/Todo>

#include <algorithm>
#include <vector>

namespace {

ReadFailure readFailureForCalendarItem(const QString &collectionId,
                                       const CalendarItemRepository::ReadResult &readResult)
{
    ItemRef storage = readResult.object.ref;
    if (storage.collectionId.isEmpty())
    {
        storage.collectionId = collectionId;
    }
    return ReadFailure{storage, readResult.status};
}

bool eventStartsBefore(const EventOccurrence &first, const EventOccurrence &second)
{
    return first.start.time() < second.start.time();
}

bool sameOccurrence(const EventOccurrence &first, const EventOccurrence &second)
{
    return first.ref.item.collectionId == second.ref.item.collectionId && first.ref.item.href == second.ref.item.href &&
           first.ref.uid == second.ref.uid && first.start == second.start;
}

bool calendarItemMatchesText(const CalendarItemDisplay &item, const QString &needleLower)
{
    for (const QString &text : item.searchableText)
    {
        if (text.toLower().contains(needleLower))
        {
            return true;
        }
    }
    return false;
}

} // namespace

EventReader::EventReader(const CollectionService &collections, const VdirIo &vdirIo)
    : m_collections(collections)
    , m_calendarItems(std::make_unique<CalendarItemStore>(collections, vdirIo))
    , m_collectionReloadSubscription(collections, [this](const QSet<QString> &changedCollectionIds) {
        clearCacheForCollections(changedCollectionIds);
    })
{
    QObject::connect(
        &collections,
        &CollectionService::itemWritten,
        m_collectionReloadSubscription.context(),
        [this](const ItemRef &ref) { invalidateItem(ref.key()); },
        Qt::DirectConnection);
}

EventReader::~EventReader() = default;

EventFields EventReader::defaultEventEditorData(const QDate &date) const
{
    return CalendarEditorMapper::defaultEvent(date);
}

EventFields EventReader::editorDataForOccurrence(const EventOccurrence &event) const
{
    return CalendarEditorMapper::fromEventOccurrence(event);
}

QList<EventOccurrence>
EventReader::occurrencesForItem(const CalendarItem &object, const QDate &rangeStart, const QDate &rangeEnd) const
{
    const OccurrenceCacheKey cacheKey{object.ref.key(), object.ref.etag, object.uid, rangeStart, rangeEnd};
    {
        QMutexLocker locker(&m_occurrenceCacheMutex);
        const auto cached = m_occurrenceCache.constFind(cacheKey);
        if (cached != m_occurrenceCache.constEnd())
        {
            m_occurrenceCacheLru.removeAll(cacheKey);
            m_occurrenceCacheLru.append(cacheKey);
            return cached.value();
        }
    }

    const KCalendarCore::MemoryCalendar::Ptr calendar = CalendarSnapshot::calendarForItem(object);
    if (calendar.isNull())
    {
        return {};
    }
    const QList<EventOccurrence> occurrences =
        EventOccurrences::occurrenceSnapshots(calendar, object.ref, object.uid, rangeStart, rangeEnd);
    {
        QMutexLocker locker(&m_occurrenceCacheMutex);
        while (m_occurrenceCache.size() >= MaxOccurrenceCacheEntries && !m_occurrenceCacheLru.isEmpty())
        {
            m_occurrenceCache.remove(m_occurrenceCacheLru.takeFirst());
        }
        m_occurrenceCache.insert(cacheKey, occurrences);
        m_occurrenceCacheLru.removeAll(cacheKey);
        m_occurrenceCacheLru.append(cacheKey);
    }
    return occurrences;
}

void EventReader::clearCache() const
{
    QMutexLocker locker(&m_occurrenceCacheMutex);
    m_occurrenceCache.clear();
    m_occurrenceCacheLru.clear();
}

void EventReader::clearCacheForCollections(const QSet<QString> &collectionIds) const
{
    if (collectionIds.isEmpty())
    {
        return;
    }

    QMutexLocker locker(&m_occurrenceCacheMutex);
    for (auto it = m_occurrenceCache.begin(); it != m_occurrenceCache.end();)
    {
        if (collectionIds.contains(it.key().item.collectionId))
        {
            m_occurrenceCacheLru.removeAll(it.key());
            it = m_occurrenceCache.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void EventReader::invalidateItem(const ItemKey &item) const
{
    if (!item.isValid())
    {
        return;
    }

    QMutexLocker locker(&m_occurrenceCacheMutex);
    for (auto it = m_occurrenceCache.begin(); it != m_occurrenceCache.end();)
    {
        if (it.key().item == item)
        {
            m_occurrenceCacheLru.removeAll(it.key());
            it = m_occurrenceCache.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

QList<CalendarItem> EventReader::calendarItemSummaries(QList<ReadFailure> *readFailures,
                                                       const CancellationToken &cancellation) const
{
    QList<CalendarItem> items;
    for (const Collection &collection : m_collections.calendarList())
    {
        if (cancellation.isCancellationRequested())
        {
            break;
        }
        const std::shared_ptr<CalendarItemRepository> repository = m_calendarItems->repositoryForCollection(collection);
        if (!repository->isValid())
        {
            continue;
        }
        repository->forEachObject(
            [&](const CalendarItemRepository::ReadResult &readResult) {
                if (!readResult.isOk())
                {
                    qCWarning(storageLog) << "Skipping calendar item" << collection.id << readResult.object.ref.href
                                          << storageStatusName(readResult.status);
                    if (readFailures)
                    {
                        readFailures->append(readFailureForCalendarItem(collection.id, readResult));
                    }
                    return true;
                }
                items.append(readResult.object);
                return true;
            },
            [&cancellation]() { return cancellation.isCancellationRequested(); });
    }
    return items;
}

EventOccurrence EventReader::eventSeries(const OccurrenceRef &occurrence) const
{
    const auto empty = [&occurrence](const char *reason) {
        qCDebug(storageLog) << "Could not resolve event series" << occurrence.item.href << reason;
        return EventOccurrence{};
    };

    if (!occurrence.item.isValid())
    {
        return empty("invalid item");
    }

    const std::optional<Collection> collection = m_collections.calendarForRead(occurrence.item.collectionId);
    if (!collection)
    {
        return empty("collection not readable");
    }

    const std::shared_ptr<CalendarItemRepository> repository = m_calendarItems->repositoryForCollection(*collection);
    if (!repository->isValid())
    {
        return empty("repository invalid");
    }
    const CalendarItemRepository::ReadResult readResult =
        repository->readCurrentObject(occurrence.item.href, occurrence.item.etag);
    if (!readResult.isOk())
    {
        qCDebug(storageLog) << "Could not resolve event series" << occurrence.item.href << "read failed"
                            << storageStatusName(readResult.status);
        return {};
    }

    const CalendarItem storedObject = readResult.object;
    const KCalendarCore::MemoryCalendar::Ptr calendar = CalendarSnapshot::calendarForItem(storedObject);
    if (calendar.isNull() || !CalendarValidation::isSupportedCalendar(calendar))
    {
        return empty("calendar null or unsupported");
    }

    const QString uid = IncidenceResolver::inferLocator(calendar, occurrence.uid).uid;
    if (uid.isEmpty())
    {
        return empty("uid empty");
    }

    const KCalendarCore::Event::Ptr event = IncidenceResolver::findMasterEvent(calendar, {uid, std::nullopt});
    if (event.isNull())
    {
        return empty("master event missing");
    }

    EventOccurrence series;
    series.ref.item = storedObject.ref;
    series.ref.uid = uid;
    series.start = event->dtStart();
    series.end = event->dtEnd().isValid() ? event->dtEnd() : event->dtStart();
    series.display = CalendarSnapshot::eventDisplayFromEvent(event, {}, series.start, series.end);
    return series;
}

QFuture<EventOccurrenceListResult> EventReader::eventOccurrencesInDateRangeAsync(
    const QDate &rangeStart, const QDate &rangeEnd, const CancellationToken &cancellation) const
{
    if (!rangeStart.isValid() || !rangeEnd.isValid() || rangeEnd < rangeStart)
    {
        return CalendarIoUtils::completedFuture(EventOccurrenceListResult{});
    }

    QList<QFuture<EventOccurrenceListResult>> perCollectionFutures;
    const VdirIo &scheduler = m_calendarItems->vdirIo();
    for (const Collection &collection : m_collections.calendarList())
    {
        if (cancellation.isCancellationRequested())
        {
            break;
        }
        const std::shared_ptr<CalendarItemRepository> repository = m_calendarItems->repositoryForCollection(collection);
        if (!repository->isValid())
        {
            continue;
        }
        const VdirPath path = VdirPath::fromCollection(collection);
        if (!path.isValid())
        {
            continue;
        }
        perCollectionFutures.append(scheduler.submit(path,
                                                     QStringLiteral("event occurrences range"),
                                                     EventOccurrenceListResult{},
                                                     [this, collection, rangeStart, rangeEnd, cancellation] {
                                                         return collectOccurrencesInRangeForCollection(
                                                             collection, rangeStart, rangeEnd, cancellation);
                                                     }));
    }
    if (perCollectionFutures.isEmpty())
    {
        return CalendarIoUtils::completedFuture(EventOccurrenceListResult{});
    }
    return QtFuture::whenAll(perCollectionFutures.begin(), perCollectionFutures.end())
        .then(QtFuture::Launch::Sync, [](const QList<QFuture<EventOccurrenceListResult>> &done) {
            EventOccurrenceListResult aggregate;
            for (const QFuture<EventOccurrenceListResult> &future : done)
            {
                EventOccurrenceListResult perCollection = future.result();
                aggregate.occurrences.append(std::move(perCollection.occurrences));
                aggregate.readFailures.append(std::move(perCollection.readFailures));
            }
            return aggregate;
        });
}

EventOccurrenceListResult
EventReader::collectOccurrencesInRangeForCollection(const Collection &collection,
                                                    const QDate &rangeStart,
                                                    const QDate &rangeEnd,
                                                    const CancellationToken &cancellation) const
{
    EventOccurrenceListResult result;
    const std::shared_ptr<CalendarItemRepository> repository = m_calendarItems->repositoryForCollection(collection);
    if (!repository->isValid())
    {
        return result;
    }
    repository->forEachObject(
        [&](const CalendarItemRepository::ReadResult &readResult) {
            if (!readResult.isOk())
            {
                qCWarning(storageLog) << "Skipping calendar item" << collection.id << readResult.object.ref.href
                                      << storageStatusName(readResult.status);
                result.readFailures.append(readFailureForCalendarItem(collection.id, readResult));
                return true;
            }
            const CalendarItem &object = readResult.object;
            const KCalendarCore::MemoryCalendar::Ptr calendar = CalendarSnapshot::calendarForItem(object);
            if (calendar.isNull() || !CalendarValidation::isSupportedCalendar(calendar))
            {
                qCWarning(storageLog) << "Skipping unsupported calendar item" << collection.id << object.ref.href;
                return true;
            }
            result.occurrences.append(occurrencesForItem(object, rangeStart, rangeEnd));
            return true;
        },
        [&cancellation]() { return cancellation.isCancellationRequested(); });
    return result;
}

std::optional<EventOccurrence>
EventReader::searchOccurrences(const QString &needle,
                               const QDate &anchor,
                               SearchDirection direction,
                               const EventOccurrence &current,
                               const std::function<bool(const QString &collectionId)> &isCollectionVisible,
                               const CancellationToken &cancellation) const
{
    const QString needleLower = needle.toLower();
    if (needleLower.isEmpty() || cancellation.isCancellationRequested())
    {
        return std::nullopt;
    }

    std::vector<CalendarItem> matchingItems;
    for (const CalendarItem &item : calendarItemSummaries(nullptr, cancellation))
    {
        if (cancellation.isCancellationRequested())
        {
            return std::nullopt;
        }
        const CalendarItemDisplay display = CalendarSnapshot::calendarItemDisplay(item);
        const bool visible = !isCollectionVisible || isCollectionVisible(item.ref.collectionId);
        if (visible && calendarItemMatchesText(display, needleLower))
        {
            matchingItems.push_back(item);
        }
    }
    if (matchingItems.empty())
    {
        return std::nullopt;
    }

    auto matchingOccurrences = [this, &matchingItems, &cancellation](const QDate &windowStart, const QDate &windowEnd) {
        QList<EventOccurrence> results;
        for (const CalendarItem &item : matchingItems)
        {
            if (cancellation.isCancellationRequested())
            {
                break;
            }
            results.append(occurrencesForItem(item, windowStart, windowEnd));
        }
        std::sort(results.begin(), results.end(), [](const EventOccurrence &first, const EventOccurrence &second) {
            const QDate firstDate = first.start.date();
            const QDate secondDate = second.start.date();
            if (firstDate != secondDate)
            {
                return firstDate < secondDate;
            }
            return eventStartsBefore(first, second);
        });
        return results;
    };

    const bool forward = direction == SearchDirection::Forward;
    const bool haveCurrent = CalendarSnapshot::hasEventRef(current);
    const QDate startDate = anchor.isValid() ? anchor : QDate::currentDate();

    QList<EventOccurrence> sameDay = matchingOccurrences(startDate, startDate);
    if (cancellation.isCancellationRequested())
    {
        return std::nullopt;
    }
    if (!sameDay.isEmpty())
    {
        int startIndex = forward ? 0 : sameDay.size() - 1;
        for (int i = 0; i < sameDay.size(); ++i)
        {
            if (haveCurrent && sameOccurrence(sameDay[i], current))
            {
                startIndex = forward ? i + 1 : i - 1;
                break;
            }
        }

        const int step = forward ? 1 : -1;
        for (int i = startIndex; i >= 0 && i < sameDay.size(); i += step)
        {
            return sameDay[i];
        }
    }

    const QDate scanBoundary = forward ? startDate.addYears(10) : startDate.addYears(-10);
    QDate cursor = forward ? startDate.addDays(1) : startDate.addDays(-1);
    int span = 1;
    while (forward ? cursor <= scanBoundary : cursor >= scanBoundary)
    {
        if (cancellation.isCancellationRequested())
        {
            return std::nullopt;
        }
        QDate windowStart;
        QDate windowEnd;
        if (forward)
        {
            windowStart = cursor;
            windowEnd = cursor.addDays(span - 1);
            if (windowEnd > scanBoundary)
            {
                windowEnd = scanBoundary;
            }
        }
        else
        {
            windowEnd = cursor;
            windowStart = cursor.addDays(-(span - 1));
            if (windowStart < scanBoundary)
            {
                windowStart = scanBoundary;
            }
        }

        QList<EventOccurrence> windowList = matchingOccurrences(windowStart, windowEnd);
        if (cancellation.isCancellationRequested())
        {
            return std::nullopt;
        }
        if (!forward)
        {
            std::reverse(windowList.begin(), windowList.end());
        }
        if (!windowList.isEmpty())
        {
            return windowList.constFirst();
        }

        if (forward)
        {
            cursor = windowEnd.addDays(1);
        }
        else
        {
            cursor = windowStart.addDays(-1);
        }
        if (span < 4096)
        {
            span *= 2;
        }
    }

    return std::nullopt;
}
