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

#include "vdiritemrepository.h"

#include "collectionutils.h"
#include "vdir.h"

#include <QFileInfo>

#include <atomic>
#include <utility>

VdirItemRepository::VdirItemRepository(const StorageCollectionRef &collection, const VdirIo &vdirIo)
    : m_collection(std::make_shared<const StorageCollectionRef>(collection))
    , m_vdirIo(vdirIo)
{
    if (!collection.canonicalPath.isEmpty())
    {
        m_vdir = std::make_unique<Vdir>(collection.canonicalPath, CollectionUtils::extensionForKind(collection.kind));
    }
}

VdirItemRepository::~VdirItemRepository() = default;

StorageCollectionRef VdirItemRepository::collection() const
{
    return *std::atomic_load_explicit(&m_collection, std::memory_order_acquire);
}

void VdirItemRepository::updateCollection(const StorageCollectionRef &collection)
{
    auto next = std::make_shared<const StorageCollectionRef>(collection);
    std::atomic_store_explicit(&m_collection, std::move(next), std::memory_order_release);
}

bool VdirItemRepository::isValid() const
{
    return m_vdir != nullptr;
}

StorageStatus VdirItemRepository::writeMetadata(const QString &name, const QString &value) const
{
    return runOnWorker(QStringLiteral("metadata write"), StorageStatus::IoError, [this, name, value] {
        return isValid() ? m_vdir->writeMetadata(name, value) : StorageStatus::IoError;
    });
}

QList<VdirItemRepository::ItemState> VdirItemRepository::itemStates() const
{
    return runOnWorker(QStringLiteral("item states"), QList<ItemState>(), [this] {
        QList<ItemState> result;
        if (!isValid())
        {
            return result;
        }
        for (const Vdir::ItemState &state : m_vdir->itemStates())
        {
            result.append(ItemState{state.href, state.etag});
        }
        return result;
    });
}

StorageStatus
VdirItemRepository::checkCurrentItem(const QString &href, const QString &expectedEtag, ItemRef *storage) const
{
    const ReadResult<ItemPayload> result = readCurrentItem(href, expectedEtag);
    if (storage)
    {
        *storage = result.object.ref;
    }
    return result.status;
}

ReadResult<VdirItemRepository::ItemPayload> VdirItemRepository::readCurrentItem(const QString &href,
                                                                                const QString &expectedEtag) const
{
    const StorageCollectionRef current = collection();
    ReadResult<ItemPayload> result;
    result.object.ref = ItemRef{current.id, href, expectedEtag};

    if (href.isEmpty() || expectedEtag.isEmpty())
    {
        result.status = StorageStatus::Unsupported;
        return result;
    }

    if (!isValid())
    {
        result.status = StorageStatus::IoError;
        return result;
    }

    const ReadResult<Vdir::ReadItem> currentItem = runOnWorker(QStringLiteral("item read"),
                                                               ReadResult<Vdir::ReadItem>{{}, StorageStatus::IoError},
                                                               [this, href] { return m_vdir->readItemWithEtag(href); });
    if (!currentItem.isOk())
    {
        result.status = currentItem.status;
        return result;
    }

    result.object.content = currentItem.object.content;
    result.object.etag = currentItem.object.etag;
    if (!result.object.isValid())
    {
        result.status = StorageStatus::IoError;
        return result;
    }
    if (result.object.etag != expectedEtag)
    {
        result.status = StorageStatus::Conflict;
        return result;
    }

    result.object.ref = ItemRef{current.id, href, result.object.etag};
    result.status = StorageStatus::Ok;
    return result;
}

StorageStatus VdirItemRepository::remove(const ItemRef &storage) const
{
    if (storage.href.isEmpty())
    {
        return StorageStatus::Unsupported;
    }
    if (storage.etag.isEmpty())
    {
        return StorageStatus::Conflict;
    }
    return remove(storage.href, storage.etag);
}

QString VdirItemRepository::hrefForUid(const QString &uid) const
{
    return isValid() ? m_vdir->hrefForUid(uid) : QString();
}

StorageStatus VdirItemRepository::create(const QString &href, const QString &text, QString *etag) const
{
    return runOnWorker(QStringLiteral("item create"), StorageStatus::IoError, [this, href, text, etag] {
        return isValid() ? m_vdir->createItem(href, text, etag) : StorageStatus::IoError;
    });
}

StorageStatus VdirItemRepository::replace(const QString &href,
                                          const QString &text,
                                          const QString &expectedEtag,
                                          QString *newEtag) const
{
    if (expectedEtag.isEmpty())
    {
        return StorageStatus::Conflict;
    }
    return runOnWorker(
        QStringLiteral("item update"), StorageStatus::IoError, [this, href, text, expectedEtag, newEtag] {
            return isValid() ? m_vdir->updateItem(href, text, expectedEtag, newEtag) : StorageStatus::IoError;
        });
}

StorageStatus VdirItemRepository::remove(const QString &href, const QString &expectedEtag) const
{
    if (expectedEtag.isEmpty())
    {
        return StorageStatus::Conflict;
    }
    return runOnWorker(QStringLiteral("item remove"), StorageStatus::IoError, [this, href, expectedEtag] {
        return isValid() ? m_vdir->removeItem(href, expectedEtag) : StorageStatus::IoError;
    });
}
