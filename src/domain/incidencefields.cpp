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

#include "incidencefields.h"

#include <KCalendarCore/Attachment>

namespace {

constexpr auto CustomPropertyApp = "LXQT-ORGANIZER";
constexpr auto HighlightPropertyKey = "HILITE";
constexpr auto EventCompletedPropertyKey = "COMPLETED";
constexpr auto RollForwardPropertyKey = "ROLLFORWARD";
constexpr auto OriginalDuePropertyKey = "ORIGINAL-DUE";

} // namespace

namespace CalendarUtils {

QString summary(const KCalendarCore::Event::Ptr &event)
{
    return event->summary();
}

QString location(const KCalendarCore::Event::Ptr &event)
{
    return event->location();
}

QString description(const KCalendarCore::Event::Ptr &event)
{
    return event->description();
}

QUrl url(const KCalendarCore::Event::Ptr &event)
{
    return event->url();
}

void setUrl(const KCalendarCore::Event::Ptr &event, const QUrl &url)
{
    event->setUrl(url);
}

QString attachmentUri(const KCalendarCore::Event::Ptr &event)
{
    const KCalendarCore::Attachment::List attachments = event->attachments();
    for (const KCalendarCore::Attachment &attachment : attachments)
    {
        if (attachment.isUri())
        {
            return attachment.uri();
        }
    }
    return {};
}

void setAttachmentUri(const KCalendarCore::Event::Ptr &event, const QString &uri)
{
    const QString trimmed = uri.trimmed();
    const KCalendarCore::Attachment::List existing = event->attachments();
    event->clearAttachments();
    bool replaced = false;
    for (const KCalendarCore::Attachment &attachment : existing)
    {
        if (!replaced && attachment.isUri())
        {
            replaced = true;
            if (!trimmed.isEmpty())
            {
                KCalendarCore::Attachment updated = attachment;
                updated.setUri(trimmed);
                event->addAttachment(updated);
            }
            continue;
        }
        event->addAttachment(attachment);
    }
    if (!replaced && !trimmed.isEmpty())
    {
        event->addAttachment(KCalendarCore::Attachment(trimmed));
    }
}

HighlightMode highlightMode(const KCalendarCore::Incidence::Ptr &incidence)
{
    const QString value = incidence->customProperty(CustomPropertyApp, HighlightPropertyKey).toLower();
    if (value == QLatin1String("never"))
    {
        return HighlightMode::Never;
    }
    if (value == QLatin1String("future") || value == QLatin1String("expire"))
    {
        return HighlightMode::Future;
    }
    return HighlightMode::Always;
}

QString highlightModeString(HighlightMode mode)
{
    switch (mode)
    {
    case HighlightMode::Never:
        return QStringLiteral("never");
    case HighlightMode::Future:
        return QStringLiteral("future");
    case HighlightMode::Always:
    default:
        return QStringLiteral("always");
    }
}

void setHighlightMode(const KCalendarCore::Incidence::Ptr &incidence, HighlightMode mode)
{
    incidence->setCustomProperty(CustomPropertyApp, HighlightPropertyKey, highlightModeString(mode));
}

bool isEventCompleted(const KCalendarCore::Event::Ptr &event)
{
    const QString value = event->customProperty(CustomPropertyApp, EventCompletedPropertyKey).trimmed().toLower();
    return value == QLatin1String("true") || value == QLatin1String("yes") || value == QLatin1String("1");
}

void setEventCompleted(const KCalendarCore::Event::Ptr &event, bool completed)
{
    if (completed)
    {
        event->setCustomProperty(CustomPropertyApp, EventCompletedPropertyKey, QStringLiteral("TRUE"));
    }
    else
    {
        event->removeCustomProperty(CustomPropertyApp, EventCompletedPropertyKey);
    }
}

bool isRollForwardEnabled(const KCalendarCore::Todo::Ptr &todo)
{
    const QString value = todo->customProperty(CustomPropertyApp, RollForwardPropertyKey).trimmed().toLower();
    return value == QLatin1String("true") || value == QLatin1String("yes") || value == QLatin1String("1");
}

void setRollForwardEnabled(const KCalendarCore::Todo::Ptr &todo, bool enabled)
{
    if (enabled)
    {
        todo->setCustomProperty(CustomPropertyApp, RollForwardPropertyKey, QStringLiteral("TRUE"));
    }
    else
    {
        todo->removeCustomProperty(CustomPropertyApp, RollForwardPropertyKey);
        todo->removeCustomProperty(CustomPropertyApp, OriginalDuePropertyKey);
    }
}

QDate originalDueDate(const KCalendarCore::Todo::Ptr &todo)
{
    return QDate::fromString(todo->customProperty(CustomPropertyApp, OriginalDuePropertyKey), Qt::ISODate);
}

void setOriginalDueDate(const KCalendarCore::Todo::Ptr &todo, const QDate &date)
{
    if (date.isValid())
    {
        todo->setCustomProperty(CustomPropertyApp, OriginalDuePropertyKey, date.toString(Qt::ISODate));
    }
    else
    {
        todo->removeCustomProperty(CustomPropertyApp, OriginalDuePropertyKey);
    }
}

} // namespace CalendarUtils
