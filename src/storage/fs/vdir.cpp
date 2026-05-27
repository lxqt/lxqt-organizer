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

#include "vdir.h"

#include "atomicfile.h"
#include "itemidentity.h"
#include "storagelog.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>
#include <QStringList>

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

namespace {

bool isUidSafe(const QString &uid)
{
    static const QString safeChars =
        QStringLiteral("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-+@");

    if (uid.isEmpty())
    {
        return false;
    }

    for (const QChar ch : uid)
    {
        if (!safeChars.contains(ch))
        {
            return false;
        }
    }
    return true;
}

QString safeHrefStem(const QString &uid)
{
    if (isUidSafe(uid))
    {
        return uid;
    }

    const QByteArray digest = QCryptographicHash::hash(uid.toUtf8(), QCryptographicHash::Sha1);
    return QString::fromLatin1(digest.toHex());
}

QString safeHrefStemForExtension(const QString &uid, const QString &extension)
{
    if (uid.isEmpty())
    {
        return randomItemUid();
    }

    QString stem = safeHrefStem(uid);
    const QString href = stem + QLatin1Char('.') + extension;
    if (QFile::encodeName(href).size() > 240)
    {
        const QByteArray digest = QCryptographicHash::hash(uid.toUtf8(), QCryptographicHash::Sha1);
        stem = QString::fromLatin1(digest.toHex());
    }
    return stem;
}

QString normalizeVdirTextLineEndings(const QString &text)
{
    QString normalized;
    normalized.reserve(text.size() + 16);
    for (qsizetype i = 0; i < text.size(); ++i)
    {
        const QChar ch = text.at(i);
        if (ch == QLatin1Char('\r'))
        {
            if (i + 1 < text.size() && text.at(i + 1) == QLatin1Char('\n'))
            {
                normalized += QStringLiteral("\r\n");
                ++i;
            }
            else
            {
                normalized += QStringLiteral("\r\n");
            }
        }
        else if (ch == QLatin1Char('\n'))
        {
            normalized += QStringLiteral("\r\n");
        }
        else
        {
            normalized += ch;
        }
    }
    return normalized;
}

} // namespace

Vdir::Vdir(const QString &path, const QString &extension)
    : m_path(path)
    , m_extension(extension.startsWith(QLatin1Char('.')) ? extension.mid(1) : extension)
{}

StorageStatus Vdir::ensure()
{
    return ensureDirectory();
}

StorageStatus Vdir::ensureDirectory()
{
    if (QFileInfo(m_path).isDir())
    {
        return StorageStatus::Ok;
    }

    QDir dir;
    if (!dir.mkpath(m_path))
    {
        qCWarning(storageLog) << "Could not create vdir" << m_path;
        const QFileInfo parent(QFileInfo(m_path).absolutePath());
        return parent.isWritable() ? StorageStatus::IoError : StorageStatus::ReadOnly;
    }
    return StorageStatus::Ok;
}

QString Vdir::path() const
{
    return m_path;
}

StorageStatus Vdir::writeMetadata(const QString &name, const QString &value) const
{
    if (!isSafeMetadataName(name))
    {
        return StorageStatus::InvalidHref;
    }

    return AtomicFile::writeTextFile(QDir(m_path).filePath(name), value.trimmed() + QLatin1Char('\n'));
}

QList<Vdir::ItemState> Vdir::itemStates() const
{
    QDir dir(m_path);
    const QFileInfoList files = dir.entryInfoList(
        QStringList{QStringLiteral("*.%1").arg(m_extension)}, QDir::Files | QDir::Hidden | QDir::Readable, QDir::Name);
    QList<ItemState> result;
    for (const QFileInfo &file : files)
    {
        const QString href = file.fileName();
        result.append(ItemState{href, etagForFile(href, file)});
    }
    return result;
}

QString Vdir::hrefForUid(const QString &uid) const
{
    const QString stem = safeHrefStemForExtension(uid, m_extension);
    return stem + QLatin1Char('.') + m_extension;
}

StorageStatus Vdir::createItem(const QString &href, const QString &text, QString *etag) const
{
    AtomicFile::CommittedSnapshot snapshot;
    bool committed = false;
    const StorageStatus status = mutateItem(href, [this, &href, &text, &snapshot, &committed](const QString &path) {
        if (hasCaseInsensitiveHrefCollision(href))
        {
            return StorageStatus::Conflict;
        }

        const StorageStatus writeStatus =
            AtomicFile::writeNewTextFile(path, normalizeVdirTextLineEndings(text), &snapshot);
        if (writeStatus == StorageStatus::Ok)
        {
            committed = true;
        }
        return writeStatus;
    });
    if (status != StorageStatus::Ok)
    {
        return status;
    }
    if (committed)
    {
        if (snapshot.etag.isEmpty())
        {
            return StorageStatus::IoError;
        }
        // Cache (size, mtime, etag) from the single fd captureSnapshot opened
        // so the tuple is internally coherent; see updateItem for the residual
        // race discussion.
        rememberEtag(href, snapshot.size, snapshot.mtimeMsecs, snapshot.etag);
        if (etag)
        {
            *etag = snapshot.etag;
        }
    }
    return StorageStatus::Ok;
}

