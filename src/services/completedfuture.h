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

#ifndef COMPLETEDFUTURE_H
#define COMPLETEDFUTURE_H

#include <QFuture>
#include <QPromise>

#include <utility>

namespace CalendarIoUtils {

template <typename Result> QFuture<Result> completedFuture(Result result)
{
    QPromise<Result> promise;
    QFuture<Result> future = promise.future();
    promise.start();
    promise.addResult(std::move(result));
    promise.finish();
    return future;
}

} // namespace CalendarIoUtils

#endif // COMPLETEDFUTURE_H
