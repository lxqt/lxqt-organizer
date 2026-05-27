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

#include "collectioncatalog.h"

#include "collectionutils.h"
#include "storagelog.h"
#include "vdir.h"

#include <XdgDirs>

#include <QDebug>
#include <QDir>
#include <QFileInfo>

namespace {

constexpr auto kPersonalId = "personal";

QString organizerDataHome()
{
    const QString xdgDataHome = XdgDirs::dataHome();
    return xdgDataHome.isEmpty() ? QString() : QDir(xdgDataHome).filePath(QStringLiteral("lxqt/lxqt-organizer"));
}

QString collectionPath(CollectionKind kind)
{
    const QString subdirectory =
        kind == CollectionKind::Calendar ? QStringLiteral("calendars/personal") : QStringLiteral("contacts/personal");
    const QString dataHome = organizerDataHome();
    return dataHome.isEmpty() ? QString() : QDir(dataHome).filePath(subdirectory);
}

CollectionAccess accessForPath(const QString &path)
{
    const QFileInfo info(path);
    return info.exists() && info.isDir() && info.isWritable() ? CollectionAccess::ReadWrite
                                                              : CollectionAccess::ReadOnly;
}

Collection personalCollection(CollectionKind kind, const QString &path)
{
    Collection collection;
    collection.id = QString::fromLatin1(kPersonalId);
    collection.kind = kind;
    collection.path = path;
    collection.canonicalPath = CollectionUtils::pathIdentity(path);
    collection.displayName = QStringLiteral("Personal");
    collection.color = CollectionUtils::defaultColor(kind);
    collection.access = accessForPath(collection.canonicalPath);
    if (!collection.isWritable())
    {
        collection.status = QStringLiteral("Personal vdir is not writable.");
    }
    return collection;
}

} // namespace

bool CollectionCatalog::load(CollectionCatalog *catalog)
{
    if (!catalog)
    {
        return false;
    }

    CollectionCatalog loaded;
    const QString calendarPath = collectionPath(CollectionKind::Calendar);
    const QString addressBookPath = collectionPath(CollectionKind::AddressBook);
    if (calendarPath.isEmpty() || addressBookPath.isEmpty())
    {
        qCWarning(storageLog) << "Could not resolve collection paths" << calendarPath << addressBookPath;
        *catalog = loaded;
        return false;
    }

    Vdir calendarVdir(calendarPath, CollectionUtils::extensionForKind(CollectionKind::Calendar));
    Vdir addressBookVdir(addressBookPath, CollectionUtils::extensionForKind(CollectionKind::AddressBook));
    const StorageStatus calendarStatus = calendarVdir.ensure();
    const StorageStatus addressBookStatus = addressBookVdir.ensure();
    if (calendarStatus != StorageStatus::Ok || addressBookStatus != StorageStatus::Ok)
    {
        qCWarning(storageLog) << "Could not ensure personal vdir" << calendarPath << storageStatusName(calendarStatus)
                              << addressBookPath << storageStatusName(addressBookStatus);
        *catalog = loaded;
        return false;
    }

    const QString canonicalCalendarPath = CollectionUtils::pathIdentity(calendarPath);
    const QString canonicalAddressBookPath = CollectionUtils::pathIdentity(addressBookPath);
    if (canonicalCalendarPath.isEmpty() || canonicalAddressBookPath.isEmpty())
    {
        qCWarning(storageLog) << "Could not canonicalize collection paths" << calendarPath << addressBookPath;
        *catalog = loaded;
        return false;
    }

    loaded.m_calendar = personalCollection(CollectionKind::Calendar, canonicalCalendarPath);
    loaded.m_addressBook = personalCollection(CollectionKind::AddressBook, canonicalAddressBookPath);

    *catalog = loaded;
    return true;
}

QList<Collection> CollectionCatalog::calendarList() const
{
    return m_calendar.isValid() ? QList<Collection>{m_calendar} : QList<Collection>{};
}

QList<Collection> CollectionCatalog::addressBookList() const
{
    return m_addressBook.isValid() ? QList<Collection>{m_addressBook} : QList<Collection>{};
}

std::optional<Collection> CollectionCatalog::calendar(const QString &id) const
{
    return m_calendar.isValid() && m_calendar.id == id ? std::optional<Collection>(m_calendar) : std::nullopt;
}

std::optional<Collection> CollectionCatalog::addressBook(const QString &id) const
{
    return m_addressBook.isValid() && m_addressBook.id == id ? std::optional<Collection>(m_addressBook) : std::nullopt;
}

std::optional<Collection> CollectionCatalog::defaultCalendar() const
{
    return defaultCollection(CollectionKind::Calendar);
}

std::optional<Collection> CollectionCatalog::defaultAddressBook() const
{
    return defaultCollection(CollectionKind::AddressBook);
}

std::optional<Collection> CollectionCatalog::defaultCollection(CollectionKind kind) const
{
    const Collection &collection = kind == CollectionKind::Calendar ? m_calendar : m_addressBook;
    return collection.isWritable() ? std::optional<Collection>(collection) : std::nullopt;
}

CollectionSummary
summarizeCollection(const CollectionCatalog &catalog, CollectionKind kind, const QString &collectionId)
{
    const QList<Collection> collections =
        kind == CollectionKind::Calendar ? catalog.calendarList() : catalog.addressBookList();
    for (const Collection &candidate : collections)
    {
        if (candidate.id == collectionId)
        {
            return CollectionSummary{candidate.displayName, candidate.color};
        }
    }
    return CollectionSummary{collectionId, {}};
}
