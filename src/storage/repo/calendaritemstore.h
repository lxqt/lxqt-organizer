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

#ifndef CALENDARITEMSTORE_H
#define CALENDARITEMSTORE_H

#include "calendaritem.h"
#include "calendaritemrepository.h"
#include "calendarsnapshot.h"
#include "collectionresolver.h"
#include "incidenceresolver.h"
#include "itemstore.h"
#include "storageresult.h"

#include <QSet>

#include <memory>
#include <optional>

class VdirIo;

template <> struct ItemTraits<CalendarItem>
{
    static bool hasPayload(const CalendarItem &object) { return CalendarSnapshot::hasCalendarItemPayload(object); }

    static QString resolveUid(const CalendarItem &stored, const QString &fallback)
    {
        const KCalendarCore::MemoryCalendar::Ptr storedCalendar = CalendarSnapshot::calendarForItem(stored);
        if (storedCalendar.isNull())
        {
            return {};
        }
        return IncidenceResolver::inferLocator(storedCalendar, fallback).uid;
    }

    static StorageStatus missingUidStatus(const CalendarItem &stored)
    {
        return CalendarSnapshot::calendarForItem(stored).isNull() ? StorageStatus::Unsupported
                                                                  : StorageStatus::NotFound;
    }

    static const char *logLabel() { return "calendar item"; }
};

// @thread any-thread-with-mutex; cache state is mutex-protected by ItemStore.
class CalendarItemStore
{
public:
    using PrecheckedWrite = ItemStore<CalendarItemRepository, CollectionKind::Calendar>::PrecheckedWrite;
    using WritableCalendarItem = WritableItem<CalendarItem>;

    explicit CalendarItemStore(const CollectionResolver &collections, const VdirIo &vdirIo);

    std::optional<Collection> collectionForRead(const QString &collectionId) const;
    std::optional<Collection> collectionForWrite(const QString &collectionId) const;
    std::optional<Collection> calendarForRead(const QString &collectionId) const;
    std::optional<Collection> calendarForWrite(const QString &collectionId) const;
    std::shared_ptr<CalendarItemRepository> repositoryForCollection(const Collection &collection) const;
    std::shared_ptr<CalendarItemRepository> repositoryForCollection(const StorageCollectionRef &collection) const;
    bool isValidForWrite(const Collection &collection) const;
    const VdirIo &vdirIo() const;
    StorageStatus writeFailureStatus(const QString &collectionId) const;
    StorageStatus calendarWriteFailureStatus(const QString &collectionId) const;

    PrecheckedWrite precheckWrite(const QString &collectionId, const QString &href, const QString &etag) const;
    CalendarItemRepository::ReadResult readPrechecked(const PrecheckedWrite &checked) const;
    WritableCalendarItem writableItemForUpdate(const QString &collectionId,
                                               const QString &href,
                                               const QString &etag,
                                               const QString &fallbackUid) const;

    StorageResult<CalendarItem> addObject(CalendarItem object) const;
    StorageResult<CalendarItem> replaceObject(CalendarItem object) const;
    StorageResult<CalendarItem> replaceWritableItem(WritableCalendarItem item) const;
    StorageStatus commitRemove(const PrecheckedWrite &checked) const;
    StorageStatus removeWritableItem(const WritableCalendarItem &item) const;
    StorageStatus rollbackInsertedItem(const std::optional<Collection> &collection, const ItemRef &inserted) const;
    MoveOutcome commitCrossCollectionMove(const PrecheckedWrite &source,
                                          const std::optional<Collection> &destinationCollection,
                                          const ItemRef &inserted) const;
    MoveOutcome commitCrossCollectionMove(const WritableCalendarItem &source,
                                          const std::optional<Collection> &destinationCollection,
                                          const ItemRef &inserted) const;
    void clearCache() const;
    void clearCacheForCollections(const QSet<QString> &collectionIds) const;
    void retainRepositoriesForCollections(const QList<Collection> &collections) const;

private:
    ItemStore<CalendarItemRepository, CollectionKind::Calendar> m_items;
};

#endif // CALENDARITEMSTORE_H
