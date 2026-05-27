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

#include "atomicfile.h"

#include "storagelog.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QUuid>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

namespace AtomicFile {
namespace {

#ifdef __linux__
enum class RenameExchangeStatus
{
    Exchanged,
    Unsupported,
    Failed,
};
#endif

bool syncParentDirectory(const QString &path);
[[nodiscard]] StorageStatus writeFailureStatus(const QString &path);
[[nodiscard]] StorageStatus createFailureStatus(const QString &path);
bool writeAndSyncFileDescriptor(int fd, const QByteArray &content, const QString &path);
[[nodiscard]] StorageStatus replaceFailureStatusFromErrno(int error);

[[nodiscard]] StorageStatus replaceFailureStatusFromErrno(int error)
{
    if (error == EACCES || error == EPERM)
    {
        return StorageStatus::ReadOnly;
    }
    return error == ENOENT ? StorageStatus::NotFound : StorageStatus::IoError;
}

bool syncParentDirectory(const QString &path)
{
    const QByteArray dirPath = QFile::encodeName(QFileInfo(path).absolutePath());
    int flags = O_RDONLY;
#ifdef O_DIRECTORY
    flags |= O_DIRECTORY;
#endif
    const int fd = ::open(dirPath.constData(), flags);
    if (fd < 0)
    {
        qCWarning(storageLog) << "Could not open parent directory for" << path;
        return false;
    }

    const bool success = ::fsync(fd) == 0;
    ::close(fd);
    if (!success)
    {
        qCWarning(storageLog) << "Could not sync parent directory for" << path;
    }
    return success;
}

[[nodiscard]] StorageStatus writeFailureStatus(const QString &path)
{
    if (!QFileInfo::exists(path))
    {
        return StorageStatus::NotFound;
    }

    const QFileInfo target(path);
    const QFileInfo parent(target.absolutePath());
    if (!target.isWritable() || !parent.isWritable())
    {
        return StorageStatus::ReadOnly;
    }
    return StorageStatus::IoError;
}

[[nodiscard]] StorageStatus createFailureStatus(const QString &path)
{
    if (QFileInfo::exists(path))
    {
        return StorageStatus::Conflict;
    }

    const QFileInfo parent(QFileInfo(path).absolutePath());
    return parent.isWritable() ? StorageStatus::IoError : StorageStatus::ReadOnly;
}

bool writeAll(int fd, const QByteArray &data)
{
    const char *next = data.constData();
    qsizetype remaining = data.size();
    while (remaining > 0)
    {
        const ssize_t written = ::write(fd, next, static_cast<size_t>(remaining));
        if (written < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return false;
        }
        if (written == 0)
        {
            return false;
        }
        next += written;
        remaining -= written;
    }
    return true;
}

bool writeAndSyncFileDescriptor(int fd, const QByteArray &content, const QString &path)
{
    if (!writeAll(fd, content))
    {
        qCWarning(storageLog) << "Could not write" << path << QString::fromLocal8Bit(strerror(errno));
        return false;
    }
    if (::fsync(fd) != 0)
    {
        qCWarning(storageLog) << "Could not sync" << path << QString::fromLocal8Bit(strerror(errno));
        return false;
    }
    return true;
}

bool sameFileInode(const QString &first, const QString &second)
{
    struct stat firstStatus;
    struct stat secondStatus;
    const QByteArray firstPath = QFile::encodeName(first);
    const QByteArray secondPath = QFile::encodeName(second);
    if (::stat(firstPath.constData(), &firstStatus) != 0 || ::stat(secondPath.constData(), &secondStatus) != 0)
    {
        return false;
    }
    return firstStatus.st_dev == secondStatus.st_dev && firstStatus.st_ino == secondStatus.st_ino;
}

QString uniqueSiblingPath(const QString &path, const QString &suffix)
{
    const QFileInfo target(path);
    return target.dir().filePath(
        QStringLiteral(".%1.%2.%3").arg(target.fileName(), QUuid::createUuid().toString(QUuid::Id128), suffix));
}

[[nodiscard]] StorageStatus
writeSyncedTemporaryTextFile(const QString &targetPath, const QString &text, QString *temporaryPath)
{
    const QFileInfo target(targetPath);
    QTemporaryFile file(target.dir().filePath(QStringLiteral(".%1.XXXXXX.tmp").arg(target.fileName())));
    file.setAutoRemove(false);
    if (!file.open())
    {
        qCWarning(storageLog) << "Could not create temporary file for" << targetPath << file.errorString();
        return createFailureStatus(targetPath);
    }

    const QByteArray content = text.toUtf8();
    if (file.handle() < 0 || !writeAndSyncFileDescriptor(file.handle(), content, targetPath))
    {
        qCWarning(storageLog) << "Could not write" << targetPath << file.errorString();
        const QString failedTemporaryPath = file.fileName();
        file.remove();
        QFile::remove(failedTemporaryPath);
        return writeFailureStatus(targetPath);
    }

    *temporaryPath = file.fileName();
    file.close();
    return StorageStatus::Ok;
}

[[nodiscard]] StorageStatus linkExistingItemGuard(const QString &path, const QString &expectedEtag, QString *guardPath)
{
    *guardPath = uniqueSiblingPath(path, QStringLiteral("cas-old"));
    const QByteArray targetPath = QFile::encodeName(path);
    const QByteArray encodedGuardPath = QFile::encodeName(*guardPath);
    if (::link(targetPath.constData(), encodedGuardPath.constData()) != 0)
    {
        const int linkError = errno;
        if (linkError == ENOENT)
        {
            return StorageStatus::NotFound;
        }
        if (linkError == EACCES || linkError == EPERM)
        {
            return StorageStatus::ReadOnly;
        }
        qCWarning(storageLog) << "Could not create CAS guard for" << path
                              << QString::fromLocal8Bit(strerror(linkError));
        return StorageStatus::IoError;
    }

    const QString guardEtag = contentEtag(*guardPath);
    if (guardEtag.isEmpty())
    {
        qCWarning(storageLog) << "Could not compute CAS guard etag for" << path;
        QFile::remove(*guardPath);
        return QFileInfo::exists(path) ? StorageStatus::IoError : StorageStatus::NotFound;
    }
    if (guardEtag != expectedEtag || !sameFileInode(path, *guardPath))
    {
        QFile::remove(*guardPath);
        return StorageStatus::Conflict;
    }
    return StorageStatus::Ok;
}

#ifdef __linux__
[[nodiscard]] StorageStatus linkReplacementGuard(const QString &path, QString *guardPath)
{
    *guardPath = uniqueSiblingPath(path, QStringLiteral("cas-new"));
    const QByteArray replacementPath = QFile::encodeName(path);
    const QByteArray encodedGuardPath = QFile::encodeName(*guardPath);
    if (::link(replacementPath.constData(), encodedGuardPath.constData()) != 0)
    {
        const int linkError = errno;
        if (linkError == EACCES || linkError == EPERM)
        {
            return StorageStatus::ReadOnly;
        }
        qCWarning(storageLog) << "Could not create CAS replacement guard for" << path
                              << QString::fromLocal8Bit(strerror(linkError));
        return StorageStatus::IoError;
    }
    return StorageStatus::Ok;
}

RenameExchangeStatus renameExchange(const QString &first, const QString &second, int *exchangeError)
{
    const QByteArray firstPath = QFile::encodeName(first);
    const QByteArray secondPath = QFile::encodeName(second);
    int result = -1;
    do
    {
        result = ::renameat2(AT_FDCWD, firstPath.constData(), AT_FDCWD, secondPath.constData(), RENAME_EXCHANGE);
    } while (result != 0 && errno == EINTR);

    if (result == 0)
    {
        return RenameExchangeStatus::Exchanged;
    }

    *exchangeError = errno;
    if (*exchangeError == ENOSYS || *exchangeError == EINVAL)
    {
        return RenameExchangeStatus::Unsupported;
    }
    return RenameExchangeStatus::Failed;
}

struct LinuxExchangeCommitResult
{
    bool available = false;
    StorageStatus status = StorageStatus::IoError;
};

LinuxExchangeCommitResult
commitReplacementWithLinuxExchange(const QString &path, const QString &replacementPath, const QString &guardPath)
{
    QString replacementGuardPath;
    StorageStatus status = linkReplacementGuard(replacementPath, &replacementGuardPath);
    if (status != StorageStatus::Ok)
    {
        return LinuxExchangeCommitResult{true, status};
    }

    // Exchange first, then verify the captured old target. If another writer
    // won the race, only roll back while the visible target is still ours.
    int exchangeError = 0;
    const RenameExchangeStatus exchangeStatus = renameExchange(replacementPath, path, &exchangeError);
    if (exchangeStatus == RenameExchangeStatus::Unsupported)
    {
        QFile::remove(replacementGuardPath);
        return LinuxExchangeCommitResult{false, StorageStatus::Ok};
    }
    if (exchangeStatus == RenameExchangeStatus::Failed)
    {
        QFile::remove(replacementGuardPath);
        return LinuxExchangeCommitResult{true, replaceFailureStatusFromErrno(exchangeError)};
    }

    if (sameFileInode(replacementPath, guardPath))
    {
        QFile::remove(replacementGuardPath);
        return LinuxExchangeCommitResult{true, StorageStatus::Ok};
    }

    if (sameFileInode(path, replacementGuardPath))
    {
        int restoreError = 0;
        const RenameExchangeStatus restoreStatus = renameExchange(replacementPath, path, &restoreError);
        if (restoreStatus != RenameExchangeStatus::Exchanged)
        {
            qCWarning(storageLog) << "Could not restore externally modified file" << path
                                  << QString::fromLocal8Bit(strerror(restoreError));
            QFile::remove(replacementGuardPath);
            return LinuxExchangeCommitResult{true, StorageStatus::IoError};
        }
        if (!syncParentDirectory(path))
        {
            QFile::remove(replacementGuardPath);
            return LinuxExchangeCommitResult{true, StorageStatus::IoError};
        }
    }

    QFile::remove(replacementGuardPath);
    return LinuxExchangeCommitResult{true, StorageStatus::Conflict};
}
#endif

} // namespace

[[nodiscard]] StorageStatus writeTextFile(const QString &path, const QString &text)
{
    const bool targetExisted = QFileInfo::exists(path);
    const QFileInfo target(path);
    QTemporaryFile file(target.dir().filePath(QStringLiteral(".%1.XXXXXX.tmp").arg(target.fileName())));
    file.setAutoRemove(true);
    if (!file.open())
    {
        qCWarning(storageLog) << "Could not write" << path << file.errorString();
        return targetExisted ? writeFailureStatus(path) : createFailureStatus(path);
    }

    const QByteArray content = text.toUtf8();
    if (file.handle() < 0 || !writeAndSyncFileDescriptor(file.handle(), content, path))
    {
        qCWarning(storageLog) << "Could not write" << path << file.errorString();
        file.remove();
        return targetExisted ? writeFailureStatus(path) : createFailureStatus(path);
    }
    file.close();

    const QByteArray temporaryPath = QFile::encodeName(file.fileName());
    const QByteArray targetPath = QFile::encodeName(path);
    if (::rename(temporaryPath.constData(), targetPath.constData()) != 0)
    {
        qCWarning(storageLog) << "Could not commit" << path << QString::fromLocal8Bit(strerror(errno));
        file.remove();
        return targetExisted ? writeFailureStatus(path) : createFailureStatus(path);
    }
    return syncParentDirectory(path) ? StorageStatus::Ok : StorageStatus::IoError;
}

[[nodiscard]] StorageStatus writeNewTextFile(const QString &path, const QString &text)
{
    const QFileInfo target(path);
    QTemporaryFile file(target.dir().filePath(QStringLiteral(".%1.XXXXXX.tmp").arg(target.fileName())));
    file.setAutoRemove(true);
    if (!file.open())
    {
        qCWarning(storageLog) << "Could not create temporary file for" << path << file.errorString();
        return createFailureStatus(path);
    }

    const QByteArray content = text.toUtf8();
    if (file.handle() < 0 || !writeAndSyncFileDescriptor(file.handle(), content, path))
    {
        qCWarning(storageLog) << "Could not write" << path << file.errorString();
        file.remove();
        return createFailureStatus(path);
    }
    file.close();

    const QByteArray temporaryPath = QFile::encodeName(file.fileName());
    const QByteArray targetPath = QFile::encodeName(path);
    if (::link(temporaryPath.constData(), targetPath.constData()) != 0)
    {
        const int linkError = errno;
        if (linkError == EEXIST)
        {
            qCWarning(storageLog) << "Could not create" << path << "because it already exists";
        }
        else
        {
            qCWarning(storageLog) << "Could not create" << path << "from" << file.fileName()
                                  << QString::fromLocal8Bit(strerror(linkError));
        }
        return createFailureStatus(path);
    }
    if (!syncParentDirectory(path))
    {
        qCWarning(storageLog) << "Removing newly created" << path << "because parent directory sync failed";
        QFile::remove(path);
        file.remove();
        return createFailureStatus(path);
    }

    file.remove();
    return StorageStatus::Ok;
}

[[nodiscard]] StorageStatus
replaceTextFileIfEtagMatches(const QString &path, const QString &text, const QString &expectedEtag, QString *newEtag)
{
    QString replacementPath;
    StorageStatus status = writeSyncedTemporaryTextFile(path, text, &replacementPath);
    if (status != StorageStatus::Ok)
    {
        return status;
    }

    QString guardPath;
    status = linkExistingItemGuard(path, expectedEtag, &guardPath);
    if (status != StorageStatus::Ok)
    {
        QFile::remove(replacementPath);
        return status;
    }

    if (!sameFileInode(path, guardPath))
    {
        QFile::remove(guardPath);
        QFile::remove(replacementPath);
        return QFileInfo::exists(path) ? StorageStatus::Conflict : StorageStatus::NotFound;
    }

#ifdef __linux__
    const LinuxExchangeCommitResult exchangeResult =
        commitReplacementWithLinuxExchange(path, replacementPath, guardPath);
    if (exchangeResult.available)
    {
        const bool synced = exchangeResult.status == StorageStatus::Ok && syncParentDirectory(path);
        QFile::remove(guardPath);
        QFile::remove(replacementPath);
        if (exchangeResult.status != StorageStatus::Ok)
        {
            return exchangeResult.status;
        }
        if (!synced)
        {
            return StorageStatus::IoError;
        }
    }
    else
#endif
    {
        const QByteArray targetPath = QFile::encodeName(path);
        const QByteArray encodedReplacementPath = QFile::encodeName(replacementPath);
        if (::rename(encodedReplacementPath.constData(), targetPath.constData()) != 0)
        {
            const int renameError = errno;
            QFile::remove(guardPath);
            QFile::remove(replacementPath);
            return replaceFailureStatusFromErrno(renameError);
        }

        const bool synced = syncParentDirectory(path);
        QFile::remove(guardPath);
        QFile::remove(replacementPath);
        if (!synced)
        {
            return StorageStatus::IoError;
        }
    }

    if (newEtag)
    {
        *newEtag = contentEtag(path);
        if (newEtag->isEmpty())
        {
            qCWarning(storageLog) << "Could not compute etag after committing" << path;
            return StorageStatus::IoError;
        }
    }
    return StorageStatus::Ok;
}

[[nodiscard]] StorageStatus removeFileIfEtagMatches(const QString &path, const QString &expectedEtag)
{
    QString guardPath;
    StorageStatus status = linkExistingItemGuard(path, expectedEtag, &guardPath);
    if (status != StorageStatus::Ok)
    {
        return status;
    }

    // Re-verify the visible target still references the captured inode just
    // before the unlink. linkExistingItemGuard's etag pass is racy against an
    // external writer (vdirsyncer, khal) that rename(2)s a new file over the
    // original between the guard hardlink and the unlink. Without this check
    // we would silently remove the racer's file.
    if (!sameFileInode(path, guardPath))
    {
        QFile::remove(guardPath);
        return QFileInfo::exists(path) ? StorageStatus::Conflict : StorageStatus::NotFound;
    }

    const QByteArray targetPath = QFile::encodeName(path);
    if (::unlink(targetPath.constData()) != 0)
    {
        const int unlinkError = errno;
        QFile::remove(guardPath);
        if (unlinkError == EACCES || unlinkError == EPERM)
        {
            return StorageStatus::ReadOnly;
        }
        return unlinkError == ENOENT ? StorageStatus::NotFound : StorageStatus::IoError;
    }

    const bool synced = syncParentDirectory(path);
    QFile::remove(guardPath);
    return synced ? StorageStatus::Ok : StorageStatus::IoError;
}

QString contentEtag(const QString &path)
{
    const QByteArray encodedPath = QFile::encodeName(path);
    int fd = -1;
    do
    {
        fd = ::open(encodedPath.constData(), O_RDONLY | O_CLOEXEC | O_NONBLOCK);
    } while (fd < 0 && errno == EINTR);
    if (fd < 0)
    {
        return QString();
    }

    struct stat status;
    if (::fstat(fd, &status) != 0 || !S_ISREG(status.st_mode))
    {
        ::close(fd);
        return QString();
    }

    QFile file;
    if (!file.open(fd, QFile::ReadOnly, QFileDevice::AutoCloseHandle))
    {
        ::close(fd);
        return QString();
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file))
    {
        return QString();
    }
    return QString::fromLatin1(hash.result().toHex());
}

} // namespace AtomicFile
