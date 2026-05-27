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

#ifndef TODOPATCHER_H
#define TODOPATCHER_H

#include "calendareditordata.h"

#include <KCalendarCore/Todo>

namespace TodoPatcher {

#define TODO_PATCHER_OPTION_FIELDS(FIELD)                                                                              \
    FIELD(due, false)                                                                                                  \
    FIELD(summary, false)                                                                                              \
    FIELD(description, false)                                                                                          \
    FIELD(priority, false)                                                                                             \
    FIELD(highlightMode, false)                                                                                        \
    FIELD(completed, false)                                                                                            \
    FIELD(rollForward, false)                                                                                          \
    FIELD(recurrence, false)

struct Options
{
#define TODO_PATCHER_DECLARE_OPTION(name, defaultValue) bool name = defaultValue;
    TODO_PATCHER_OPTION_FIELDS(TODO_PATCHER_DECLARE_OPTION)
#undef TODO_PATCHER_DECLARE_OPTION
};

template <typename Visitor> void forEachOption(Options &options, Visitor visitor)
{
#define TODO_PATCHER_VISIT_OPTION(name, defaultValue) visitor(options.name);
    TODO_PATCHER_OPTION_FIELDS(TODO_PATCHER_VISIT_OPTION)
#undef TODO_PATCHER_VISIT_OPTION
}

template <typename Visitor> void forEachOption(const Options &options, Visitor visitor)
{
#define TODO_PATCHER_VISIT_OPTION(name, defaultValue) visitor(options.name);
    TODO_PATCHER_OPTION_FIELDS(TODO_PATCHER_VISIT_OPTION)
#undef TODO_PATCHER_VISIT_OPTION
}

#undef TODO_PATCHER_OPTION_FIELDS

Options editableOptions();
Options changedOptionsForTodo(const KCalendarCore::Todo::Ptr &todo, const TaskFields &patch);
Options changedOptionsForTodoEdit(const KCalendarCore::Todo::Ptr &stored, const KCalendarCore::Todo::Ptr &edited);
bool hasChanges(const Options &options);
void applyToTodo(const KCalendarCore::Todo::Ptr &todo, const TaskFields &patch, Options options = {});
void applyTodoEdit(const KCalendarCore::Todo::Ptr &todo, const KCalendarCore::Todo::Ptr &edited, Options options);

} // namespace TodoPatcher

#endif // TODOPATCHER_H
