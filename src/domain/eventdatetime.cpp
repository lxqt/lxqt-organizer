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

#include "eventdatetime.h"

#include <QDateTime>
#include <QTimeZone>

#include <KCalendarCore/Duration>


namespace {

QDateTime allDayDateTime(const QDate &date, const QDateTime &previous)
{
    const QTime time(0, 0);
    if (previous.timeSpec() != Qt::LocalTime && previous.timeZone().isValid())
    {
        return QDateTime(date, time, previous.timeZone());
    }
    return QDateTime(date, time);
}

QDateTime editedDateTime(const QDate &date, const QTime &time, const QDateTime &previous)
{
    if (previous.timeSpec() == Qt::UTC)
    {
        return QDateTime(date, time, QTimeZone::systemTimeZone()).toUTC();
    }
    if (previous.timeSpec() == Qt::TimeZone && previous.timeZone().isValid())
    {
        return QDateTime(date, time, previous.timeZone());
    }
    return QDateTime(date, time);
}

int allDayDurationDays(const QDateTime &start, const QDateTime &end, bool allDay)
{
    if (!start.isValid())
    {
        return 1;
    }

    const QDate startDate = allDay ? start.date() : EventDateTime::displayDateTime(start).date();
    const QDateTime effectiveEnd = end.isValid() ? end : start.addDays(1);
    const QDate endDate = allDay ? effectiveEnd.date() : EventDateTime::displayDateTime(effectiveEnd).date();
    if (!startDate.isValid() || !endDate.isValid())
    {
        return 1;
    }

    const int days = startDate.daysTo(endDate);
    if (allDay || effectiveEnd.time() == QTime(0, 0))
    {
        return qMax(1, days);
    }
    return qMax(1, days + 1);
}

} // namespace

