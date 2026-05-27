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

#include "monthchromewidget.h"
#include "ui_monthchromewidget.h"

#include "eventoccurrences.h"
#include "monthcalendarwidget.h"

#include <QCalendarWidget>
#include <QIcon>
#include <QMenu>
#include <QSignalBlocker>
#include <QToolButton>

namespace {
QDate firstDateOfMonth(int year, int month)
{
    return QDate(year, month, 1);
}

QDate lastDateOfMonth(int year, int month)
{
    const QDate first = firstDateOfMonth(year, month);
    return first.isValid() ? first.addMonths(1).addDays(-1) : QDate();
}
} // namespace

MonthChromeWidget::MonthChromeWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MonthChromeWidget)
    , m_locale(QLocale::system())
{
    ui->setupUi(this);

    ui->tableWidgetCalendar->setVerticalHeaderFormat(QCalendarWidget::ISOWeekNumbers);
    ui->tableWidgetCalendar->setHorizontalHeaderFormat(QCalendarWidget::ShortDayNames);
    ui->tableWidgetCalendar->setGridVisible(false);
    ui->tableWidgetCalendar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    ui->calendarNavigationBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    ui->pushButtonPreviousCalendar->setText(tr("Prev"));
    ui->pushButtonTodayCalendar->setText(tr("Today"));
    ui->pushButtonNextCalendar->setText(tr("Next"));
    ui->pushButtonPreviousCalendar->setIcon(QIcon::fromTheme(QStringLiteral("go-previous")));
    ui->pushButtonTodayCalendar->setIcon(QIcon());
    ui->pushButtonNextCalendar->setIcon(QIcon::fromTheme(QStringLiteral("go-next")));
    ui->pushButtonPreviousCalendar->setLayoutDirection(Qt::LeftToRight);
    ui->pushButtonTodayCalendar->setLayoutDirection(Qt::LeftToRight);
    ui->pushButtonNextCalendar->setLayoutDirection(Qt::RightToLeft);
    for (QToolButton *button :
         {ui->pushButtonPreviousCalendar, ui->pushButtonTodayCalendar, ui->pushButtonNextCalendar})
    {
        button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    }
    ui->pushButtonTodayCalendar->setToolButtonStyle(Qt::ToolButtonTextOnly);

    connect(ui->pushButtonPreviousCalendar, &QToolButton::clicked, this, &MonthChromeWidget::previousMonthRequested);
    connect(ui->pushButtonTodayCalendar, &QToolButton::clicked, this, &MonthChromeWidget::todayRequested);
    connect(ui->pushButtonNextCalendar, &QToolButton::clicked, this, &MonthChromeWidget::nextMonthRequested);
    connect(ui->tableWidgetCalendar, &QCalendarWidget::clicked, this, &MonthChromeWidget::dateSelected);
    connect(ui->tableWidgetCalendar, &QCalendarWidget::activated, this, &MonthChromeWidget::dateActivated);
    connect(ui->tableWidgetCalendar,
            &MonthCalendarWidget::dateContextMenuRequested,
            this,
            [this](const QDate &date, const QPoint &globalPos) {
                QMenu menu(this);
                QAction *newEventAction =
                    menu.addAction(QIcon::fromTheme(QStringLiteral("view-calendar-day")), tr("New Event"));
                QAction *newTaskAction =
                    menu.addAction(QIcon::fromTheme(QStringLiteral("view-calendar-tasks")), tr("New Task"));

                QAction *selectedAction = menu.exec(globalPos);
                if (selectedAction == newEventAction)
                {
                    Q_EMIT newEventRequested(date);
                }
                else if (selectedAction == newTaskAction)
                {
                    Q_EMIT newTaskRequested(date);
                }
            });
    connect(ui->tableWidgetCalendar, &QCalendarWidget::currentPageChanged, this, [this](int year, int month) {
        if (!m_selectedDate.isValid())
        {
            return;
        }
        if (m_selectedDate.year() != year || m_selectedDate.month() != month)
        {
            const int day = qMin(m_selectedDate.day(), QDate(year, month, 1).daysInMonth());
            Q_EMIT dateSelected(QDate(year, month, day));
        }
    });
}

MonthChromeWidget::~MonthChromeWidget()
{
    delete ui;
}

