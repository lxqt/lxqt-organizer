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

#ifndef INCIDENCERESOLVER_H
#define INCIDENCERESOLVER_H

#include "incidencelocator.h"

#include <QString>

#include <KCalendarCore/Event>
#include <KCalendarCore/MemoryCalendar>
#include <KCalendarCore/Todo>

namespace IncidenceResolver {

IncidenceLocator inferLocator(const KCalendarCore::MemoryCalendar::Ptr &calendar, const QString &preferredUid = {});
KCalendarCore::Event::Ptr findMasterEvent(const KCalendarCore::MemoryCalendar::Ptr &calendar,
                                          const IncidenceLocator &locator);
KCalendarCore::Event::Ptr findEvent(const KCalendarCore::MemoryCalendar::Ptr &calendar,
                                    const IncidenceLocator &locator);
KCalendarCore::Todo::Ptr findTodo(const KCalendarCore::MemoryCalendar::Ptr &calendar, const IncidenceLocator &locator);
bool contains(const KCalendarCore::MemoryCalendar::Ptr &calendar, const QString &uid);

} // namespace IncidenceResolver

#endif // INCIDENCERESOLVER_H