StorageStatus Vdir::updateItem(const QString &href, const QString &text, const QString &etag, QString *newEtag) const
{
    if (etag.isEmpty())
    {
        return StorageStatus::Conflict;
    }

    AtomicFile::CommittedSnapshot snapshot;
    bool committed = false;
    const StorageStatus writeStatus = mutateItem(href, [&snapshot, &committed, &text, &etag](const QString &path) {
        const StorageStatus status =
            AtomicFile::replaceTextFileIfEtagMatches(path, normalizeVdirTextLineEndings(text), etag, &snapshot);
        if (status == StorageStatus::Ok)
        {
            committed = true;
        }
        return status;
    });
    if (writeStatus != StorageStatus::Ok)
    {
        return writeStatus;
    }
    if (committed)
    {
        if (snapshot.etag.isEmpty())
        {
            return StorageStatus::IoError;
        }
        // (size, mtime, etag) come from one open(2) inside captureSnapshot, so
        // the cached tuple is internally coherent. A residual race remains:
        // an external writer that races between this snapshot and the next
        // reader's QFileInfo could bind a stale etag to a matching (size,
        // mtime) — only realistic if the writer deliberately restores mtime,
        // which vdirsyncer/khal do not. Worst case is a phantom Conflict on
        // the next CAS, not corruption.
        rememberEtag(href, snapshot.size, snapshot.mtimeMsecs, snapshot.etag);
        if (newEtag)
        {
            *newEtag = snapshot.etag;
        }
    }
    return StorageStatus::Ok;
}

ReadResult<Vdir::ReadItem> Vdir::readItemWithEtag(const QString &href) const
{
    const QString path = pathForHref(href);
    if (path.isEmpty())
    {
        return ReadResult<ReadItem>{{}, StorageStatus::InvalidHref};
    }

    const QByteArray encodedPath = QFile::encodeName(path);
    int fd = -1;
    do
    {
        fd = ::open(encodedPath.constData(), O_RDONLY | O_CLOEXEC | O_NONBLOCK | O_NOFOLLOW);
    } while (fd < 0 && errno == EINTR);
    if (fd < 0)
    {
        const int openError = errno;
        qCWarning(storageLog) << "Could not read" << path << QString::fromLocal8Bit(strerror(errno));
        if (openError == ENOENT)
        {
            return ReadResult<ReadItem>{{}, StorageStatus::NotFound};
        }
        if (openError == EACCES || openError == EPERM)
        {
            return ReadResult<ReadItem>{{}, StorageStatus::ReadOnly};
        }
        return ReadResult<ReadItem>{{}, StorageStatus::IoError};
    }

    struct stat status;
    if (::fstat(fd, &status) != 0 || !S_ISREG(status.st_mode))
    {
        ::close(fd);
        return ReadResult<ReadItem>{{}, StorageStatus::NotFound};
    }

    QFile file;
    if (!file.open(fd, QFile::ReadOnly, QFileDevice::AutoCloseHandle))
    {
        ::close(fd);
        qCWarning(storageLog) << "Could not read" << path << file.errorString();
        return ReadResult<ReadItem>{{}, StorageStatus::IoError};
    }

    const QByteArray content = file.readAll();
    if (file.error() != QFileDevice::NoError)
    {
        qCWarning(storageLog) << "Could not read" << path << file.errorString();
        return ReadResult<ReadItem>{{}, StorageStatus::IoError};
    }

    const QByteArray digest = QCryptographicHash::hash(content, QCryptographicHash::Sha256);
    return ReadResult<ReadItem>{ReadItem{QString::fromUtf8(content), QString::fromLatin1(digest.toHex())},
                                StorageStatus::Ok};
}

StorageStatus Vdir::removeItem(const QString &href, const QString &etag) const
{
    if (etag.isEmpty())
    {
        return StorageStatus::Conflict;
    }

    return mutateItem(href, [&etag](const QString &path) { return AtomicFile::removeFileIfEtagMatches(path, etag); });
}

QString Vdir::etagForHref(const QString &href) const
{
    return etagForHrefResult(href).etag;
}

Vdir::EtagReadResult Vdir::etagForHrefResult(const QString &href) const
{
    const QString path = pathForHref(href);
    if (path.isEmpty())
    {
        return EtagReadResult{QString(), StorageStatus::InvalidHref};
    }
    return etagForFileResult(href, QFileInfo(path));
}

QString Vdir::etagForFile(const QString &href, const QFileInfo &file) const
{
    return etagForFileResult(href, file).etag;
}

