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

#ifndef CANCELLATIONTOKEN_H
#define CANCELLATIONTOKEN_H

#include <atomic>
#include <functional>
#include <utility>

class CancellationToken
{
public:
    using StopRequested = std::function<bool()>;

    CancellationToken() = default;
    explicit CancellationToken(StopRequested stopRequested)
        : m_stopRequested(std::move(stopRequested))
    {}
    explicit CancellationToken(const std::atomic_bool &cancellationRequested)
        : m_stopRequested([state = &cancellationRequested]() { return state->load(); })
    {}

    bool isCancellationRequested() const { return m_stopRequested && m_stopRequested(); }

private:
    StopRequested m_stopRequested;
};

#endif // CANCELLATIONTOKEN_H
