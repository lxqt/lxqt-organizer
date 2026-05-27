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

#include "daytimelinewidget.h"

#include <QColor>
#include <QContextMenuEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QScrollBar>
#include <QTime>

#include <algorithm>
#include <optional>
#include <utility>

namespace {

constexpr int resizeMargin = 7;

EventTimelineRow eventRow(const EventOccurrence &event, const CollectionSummary &collection)
{
    return {CalendarSnapshot::eventDisplay(event, collection), CalendarSnapshot::cloneEventOccurrence(event)};
}

QList<EventTimelineRow> eventRows(const QList<EventOccurrence> &events, const CollectionSummaryProvider &provider)
{
    QList<EventTimelineRow> rows;
    rows.reserve(events.size());
    for (const EventOccurrence &event : events)
    {
        const CollectionSummary collection =
            provider ? provider(event.ref.item.collectionId) : CollectionSummary{{}, {}};
        rows.append(eventRow(event, collection));
    }
    return rows;
}

QTime eventStartTime(const EventDisplay &event)
{
    return event.start.time();
}

QTime eventStartTime(const EventOccurrence &event)
{
    return event.start.time();
}

QTime eventEndTime(const EventDisplay &event)
{
    return event.end.time();
}

QTime eventEndTime(const EventOccurrence &event)
{
    return event.end.time();
}

bool isFullDayEvent(const EventDisplay &event)
{
    return event.allDay;
}

QDate eventDisplayEndDate(const EventDisplay &event)
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

bool eventOverlapsDate(const EventDisplay &event, const QDate &date)
{
    if (!date.isValid() || !event.start.date().isValid())
    {
        return false;
    }

    const QDate lastDate = eventDisplayEndDate(event);
    return event.start.date() <= date && (!lastDate.isValid() || lastDate >= date);
}

bool isMultiDayTimedEvent(const EventDisplay &event)
{
    if (isFullDayEvent(event) || !event.start.date().isValid())
    {
        return false;
    }

    const QDate displayEndDate = eventDisplayEndDate(event);
    return displayEndDate.isValid() && displayEndDate != event.start.date();
}

QColor colorOrDefault(const QString &color, const QColor &fallback)
{
    const QColor parsed(color);
    return parsed.isValid() ? parsed : fallback;
}

} // namespace

DayTimelineWidget::DayTimelineWidget(QWidget *parent)
    : QAbstractScrollArea(parent)
    , m_locale(QLocale::system())
{
    m_headerLabel = new QLabel(this);
    m_headerLabel->setAlignment(Qt::AlignCenter);
    m_headerLabel->setAutoFillBackground(true);
    QFont headerFont = m_headerLabel->font();
    headerFont.setBold(true);
    m_headerLabel->setFont(headerFont);
    setViewportMargins(0, headerHeight(), 0, 0);
    setFrameShape(QFrame::NoFrame);
    setFocusPolicy(Qt::StrongFocus);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setBackgroundRole(QPalette::Base);
    viewport()->setAutoFillBackground(true);
    viewport()->setMouseTracking(true);
    verticalScrollBar()->setSingleStep(24);
    updateHeader();
}

void DayTimelineWidget::setCollectionSummaryProvider(CollectionSummaryProvider provider)
{
    m_collectionSummaryProvider = std::move(provider);
    for (EventTimelineRow &row : m_events)
    {
        row.display = CalendarSnapshot::eventDisplay(row.edit, collectionSummary(row.edit.ref.item.collectionId));
    }
    viewport()->update();
}

void DayTimelineWidget::setEvents(const QList<EventOccurrence> &events)
{
    if (m_dragMode != DragMode::None)
    {
        m_dragMode = DragMode::None;
        m_dragEvent = EventOccurrence();
        m_dragOriginalStart = QTime();
        m_dragOriginalEnd = QTime();
        m_createStartTime = QTime();
        m_createEndTime = QTime();
        m_dragStartY = 0;
        viewport()->unsetCursor();
    }

    m_events = eventRows(events, m_collectionSummaryProvider);
    bool selectedStillVisible = false;
    for (const EventTimelineRow &event : m_events)
    {
        if (sameEvent(event.edit, m_selectedEvent))
        {
            selectedStillVisible = true;
            m_selectedEvent = CalendarSnapshot::cloneEventOccurrence(event.edit);
            break;
        }
    }
    if (!selectedStillVisible)
    {
        m_selectedEvent = EventOccurrence();
    }
    updateScrollBars();
    viewport()->update();
}

