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

#ifndef INCIDENCEFIELDS_H
#define INCIDENCEFIELDS_H

#include "highlightmode.h"

#include <QDate>
#include <QString>
#include <QUrl>

#include <KCalendarCore/Event>
#include <KCalendarCore/Incidence>
#include <KCalendarCore/Todo>

namespace CalendarUtils {

QString summary(const KCalendarCore::Event::Ptr &event);
QString location(const KCalendarCore::Event::Ptr &event);
QString description(const KCalendarCore::Event::Ptr &event);

QUrl url(const KCalendarCore::Event::Ptr &event);
void setUrl(const KCalendarCore::Event::Ptr &event, const QUrl &url);

QString attachmentUri(const KCalendarCore::Event::Ptr &event);
void setAttachmentUri(const KCalendarCore::Event::Ptr &event, const QString &uri);

HighlightMode highlightMode(const KCalendarCore::Incidence::Ptr &incidence);
QString highlightModeString(HighlightMode mode);
void setHighlightMode(const KCalendarCore::Incidence::Ptr &incidence, HighlightMode mode);

bool isEventCompleted(const KCalendarCore::Event::Ptr &event);
void setEventCompleted(const KCalendarCore::Event::Ptr &event, bool completed);

bool isRollForwardEnabled(const KCalendarCore::Todo::Ptr &todo);
void setRollForwardEnabled(const KCalendarCore::Todo::Ptr &todo, bool enabled);
QDate originalDueDate(const KCalendarCore::Todo::Ptr &todo);
void setOriginalDueDate(const KCalendarCore::Todo::Ptr &todo, const QDate &date);

} // namespace CalendarUtils

#endif // INCIDENCEFIELDS_H
