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

#include "eventtimezone.h"

#include <QDate>
#include <QDateTime>
#include <QTime>
#include <QTimeZone>

namespace EventTimeZone {

QString timeZoneId(const KCalendarCore::Event::Ptr &event)
{
    if (!event->dtStart().isValid())
    {
        return QString::fromUtf8(QTimeZone::systemTimeZoneId());
    }

    const QTimeZone zone = event->dtStart().timeZone();
    return zone.isValid() ? QString::fromUtf8(zone.id()) : QString::fromUtf8(QTimeZone::systemTimeZoneId());
}

void setTimeZoneId(const KCalendarCore::Event::Ptr &event, const QString &timeZoneId)
{
    QTimeZone zone(timeZoneId.trimmed().toUtf8());
    if (!zone.isValid())
    {
        zone = QTimeZone::systemTimeZone();
    }

    const QDateTime oldStart = event->dtStart();
    const QDateTime oldEnd = event->dtEnd().isValid() ? event->dtEnd() : event->dtStart();

    if (event->allDay())
    {
        const QDate startDate = oldStart.date().isValid() ? oldStart.date() : QDate::currentDate();
        const QDate endDate = oldEnd.date().isValid() ? oldEnd.date() : startDate;
        event->setDtStart(QDateTime(startDate, QTime(0, 0), zone));
        event->setDtEnd(QDateTime(endDate, QTime(0, 0), zone));
        return;
    }

    if (oldStart.isValid())
    {
        event->setDtStart(oldStart.toTimeZone(zone));
        event->setDtEnd(oldEnd.isValid() ? oldEnd.toTimeZone(zone) : event->dtStart());
        return;
    }

    const QDate startDate = QDate::currentDate();
    event->setDtStart(QDateTime(startDate, QTime(0, 0), zone));
    event->setDtEnd(event->dtStart());
}

} // namespace EventTimeZone
