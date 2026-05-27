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

#ifndef MONTHCALENDARWIDGET_H
#define MONTHCALENDARWIDGET_H

#include <QCalendarWidget>
#include <QHash>

class MonthCalendarWidget : public QCalendarWidget
{
    Q_OBJECT

public:
    struct DateIndicator
    {
        int count = 0;

        bool operator==(const DateIndicator &other) const { return count == other.count; }
    };

    explicit MonthCalendarWidget(QWidget *parent = nullptr);

    void setDateIndicators(const QHash<QDate, DateIndicator> &indicators);

Q_SIGNALS:
    void dateContextMenuRequested(const QDate &date, const QPoint &globalPos);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void paintCell(QPainter *painter, const QRect &rect, QDate date) const override;
    void showEvent(QShowEvent *event) override;

private:
    QDate dateForCell(int row, int column) const;
    void installCalendarViewEventFilters();

    QHash<QDate, DateIndicator> m_indicators;
    bool m_contextMenuOpenedOnPress = false;
};

#endif // MONTHCALENDARWIDGET_H
