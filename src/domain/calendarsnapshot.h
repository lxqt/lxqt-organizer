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

#ifndef CALENDARSNAPSHOT_H
#define CALENDARSNAPSHOT_H

#include <QSharedPointer>

#include <variant>

namespace KCalendarCore {
class Event;
class MemoryCalendar;
class Todo;
} // namespace KCalendarCore

// Aliasing rule for snapshot smart pointers:
//
//   *Ptr (CalendarPtr/EventPtr/TodoPtr) are QSharedPointer<const T>. The
//   pointed-to KCalendarCore object is conceptually immutable. The shared
//   instance is reachable from any cached payload (CalendarItem::payload,
//   EventReader occurrence cache, etc.) and from any UI snapshot that has
//   been handed out. Mutating it observably tears those caches.
//
//   - Never qSharedPointerConstCast a *Ptr.
//   - To edit, use cloneEvent/cloneTodo/cloneCalendar to obtain a
//     Mutable*Ptr backed by a fresh deep copy. Mutate the copy. When done,
//     produce a new snapshot via eventSnapshot/todoSnapshot/calendarSnapshot.
//   - calendarForItem(item) returns a MutableCalendarPtr that IS a deep copy
//     of the stored payload; mutating it is safe. Do NOT keep the returned
//     pointer alongside the original CalendarItem.
//
// Violations are silent corruption.
namespace CalendarSnapshot {

using MutableCalendarPtr = QSharedPointer<KCalendarCore::MemoryCalendar>;
using MutableEventPtr = QSharedPointer<KCalendarCore::Event>;
using MutableTodoPtr = QSharedPointer<KCalendarCore::Todo>;

using CalendarPtr = QSharedPointer<const KCalendarCore::MemoryCalendar>;
using EventPtr = QSharedPointer<const KCalendarCore::Event>;
using TodoPtr = QSharedPointer<const KCalendarCore::Todo>;
using CalendarPayload = std::variant<std::monostate, CalendarPtr, EventPtr, TodoPtr>;

enum class PayloadShape
{
    // Infer intentionally switches a single event/todo item to Calendar when
    // the stored item also contains exceptions or mixed incidence payloads.
    Auto,
    Calendar,
    Event,
    Todo
};

CalendarPtr calendarSnapshot(const MutableCalendarPtr &calendar);
EventPtr eventSnapshot(const MutableEventPtr &event);
TodoPtr todoSnapshot(const MutableTodoPtr &todo);

MutableEventPtr cloneEvent(const MutableEventPtr &event);
MutableEventPtr cloneEvent(const EventPtr &event);
MutableTodoPtr cloneTodo(const MutableTodoPtr &todo);
MutableTodoPtr cloneTodo(const TodoPtr &todo);
MutableCalendarPtr cloneCalendar(const MutableCalendarPtr &calendar);
MutableCalendarPtr cloneCalendar(const CalendarPtr &calendar);

} // namespace CalendarSnapshot

#endif // CALENDARSNAPSHOT_H