void DayTimelineWidget::setDate(const QDate &date)
{
    if (m_date == date)
    {
        return;
    }

    m_date = date;
    updateHeader();
    viewport()->update();
}

CollectionSummary DayTimelineWidget::collectionSummary(const QString &collectionId) const
{
    return m_collectionSummaryProvider ? m_collectionSummaryProvider(collectionId) : CollectionSummary{{}, {}};
}

void DayTimelineWidget::setLocale(const QLocale &locale)
{
    m_locale = locale;
    updateHeader();
    viewport()->update();
}

void DayTimelineWidget::setDayViewTimeRange(int startHour, int finishHour)
{
    const int boundedStart = qBound(0, startHour, 23);
    const int boundedFinish = qBound(boundedStart + 1, finishHour, 24);
    if (m_dayViewStartHour == boundedStart && m_dayViewFinishHour == boundedFinish)
    {
        return;
    }

    m_dayViewStartHour = boundedStart;
    m_dayViewFinishHour = boundedFinish;
    m_initialScrollApplied = false;
    updateScrollBars();
    viewport()->update();
}

EventOccurrence DayTimelineWidget::selectedEvent() const
{
    return CalendarSnapshot::cloneEventOccurrence(m_selectedEvent);
}

bool DayTimelineWidget::selectEvent(const EventOccurrence &event)
{
    for (const EventTimelineRow &candidate : std::as_const(m_events))
    {
        if (!sameEvent(candidate.edit, event))
        {
            continue;
        }

        m_selectedEvent = CalendarSnapshot::cloneEventOccurrence(candidate.edit);
        const int scrollTarget =
            qMax(0,
                 TimelineLayout::timeToY(eventStartTime(candidate.display), resolvedLayoutMetrics()) -
                     viewport()->height() / 4);
        verticalScrollBar()->setValue(scrollTarget);
        viewport()->update();
        Q_EMIT eventSelected(CalendarSnapshot::cloneEventOccurrence(m_selectedEvent));
        return true;
    }

    return false;
}

void DayTimelineWidget::clearSelection()
{
    m_selectedEvent = EventOccurrence();
    viewport()->update();
}

void DayTimelineWidget::contextMenuEvent(QContextMenuEvent *event)
{
    const QPoint point(event->pos().x(), event->pos().y() + verticalScrollBar()->value());
    for (const TimelinePainter::PaintedEvent &painted : std::as_const(m_paintedEvents))
    {
        if (!painted.rect.contains(point))
        {
            continue;
        }

        m_selectedEvent = CalendarSnapshot::cloneEventOccurrence(painted.event.edit);
        Q_EMIT eventSelected(CalendarSnapshot::cloneEventOccurrence(m_selectedEvent));
        viewport()->update();
        Q_EMIT contextMenuRequested(CalendarSnapshot::cloneEventOccurrence(painted.event.edit),
                                    eventStartTime(painted.event.display),
                                    eventEndTime(painted.event.display),
                                    event->globalPos());
        event->accept();
        return;
    }

    const TimelineLayout::Metrics metrics = resolvedLayoutMetrics();
    const int rangeStart = TimelineLayout::visibleStartMinutes(metrics);
    const int rangeEnd = TimelineLayout::visibleEndMinutes(metrics);
    const int startMinutes = qBound(
        rangeStart, TimelineLayout::yToMinutes(point.y(), metrics), rangeEnd - TimelineLayout::minimumDurationMinutes);
    const QTime startTime = TimelineLayout::minutesToTime(startMinutes);
    const QTime endTime = TimelineLayout::minutesToTime(qMin(rangeEnd, startMinutes + 60));
    m_selectedEvent = EventOccurrence();
    viewport()->update();
    Q_EMIT eventSelected(EventOccurrence());
    Q_EMIT contextMenuRequested(EventOccurrence(), startTime, endTime, event->globalPos());
    event->accept();
}

void DayTimelineWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (m_dragMode != DragMode::None)
    {
        return;
    }

    const QPoint point(event->pos().x(), event->pos().y() + verticalScrollBar()->value());
    for (const TimelinePainter::PaintedEvent &painted : std::as_const(m_paintedEvents))
    {
        if (painted.rect.contains(point))
        {
            Q_EMIT eventActivated(CalendarSnapshot::cloneEventOccurrence(painted.event.edit));
            return;
        }
    }

    QAbstractScrollArea::mouseDoubleClickEvent(event);
}

void DayTimelineWidget::mouseMoveEvent(QMouseEvent *event)
{
    const QPoint point(event->pos().x(), event->pos().y() + verticalScrollBar()->value());
    if (m_dragMode == DragMode::None)
    {
        DragMode hoverMode = DragMode::None;
        for (const TimelinePainter::PaintedEvent &painted : std::as_const(m_paintedEvents))
        {
            hoverMode = dragModeAt(painted, point);
            if (hoverMode != DragMode::None)
            {
                break;
            }
        }

        if (hoverMode == DragMode::ResizeStart || hoverMode == DragMode::ResizeEnd)
        {
            viewport()->setCursor(Qt::SizeVerCursor);
        }
        else if (hoverMode == DragMode::Move)
        {
            viewport()->setCursor(Qt::OpenHandCursor);
        }
        else
        {
            viewport()->unsetCursor();
        }
        QAbstractScrollArea::mouseMoveEvent(event);
        return;
    }

    if (m_dragMode == DragMode::Create)
    {
        const TimelineLayout::Metrics metrics = resolvedLayoutMetrics();
        const int rangeStart = TimelineLayout::visibleStartMinutes(metrics);
        const int rangeEnd = TimelineLayout::visibleEndMinutes(metrics);
        const int anchorMinutes = TimelineLayout::timeToMinutes(m_dragOriginalStart);
        const int currentMinutes = TimelineLayout::yToMinutes(point.y(), metrics);
        int startMinutes = qMin(anchorMinutes, currentMinutes);
        int endMinutes = qMax(anchorMinutes, currentMinutes);
        if (endMinutes - startMinutes < TimelineLayout::minimumDurationMinutes)
        {
            if (currentMinutes < anchorMinutes)
            {
                startMinutes = qMax(rangeStart, anchorMinutes - TimelineLayout::minimumDurationMinutes);
                endMinutes = anchorMinutes;
            }
            else
            {
                startMinutes = anchorMinutes;
                endMinutes = qMin(rangeEnd, anchorMinutes + TimelineLayout::minimumDurationMinutes);
            }
        }
        m_createStartTime = TimelineLayout::minutesToTime(
            qBound(rangeStart, startMinutes, rangeEnd - TimelineLayout::minimumDurationMinutes));
        m_createEndTime = TimelineLayout::minutesToTime(
            qBound(TimelineLayout::timeToMinutes(m_createStartTime) + TimelineLayout::minimumDurationMinutes,
                   endMinutes,
                   rangeEnd));
        viewport()->update();
        return;
    }

    const TimelineLayout::Metrics metrics = resolvedLayoutMetrics();
    const int rangeStart = TimelineLayout::visibleStartMinutes(metrics);
    const int rangeEnd = TimelineLayout::visibleEndMinutes(metrics);
    const int deltaMinutes =
        TimelineLayout::snapToGranularity(qRound((point.y() - m_dragStartY) * 60.0 / hourHeight()), metrics);
    int startMinutes = TimelineLayout::timeToMinutes(m_dragOriginalStart);
    int endMinutes = TimelineLayout::timeToMinutes(m_dragOriginalEnd);

    if (m_dragMode == DragMode::Move)
    {
        const int duration = endMinutes - startMinutes;
        startMinutes = qBound(rangeStart, startMinutes + deltaMinutes, rangeEnd - duration);
        endMinutes = startMinutes + duration;
    }
    else if (m_dragMode == DragMode::ResizeStart)
    {
        startMinutes =
            qBound(rangeStart, startMinutes + deltaMinutes, endMinutes - TimelineLayout::minimumDurationMinutes);
    }
    else if (m_dragMode == DragMode::ResizeEnd)
    {
        endMinutes = qBound(startMinutes + TimelineLayout::minimumDurationMinutes, endMinutes + deltaMinutes, rangeEnd);
    }

    updateEventPreview(
        m_dragEvent, TimelineLayout::minutesToTime(startMinutes), TimelineLayout::minutesToTime(endMinutes));
}

void DayTimelineWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
    {
        QAbstractScrollArea::mousePressEvent(event);
        return;
    }

    const QPoint point(event->pos().x(), event->pos().y() + verticalScrollBar()->value());
    for (const TimelinePainter::PaintedEvent &painted : std::as_const(m_paintedEvents))
    {
        const DragMode mode = dragModeAt(painted, point);
        if (mode == DragMode::None)
        {
            continue;
        }

        m_selectedEvent = CalendarSnapshot::cloneEventOccurrence(painted.event.edit);
        Q_EMIT eventSelected(CalendarSnapshot::cloneEventOccurrence(m_selectedEvent));
        if (isMultiDayTimedEvent(painted.event.display))
        {
            viewport()->update();
            event->accept();
            return;
        }

        m_dragMode = mode;
        m_dragEvent = CalendarSnapshot::cloneEventOccurrence(painted.event.edit);
        m_dragOriginalStart = eventStartTime(painted.event.display);
        m_dragOriginalEnd = eventEndTime(painted.event.display);
        if (!m_dragOriginalEnd.isValid() || m_dragOriginalEnd <= m_dragOriginalStart)
        {
            m_dragOriginalEnd = m_dragOriginalStart.addSecs(TimelineLayout::minimumDurationMinutes * 60);
        }
        m_dragStartY = point.y();
        viewport()->setCursor(mode == DragMode::Move ? Qt::ClosedHandCursor : Qt::SizeVerCursor);
        event->accept();
        return;
    }

    m_selectedEvent = EventOccurrence();
    Q_EMIT eventSelected(EventOccurrence());
    const TimelineLayout::Metrics metrics = resolvedLayoutMetrics();
    if (point.x() >= TimelineLayout::labelWidth && point.y() >= TimelineLayout::topPadding &&
        point.y() <= timelineHeight() - TimelineLayout::bottomPadding)
    {
        const int rangeStart = TimelineLayout::visibleStartMinutes(metrics);
        const int rangeEnd = TimelineLayout::visibleEndMinutes(metrics);
        const int startMinutes = qBound(rangeStart,
                                        TimelineLayout::yToMinutes(point.y(), metrics),
                                        rangeEnd - TimelineLayout::minimumDurationMinutes);
        m_dragMode = DragMode::Create;
        m_dragOriginalStart = TimelineLayout::minutesToTime(startMinutes);
        m_dragOriginalEnd = TimelineLayout::minutesToTime(startMinutes + TimelineLayout::minimumDurationMinutes);
        m_createStartTime = m_dragOriginalStart;
        m_createEndTime = m_dragOriginalEnd;
        m_dragStartY = point.y();
        viewport()->setCursor(Qt::CrossCursor);
        viewport()->update();
        event->accept();
        return;
    }

    QAbstractScrollArea::mousePressEvent(event);
}

void DayTimelineWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || m_dragMode == DragMode::None)
    {
        QAbstractScrollArea::mouseReleaseEvent(event);
        return;
    }

    if (m_dragMode == DragMode::Create)
    {
        const QTime startTime = m_createStartTime;
        const QTime endTime = m_createEndTime;
        m_dragMode = DragMode::None;
        m_createStartTime = QTime();
        m_createEndTime = QTime();
        viewport()->unsetCursor();
        viewport()->update();
        if (startTime.isValid() && endTime.isValid() && startTime < endTime)
        {
            Q_EMIT eventCreationRequested(startTime, endTime);
        }
        event->accept();
        return;
    }

    const QTime startTime = eventStartTime(m_dragEvent);
    const QTime endTime = eventEndTime(m_dragEvent);
    const EventOccurrence draggedEvent = CalendarSnapshot::cloneEventOccurrence(m_dragEvent);
    m_dragMode = DragMode::None;
    viewport()->unsetCursor();

    if (startTime.isValid() && endTime.isValid() && startTime < endTime &&
        (startTime != m_dragOriginalStart || endTime != m_dragOriginalEnd))
    {
        Q_EMIT eventTimeChangeRequested(draggedEvent, startTime, endTime);
    }
    else
    {
        viewport()->update();
    }
    event->accept();
}

void DayTimelineWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(viewport());
    painter.setRenderHint(QPainter::Antialiasing, false);

    const QPalette pal = palette();
    const QFont currentFont = font();
    const QList<EventTimelineRow> events = timedEvents();
    const QList<TimelineLayout::LaidOutEvent> laidOutEvents = TimelineLayout::layoutEvents(events, m_date);
    const TimelineLayout::Metrics metrics = resolvedLayoutMetrics(events);
    std::optional<std::pair<QTime, QTime>> createPreview;
    if (m_dragMode == DragMode::Create && m_createStartTime.isValid() && m_createEndTime.isValid())
    {
        createPreview = std::make_pair(m_createStartTime, m_createEndTime);
    }

    TimelinePainter::Context ctx;
    ctx.events = &events;
    ctx.laidOutEvents = &laidOutEvents;
    ctx.metrics = &metrics;
    ctx.palette = &pal;
    ctx.font = &currentFont;
    ctx.locale = &m_locale;
    ctx.viewport = viewport()->rect();
    ctx.scrollOffset = verticalScrollBar()->value();
    ctx.date = m_date;
    ctx.nowMarker = m_date == QDate::currentDate() ? std::optional<QTime>(QTime::currentTime()) : std::nullopt;
    ctx.createPreview = createPreview;
    ctx.createPreviewText = tr("New event");
    ctx.selectedEvent = m_selectedEvent;
    ctx.eventColorFn = [this](const EventDisplay &display) { return eventColor(display); };
    ctx.eventTextFn = [this](const EventDisplay &display) { return eventText(display); };
    ctx.eventTimeTextFn = [this](const EventDisplay &display) { return eventTimeText(display); };

    m_paintedEvents = TimelinePainter::paint(painter, ctx);
}

void DayTimelineWidget::resizeEvent(QResizeEvent *event)
{
    QAbstractScrollArea::resizeEvent(event);
    updateHeader();
    updateScrollBars();
    if (!m_initialScrollApplied && verticalScrollBar()->maximum() > 0)
    {
        const TimelineLayout::Metrics metrics = resolvedLayoutMetrics();
        verticalScrollBar()->setValue(
            qMin(verticalScrollBar()->maximum(),
                 TimelineLayout::timeToYMinutes(TimelineLayout::configuredStartMinutes(metrics), metrics) -
                     TimelineLayout::topPadding));
        m_initialScrollApplied = true;
    }
}

int DayTimelineWidget::hourHeight() const
{
    return qMax(48, fontMetrics().height() * 3);
}

int DayTimelineWidget::timelineHeight() const
{
    return TimelineLayout::timelineHeight(resolvedLayoutMetrics());
}

TimelineLayout::Metrics DayTimelineWidget::layoutMetrics() const
{
    TimelineLayout::Metrics metrics;
    metrics.hourHeight = hourHeight();
    metrics.dayViewStartHour = m_dayViewStartHour;
    metrics.dayViewFinishHour = m_dayViewFinishHour;
    return metrics;
}

TimelineLayout::Metrics DayTimelineWidget::resolvedLayoutMetrics() const
{
    return resolvedLayoutMetrics(timedEvents());
}

