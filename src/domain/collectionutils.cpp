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

#include "collectionutils.h"

#include <QDir>
#include <QFileInfo>

namespace CollectionUtils {

QString extensionForKind(CollectionKind kind)
{
    return kind == CollectionKind::Calendar ? QStringLiteral("ics") : QStringLiteral("vcf");
}

QString defaultColor(CollectionKind kind)
{
    return kind == CollectionKind::Calendar ? QStringLiteral("#51BAF2") : QStringLiteral("#77B255");
}

QString pathIdentity(const QString &path)
{
    const QFileInfo info(path);
    if (!info.exists())
    {
        return QString();
    }
    const QString canonical = info.canonicalFilePath();
    return canonical.isEmpty() ? QDir::cleanPath(info.absoluteFilePath()) : canonical;
}

} // namespace CollectionUtils
