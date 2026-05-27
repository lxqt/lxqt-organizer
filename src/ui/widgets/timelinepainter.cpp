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

#include "timelinepainter.h"

#include <QPolygon>

namespace {

QColor blendedColor(const QColor &first, const QColor &second, qreal ratio)
{
    const qreal inverse = 1.0 - ratio;
    return QColor(qRound(first.red() * inverse + second.red() * ratio),
                  qRound(first.green() * inverse + second.green() * ratio),
                  qRound(first.blue() * inverse + second.blue() * ratio));
}

bool sameEvent(const EventOccurrence &first, const EventOccurrence &second)
{
    if (first.ref.item.href.isEmpty() || second.ref.item.href.isEmpty())
    {
        return false;
    }
    return first.ref.item.collectionId == second.ref.item.collectionId && first.ref.item.href == second.ref.item.href &&
           first.ref.uid == second.ref.uid && first.start == second.start;
}

} // namespace

QList<TimelinePainter::PaintedEvent> TimelinePainter::paint(QPainter &painter, const Context &ctx)
{
    QList<PaintedEvent> paintedEvents;
    if (!ctx.laidOutEvents || !ctx.metrics || !ctx.palette || !ctx.font || !ctx.locale)
    {
        return paintedEvents;
    }

    const QPalette &pal = *ctx.palette;
    const TimelineLayout::Metrics &metrics = *ctx.metrics;
    const QColor background = pal.color(QPalette::Window);
    const QColor base = pal.color(QPalette::Base);
    const QColor grid = pal.color(QPalette::Mid);
    const QColor lightGrid = pal.color(QPalette::Light);
    const QColor text = pal.color(QPalette::Text);
    const int width = ctx.viewport.width();
    const int gridLeft = TimelineLayout::labelWidth;
    const int gridRight = qMax(gridLeft, width - TimelineLayout::rightPadding);
    const int height = TimelineLayout::timelineHeight(metrics);

    painter.fillRect(ctx.viewport, base);
    painter.translate(0, -ctx.scrollOffset);

    const int firstMinute = TimelineLayout::visibleStartMinutes(metrics);
    const int lastMinute = TimelineLayout::visibleEndMinutes(metrics);
    painter.fillRect(0, 0, TimelineLayout::labelWidth, height, background);
    painter.fillRect(gridLeft, 0, gridRight - gridLeft, height, background);

    painter.setFont(*ctx.font);
    for (int minute = firstMinute; minute <= lastMinute; minute += 60)
    {
        const int y = TimelineLayout::timeToYMinutes(minute, metrics);
        painter.setPen(grid);
        painter.drawLine(gridLeft, y, gridRight, y);

        if (minute + 30 < lastMinute)
        {
            painter.setPen(QPen(lightGrid, 1, Qt::DotLine));
            painter.drawLine(gridLeft, y + metrics.hourHeight / 2, gridRight, y + metrics.hourHeight / 2);
        }

        if (minute < TimelineLayout::minutesPerDay)
        {
            painter.setPen(text);
            const QString time = ctx.locale->toString(TimelineLayout::minutesToTime(minute), QLocale::ShortFormat);
            painter.drawText(QRect(0,
                                   y - painter.fontMetrics().height() / 2,
                                   TimelineLayout::labelWidth - 4,
                                   painter.fontMetrics().height() + 2),
                             Qt::AlignRight | Qt::AlignVCenter,
                             time);
        }
    }

    if (ctx.createPreview.has_value() && ctx.createPreview->first.isValid() && ctx.createPreview->second.isValid())
    {
        const QRect createRect(gridLeft + 4,
                               TimelineLayout::timeToY(ctx.createPreview->first, metrics) + 2,
                               qMax(40, gridRight - gridLeft - 9),
                               qMax(painter.fontMetrics().height() + 8,
                                    TimelineLayout::timeToY(ctx.createPreview->second, metrics) -
                                        TimelineLayout::timeToY(ctx.createPreview->first, metrics) - 4));
        painter.setPen(QPen(pal.color(QPalette::Highlight), 1, Qt::DashLine));
        painter.setBrush(blendedColor(pal.color(QPalette::Highlight), base, 0.78));
        painter.drawRect(createRect.adjusted(0, 0, -1, -1));
        painter.setPen(text);
        painter.drawText(
            createRect.adjusted(6, 1, -6, -1),
            Qt::AlignLeft | Qt::AlignVCenter,
            painter.fontMetrics().elidedText(ctx.createPreviewText, Qt::ElideRight, createRect.width() - 12));
    }

    for (const TimelineLayout::LaidOutEvent &laidOut : *ctx.laidOutEvents)
    {
        const EventTimelineRow &event = laidOut.event;
        const int y1 = TimelineLayout::timeToYMinutes(laidOut.startMinutes, metrics);
        const int y2 =
            qMax(y1 + painter.fontMetrics().height() + 8, TimelineLayout::timeToYMinutes(laidOut.endMinutes, metrics));
        const int horizontalGap = laidOut.columnCount > 1 ? 3 : 0;
        const int availableWidth = qMax(40, gridRight - gridLeft - 9);
        const int columnWidth =
            qMax(36, (availableWidth - horizontalGap * (laidOut.columnCount - 1)) / laidOut.columnCount);
        const int x = gridLeft + 4 + laidOut.column * (columnWidth + horizontalGap);
        const int eventWidth = laidOut.column == laidOut.columnCount - 1 ? gridRight - x - 5 : columnWidth;
        const QRect rect(x, y1 + 2, qMax(36, eventWidth), y2 - y1 - 4);

        const QColor color = ctx.eventColorFn ? ctx.eventColorFn(event.display) : pal.color(QPalette::Highlight);
        const bool completed = event.display.completed;
        const bool selected = sameEvent(event.edit, ctx.selectedEvent);
        const QColor border = completed ? pal.color(QPalette::Mid) : color.darker(135);
        painter.setPen(QPen(selected ? pal.color(QPalette::Highlight) : border, selected ? 2 : 1));
        painter.setBrush(blendedColor(color, base, completed ? 0.86 : 0.68));
        painter.drawRect(rect.adjusted(0, 0, -1, -1));

        const QRect textRect = rect.adjusted(6, 3, -6, -3);
        QFont titleFont = painter.font();
        titleFont.setBold(true);
        painter.setFont(titleFont);
        painter.setPen(completed ? pal.color(QPalette::Disabled, QPalette::Text) : text);
        const int lineHeight = painter.fontMetrics().height();
        const QString title = ctx.eventTextFn ? ctx.eventTextFn(event.display) : QString();
        painter.drawText(QRect(textRect.left(), textRect.top(), textRect.width(), lineHeight),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         painter.fontMetrics().elidedText(title, Qt::ElideRight, textRect.width()));

        QFont timeFont = painter.font();
        timeFont.setBold(false);
        painter.setFont(timeFont);
        if (textRect.height() >= lineHeight * 2)
        {
            const QString timeText = ctx.eventTimeTextFn ? ctx.eventTimeTextFn(event.display) : QString();
            painter.drawText(QRect(textRect.left(), textRect.top() + lineHeight, textRect.width(), lineHeight),
                             Qt::AlignLeft | Qt::AlignVCenter,
                             painter.fontMetrics().elidedText(timeText, Qt::ElideRight, textRect.width()));
        }
        paintedEvents.append({rect, event});
    }

    if (ctx.nowMarker.has_value() && TimelineLayout::timeToMinutes(*ctx.nowMarker) >= firstMinute &&
        TimelineLayout::timeToMinutes(*ctx.nowMarker) <= lastMinute)
    {
        const int nowY = TimelineLayout::timeToY(*ctx.nowMarker, metrics);
        const QColor nowColor = pal.color(QPalette::Highlight);
        QColor nowLine = nowColor;
        nowLine.setAlpha(190);
        painter.setPen(Qt::NoPen);
        painter.setBrush(nowLine);
        const int markerLeft = gridLeft;
        const int markerTip = gridLeft + 10;
        const QPolygon marker({QPoint(markerLeft, nowY - 6), QPoint(markerLeft, nowY + 6), QPoint(markerTip, nowY)});
        painter.drawPolygon(marker);
        painter.setPen(QPen(nowLine, 2));
        painter.drawLine(markerTip, nowY, gridRight, nowY);
    }

    return paintedEvents;
}
