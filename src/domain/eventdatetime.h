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

#ifndef EVENTDATETIME_H
#define EVENTDATETIME_H

#include "highlightmode.h"

#include <QDate>
#include <QDateTime>
#include <QTime>

#include <KCalendarCore/Event>
#include <KCalendarCore/Incidence>
#include <KCalendarCore/Todo>

class QTimeZone;

namespace EventDateTime {

KCalendarCore::Event::Ptr createEvent();

QDate parseDate(const QString &value);
QTime parseTime(const QString &value);
int normalizedPriority(int priority);

QDateTime displayDateTime(const QDateTime &dateTime);
QDateTime displayDateTime(const QDateTime &dateTime, const QTimeZone &displayTimeZone);

QDate date(const KCalendarCore::Event::Ptr &event);
QString dateString(const KCalendarCore::Event::Ptr &event);
void setDate(const KCalendarCore::Event::Ptr &event, const QDate &date);

QTime startTime(const KCalendarCore::Event::Ptr &event);
QString startTimeString(const KCalendarCore::Event::Ptr &event);
void setStartTime(const KCalendarCore::Event::Ptr &event, const QTime &startTime);

QTime endTime(const KCalendarCore::Event::Ptr &event);
QString endTimeString(const KCalendarCore::Event::Ptr &event);
void setEndTime(const KCalendarCore::Event::Ptr &event, const QTime &endTime);

int isFullDay(const KCalendarCore::Event::Ptr &event);
void setFullDay(const KCalendarCore::Event::Ptr &event, int isFullDay);

} // namespace EventDateTime

#endif // EVENTDATETIME_H
