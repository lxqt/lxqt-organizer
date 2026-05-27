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

#ifndef COLLECTIONRELOADSUBSCRIPTION_H
#define COLLECTIONRELOADSUBSCRIPTION_H

#include <QObject>
#include <QSet>
#include <QString>

#include <functional>

class CollectionService;

// Owns the QObject context for collection-reload handlers.
//
// The handler is connected with Qt::DirectConnection, so it runs synchronously
// on the collectionsReloaded emitter's thread. Declare this member after any
// state captured by the handler so destruction disconnects before that state is
// torn down.
class CollectionReloadSubscription
{
public:
    using Handler = std::function<void(const QSet<QString> &)>;

    CollectionReloadSubscription(const CollectionService &service, Handler handler);

    CollectionReloadSubscription(const CollectionReloadSubscription &) = delete;
    CollectionReloadSubscription &operator=(const CollectionReloadSubscription &) = delete;

    QObject *context() { return &m_guard; }

private:
    QObject m_guard;
};

#endif // COLLECTIONRELOADSUBSCRIPTION_H
