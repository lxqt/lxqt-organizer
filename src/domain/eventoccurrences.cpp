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

#include "eventoccurrences.h"

#include "eventdatetime.h"

#include <QTimeZone>

#include <KCalendarCore/OccurrenceIterator>

#include <optional>

namespace {

QTimeZone calendarTimeZone(const KCalendarCore::MemoryCalendar::Ptr &calendar)
{
    return calendar->timeZone().isValid() ? calendar->timeZone() : QTimeZone::systemTimeZone();
}

EventOccurrence occurrenceFromEvent(const KCalendarCore::Event::Ptr &event,
                                    const ItemRef &storage,
                                    const QString &fallbackUid,
                                    const QDateTime &start,
                                    const QDateTime &end,
                                    const QTimeZone &displayTimeZone,
                                    const std::optional<QDateTime> &recurrenceId = std::nullopt)
{
    EventOccurrence occurrence;
    occurrence.ref.item = storage;
    occurrence.ref.uid = event->uid().isEmpty() ? fallbackUid : event->uid();
    occurrence.start = EventDateTime::displayDateTime(start, displayTimeZone);
    occurrence.end = EventDateTime::displayDateTime(end.isValid() ? end : start, displayTimeZone);
    occurrence.display = CalendarSnapshot::eventDisplayFromEvent(event, {}, occurrence.start, occurrence.end);
    occurrence.recurrenceId = recurrenceId;
    return occurrence;
}

std::optional<QDateTime> recurrenceIdFromDateTime(const QDateTime &recurrenceId)
{
    return recurrenceId.isValid() ? std::optional<QDateTime>(recurrenceId) : std::nullopt;
}

QDateTime rangeStartDateTime(const QDate &date, const QTimeZone &timeZone)
{
    return QDateTime(date, QTime(0, 0), timeZone);
}

QDateTime rangeEndDateTime(const QDate &date, const QTimeZone &timeZone)
{
    return QDateTime(date, QTime(23, 59, 59, 999), timeZone);
}

QDate occurrenceDisplayEndDate(const EventOccurrence &event)
{
    if (!event.end.isValid())
    {
        return event.start.date();
    }
    if (event.end.date() > event.start.date() && event.end.time() == QTime(0, 0))
    {
        return event.end.date().addDays(-1);
    }
    return event.end.date();
}

bool occurrenceOverlapsRange(const EventOccurrence &occurrence, const QDate &rangeStart, const QDate &rangeEnd)
{
    if (!rangeStart.isValid() && !rangeEnd.isValid())
    {
        return true;
    }
    if (!occurrence.start.date().isValid())
    {
        return false;
    }

    const QDate occurrenceEnd =
        occurrenceDisplayEndDate(occurrence).isValid() ? occurrenceDisplayEndDate(occurrence) : occurrence.start.date();
    if (rangeStart.isValid() && occurrenceEnd < rangeStart)
    {
        return false;
    }
    if (rangeEnd.isValid() && occurrence.start.date() > rangeEnd)
    {
        return false;
    }
    return true;
}

bool hasEditableEventTimes(const KCalendarCore::Event::Ptr &event)
{
    if (event->allDay())
    {
        return true;
    }
    const QDateTime start = event->dtStart();
    const QDateTime end = event->dtEnd();
    return start.isValid() && end.isValid();
}

QList<EventOccurrence> eventSnapshots(const KCalendarCore::MemoryCalendar::Ptr &calendar,
                                      const ItemRef &storage,
                                      const QString &uid,
                                      const QDate &rangeStart,
                                      const QDate &rangeEnd)
{
    QList<EventOccurrence> results;
    if (!rangeStart.isValid() || !rangeEnd.isValid() || rangeEnd < rangeStart)
    {
        return results;
    }

    const QTimeZone displayTimeZone = calendarTimeZone(calendar);
    for (const KCalendarCore::Event::Ptr &event : calendar->events())
    {
        if (event->hasRecurrenceId() || !hasEditableEventTimes(event))
        {
            continue;
        }
        if (event->recurs())
        {
            KCalendarCore::OccurrenceIterator occurrence(*calendar,
                                                         event.dynamicCast<KCalendarCore::Incidence>(),
                                                         rangeStartDateTime(rangeStart, displayTimeZone),
                                                         rangeEndDateTime(rangeEnd, displayTimeZone));
            while (occurrence.hasNext())
            {
                occurrence.next();
                const KCalendarCore::Event::Ptr occurrenceEvent =
                    occurrence.incidence().dynamicCast<KCalendarCore::Event>();
                if (occurrenceEvent.isNull())
                {
                    continue;
                }
                const QDateTime start = occurrence.occurrenceStartDate();
                const QDateTime end = occurrence.occurrenceEndDate().isValid() ? occurrence.occurrenceEndDate() : start;
                const EventOccurrence snapshot =
                    occurrenceFromEvent(occurrenceEvent,
                                        storage,
                                        uid,
                                        start,
                                        end,
                                        displayTimeZone,
                                        recurrenceIdFromDateTime(occurrence.recurrenceId()));
                if (occurrenceOverlapsRange(snapshot, rangeStart, rangeEnd))
                {
                    results.append(snapshot);
                }
            }
            continue;
        }
        const EventOccurrence occurrence =
            occurrenceFromEvent(event,
                                storage,
                                uid,
                                event->dtStart(),
                                event->dtEnd().isValid() ? event->dtEnd() : event->dtStart(),
                                displayTimeZone);
        if (occurrenceOverlapsRange(occurrence, rangeStart, rangeEnd))
        {
            results.append(occurrence);
        }
    }
    return results;
}

} // namespace

namespace EventOccurrences {

QDate displayEndDate(const EventOccurrence &event)
{
    if (!event.end.isValid())
    {
        return event.start.date();
    }
    if (event.end.date() > event.start.date() && event.end.time() == QTime(0, 0))
    {
        return event.end.date().addDays(-1);
    }
    return event.end.date();
}

bool overlapsDate(const EventOccurrence &event, const QDate &date)
{
    if (!date.isValid() || !event.start.date().isValid())
    {
        return false;
    }

    const QDate lastDate = displayEndDate(event);
    return event.start.date() <= date && (!lastDate.isValid() || lastDate >= date);
}

QList<EventOccurrence> occurrenceSnapshots(const KCalendarCore::MemoryCalendar::Ptr &calendar,
                                           const ItemRef &storage,
                                           const QString &uid,
                                           const QDate &rangeStart,
                                           const QDate &rangeEnd)
{
    return eventSnapshots(calendar, storage, uid, rangeStart, rangeEnd);
}

KCalendarCore::Event::Ptr editableSingleEvent(const KCalendarCore::MemoryCalendar::Ptr &calendar)
{
    const KCalendarCore::Event::List events = calendar->events();
    if (events.size() != 1 || !hasEditableEventTimes(events.constFirst()))
    {
        return {};
    }
    return events.constFirst();
}

bool canEditAsEvent(const KCalendarCore::MemoryCalendar::Ptr &calendar)
{
    return !editableSingleEvent(calendar).isNull();
}

} // namespace EventOccurrences
