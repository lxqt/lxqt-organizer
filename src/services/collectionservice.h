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

#ifndef COLLECTIONSERVICE_H
#define COLLECTIONSERVICE_H

#include "collectionresolver.h"
#include "collectiontypes.h"
#include "itemidentity.h"
#include "storageresult.h"

#include <QList>
#include <QObject>
#include <QSet>
#include <QString>

#include <functional>
#include <memory>
#include <optional>

class CollectionCatalog;

// @thread GUI-affine QObject; notifications are emitted on the GUI thread.
// Read accessors are any-thread because catalog access is snapshot-based.
class CollectionService : public QObject, public CollectionResolver
{
    Q_OBJECT

public:
    using CatalogAccessor = std::function<std::shared_ptr<const CollectionCatalog>()>;

    // @thread GUI
    explicit CollectionService(const std::shared_ptr<const CollectionCatalog> &catalog, QObject *parent = nullptr);
    // @thread GUI
    explicit CollectionService(CatalogAccessor catalogAccessor, QObject *parent = nullptr);

    std::shared_ptr<const CollectionCatalog> catalogSnapshot() const;
    QList<Collection> calendarList() const;
    QList<Collection> addressBookList() const;
    std::optional<Collection> defaultCalendar() const;
    std::optional<Collection> defaultAddressBook() const;
    std::optional<Collection> calendarForRead(const QString &collectionId) const override;
    std::optional<Collection> addressBookForRead(const QString &collectionId) const override;
    std::optional<Collection> calendarForWrite(const QString &collectionId) const override;
    std::optional<Collection> addressBookForWrite(const QString &collectionId) const override;
    StorageStatus calendarWriteFailureStatus(const QString &collectionId) const override;
    StorageStatus addressBookWriteFailureStatus(const QString &collectionId) const override;

    bool isCalendarWritable(const QString &collectionId) const;
    bool isAddressBookWritable(const QString &collectionId) const;

    // @thread any-thread; marshalled to the service thread
    void notifyReloaded(const QSet<QString> &changedCollectionIds);
    // @thread any-thread; marshalled to the service thread
    void notifyItemWritten(const ItemRef &ref) const;

Q_SIGNALS:
    void collectionsReloaded(const QSet<QString> &changedCollectionIds);
    void itemWritten(const ItemRef &ref) const;

private:
    CatalogAccessor m_catalogAccessor;
};

#endif // COLLECTIONSERVICE_H
