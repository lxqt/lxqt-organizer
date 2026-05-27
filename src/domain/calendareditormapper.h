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

#ifndef CALENDAREDITORMAPPER_H
#define CALENDAREDITORMAPPER_H

#include "calendareditordata.h"
#include "calendaritem.h"

#include <KCalendarCore/Event>
#include <KCalendarCore/Todo>

#include <QDate>

namespace CalendarEditorMapper {

EventFields defaultEvent(const QDate &date);
EventFields fromEventOccurrence(const EventOccurrence &event);
KCalendarCore::Event::Ptr createEvent(const EventFields &data);

TaskFields fromTask(const Task &task, const QDate &defaultDueDate);
KCalendarCore::Todo::Ptr createTodo(const TaskFields &data);
Task createTask(const TaskFields &data);
Task applyToTask(Task task, const TaskFields &data);

} // namespace CalendarEditorMapper

#endif // CALENDAREDITORMAPPER_H
