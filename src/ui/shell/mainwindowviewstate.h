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

#ifndef MAINWINDOWVIEWSTATE_H
#define MAINWINDOWVIEWSTATE_H

#include <QByteArray>
#include <QDate>
#include <QObject>

// Why two classes? MainWindowViewState owns window-scoped UI state:
// selected date, splitter geometry, and pane visibility. It is persisted via
// WindowSettingsStore, but it does not hold user preferences like font size or
// day-view hours; those live in PreferencesController.
class MainWindowViewState : public QObject
{
    Q_OBJECT

public:
    explicit MainWindowViewState(QObject *parent = nullptr);

    QDate selectedDate() const;
    QByteArray calendarSplitterState() const;
    bool contactDetailsVisible() const;
    bool taskPaneVisible() const;

public Q_SLOTS:
    void setSelectedDate(const QDate &date);
    void setCalendarSplitterState(const QByteArray &state);
    void setContactDetailsVisible(bool visible);
    void setTaskPaneVisible(bool visible);

Q_SIGNALS:
    void selectedDateChanged(const QDate &date);
    void calendarSplitterStateChanged(const QByteArray &state);
    void contactDetailsVisibleChanged(bool visible);
    void taskPaneVisibleChanged(bool visible);

private:
    QDate m_selectedDate;
    QByteArray m_calendarSplitterState;
    bool m_contactDetailsVisible = false;
    bool m_taskPaneVisible = true;
};

#endif // MAINWINDOWVIEWSTATE_H