Vdir::EtagReadResult Vdir::etagForFileResult(const QString &href, const QFileInfo &file) const
{
    if (!file.exists() || !file.isFile())
    {
        forgetEtag(href);
        return EtagReadResult{QString(), StorageStatus::NotFound};
    }

    const qint64 size = file.size();
    const qint64 lastModifiedMsecs = file.lastModified().toMSecsSinceEpoch();
    {
        QMutexLocker locker(&m_etagCacheMutex);
        const auto cached = m_etagCache.constFind(href);
        if (cached != m_etagCache.constEnd() && cached->size == size && cached->lastModifiedMsecs == lastModifiedMsecs)
        {
            return EtagReadResult{cached->etag, StorageStatus::Ok};
        }
    }

    // SHA-256 outside the mutex; cache hits above avoid this cost on the
    // hot list-view refresh path.
    const QString etag = AtomicFile::contentEtag(file.filePath());
    const EtagReadResult result{
        etag,
        etag.isEmpty() ? (QFileInfo::exists(file.filePath()) ? StorageStatus::IoError : StorageStatus::NotFound)
                       : StorageStatus::Ok};

    QMutexLocker locker(&m_etagCacheMutex);
    if (!etag.isEmpty())
    {
        const QFileInfo currentFile(file.filePath());
        if (!currentFile.exists() || !currentFile.isFile())
        {
            forgetEtagLocked(href);
            return EtagReadResult{QString(), StorageStatus::NotFound};
        }

        const qint64 currentSize = currentFile.size();
        const qint64 currentLastModifiedMsecs = currentFile.lastModified().toMSecsSinceEpoch();
        if (currentSize != size || currentLastModifiedMsecs != lastModifiedMsecs)
        {
            return EtagReadResult{QString(), StorageStatus::IoError};
        }

        rememberEtagLocked(href, size, lastModifiedMsecs, etag);
    }
    else
    {
        forgetEtagLocked(href);
    }
    return result;
}

void Vdir::rememberEtag(const QString &href, const QFileInfo &file, const QString &etag) const
{
    if (!href.isEmpty() && file.exists() && !etag.isEmpty())
    {
        QMutexLocker locker(&m_etagCacheMutex);
        rememberEtagLocked(href, file, etag);
    }
}

void Vdir::rememberEtag(const QString &href, qint64 size, qint64 lastModifiedMsecs, const QString &etag) const
{
    if (!href.isEmpty() && !etag.isEmpty())
    {
        QMutexLocker locker(&m_etagCacheMutex);
        rememberEtagLocked(href, size, lastModifiedMsecs, etag);
    }
}

void Vdir::rememberEtagLocked(const QString &href, const QFileInfo &file, const QString &etag) const
{
    rememberEtagLocked(href, file.size(), file.lastModified().toMSecsSinceEpoch(), etag);
}

void Vdir::rememberEtagLocked(const QString &href, qint64 size, qint64 lastModifiedMsecs, const QString &etag) const
{
    m_etagCache.insert(href, CachedEtag{size, lastModifiedMsecs, etag});
}

void Vdir::forgetEtag(const QString &href) const
{
    QMutexLocker locker(&m_etagCacheMutex);
    forgetEtagLocked(href);
}

void Vdir::forgetEtagLocked(const QString &href) const
{
    m_etagCache.remove(href);
}

void Vdir::forgetItemCaches(const QString &href) const
{
    forgetEtag(href);
}

bool Vdir::isSafeHref(const QString &href, const QString &extension)
{
    if (href.isEmpty() || href == QLatin1String(".") || href == QLatin1String(".."))
    {
        return false;
    }
    if (QFileInfo(href).fileName() != href || href.contains(QLatin1Char('/')) || href.contains(QLatin1Char('\\')))
    {
        return false;
    }
    if (!extension.isEmpty())
    {
        const QString normalized = extension.startsWith(QLatin1Char('.')) ? extension.mid(1) : extension;
        if (QFileInfo(href).suffix().compare(normalized, Qt::CaseSensitive) != 0)
        {
            return false;
        }
    }
    return true;
}

bool Vdir::isSafeMetadataName(const QString &name)
{
    return isSafeHref(name) && !name.contains(QLatin1Char('.'));
}

QString Vdir::pathForHref(const QString &href) const
{
    if (!isSafeHref(href, m_extension))
    {
        qCWarning(storageLog) << "Rejected unsafe vdir href" << href;
        return QString();
    }
    return QDir(m_path).filePath(href);
}

bool Vdir::hasCaseInsensitiveHrefCollision(const QString &href) const
{
    const QList<ItemState> states = itemStates();
    for (const ItemState &state : states)
    {
        const QString existing = state.href;
        if (existing.compare(href, Qt::CaseInsensitive) == 0 && existing != href)
        {
            qCDebug(storageLog) << "Rejected vdir href case collision" << href << "with" << existing;
            return true;
        }
    }
    return false;
}
