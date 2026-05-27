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

#ifndef ATOMICFILE_H
#define ATOMICFILE_H

#include "storageresult.h"

#include <QString>

// Atomic file-replacement primitives shared by vdir storage. Writes go through
// a synced temporary file in the target's directory and are committed by
// rename(2) or, on Linux, renameat2(RENAME_EXCHANGE) for compare-and-swap
// against an etag captured under a hardlink guard.
namespace AtomicFile {
[[nodiscard]] StorageStatus writeTextFile(const QString &path, const QString &text);
[[nodiscard]] StorageStatus writeNewTextFile(const QString &path, const QString &text);
[[nodiscard]] StorageStatus
replaceTextFileIfEtagMatches(const QString &path, const QString &text, const QString &expectedEtag, QString *newEtag);
[[nodiscard]] StorageStatus removeFileIfEtagMatches(const QString &path, const QString &expectedEtag);
QString contentEtag(const QString &path);
} // namespace AtomicFile

#endif // ATOMICFILE_H
