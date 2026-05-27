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

#ifndef DAYTIMELINEWIDGET_H
#define DAYTIMELINEWIDGET_H

#include "collectionsummary.h"
#include "timelinelayout.h"
#include "timelinepainter.h"

#include <QAbstractScrollArea>
#include <QDate>
#include <QList>
#include <QLocale>
#include <QRect>
#include <QTime>

class QLabel;

class DayTimelineWidget : public QAbstractScrollArea
{
    Q_OBJECT

public:
    explicit DayTimelineWidget(QWidget *parent = nullptr);

    void setCollectionSummaryProvider(CollectionSummaryProvider provider);
    void setEvents(const QList<EventOccurrence> &events);
    void setDate(const QDate &date);
    void setLocale(const QLocale &locale);
    void setDayViewTimeRange(int startHour, int finishHour);
    EventOccurrence selectedEvent() const;
    bool selectEvent(const EventOccurrence &event);
    void clearSelection();

Q_SIGNALS:
    void eventActivated(const EventOccurrence &event);
    void eventSelected(const EventOccurrence &event);
    void contextMenuRequested(const EventOccurrence &event,
                              const QTime &startTime,
                              const QTime &endTime,
                              const QPoint &globalPos);
    void eventTimeChangeRequested(const EventOccurrence &event, const QTime &startTime, const QTime &endTime);
    void eventCreationRequested(const QTime &startTime, const QTime &endTime);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    enum class DragMode
    {
        None,
        Move,
        ResizeStart,
        ResizeEnd,
        Create
    };

    int hourHeight() const;
    int timelineHeight() const;
    TimelineLayout::Metrics layoutMetrics() const;
    TimelineLayout::Metrics resolvedLayoutMetrics() const;
    TimelineLayout::Metrics resolvedLayoutMetrics(const QList<EventTimelineRow> &events) const;
    DragMode dragModeAt(const TimelinePainter::PaintedEvent &painted, const QPoint &point) const;
    bool sameEvent(const EventOccurrence &first, const EventOccurrence &second) const;
    void updateEventPreview(const EventOccurrence &event, const QTime &startTime, const QTime &endTime);
    QColor eventColor(const EventDisplay &event) const;
    QString eventText(const EventDisplay &event) const;
    QString eventTimeText(const EventDisplay &event) const;
    QList<EventTimelineRow> timedEvents() const;
    int headerHeight() const;
    void updateHeader();
    void updateScrollBars();
    CollectionSummary collectionSummary(const QString &collectionId) const;

    QList<EventTimelineRow> m_events;
    QList<TimelinePainter::PaintedEvent> m_paintedEvents;
    QDate m_date;
    QLocale m_locale;
    int m_dayViewStartHour = 0;
    int m_dayViewFinishHour = 24;
    DragMode m_dragMode = DragMode::None;
    EventOccurrence m_dragEvent;
    EventOccurrence m_selectedEvent;
    QTime m_dragOriginalStart;
    QTime m_dragOriginalEnd;
    int m_dragStartY = 0;
    QTime m_createStartTime;
    QTime m_createEndTime;
    bool m_initialScrollApplied = false;
    QLabel *m_headerLabel = nullptr;
    CollectionSummaryProvider m_collectionSummaryProvider;
};

#endif // DAYTIMELINEWIDGET_H
