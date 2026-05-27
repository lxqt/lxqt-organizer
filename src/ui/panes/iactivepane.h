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

#ifndef IACTIVEPANE_H
#define IACTIVEPANE_H

#include "collectiontypes.h"

#include <QList>
#include <QString>

class QAction;

class IActivePane
{
public:
    virtual ~IActivePane() = default;
    virtual QString displayName() const = 0;
    virtual CollectionKind collectionKind() const = 0;

    virtual QList<QAction *> contextActions() const = 0;

    virtual QString selectionStatusText() const = 0;
    virtual int visibleItemCount() const = 0;
};

#endif // IACTIVEPANE_H
