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

#include "monthcalendarwidget.h"

#include <QAbstractItemView>
#include <QContextMenuEvent>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QShowEvent>

MonthCalendarWidget::MonthCalendarWidget(QWidget *parent)
    : QCalendarWidget(parent)
{
    installCalendarViewEventFilters();
}

void MonthCalendarWidget::setDateIndicators(const QHash<QDate, DateIndicator> &indicators)
{
    if (m_indicators == indicators)
    {
        return;
    }

    m_indicators = indicators;
    updateCells();
}

bool MonthCalendarWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() != QEvent::ContextMenu && event->type() != QEvent::MouseButtonPress)
    {
        return QCalendarWidget::eventFilter(watched, event);
    }

    QAbstractItemView *view = nullptr;
    if (QWidget *viewport = qobject_cast<QWidget *>(watched))
    {
        view = qobject_cast<QAbstractItemView *>(viewport->parentWidget());
    }
    if (!view)
    {
        return QCalendarWidget::eventFilter(watched, event);
    }

    QPoint pos;
    QPoint globalPos;
    if (event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() != Qt::RightButton)
        {
            return QCalendarWidget::eventFilter(watched, event);
        }
        pos = mouseEvent->pos();
        globalPos = mouseEvent->globalPosition().toPoint();
    }
    else
    {
        if (m_contextMenuOpenedOnPress)
        {
            m_contextMenuOpenedOnPress = false;
            return true;
        }
        QContextMenuEvent *contextEvent = static_cast<QContextMenuEvent *>(event);
        pos = contextEvent->pos();
        globalPos = contextEvent->globalPos();
    }

    const QModelIndex index = view->indexAt(pos);
    if (!index.isValid())
    {
        return QCalendarWidget::eventFilter(watched, event);
    }

    const QDate date = dateForCell(index.row(), index.column());
    if (!date.isValid())
    {
        return QCalendarWidget::eventFilter(watched, event);
    }

    setSelectedDate(date);
    m_contextMenuOpenedOnPress = event->type() == QEvent::MouseButtonPress;
    Q_EMIT dateContextMenuRequested(date, globalPos);
    return true;
}

void MonthCalendarWidget::paintCell(QPainter *painter, const QRect &rect, QDate date) const
{
    QCalendarWidget::paintCell(painter, rect, date);

    const DateIndicator indicator = m_indicators.value(date);
    const int count = indicator.count;
    if (count <= 0)
    {
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const int diameter = qMin(rect.width(), rect.height()) / 8;
    const int dotSize = qBound(4, diameter, 8);
    const QPoint center(rect.center().x(), rect.bottom() - dotSize - 4);
    const QColor normalColor = palette().color(QPalette::Highlight);

    painter->setPen(Qt::NoPen);
    painter->setBrush(normalColor);
    painter->drawEllipse(center, dotSize, dotSize);

    if (count > 1)
    {
        const QString text = count > 9 ? QStringLiteral("9+") : QString::number(count);
        QFont font = painter->font();
        font.setBold(true);
        font.setPointSizeF(qMax(6.0, font.pointSizeF() * 0.65));
        painter->setFont(font);
        painter->setPen(palette().color(QPalette::HighlightedText));
        const QRect textRect(center.x() - dotSize, center.y() - dotSize, dotSize * 2, dotSize * 2);
        painter->drawText(textRect, Qt::AlignCenter, text);
    }

    painter->restore();
}

void MonthCalendarWidget::showEvent(QShowEvent *event)
{
    installCalendarViewEventFilters();
    QCalendarWidget::showEvent(event);
}

QDate MonthCalendarWidget::dateForCell(int row, int column) const
{
    const int dateRow = row - (horizontalHeaderFormat() == QCalendarWidget::NoHorizontalHeader ? 0 : 1);
    const int dateColumn = column - (verticalHeaderFormat() == QCalendarWidget::NoVerticalHeader ? 0 : 1);
    if (dateRow < 0 || dateRow >= 6 || dateColumn < 0 || dateColumn >= 7)
    {
        return QDate();
    }

    const QDate firstOfMonth(yearShown(), monthShown(), 1);
    if (!firstOfMonth.isValid())
    {
        return QDate();
    }

    const int firstColumn = (firstOfMonth.dayOfWeek() - firstDayOfWeek() + 7) % 7;
    return firstOfMonth.addDays((dateRow * 7) + dateColumn - firstColumn);
}

void MonthCalendarWidget::installCalendarViewEventFilters()
{
    const QList<QAbstractItemView *> views = findChildren<QAbstractItemView *>();
    for (QAbstractItemView *view : views)
    {
        if (QWidget *viewport = view->viewport())
        {
            viewport->installEventFilter(this);
        }
    }
}
