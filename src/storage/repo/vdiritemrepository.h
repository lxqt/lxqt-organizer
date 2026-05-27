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

#ifndef VDIRITEMREPOSITORY_H
#define VDIRITEMREPOSITORY_H

#include "itemidentity.h"
#include "storagecollectionref.h"
#include "storageresult.h"
#include "vdirio.h"

#include <QList>
#include <QString>

#include <memory>
#include <type_traits>
#include <utility>

class Vdir;
class ContactRepository;
class CalendarItemRepository;

// @thread vdir-worker; public methods must be entered through VdirIo.
// Intentionally outlives catalog reload through ItemStore shared_ptr caches;
// do not capture &collection outside this object.
class VdirItemRepository
{
public:
    struct ItemState
    {
        QString href;
        QString etag;
    };

    explicit VdirItemRepository(const StorageCollectionRef &collection, const VdirIo &vdirIo);
    ~VdirItemRepository();

    StorageCollectionRef collection() const;
    void updateCollection(const StorageCollectionRef &collection);
    bool isValid() const;
    StorageStatus writeMetadata(const QString &name, const QString &value) const;
    QList<ItemState> itemStates() const;
    StorageStatus checkCurrentItem(const QString &href, const QString &expectedEtag, ItemRef *storage = nullptr) const;
    StorageStatus remove(const ItemRef &storage) const;

    QString hrefForUid(const QString &uid) const;
    StorageStatus create(const QString &href, const QString &text, QString *etag = nullptr) const;
    StorageStatus
    replace(const QString &href, const QString &text, const QString &expectedEtag, QString *newEtag = nullptr) const;

    // Run `callable` on this collection's vdir worker, ordered FIFO against
    // any other queued reads/writes for the same canonical path. If already on
    // the worker, runs inline. Use this to wrap bulk read iterations so they
    // hold the FIFO slot for the full traversal and cannot be torn by an
    // interleaving write.
    template <typename Callable>
    auto runOnWorker(const QString &label,
                     std::invoke_result_t<std::decay_t<Callable>> failureValue,
                     Callable &&callable) const -> std::invoke_result_t<std::decay_t<Callable>>;

    template <typename Callable>
    auto runOnWorker(const QString &label, Callable &&callable) const -> std::invoke_result_t<std::decay_t<Callable>>;

private:
    friend class ContactRepository;
    friend class CalendarItemRepository;

    struct ItemPayload
    {
        QString content;
        QString etag;
        ItemRef ref;

        bool isValid() const { return !content.isEmpty() && !etag.isEmpty(); }
    };

    ReadResult<ItemPayload> readCurrentItem(const QString &href, const QString &expectedEtag) const;
    StorageStatus remove(const QString &href, const QString &expectedEtag) const;

    std::shared_ptr<const StorageCollectionRef> m_collection;
    std::unique_ptr<Vdir> m_vdir;
    const VdirIo &m_vdirIo;
};

template <typename Callable>
auto VdirItemRepository::runOnWorker(const QString &label,
                                     std::invoke_result_t<std::decay_t<Callable>> failureValue,
                                     Callable &&callable) const -> std::invoke_result_t<std::decay_t<Callable>>
{
    const VdirPath path = VdirPath::fromCanonicalPath(collection().canonicalPath);
    const VdirIo &scheduler = m_vdirIo;
    return scheduler.runSync(
        path,
        label,
        std::move(failureValue),
        [&scheduler, path, callable = std::decay_t<Callable>(std::forward<Callable>(callable))]() mutable {
            Q_ASSERT_X(scheduler.isWorkerThreadForPath(path),
                       "VdirItemRepository::runOnWorker",
                       "vdir filesystem operation must run on its path worker");
            return callable();
        });
}

template <typename Callable>
auto VdirItemRepository::runOnWorker(const QString &label, Callable &&callable) const
    -> std::invoke_result_t<std::decay_t<Callable>>
{
    static_assert(std::is_void_v<std::invoke_result_t<std::decay_t<Callable>>>,
                  "non-void runOnWorker calls must provide an explicit failure value");
    const VdirPath path = VdirPath::fromCanonicalPath(collection().canonicalPath);
    const VdirIo &scheduler = m_vdirIo;
    scheduler.runSync(
        path, label, [&scheduler, path, callable = std::decay_t<Callable>(std::forward<Callable>(callable))]() mutable {
            Q_ASSERT_X(scheduler.isWorkerThreadForPath(path),
                       "VdirItemRepository::runOnWorker",
                       "vdir filesystem operation must run on its path worker");
            callable();
        });
}

#endif // VDIRITEMREPOSITORY_H
