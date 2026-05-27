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

#ifndef EDITORCOMBOS_H
#define EDITORCOMBOS_H

#include "highlightmode.h"

#include <QComboBox>

class HighlightComboBox : public QComboBox
{
    Q_OBJECT

public:
    explicit HighlightComboBox(QWidget *parent = nullptr);

    CalendarUtils::HighlightMode mode() const;
    void setMode(CalendarUtils::HighlightMode mode);
};

class PriorityComboBox : public QComboBox
{
    Q_OBJECT

public:
    explicit PriorityComboBox(QWidget *parent = nullptr);

    int priority() const;
    void setPriority(int priority);

private:
    int m_priority = 5;
};

class TimeZoneComboBox : public QComboBox
{
    Q_OBJECT

public:
    explicit TimeZoneComboBox(QWidget *parent = nullptr);

    QString timeZoneId() const;
    void setTimeZoneId(const QString &timeZoneId);
};

#endif // EDITORCOMBOS_H
