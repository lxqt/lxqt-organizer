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

#ifndef CALENDARITEMMUTATOR_H
#define CALENDARITEMMUTATOR_H

#include <QDateTime>
#include <QString>

#include <KCalendarCore/Event>
#include <KCalendarCore/MemoryCalendar>
#include <KCalendarCore/Todo>

#include <optional>

namespace CalendarItemMutator {

bool hasIncidences(const KCalendarCore::MemoryCalendar::Ptr &calendar);
KCalendarCore::Event::Ptr editableSingleEventClone(const KCalendarCore::MemoryCalendar::Ptr &calendar);
KCalendarCore::Event::Ptr eventCloneForEdit(const KCalendarCore::Event::Ptr &event,
                                            const std::optional<QDateTime> &recurrenceId = std::nullopt);
KCalendarCore::Event::Ptr createException(const KCalendarCore::Event::Ptr &master, const QDateTime &recurrenceId);
bool insertEvent(const KCalendarCore::MemoryCalendar::Ptr &calendar, const KCalendarCore::Event::Ptr &event);
bool ensureEventUid(const KCalendarCore::Event::Ptr &event, const QString &uid);
bool ensureTodoUid(const KCalendarCore::Todo::Ptr &todo, const QString &uid);
bool replaceEventByUid(const KCalendarCore::MemoryCalendar::Ptr &calendar,
                       const QString &uid,
                       const KCalendarCore::Event::Ptr &replacement);
bool replaceEventInstance(const KCalendarCore::MemoryCalendar::Ptr &calendar,
                          const KCalendarCore::Event::Ptr &storedEvent,
                          const KCalendarCore::Event::Ptr &replacement);
bool saveEventInstance(const KCalendarCore::MemoryCalendar::Ptr &calendar,
                       const KCalendarCore::Event::Ptr &storedEvent,
                       const KCalendarCore::Event::Ptr &replacement,
                       bool insertNewException);
bool replaceSingleEvent(const KCalendarCore::MemoryCalendar::Ptr &calendar,
                        const KCalendarCore::Event::Ptr &replacement);
bool replaceTodoByUid(const KCalendarCore::MemoryCalendar::Ptr &calendar,
                      const QString &uid,
                      const KCalendarCore::Todo::Ptr &replacement);
bool replaceSingleTodo(const KCalendarCore::MemoryCalendar::Ptr &calendar, const KCalendarCore::Todo::Ptr &replacement);
bool removeEventsByUid(const KCalendarCore::MemoryCalendar::Ptr &calendar, const QString &uid);
bool removeTodoByUid(const KCalendarCore::MemoryCalendar::Ptr &calendar, const QString &uid);

} // namespace CalendarItemMutator

#endif // CALENDARITEMMUTATOR_H
