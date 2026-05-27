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

#ifndef TIMELINEPAINTER_H
#define TIMELINEPAINTER_H

#include "timelinelayout.h"

#include <QColor>
#include <QFont>
#include <QList>
#include <QLocale>
#include <QPalette>
#include <QPainter>
#include <QRect>
#include <QString>

#include <functional>
#include <optional>
#include <utility>

class TimelinePainter
{
public:
    struct Context
    {
        const QList<EventTimelineRow> *events = nullptr;
        const QList<TimelineLayout::LaidOutEvent> *laidOutEvents = nullptr;
        const TimelineLayout::Metrics *metrics = nullptr;
        const QPalette *palette = nullptr;
        const QFont *font = nullptr;
        const QLocale *locale = nullptr;
        QRect viewport;
        int scrollOffset = 0;
        QDate date;
        std::optional<QTime> nowMarker;
        std::optional<std::pair<QTime, QTime>> createPreview;
        QString createPreviewText;
        EventOccurrence selectedEvent;
        std::function<QColor(const EventDisplay &)> eventColorFn;
        std::function<QString(const EventDisplay &)> eventTextFn;
        std::function<QString(const EventDisplay &)> eventTimeTextFn;
    };

    struct PaintedEvent
    {
        QRect rect;
        EventTimelineRow event;
    };

    static QList<PaintedEvent> paint(QPainter &painter, const Context &ctx);
};

#endif // TIMELINEPAINTER_H
