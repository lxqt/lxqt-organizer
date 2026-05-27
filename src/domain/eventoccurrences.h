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

#ifndef EVENTOCCURRENCES_H
#define EVENTOCCURRENCES_H

#include "calendaritem.h"

#include <QDate>
#include <QDateTime>
#include <QList>

#include <KCalendarCore/Event>
#include <KCalendarCore/MemoryCalendar>

namespace EventOccurrences {

QList<EventOccurrence> occurrenceSnapshots(const KCalendarCore::MemoryCalendar::Ptr &calendar,
                                           const ItemRef &storage,
                                           const QString &uid,
                                           const QDate &rangeStart,
                                           const QDate &rangeEnd);
QDate displayEndDate(const EventOccurrence &event);
bool overlapsDate(const EventOccurrence &event, const QDate &date);
KCalendarCore::Event::Ptr editableSingleEvent(const KCalendarCore::MemoryCalendar::Ptr &calendar);
bool canEditAsEvent(const KCalendarCore::MemoryCalendar::Ptr &calendar);

} // namespace EventOccurrences

#endif // EVENTOCCURRENCES_H
