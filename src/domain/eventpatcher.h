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

#ifndef EVENTPATCHER_H
#define EVENTPATCHER_H

#include "calendareditordata.h"

#include <QDateTime>

#include <KCalendarCore/Event>

namespace EventPatcher {

#define EVENT_PATCHER_OPTION_FIELDS(FIELD)                                                                             \
    FIELD(attachment, false)                                                                                           \
    FIELD(dateTime, false)                                                                                             \
    FIELD(timeZone, false)                                                                                             \
    FIELD(summary, false)                                                                                              \
    FIELD(location, false)                                                                                             \
    FIELD(description, false)                                                                                          \
    FIELD(url, false)                                                                                                  \
    FIELD(priority, false)                                                                                             \
    FIELD(highlightMode, false)                                                                                        \
    FIELD(completed, false)                                                                                            \
    FIELD(alarms, false)                                                                                               \
    FIELD(recurrence, false)

struct Options
{
#define EVENT_PATCHER_DECLARE_OPTION(name, defaultValue) bool name = defaultValue;
    EVENT_PATCHER_OPTION_FIELDS(EVENT_PATCHER_DECLARE_OPTION)
#undef EVENT_PATCHER_DECLARE_OPTION
};

template <typename Visitor> void forEachOption(Options &options, Visitor visitor)
{
#define EVENT_PATCHER_VISIT_OPTION(name, defaultValue) visitor(options.name);
    EVENT_PATCHER_OPTION_FIELDS(EVENT_PATCHER_VISIT_OPTION)
#undef EVENT_PATCHER_VISIT_OPTION
}

template <typename Visitor> void forEachOption(const Options &options, Visitor visitor)
{
#define EVENT_PATCHER_VISIT_OPTION(name, defaultValue) visitor(options.name);
    EVENT_PATCHER_OPTION_FIELDS(EVENT_PATCHER_VISIT_OPTION)
#undef EVENT_PATCHER_VISIT_OPTION
}

#undef EVENT_PATCHER_OPTION_FIELDS

Options editableOptions();
bool dateTimeFieldsChanged(const EventFields &patch, const QDateTime &start, const QDateTime &end, bool allDay);
Options changedOptionsForEvent(const KCalendarCore::Event::Ptr &event, const EventFields &patch);
bool hasChanges(const Options &options);
void applyToEvent(const KCalendarCore::Event::Ptr &event, const EventFields &patch, Options options = {});

} // namespace EventPatcher

#endif // EVENTPATCHER_H
