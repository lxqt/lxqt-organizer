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

#ifndef TIMELINELAYOUT_H
#define TIMELINELAYOUT_H

#include "calendaritem.h"

#include <QDate>
#include <QList>
#include <QTime>

struct EventTimelineRow
{
    EventDisplay display;
    EventOccurrence edit;
};

namespace TimelineLayout {

constexpr int labelWidth = 72;
constexpr int leftGutter = 12;
constexpr int rightPadding = 10;
constexpr int topPadding = 8;
constexpr int bottomPadding = 8;
constexpr int minimumDurationMinutes = 15;
constexpr int minutesPerDay = 24 * 60;

struct Metrics
{
    int hourHeight = 48;
    int dayViewStartHour = 0;
    int dayViewFinishHour = 24;
    int snapMinutes = 15;
    int effectiveStartMinutes = -1;
    int effectiveEndMinutes = -1;
};

struct LaidOutEvent
{
    EventTimelineRow event;
    int startMinutes = 0;
    int endMinutes = 0;
    int column = 0;
    int columnCount = 1;
};

int configuredStartMinutes(const Metrics &metrics);
int configuredEndMinutes(const Metrics &metrics);
int visibleStartMinutes(const Metrics &metrics);
int visibleEndMinutes(const Metrics &metrics);
int timeToY(const QTime &time, const Metrics &metrics);
int timeToYMinutes(int minutes, const Metrics &metrics);
int yToMinutes(int y, const Metrics &metrics);
int timeToMinutes(const QTime &time);
QTime minutesToTime(int minutes);
int snapToGranularity(int minutes, const Metrics &metrics);
int effectiveStartMinutes(const QList<EventTimelineRow> &events, const QDate &date, const Metrics &metrics);
int effectiveEndMinutes(const QList<EventTimelineRow> &events, const QDate &date, const Metrics &metrics);
Metrics resolvedMetrics(const QList<EventTimelineRow> &events, const QDate &date, const Metrics &metrics);
int timelineHeight(const Metrics &metrics);

QList<LaidOutEvent> layoutEvents(const QList<EventTimelineRow> &events, const QDate &date);

} // namespace TimelineLayout

#endif // TIMELINELAYOUT_H
