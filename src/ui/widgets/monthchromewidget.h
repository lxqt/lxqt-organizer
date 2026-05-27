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

#ifndef MONTHCHROMEWIDGET_H
#define MONTHCHROMEWIDGET_H

#include "calendaritem.h"

#include <QDate>
#include <QLocale>
#include <QWidget>

namespace Ui {
class MonthChromeWidget;
}

class MonthChromeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MonthChromeWidget(QWidget *parent = nullptr);
    ~MonthChromeWidget() override;

    void setCalendarData(const QList<EventOccurrence> &events, const QList<Task> &tasks);
    void setSelectedDate(const QDate &date);
    void setLocale(const QLocale &locale);
    void setCalendarFontSize(int fontSize);
    void renderMonthView();
    void installCalendarEventFilter(QObject *filter);
    bool isCalendarWidget(QObject *object) const;

Q_SIGNALS:
    void dateSelected(const QDate &date);
    void dateActivated(const QDate &date);
    void previousMonthRequested();
    void nextMonthRequested();
    void todayRequested();
    void newEventRequested(const QDate &date);
    void newTaskRequested(const QDate &date);

private:
    Ui::MonthChromeWidget *ui = nullptr;
    QList<EventOccurrence> m_events;
    QList<Task> m_tasks;
    QDate m_selectedDate;
    QLocale m_locale;
};

#endif // MONTHCHROMEWIDGET_H
