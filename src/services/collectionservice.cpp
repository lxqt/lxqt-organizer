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

#include "collectionservice.h"

#include "collectioncatalog.h"

#include <memory>

#include <QCoreApplication>
#include <QMetaObject>
#include <QThread>

CollectionService::CollectionService(const std::shared_ptr<const CollectionCatalog> &catalog, QObject *parent)
    : CollectionService([catalog]() { return catalog; }, parent)
{}

// @thread GUI
CollectionService::CollectionService(CatalogAccessor catalogAccessor, QObject *parent)
    : QObject(parent)
    , m_catalogAccessor(std::move(catalogAccessor))
{
    Q_ASSERT_X(qApp != nullptr, "CollectionService::CollectionService", "requires a QCoreApplication instance");
    if (qApp)
    {
        Q_ASSERT_X(QThread::currentThread() == qApp->thread(),
                   "CollectionService::CollectionService",
                   "must be constructed on the GUI thread");
        moveToThread(qApp->thread());
    }
}

std::shared_ptr<const CollectionCatalog> CollectionService::catalogSnapshot() const
{
    return m_catalogAccessor ? m_catalogAccessor() : std::make_shared<const CollectionCatalog>();
}

QList<Collection> CollectionService::calendarList() const
{
    const auto catalog = catalogSnapshot();
    return catalog->calendarList();
}

QList<Collection> CollectionService::addressBookList() const
{
    const auto catalog = catalogSnapshot();
    return catalog->addressBookList();
}

std::optional<Collection> CollectionService::defaultCalendar() const
{
    const auto catalog = catalogSnapshot();
    return catalog->defaultCalendar();
}

std::optional<Collection> CollectionService::defaultAddressBook() const
{
    const auto catalog = catalogSnapshot();
    return catalog->defaultAddressBook();
}

std::optional<Collection> CollectionService::calendarForRead(const QString &collectionId) const
{
    const auto catalog = catalogSnapshot();
    return catalog->calendar(collectionId);
}

std::optional<Collection> CollectionService::addressBookForRead(const QString &collectionId) const
{
    const auto catalog = catalogSnapshot();
    return catalog->addressBook(collectionId);
}

std::optional<Collection> CollectionService::calendarForWrite(const QString &collectionId) const
{
    const auto currentCatalog = catalogSnapshot();
    const std::optional<Collection> collection =
        collectionId.isEmpty() ? currentCatalog->defaultCalendar() : currentCatalog->calendar(collectionId);
    return collection && collection->isWritable() ? collection : std::nullopt;
}

std::optional<Collection> CollectionService::addressBookForWrite(const QString &collectionId) const
{
    const auto currentCatalog = catalogSnapshot();
    const std::optional<Collection> collection =
        collectionId.isEmpty() ? currentCatalog->defaultAddressBook() : currentCatalog->addressBook(collectionId);
    return collection && collection->isWritable() ? collection : std::nullopt;
}

StorageStatus CollectionService::calendarWriteFailureStatus(const QString &collectionId) const
{
    const auto currentCatalog = catalogSnapshot();
    const std::optional<Collection> configuredCollection =
        collectionId.isEmpty() ? currentCatalog->defaultCalendar() : currentCatalog->calendar(collectionId);
    return configuredCollection ? StorageStatus::ReadOnly : StorageStatus::NotFound;
}

StorageStatus CollectionService::addressBookWriteFailureStatus(const QString &collectionId) const
{
    const auto currentCatalog = catalogSnapshot();
    const std::optional<Collection> configuredCollection =
        collectionId.isEmpty() ? currentCatalog->defaultAddressBook() : currentCatalog->addressBook(collectionId);
    return configuredCollection ? StorageStatus::ReadOnly : StorageStatus::NotFound;
}

bool CollectionService::isCalendarWritable(const QString &collectionId) const
{
    return calendarForWrite(collectionId).has_value();
}

bool CollectionService::isAddressBookWritable(const QString &collectionId) const
{
    return addressBookForWrite(collectionId).has_value();
}

// @thread any-thread; marshalled to the service thread
void CollectionService::notifyReloaded(const QSet<QString> &changedCollectionIds)
{
    if (QThread::currentThread() == this->thread())
    {
        Q_EMIT collectionsReloaded(changedCollectionIds);
        return;
    }

    QMetaObject::invokeMethod(
        this,
        [this, changedCollectionIds]() { Q_EMIT collectionsReloaded(changedCollectionIds); },
        Qt::QueuedConnection);
}

// @thread any-thread; marshalled to the service thread
void CollectionService::notifyItemWritten(const ItemRef &ref) const
{
    if (QThread::currentThread() == this->thread())
    {
        Q_EMIT itemWritten(ref);
        return;
    }

    QMetaObject::invokeMethod(
        const_cast<CollectionService *>(this), [this, ref]() { Q_EMIT itemWritten(ref); }, Qt::QueuedConnection);
}