TimelineLayout::Metrics DayTimelineWidget::resolvedLayoutMetrics(const QList<EventTimelineRow> &events) const
{
    return TimelineLayout::resolvedMetrics(events, m_date, layoutMetrics());
}

DayTimelineWidget::DragMode DayTimelineWidget::dragModeAt(const TimelinePainter::PaintedEvent &painted,
                                                          const QPoint &point) const
{
    if (!painted.rect.contains(point))
    {
        return DragMode::None;
    }
    if (point.y() <= painted.rect.top() + resizeMargin)
    {
        return DragMode::ResizeStart;
    }
    if (point.y() >= painted.rect.bottom() - resizeMargin)
    {
        return DragMode::ResizeEnd;
    }
    return DragMode::Move;
}

bool DayTimelineWidget::sameEvent(const EventOccurrence &first, const EventOccurrence &second) const
{
    if (first.ref.item.href.isEmpty() || second.ref.item.href.isEmpty())
    {
        return false;
    }
    return first.ref.item.collectionId == second.ref.item.collectionId && first.ref.item.href == second.ref.item.href &&
           first.ref.uid == second.ref.uid && first.start == second.start;
}

void DayTimelineWidget::updateEventPreview(const EventOccurrence &event, const QTime &startTime, const QTime &endTime)
{
    for (EventTimelineRow &candidate : m_events)
    {
        if (!sameEvent(candidate.edit, event))
        {
            continue;
        }
        candidate.display.start.setTime(startTime);
        candidate.display.end.setTime(endTime);
        candidate.edit.start.setTime(startTime);
        candidate.edit.end.setTime(endTime);
        m_dragEvent.start.setTime(startTime);
        m_dragEvent.end.setTime(endTime);
        viewport()->update();
        return;
    }
}

QColor DayTimelineWidget::eventColor(const EventDisplay &event) const
{
    return colorOrDefault(event.collection.color, palette().color(QPalette::Highlight));
}

QString DayTimelineWidget::eventText(const EventDisplay &event) const
{
    QString summary = event.summary.trimmed();
    if (summary.isEmpty())
    {
        summary = tr("Untitled");
    }

    return summary;
}

QString DayTimelineWidget::eventTimeText(const EventDisplay &event) const
{
    return QStringLiteral("%1 - %2").arg(m_locale.toString(eventStartTime(event), QLocale::ShortFormat),
                                         m_locale.toString(eventEndTime(event), QLocale::ShortFormat));
}

QList<EventTimelineRow> DayTimelineWidget::timedEvents() const
{
    QList<EventTimelineRow> events;
    for (const EventTimelineRow &event : m_events)
    {
        if (isFullDayEvent(event.display))
        {
            continue;
        }
        if (m_date.isValid() && !eventOverlapsDate(event.display, m_date))
        {
            continue;
        }
        events.append(event);
    }

    std::sort(events.begin(), events.end(), [](const EventTimelineRow &first, const EventTimelineRow &second) {
        return eventStartTime(first.display) < eventStartTime(second.display);
    });
    return events;
}

int DayTimelineWidget::headerHeight() const
{
    return fontMetrics().height() + 12;
}

void DayTimelineWidget::updateHeader()
{
    const int height = headerHeight();
    if (viewportMargins().top() != height)
    {
        setViewportMargins(0, height, 0, 0);
    }

    const QRect viewportRect = viewport()->geometry();
    m_headerLabel->setGeometry(
        TimelineLayout::labelWidth, 0, qMax(0, viewportRect.width() - TimelineLayout::labelWidth), height);
    m_headerLabel->setText(m_date.isValid() ? m_locale.toString(m_date, QStringLiteral("dddd, MMMM d, yyyy"))
                                            : QString());
    m_headerLabel->raise();
}

void DayTimelineWidget::updateScrollBars()
{
    const int max = qMax(0, timelineHeight() - viewport()->height());
    verticalScrollBar()->setRange(0, max);
    verticalScrollBar()->setPageStep(viewport()->height());
}
