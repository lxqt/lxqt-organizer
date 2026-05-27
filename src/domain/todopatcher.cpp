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

#include "todopatcher.h"

#include "eventdatetime.h"
#include "incidencefields.h"

#include <QDateTime>
#include <QTimeZone>

#include <KCalendarCore/Recurrence>
#include <KCalendarCore/Todo>

namespace TodoPatcher {

namespace {

Options noOptions()
{
    Options options;
    forEachOption(options, [](bool &option) { option = false; });
    return options;
}

QDateTime editedTaskDueDateTime(const QDateTime &due, const QDateTime &previous)
{
    if (!due.isValid() || !previous.isValid())
    {
        return due;
    }

    if (previous.timeSpec() == Qt::UTC)
    {
        return QDateTime(due.date(), due.time(), QTimeZone::UTC);
    }
    if (previous.timeSpec() == Qt::TimeZone && previous.timeZone().isValid())
    {
        return QDateTime(due.date(), due.time(), previous.timeZone());
    }
    if (previous.timeSpec() == Qt::OffsetFromUTC)
    {
        return QDateTime(due.date(), due.time(), QTimeZone::fromSecondsAheadOfUtc(previous.offsetFromUtc()));
    }
    return QDateTime(due.date(), due.time());
}

} // namespace

Options editableOptions()
{
    Options options;
    options.due = true;
    options.summary = true;
    options.description = true;
    options.priority = true;
    options.highlightMode = true;
    options.completed = true;
    options.rollForward = true;
    return options;
}

Options changedOptionsForTodo(const KCalendarCore::Todo::Ptr &todo, const TaskFields &patch)
{
    Options options = noOptions();

    const bool hadDue = todo->hasDueDate();
    QDateTime due;
    if (patch.hasDue)
    {
        const QDateTime previousDue = hadDue ? todo->dtDue() : QDateTime();
        due = editedTaskDueDateTime(patch.due, previousDue);
    }

    options.summary = patch.title != todo->summary();
    options.description = patch.description != todo->description();
    options.highlightMode =
        patch.highlightMode != CalendarUtils::highlightMode(todo.dynamicCast<KCalendarCore::Incidence>());
    options.due = patch.hasDue != hadDue || (patch.hasDue && due != todo->dtDue());
    options.completed = patch.completed != todo->isCompleted();
    options.priority = patch.priority != todo->priority();
    options.rollForward = patch.rollForward != CalendarUtils::isRollForwardEnabled(todo);
    return options;
}

Options changedOptionsForTodoEdit(const KCalendarCore::Todo::Ptr &stored, const KCalendarCore::Todo::Ptr &edited)
{
    Options options = noOptions();

    const bool storedHasDue = stored->hasDueDate();
    const bool editedHasDue = edited->hasDueDate();
    options.summary = edited->summary() != stored->summary();
    options.description = edited->description() != stored->description();
    options.highlightMode = CalendarUtils::highlightMode(edited.dynamicCast<KCalendarCore::Incidence>()) !=
                            CalendarUtils::highlightMode(stored.dynamicCast<KCalendarCore::Incidence>());
    options.due = editedHasDue != storedHasDue || (editedHasDue && edited->dtDue() != stored->dtDue());
    options.completed = edited->isCompleted() != stored->isCompleted();
    options.priority = edited->priority() != stored->priority();
    options.rollForward = CalendarUtils::isRollForwardEnabled(edited) != CalendarUtils::isRollForwardEnabled(stored);
    return options;
}

bool hasChanges(const Options &options)
{
    bool changed = false;
    forEachOption(options, [&changed](bool option) { changed = changed || option; });
    return changed;
}

void applyToTodo(const KCalendarCore::Todo::Ptr &todo, const TaskFields &patch, Options options)
{
    if (options.summary)
    {
        todo->setSummary(patch.title);
    }
    if (options.description)
    {
        todo->setDescription(patch.description);
    }
    if (options.highlightMode)
    {
        CalendarUtils::setHighlightMode(todo.dynamicCast<KCalendarCore::Incidence>(), patch.highlightMode);
    }

    QDateTime due;
    const bool hadDue = todo->hasDueDate();
    if (patch.hasDue)
    {
        const QDateTime previousDue = hadDue ? todo->dtDue() : QDateTime();
        due = editedTaskDueDateTime(patch.due, previousDue);
    }
    if (options.due)
    {
        todo->setDtDue(due);
    }

    if (options.completed)
    {
        todo->setCompleted(patch.completed);
    }
    if (options.priority)
    {
        todo->setPriority(patch.priority);
    }
    if (options.rollForward)
    {
        CalendarUtils::setRollForwardEnabled(todo, patch.rollForward);
    }
    if (options.recurrence && todo->recurrence())
    {
        todo->recurrence()->unsetRecurs();
    }
}

void applyTodoEdit(const KCalendarCore::Todo::Ptr &todo, const KCalendarCore::Todo::Ptr &edited, Options options)
{
    if (options.summary)
    {
        todo->setSummary(edited->summary());
    }
    if (options.description)
    {
        todo->setDescription(edited->description());
    }
    if (options.highlightMode)
    {
        CalendarUtils::setHighlightMode(todo.dynamicCast<KCalendarCore::Incidence>(),
                                        CalendarUtils::highlightMode(edited.dynamicCast<KCalendarCore::Incidence>()));
    }
    if (options.due)
    {
        todo->setDtDue(edited->hasDueDate() ? edited->dtDue() : QDateTime());
    }
    if (options.completed)
    {
        todo->setCompleted(edited->isCompleted());
    }
    if (options.priority)
    {
        todo->setPriority(edited->priority());
    }
    if (options.rollForward)
    {
        CalendarUtils::setRollForwardEnabled(todo, CalendarUtils::isRollForwardEnabled(edited));
    }
    if (options.recurrence && todo->recurrence())
    {
        todo->recurrence()->unsetRecurs();
    }
}

} // namespace TodoPatcher
