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

#ifndef FUTUREWATCH_H
#define FUTUREWATCH_H

#include <QFuture>
#include <QFutureWatcher>
#include <QObject>
#include <QPointer>

#include <type_traits>

namespace FutureWatcher {

template <typename T, typename Callback> void watch(QObject *parent, QFuture<T> future, Callback callback)
{
    auto *watcher = new QFutureWatcher<T>(parent);
    QObject::connect(watcher,
                     &QFutureWatcher<T>::finished,
                     parent,
                     [watcher, callback, guard = QPointer<QObject>(parent)]() mutable {
                         if (!guard)
                         {
                             return;
                         }
                         if constexpr (std::is_void_v<T>)
                         {
                             watcher->deleteLater();
                             callback();
                         }
                         else
                         {
                             const T result = watcher->result();
                             watcher->deleteLater();
                             callback(result);
                         }
                     });
    watcher->setFuture(future);
}

} // namespace FutureWatcher

#endif // FUTUREWATCH_H
