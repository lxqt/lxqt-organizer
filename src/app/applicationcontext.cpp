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

#include "applicationcontext.h"

#include "eventreader.h"
#include "eventservice.h"
#include "collectioncatalog.h"
#include "collectionservice.h"
#include "contactservice.h"
#include "taskservice.h"
#include "vdirioscheduler.h"

#include <atomic>

#include <QFuture>
#include <QHash>
#include <QList>
#include <QMetaObject>
#include <QSet>
#include <QThread>
#include <QtConcurrent/QtConcurrentRun>

namespace {

QHash<QString, Collection> collectionsById(const QList<Collection> &collections)
{
    QHash<QString, Collection> byId;
    for (const Collection &collection : collections)
    {
        if (collection.isValid() && !collection.id.isEmpty())
        {
            byId.insert(collection.id, collection);
        }
    }
    return byId;
}

void addChangedCollectionIds(QSet<QString> *changedIds,
                             const QList<Collection> &oldCollections,
                             const QList<Collection> &newCollections)
{
    const QHash<QString, Collection> oldById = collectionsById(oldCollections);
    const QHash<QString, Collection> newById = collectionsById(newCollections);

    for (auto it = oldById.cbegin(); it != oldById.cend(); ++it)
    {
        const auto current = newById.constFind(it.key());
        if (current == newById.cend() || current.value() != it.value())
        {
            changedIds->insert(it.key());
        }
    }

    for (auto it = newById.cbegin(); it != newById.cend(); ++it)
    {
        const auto previous = oldById.constFind(it.key());
        if (previous == oldById.cend() || previous.value() != it.value())
        {
            changedIds->insert(it.key());
        }
    }
}

QSet<QString> changedCollectionIds(const CollectionCatalog &oldCatalog, const CollectionCatalog &newCatalog)
{
    QSet<QString> changedIds;
    addChangedCollectionIds(&changedIds, oldCatalog.calendarList(), newCatalog.calendarList());
    addChangedCollectionIds(&changedIds, oldCatalog.addressBookList(), newCatalog.addressBookList());
    return changedIds;
}

} // namespace

class ApplicationContext::Impl
{
public:
    Impl();

    std::shared_ptr<const CollectionCatalog> loadCatalog() const;
    void storeCatalog(std::shared_ptr<const CollectionCatalog> catalog);
    void commitCatalogReload(std::shared_ptr<const CollectionCatalog> newCatalog);
    void notifyCollectionsReloaded(const QSet<QString> &changedIds);
    bool reloadCollections();

    std::shared_ptr<const CollectionCatalog> catalog;
    // Declaration order is load-bearing: repositories/services hold scheduler
    // references, so the scheduler must be constructed first and destroyed last.
    VdirIoScheduler vdirIoScheduler;
    CollectionService collectionService;
    EventReader eventReader;
    EventService eventService;
    TaskService taskService;
    ContactService contactService;
};

ApplicationContext::Impl::Impl()
    : catalog(std::make_shared<CollectionCatalog>())
    , collectionService([this]() { return loadCatalog(); })
    , eventReader(collectionService, vdirIoScheduler)
    , eventService(collectionService, vdirIoScheduler)
    , taskService(collectionService, vdirIoScheduler)
    , contactService(collectionService, vdirIoScheduler)
{}

std::shared_ptr<const CollectionCatalog> ApplicationContext::Impl::loadCatalog() const
{
    return std::atomic_load(&catalog);
}

void ApplicationContext::Impl::storeCatalog(std::shared_ptr<const CollectionCatalog> catalog)
{
    std::atomic_store(&this->catalog, std::move(catalog));
}

void ApplicationContext::Impl::notifyCollectionsReloaded(const QSet<QString> &changedIds)
{
    collectionService.notifyReloaded(changedIds);
}

// @thread GUI
void ApplicationContext::Impl::commitCatalogReload(std::shared_ptr<const CollectionCatalog> newCatalog)
{
    Q_ASSERT_X(QThread::currentThread() == collectionService.thread(),
               "ApplicationContext::Impl::commitCatalogReload",
               "must mutate CollectionCatalog on the CollectionService thread");
    const std::shared_ptr<const CollectionCatalog> oldCatalog = loadCatalog();
    const QSet<QString> changedIds = changedCollectionIds(oldCatalog ? *oldCatalog : CollectionCatalog(), *newCatalog);
    storeCatalog(std::move(newCatalog));
    notifyCollectionsReloaded(changedIds);
}

bool ApplicationContext::Impl::reloadCollections()
{
    CollectionCatalog catalog;
    if (!CollectionCatalog::load(&catalog))
    {
        return false;
    }

    auto newCatalog = std::make_shared<const CollectionCatalog>(std::move(catalog));
    if (QThread::currentThread() == collectionService.thread())
    {
        commitCatalogReload(std::move(newCatalog));
        return true;
    }

    bool committed = false;
    QMetaObject::invokeMethod(
        &collectionService,
        [this, newCatalog = std::move(newCatalog), &committed]() mutable {
            commitCatalogReload(std::move(newCatalog));
            committed = true;
        },
        Qt::BlockingQueuedConnection);
    return committed;
}

ApplicationContext::ApplicationContext()
    : m_impl(std::make_unique<Impl>())
{}

ApplicationContext::~ApplicationContext() = default;

bool ApplicationContext::loadCollections()
{
    CollectionCatalog catalog;
    const bool loaded = CollectionCatalog::load(&catalog);
    if (loaded)
    {
        auto newCatalog = std::make_shared<const CollectionCatalog>(std::move(catalog));
        m_impl->commitCatalogReload(std::move(newCatalog));
    }
    return loaded;
}

QFuture<bool> ApplicationContext::reloadCollections()
{
    return QtConcurrent::run([this] { return m_impl->reloadCollections(); });
}

std::shared_ptr<const CollectionCatalog> ApplicationContext::catalog() const
{
    return catalogSnapshot();
}

std::shared_ptr<const CollectionCatalog> ApplicationContext::catalogSnapshot() const
{
    return m_impl->loadCatalog();
}

CollectionService &ApplicationContext::collectionService()
{
    return m_impl->collectionService;
}

EventReader &ApplicationContext::eventReader()
{
    return m_impl->eventReader;
}

EventService &ApplicationContext::eventService()
{
    return m_impl->eventService;
}

TaskService &ApplicationContext::taskService()
{
    return m_impl->taskService;
}

ContactService &ApplicationContext::contactService()
{
    return m_impl->contactService;
}