void MonthChromeWidget::setCalendarData(const QList<EventOccurrence> &events, const QList<Task> &tasks)
{
    m_events = events;
    m_tasks = tasks;
    renderMonthView();
}

void MonthChromeWidget::setSelectedDate(const QDate &date)
{
    if (!date.isValid())
    {
        return;
    }
    m_selectedDate = date;
    renderMonthView();
}

void MonthChromeWidget::setLocale(const QLocale &locale)
{
    m_locale = locale;
    renderMonthView();
}

void MonthChromeWidget::setCalendarFontSize(int fontSize)
{
    QFont itemFontSize = ui->tableWidgetCalendar->font();
    itemFontSize.setPointSize(qBound(8, fontSize, 36));
    ui->tableWidgetCalendar->setFont(itemFontSize);
}

void MonthChromeWidget::renderMonthView()
{
    if (!m_selectedDate.isValid())
    {
        return;
    }

    const int selectedYear = m_selectedDate.year();
    const int selectedMonth = m_selectedDate.month();
    QHash<QDate, MonthCalendarWidget::DateIndicator> indicators;
    auto addIndicator = [&indicators](const QDate &date) {
        if (date.isValid())
        {
            MonthCalendarWidget::DateIndicator indicator = indicators.value(date);
            ++indicator.count;
            indicators[date] = indicator;
        }
    };
    auto shouldHighlight = [](CalendarUtils::HighlightMode mode, const QDate &date) {
        switch (mode)
        {
        case CalendarUtils::HighlightMode::Never:
            return false;
        case CalendarUtils::HighlightMode::Future:
            return date >= QDate::currentDate();
        case CalendarUtils::HighlightMode::Always:
        default:
            return true;
        }
    };

    const QDate monthStart = firstDateOfMonth(selectedYear, selectedMonth);
    const QDate monthEnd = lastDateOfMonth(selectedYear, selectedMonth);
    for (const EventOccurrence &event : std::as_const(m_events))
    {
        if (event.ref.item.collectionId.isEmpty())
        {
            continue;
        }
        const QDate eventStart = event.start.date();
        const QDate displayEndDate = EventOccurrences::displayEndDate(event);
        const QDate eventEnd = displayEndDate.isValid() ? displayEndDate : eventStart;
        const CalendarUtils::HighlightMode mode = CalendarSnapshot::eventDisplay(event).highlightMode;
        if (eventStart.isValid() && monthStart.isValid() && monthEnd.isValid() && eventStart <= monthEnd &&
            eventEnd >= monthStart)
        {
            const QDate firstMarkedDate = eventStart < monthStart ? monthStart : eventStart;
            const QDate lastMarkedDate = eventEnd > monthEnd ? monthEnd : eventEnd;
            for (QDate date = firstMarkedDate; date <= lastMarkedDate; date = date.addDays(1))
            {
                if (shouldHighlight(mode, date))
                {
                    addIndicator(date);
                }
            }
        }
    }

    for (const Task &task : std::as_const(m_tasks))
    {
        if (!CalendarSnapshot::hasTask(task) || task.ref.collectionId.isEmpty())
        {
            continue;
        }
        const QDate dueDate = CalendarSnapshot::taskDueDate(task);
        if (!dueDate.isValid())
        {
            continue;
        }

        const CalendarUtils::HighlightMode mode = CalendarSnapshot::taskDisplay(task).highlightMode;
        if (dueDate.year() == selectedYear && dueDate.month() == selectedMonth && shouldHighlight(mode, dueDate))
        {
            addIndicator(dueDate);
        }
    }

    const QSignalBlocker blocker(ui->tableWidgetCalendar);
    ui->tableWidgetCalendar->setFirstDayOfWeek(m_locale.firstDayOfWeek());
    ui->tableWidgetCalendar->setSelectionMode(QCalendarWidget::SingleSelection);
    ui->tableWidgetCalendar->setSelectedDate(m_selectedDate);
    ui->tableWidgetCalendar->setCurrentPage(selectedYear, selectedMonth);
    ui->tableWidgetCalendar->setDateIndicators(indicators);
}

void MonthChromeWidget::installCalendarEventFilter(QObject *filter)
{
    ui->tableWidgetCalendar->installEventFilter(filter);
}

bool MonthChromeWidget::isCalendarWidget(QObject *object) const
{
    return object == ui->tableWidgetCalendar;
}