namespace EventDateTime {

QDateTime displayDateTime(const QDateTime &dateTime)
{
    return displayDateTime(dateTime, QTimeZone::systemTimeZone());
}

QDateTime displayDateTime(const QDateTime &dateTime, const QTimeZone &displayTimeZone)
{
    if (!dateTime.isValid())
    {
        return {};
    }
    if (dateTime.timeSpec() == Qt::UTC || dateTime.timeSpec() == Qt::TimeZone)
    {
        return dateTime.toTimeZone(displayTimeZone);
    }
    return dateTime;
}

KCalendarCore::Event::Ptr createEvent()
{
    KCalendarCore::Event::Ptr event(new KCalendarCore::Event);
    event->setCreated(QDateTime::currentDateTimeUtc());
    return event;
}

QDate parseDate(const QString &value)
{
    QDate date = QDate::fromString(value);
    if (!date.isValid())
    {
        date = QDate::fromString(value, Qt::ISODate);
    }
    if (!date.isValid())
    {
        date = QDate::fromString(value, QStringLiteral("yyyyMMdd"));
    }
    return date;
}

QTime parseTime(const QString &value)
{
    QTime time = QTime::fromString(value);
    if (!time.isValid())
    {
        time = QTime::fromString(value, QStringLiteral("HHmmss"));
    }
    if (!time.isValid())
    {
        time = QTime::fromString(value, QStringLiteral("HH:mm:ss"));
    }
    return time.isValid() ? time : QTime(0, 0);
}

int normalizedPriority(int priority)
{
    if (priority > 0 && priority <= 3)
    {
        return 1;
    }
    if (priority >= 7)
    {
        return 9;
    }
    return 5;
}

QDate date(const KCalendarCore::Event::Ptr &event)
{
    return event->dtStart().date();
}

QString dateString(const KCalendarCore::Event::Ptr &event)
{
    return date(event).toString();
}

void setDate(const KCalendarCore::Event::Ptr &event, const QDate &date)
{
    if (!date.isValid())
    {
        return;
    }

    if (event->allDay())
    {
        const QDateTime oldStart = event->dtStart();
        const QDateTime oldEnd = event->dtEnd().isValid() ? event->dtEnd() : oldStart.addDays(1);
        const QDate oldStartDate = oldStart.date();
        const QDate oldEndDate = oldEnd.date();
        const int durationDays =
            oldStartDate.isValid() && oldEndDate.isValid() ? qMax(1, oldStartDate.daysTo(oldEndDate)) : 1;
        event->setDtStart(allDayDateTime(date, oldStart));
        event->setDtEnd(allDayDateTime(date.addDays(durationDays), oldStart));
        return;
    }

    const QDateTime oldStart = event->dtStart();
    const QDateTime oldEnd = event->dtEnd().isValid() ? event->dtEnd() : oldStart;
    const QDateTime displayStart = displayDateTime(oldStart);
    const QDateTime displayEnd = displayDateTime(oldEnd);
    const QTime start = displayStart.time().isValid() ? displayStart.time() : QTime(0, 0);
    const QTime end = displayEnd.time().isValid() ? displayEnd.time() : QTime(0, 0);
    const int endDayOffset = displayStart.date().isValid() && displayEnd.date().isValid()
                                 ? displayStart.date().daysTo(displayEnd.date())
                                 : 0;
    event->setDtStart(editedDateTime(date, start, oldStart));
    event->setDtEnd(editedDateTime(date.addDays(endDayOffset), end, oldEnd));
}

QTime startTime(const KCalendarCore::Event::Ptr &event)
{
    return event->dtStart().time();
}

QString startTimeString(const KCalendarCore::Event::Ptr &event)
{
    return startTime(event).toString();
}

void setStartTime(const KCalendarCore::Event::Ptr &event, const QTime &startTime)
{
    if (event->allDay())
    {
        return;
    }

    const QDateTime oldStart = event->dtStart();
    const QDate date =
        displayDateTime(oldStart).date().isValid() ? displayDateTime(oldStart).date() : QDate::currentDate();
    event->setDtStart(editedDateTime(date, startTime, oldStart));
}

QTime endTime(const KCalendarCore::Event::Ptr &event)
{
    const QDateTime end = event->dtEnd().isValid() ? event->dtEnd() : event->dtStart();
    return end.time();
}

QString endTimeString(const KCalendarCore::Event::Ptr &event)
{
    return endTime(event).toString();
}

void setEndTime(const KCalendarCore::Event::Ptr &event, const QTime &endTime)
{
    if (event->allDay())
    {
        return;
    }

    const QDateTime oldEnd = event->dtEnd().isValid() ? event->dtEnd() : event->dtStart();
    const QDate date = displayDateTime(oldEnd).date().isValid()             ? displayDateTime(oldEnd).date()
                       : displayDateTime(event->dtStart()).date().isValid() ? displayDateTime(event->dtStart()).date()
                                                                            : QDate::currentDate();
    event->setDtEnd(editedDateTime(date, endTime, oldEnd));
}

int isFullDay(const KCalendarCore::Event::Ptr &event)
{
    return event->allDay() ? 1 : 0;
}

void setFullDay(const KCalendarCore::Event::Ptr &event, int isFullDay)
{
    const bool wasFullDay = event->allDay();
    const QDateTime oldStart = event->dtStart();
    const QDateTime displayStart = displayDateTime(oldStart);
    const QDate date = wasFullDay && oldStart.date().isValid() ? oldStart.date()
                       : displayStart.date().isValid()         ? displayStart.date()
                                                               : QDate::currentDate();
    if (isFullDay == 1)
    {
        const int durationDays = allDayDurationDays(oldStart, event->dtEnd(), wasFullDay);
        event->setAllDay(true);
        event->setDtStart(allDayDateTime(date, oldStart));
        event->setDtEnd(allDayDateTime(date.addDays(durationDays), oldStart));
    }
    else
    {
        event->setAllDay(false);
        if (wasFullDay)
        {
            event->setDtStart(editedDateTime(date, QTime(0, 0), oldStart));
            event->setDtEnd(editedDateTime(date, QTime(0, 0), oldStart));
            return;
        }
        if (!event->dtEnd().isValid())
        {
            event->setDtEnd(event->dtStart());
        }
    }
}

} // namespace EventDateTime
