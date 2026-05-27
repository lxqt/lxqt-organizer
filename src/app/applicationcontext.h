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

#ifndef APPLICATIONCONTEXT_H
#define APPLICATIONCONTEXT_H

#include <QFuture>

#include <memory>

class EventReader;
class EventService;
class CollectionCatalog;
class CollectionService;
class ContactService;
class TaskService;

// @thread GUI; process/window-lifetime owner of CollectionCatalog snapshots,
// VdirIoScheduler, and all services/readers. MainWindow and panes receive
// references or non-owning pointers to these objects; they must not delete or
// retain them beyond the context lifetime. Services returned by reference are
// mutex-protected and may be called from any thread. Their signals emit from
// the GUI thread; connect freely. Do not store the reference past the
// ApplicationContext lifetime.
class ApplicationContext
{
public:
    ApplicationContext();
    ~ApplicationContext();
    ApplicationContext(const ApplicationContext &) = delete;
    ApplicationContext &operator=(const ApplicationContext &) = delete;
    ApplicationContext(ApplicationContext &&) = delete;
    ApplicationContext &operator=(ApplicationContext &&) = delete;

    bool loadCollections();
    QFuture<bool> reloadCollections();

    std::shared_ptr<const CollectionCatalog> catalog() const;
    std::shared_ptr<const CollectionCatalog> catalogSnapshot() const;
    CollectionService &collectionService();
    EventReader &eventReader();
    EventService &eventService();
    TaskService &taskService();
    ContactService &contactService();

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

#endif // APPLICATIONCONTEXT_H
