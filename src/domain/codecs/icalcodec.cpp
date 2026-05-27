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

#include "icalcodec.h"

#include <QTimeZone>

#include <KCalendarCore/ICalFormat>

namespace {

KCalendarCore::MemoryCalendar::Ptr newCalendar()
{
    return KCalendarCore::MemoryCalendar::Ptr(new KCalendarCore::MemoryCalendar(QTimeZone::systemTimeZone()));
}

} // namespace

namespace ICalCodec {

std::optional<KCalendarCore::MemoryCalendar::Ptr> parse(const QString &text)
{
    if (text.trimmed().isEmpty())
    {
        return std::nullopt;
    }

    KCalendarCore::MemoryCalendar::Ptr calendar = newCalendar();
    KCalendarCore::ICalFormat format;
    if (!format.fromRawString(calendar, text.toUtf8()))
    {
        return std::nullopt;
    }
    return calendar;
}

QString serialize(const KCalendarCore::MemoryCalendar::Ptr &calendar)
{
    KCalendarCore::ICalFormat format;
    const QString text = format.toString(calendar);
    return text.isEmpty() ? QString() : text;
}

} // namespace ICalCodec
