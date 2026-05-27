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

#include "mainwindowviewstate.h"

MainWindowViewState::MainWindowViewState(QObject *parent)
    : QObject(parent)
{}

QDate MainWindowViewState::selectedDate() const
{
    return m_selectedDate.isValid() ? m_selectedDate : QDate::currentDate();
}

QByteArray MainWindowViewState::calendarSplitterState() const
{
    return m_calendarSplitterState;
}

bool MainWindowViewState::contactDetailsVisible() const
{
    return m_contactDetailsVisible;
}

bool MainWindowViewState::taskPaneVisible() const
{
    return m_taskPaneVisible;
}

void MainWindowViewState::setSelectedDate(const QDate &date)
{
    if (!date.isValid() || m_selectedDate == date)
    {
        return;
    }

    m_selectedDate = date;
    Q_EMIT selectedDateChanged(m_selectedDate);
}

void MainWindowViewState::setCalendarSplitterState(const QByteArray &state)
{
    if (m_calendarSplitterState == state)
    {
        return;
    }

    m_calendarSplitterState = state;
    Q_EMIT calendarSplitterStateChanged(m_calendarSplitterState);
}

void MainWindowViewState::setContactDetailsVisible(bool visible)
{
    if (m_contactDetailsVisible == visible)
    {
        return;
    }

    m_contactDetailsVisible = visible;
    Q_EMIT contactDetailsVisibleChanged(m_contactDetailsVisible);
}

void MainWindowViewState::setTaskPaneVisible(bool visible)
{
    if (m_taskPaneVisible == visible)
    {
        return;
    }

    m_taskPaneVisible = visible;
    Q_EMIT taskPaneVisibleChanged(m_taskPaneVisible);
}
