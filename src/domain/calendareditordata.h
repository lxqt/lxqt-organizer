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

#ifndef CALENDAREDITORDATA_H
#define CALENDAREDITORDATA_H

#include "highlightmode.h"
#include "incidencelocator.h"

#include <QDate>
#include <QDateTime>
#include <QString>
#include <QTime>

#include <optional>

#define EVENT_FIELDS(FIELD)                                                                                            \
    FIELD(QString, title, )                                                                                            \
    FIELD(QString, location, )                                                                                         \
    FIELD(QString, description, )                                                                                      \
    FIELD(QString, url, )                                                                                              \
    FIELD(QString, attachmentUri, )                                                                                    \
    FIELD(QString, timeZoneId, )                                                                                       \
    FIELD(QString, displayTimeZoneId, )                                                                                \
    FIELD(QString, collectionId, )                                                                                     \
    FIELD(QDate, date, )                                                                                               \
    FIELD(QTime, startTime, )                                                                                          \
    FIELD(QTime, endTime, )                                                                                            \
    FIELD(bool, allDay, = false)                                                                                       \
    FIELD(int, priority, = 5)                                                                                          \
    FIELD(bool, completed, = false)                                                                                    \
    FIELD(CalendarUtils::HighlightMode, highlightMode, = CalendarUtils::HighlightMode::Always)

struct EventFields
{
#define DECLARE_EVENT_FIELD(type, name, defaultValue) type name defaultValue;
    EVENT_FIELDS(DECLARE_EVENT_FIELD)
#undef DECLARE_EVENT_FIELD
    std::optional<IncidenceLocator> locator;

    QString validationError() const;
    bool operator==(const EventFields &other) const
    {
#define COMPARE_EVENT_FIELD(type, name, defaultValue)                                                                  \
    if (name != other.name)                                                                                            \
    {                                                                                                                  \
        return false;                                                                                                  \
    }
        EVENT_FIELDS(COMPARE_EVENT_FIELD)
#undef COMPARE_EVENT_FIELD
        return locator == other.locator;
    }
};

#undef EVENT_FIELDS

#define TASK_FIELDS(FIELD)                                                                                             \
    FIELD(QString, title, )                                                                                            \
    FIELD(QString, description, )                                                                                      \
    FIELD(QString, collectionId, )                                                                                     \
    FIELD(bool, hasDue, = false)                                                                                       \
    FIELD(QDateTime, due, )                                                                                            \
    FIELD(int, priority, = 5)                                                                                          \
    FIELD(bool, completed, = false)                                                                                    \
    FIELD(bool, rollForward, = false)                                                                                  \
    FIELD(CalendarUtils::HighlightMode, highlightMode, = CalendarUtils::HighlightMode::Always)

struct TaskFields
{
#define DECLARE_TASK_FIELD(type, name, defaultValue) type name defaultValue;
    TASK_FIELDS(DECLARE_TASK_FIELD)
#undef DECLARE_TASK_FIELD

    QString validationError() const;
    bool operator==(const TaskFields &other) const
    {
#define COMPARE_TASK_FIELD(type, name, defaultValue)                                                                   \
    if (name != other.name)                                                                                            \
    {                                                                                                                  \
        return false;                                                                                                  \
    }
        TASK_FIELDS(COMPARE_TASK_FIELD)
#undef COMPARE_TASK_FIELD
        return true;
    }
};

#undef TASK_FIELDS

#endif // CALENDAREDITORDATA_H
