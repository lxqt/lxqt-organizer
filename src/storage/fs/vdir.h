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

#ifndef VDIR_H
#define VDIR_H

#include "storageresult.h"

#include <QHash>
#include <QList>
#include <QMutex>
#include <QString>

class QFileInfo;

// @thread Thread-compatible, with normal application access serialized by the
// path's VdirIoScheduler worker. Standalone callers may use Vdir without a
// scheduler, but must not issue concurrent filesystem operations on the same
// instance. The leaf cache mutex protects mutable const-method cache and
// targeted invalidation; they do not serialize complete vdir operations.
//
// Logical const: const methods may mutate the filesystem and the leaf etag
// cache. Const here means "the Vdir's identity (path, extension) does not
// change." Use a const Vdir & to pass to layers that should not reseat the
// Vdir; do not assume const means read-only.
class Vdir
{
public:
    struct ItemState
    {
        QString href;
        QString etag;
    };

    struct ReadItem
    {
        QString content;
        QString etag;
        bool isValid() const { return !etag.isEmpty(); }
    };

    Vdir(const QString &path, const QString &extension);

    StorageStatus ensure();
    StorageStatus ensureDirectory();
    QString path() const;
    [[nodiscard]] StorageStatus writeMetadata(const QString &name, const QString &value) const;
    QList<ItemState> itemStates() const;
    QString hrefForUid(const QString &uid) const;
    [[nodiscard]] StorageStatus createItem(const QString &href, const QString &text, QString *etag = nullptr) const;
    // Replaces with a synced temp file after guarded etag conflict detection.
    // On Linux, renameat2(RENAME_EXCHANGE) is used when available so the final
    // target is verified against the guard after the atomic exchange and rolled
    // back on conflict. Other platforms fall back to rename(2), where the etag
    // check is best-effort because POSIX has no portable compare-and-swap
    // rename primitive.
    [[nodiscard]] StorageStatus
    updateItem(const QString &href, const QString &text, const QString &etag, QString *newEtag = nullptr) const;
    ReadResult<ReadItem> readItemWithEtag(const QString &href) const;
    [[nodiscard]] StorageStatus removeItem(const QString &href, const QString &etag) const;

    static bool isSafeHref(const QString &href, const QString &extension = QString());
    static bool isSafeMetadataName(const QString &name);

private:
    struct CachedEtag
    {
        qint64 size = -1;
        qint64 lastModifiedMsecs = -1;
        QString etag;
    };
    struct EtagReadResult
    {
        QString etag;
        StorageStatus status = StorageStatus::IoError;

        bool isOk() const { return status == StorageStatus::Ok && !etag.isEmpty(); }
    };

    QString pathForHref(const QString &href) const;
    bool hasCaseInsensitiveHrefCollision(const QString &href) const;
    template <typename Callable> StorageStatus mutateItem(const QString &href, Callable &&callable) const;
    QString etagForHref(const QString &href) const;
    EtagReadResult etagForHrefResult(const QString &href) const;
    QString etagForFile(const QString &href, const QFileInfo &file) const;
    EtagReadResult etagForFileResult(const QString &href, const QFileInfo &file) const;
    void rememberEtag(const QString &href, const QFileInfo &file, const QString &etag) const;
    void rememberEtag(const QString &href, qint64 size, qint64 lastModifiedMsecs, const QString &etag) const;
    void rememberEtagLocked(const QString &href, const QFileInfo &file, const QString &etag) const;
    void rememberEtagLocked(const QString &href, qint64 size, qint64 lastModifiedMsecs, const QString &etag) const;
    void forgetEtag(const QString &href) const;
    void forgetEtagLocked(const QString &href) const;
    void forgetItemCaches(const QString &href) const;

    QString m_path;
    QString m_extension;
    // Leaf cache mutex: must not call out while held. Const read/write methods
    // update the etag cache so repositories can be reused across worker calls.
    mutable QHash<QString, CachedEtag> m_etagCache;
    mutable QMutex m_etagCacheMutex;
};

template <typename Callable> StorageStatus Vdir::mutateItem(const QString &href, Callable &&callable) const
{
    const QString path = pathForHref(href);
    if (path.isEmpty())
    {
        return StorageStatus::InvalidHref;
    }

    const StorageStatus status = callable(path);
    // Every item writer reaches this helper, so item-state and etag caches are
    // invalidated after successful writes and after failed cleanup paths.
    forgetItemCaches(href);
    return status;
}

#endif // VDIR_H
