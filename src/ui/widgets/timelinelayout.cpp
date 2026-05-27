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

#include "timelinelayout.h"

#include <QVector>

#include <algorithm>
#include <utility>

namespace {

int roundedDownHour(int minutes)
{
    return qBound(0, minutes / 60 * 60, TimelineLayout::minutesPerDay - 60);
}

int roundedUpHour(int minutes)
{
    return qBound(60, ((minutes + 59) / 60) * 60, TimelineLayout::minutesPerDay);
}

QTime eventStartTime(const EventDisplay &event)
{
    return event.start.time();
}

QTime eventEndTime(const EventDisplay &event)
{
    return event.end.time();
}

int eventStartMinutes(const EventDisplay &event, const QDate &date)
{
    if (date.isValid() && event.start.date().isValid() && event.start.date() < date)
    {
        return 0;
    }

    const QTime start = eventStartTime(event);
    return start.isValid() ? start.hour() * 60 + start.minute() : 0;
}

int eventEndMinutes(const EventDisplay &event, const QDate &date)
{
    if (date.isValid() && event.end.date().isValid() && event.end.date() > date)
    {
        return TimelineLayout::minutesPerDay;
    }

    const int startMinutes = eventStartMinutes(event, date);
    const QTime end = eventEndTime(event);
    const int endMinutes = end.isValid() ? end.hour() * 60 + end.minute() : startMinutes + 30;
    const int minimumEndMinutes =
        qMin(startMinutes + TimelineLayout::minimumDurationMinutes, TimelineLayout::minutesPerDay);
    return qBound(minimumEndMinutes, endMinutes, TimelineLayout::minutesPerDay);
}

} // namespace

namespace TimelineLayout {

int configuredStartMinutes(const Metrics &metrics)
{
    return qBound(0, metrics.dayViewStartHour, 23) * 60;
}

int configuredEndMinutes(const Metrics &metrics)
{
    return qBound(metrics.dayViewStartHour + 1, metrics.dayViewFinishHour, 24) * 60;
}

int visibleStartMinutes(const Metrics &metrics)
{
    return metrics.effectiveStartMinutes >= 0 ? metrics.effectiveStartMinutes : configuredStartMinutes(metrics);
}

int visibleEndMinutes(const Metrics &metrics)
{
    return metrics.effectiveEndMinutes >= 0 ? metrics.effectiveEndMinutes : configuredEndMinutes(metrics);
}

int timeToY(const QTime &time, const Metrics &metrics)
{
    const QTime validTime = time.isValid() ? time : QTime(0, 0);
    const int minutes = validTime.hour() * 60 + validTime.minute();
    return timeToYMinutes(minutes, metrics);
}

int timeToYMinutes(int minutes, const Metrics &metrics)
{
    const int bounded = qBound(visibleStartMinutes(metrics), minutes, visibleEndMinutes(metrics));
    return topPadding + qRound((bounded - visibleStartMinutes(metrics)) * metrics.hourHeight / 60.0);
}

int yToMinutes(int y, const Metrics &metrics)
{
    const int minutes = visibleStartMinutes(metrics) + qRound((y - topPadding) * 60.0 / metrics.hourHeight);
    return qBound(visibleStartMinutes(metrics), snapToGranularity(minutes, metrics), visibleEndMinutes(metrics));
}

int timeToMinutes(const QTime &time)
{
    const QTime validTime = time.isValid() ? time : QTime(0, 0);
    return validTime.hour() * 60 + validTime.minute();
}

QTime minutesToTime(int minutes)
{
    const int bounded = qBound(0, minutes, minutesPerDay);
    if (bounded == minutesPerDay)
    {
        return QTime(23, 59, 59);
    }
    return QTime(bounded / 60, bounded % 60);
}

int snapToGranularity(int minutes, const Metrics &metrics)
{
    const int granularity = qMax(1, metrics.snapMinutes);
    return qRound(minutes / static_cast<qreal>(granularity)) * granularity;
}

int effectiveStartMinutes(const QList<EventTimelineRow> &events, const QDate &date, const Metrics &metrics)
{
    int start = configuredStartMinutes(metrics);
    for (const EventTimelineRow &event : events)
    {
        start = qMin(start, roundedDownHour(eventStartMinutes(event.display, date)));
    }
    return qBound(0, start, minutesPerDay - minimumDurationMinutes);
}

int effectiveEndMinutes(const QList<EventTimelineRow> &events, const QDate &date, const Metrics &metrics)
{
    int end = configuredEndMinutes(metrics);
    for (const EventTimelineRow &event : events)
    {
        end = qMax(end, roundedUpHour(eventEndMinutes(event.display, date)));
    }
    const int start = effectiveStartMinutes(events, date, metrics);
    return qBound(start + minimumDurationMinutes, end, minutesPerDay);
}

Metrics resolvedMetrics(const QList<EventTimelineRow> &events, const QDate &date, const Metrics &metrics)
{
    Metrics resolved = metrics;
    resolved.effectiveStartMinutes = effectiveStartMinutes(events, date, metrics);
    resolved.effectiveEndMinutes = effectiveEndMinutes(events, date, metrics);
    return resolved;
}

int timelineHeight(const Metrics &metrics)
{
    return topPadding +
           qRound((visibleEndMinutes(metrics) - visibleStartMinutes(metrics)) * metrics.hourHeight / 60.0) +
           bottomPadding;
}

QList<LaidOutEvent> layoutEvents(const QList<EventTimelineRow> &events, const QDate &date)
{
    QList<LaidOutEvent> sortedEvents;
    sortedEvents.reserve(events.size());
    for (const EventTimelineRow &event : events)
    {
        LaidOutEvent item;
        item.event = event;
        item.startMinutes = eventStartMinutes(event.display, date);
        item.endMinutes = eventEndMinutes(event.display, date);
        sortedEvents.append(item);
    }
    std::stable_sort(sortedEvents.begin(), sortedEvents.end(), [](const LaidOutEvent &left, const LaidOutEvent &right) {
        return left.startMinutes < right.startMinutes;
    });

    QList<LaidOutEvent> laidOut;
    QList<LaidOutEvent> group;
    int groupEnd = -1;

    auto flushGroup = [&laidOut, &group]() {
        if (group.isEmpty())
        {
            return;
        }

        QVector<int> columnEnds;
        int maxColumns = 1;
        for (LaidOutEvent &item : group)
        {
            int column = 0;
            while (column < columnEnds.size() && columnEnds.at(column) > item.startMinutes)
            {
                ++column;
            }
            if (column == columnEnds.size())
            {
                columnEnds.append(item.endMinutes);
            }
            else
            {
                columnEnds[column] = item.endMinutes;
            }
            item.column = column;
            maxColumns = qMax(maxColumns, columnEnds.size());
        }

        for (LaidOutEvent &item : group)
        {
            item.columnCount = maxColumns;
            laidOut.append(item);
        }
        group.clear();
    };

    for (const LaidOutEvent &item : std::as_const(sortedEvents))
    {
        if (!group.isEmpty() && item.startMinutes >= groupEnd)
        {
            flushGroup();
            groupEnd = -1;
        }

        group.append(item);
        groupEnd = qMax(groupEnd, item.endMinutes);
    }
    flushGroup();

    return laidOut;
}

} // namespace TimelineLayout
