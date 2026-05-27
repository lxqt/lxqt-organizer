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

#include "eventpatcher.h"

#include "eventdatetime.h"
#include "incidencefields.h"
#include "eventtimezone.h"

#include <QTimeZone>
#include <QUrl>

#include <KCalendarCore/Event>
#include <KCalendarCore/Recurrence>

namespace EventPatcher {

namespace {

Options noOptions()
{
    Options options;
    forEachOption(options, [](bool &option) { option = false; });
    return options;
}

QDateTime dateTimeForComparison(const QDateTime &dateTime, const QString &displayTimeZoneId)
{
    if (!dateTime.isValid())
    {
        return {};
    }
    const QTimeZone displayTimeZone(displayTimeZoneId.trimmed().toUtf8());
    if (!displayTimeZone.isValid() || dateTime.timeSpec() == Qt::LocalTime)
    {
        return dateTime;
    }
    return dateTime.toTimeZone(displayTimeZone);
}

} // namespace

Options editableOptions()
{
    Options options;
    options.attachment = true;
    options.dateTime = true;
    options.timeZone = true;
    options.summary = true;
    options.location = true;
    options.description = true;
    options.url = true;
    options.priority = true;
    options.highlightMode = true;
    options.completed = true;
    return options;
}

bool dateTimeFieldsChanged(const EventFields &patch, const QDateTime &start, const QDateTime &end, bool allDay)
{
    const QDateTime effectiveEnd = end.isValid() ? end : start;
    if (patch.allDay != allDay)
    {
        return true;
    }
    if (patch.date != start.date())
    {
        return true;
    }
    if (!allDay && patch.startTime != start.time())
    {
        return true;
    }
    return !allDay && patch.endTime != effectiveEnd.time();
}

Options changedOptionsForEvent(const KCalendarCore::Event::Ptr &event, const EventFields &patch)
{
    Options options = noOptions();

    // Free text is compared exactly; URI-like fields keep their existing normalization below.
    options.summary = patch.title != event->summary();
    options.location = patch.location != event->location();
    options.description = patch.description != event->description();
    options.url = QUrl(patch.url.trimmed()) != event->url();
    options.attachment = patch.attachmentUri != CalendarUtils::attachmentUri(event).trimmed();
    options.dateTime = dateTimeFieldsChanged(patch,
                                             dateTimeForComparison(event->dtStart(), patch.displayTimeZoneId),
                                             dateTimeForComparison(event->dtEnd(), patch.displayTimeZoneId),
                                             event->allDay());
    options.timeZone = patch.timeZoneId != EventTimeZone::timeZoneId(event);
    options.highlightMode =
        patch.highlightMode != CalendarUtils::highlightMode(event.dynamicCast<KCalendarCore::Incidence>());
    options.priority = patch.priority != event->priority();
    options.completed = patch.completed != CalendarUtils::isEventCompleted(event);
    return options;
}

bool hasChanges(const Options &options)
{
    bool changed = false;
    forEachOption(options, [&changed](bool option) { changed = changed || option; });
    return changed;
}

void applyToEvent(const KCalendarCore::Event::Ptr &event, const EventFields &patch, Options options)
{
    if (options.summary)
    {
        event->setSummary(patch.title);
    }
    if (options.location)
    {
        event->setLocation(patch.location);
    }
    if (options.description)
    {
        event->setDescription(patch.description);
    }
    if (options.url)
    {
        CalendarUtils::setUrl(event, QUrl(patch.url.trimmed()));
    }
    if (options.attachment)
    {
        CalendarUtils::setAttachmentUri(event, patch.attachmentUri);
    }
    const bool applyTimeZoneBeforeDateTime =
        options.dateTime && options.timeZone &&
        dateTimeFieldsChanged(patch, event->dtStart(), event->dtEnd(), event->allDay());
    if (applyTimeZoneBeforeDateTime)
    {
        EventTimeZone::setTimeZoneId(event, patch.timeZoneId);
    }
    if (options.dateTime)
    {
        EventDateTime::setFullDay(event, patch.allDay ? 1 : 0);
        EventDateTime::setDate(event, patch.date);
        EventDateTime::setStartTime(event, patch.startTime);
        EventDateTime::setEndTime(event, patch.endTime);
    }
    if (options.timeZone && !applyTimeZoneBeforeDateTime)
    {
        EventTimeZone::setTimeZoneId(event, patch.timeZoneId);
    }
    if (options.highlightMode)
    {
        CalendarUtils::setHighlightMode(event.dynamicCast<KCalendarCore::Incidence>(), patch.highlightMode);
    }
    if (options.priority)
    {
        event->setPriority(patch.priority);
    }
    if (options.completed)
    {
        CalendarUtils::setEventCompleted(event, patch.completed);
    }
    if (options.alarms)
    {
        event->clearAlarms();
    }
    if (options.recurrence && event->recurrence())
    {
        event->recurrence()->unsetRecurs();
    }
}

} // namespace EventPatcher
