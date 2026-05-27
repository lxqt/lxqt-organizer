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

#include "editorcombos.h"

#include "eventdatetime.h"

#include <algorithm>

#include <QSignalBlocker>
#include <QTimeZone>

HighlightComboBox::HighlightComboBox(QWidget *parent)
    : QComboBox(parent)
{
    addItem(tr("Always Highlight"), static_cast<int>(CalendarUtils::HighlightMode::Always));
    addItem(tr("Never Highlight"), static_cast<int>(CalendarUtils::HighlightMode::Never));
    addItem(tr("Highlight Future"), static_cast<int>(CalendarUtils::HighlightMode::Future));
}

CalendarUtils::HighlightMode HighlightComboBox::mode() const
{
    return static_cast<CalendarUtils::HighlightMode>(currentData().toInt());
}

void HighlightComboBox::setMode(CalendarUtils::HighlightMode mode)
{
    const int index = findData(static_cast<int>(mode));
    if (index >= 0)
    {
        setCurrentIndex(index);
    }
}

PriorityComboBox::PriorityComboBox(QWidget *parent)
    : QComboBox(parent)
{
    addItem(tr("Low"), 9);
    addItem(tr("Medium"), 5);
    addItem(tr("High"), 1);
    setCurrentIndex(findData(m_priority));

    connect(
        this, qOverload<int>(&QComboBox::currentIndexChanged), this, [this] { m_priority = currentData().toInt(); });
}

int PriorityComboBox::priority() const
{
    return m_priority;
}

void PriorityComboBox::setPriority(int priority)
{
    const int index = findData(EventDateTime::normalizedPriority(priority));
    if (index >= 0)
    {
        const QSignalBlocker blocker(this);
        setCurrentIndex(index);
    }
    m_priority = priority;
}

TimeZoneComboBox::TimeZoneComboBox(QWidget *parent)
    : QComboBox(parent)
{
    setEditable(true);
    setTimeZoneId(QString::fromUtf8(QTimeZone::systemTimeZoneId()));
}

QString TimeZoneComboBox::timeZoneId() const
{
    const QString id = currentText().trimmed();
    return id.isEmpty() ? QString::fromUtf8(QTimeZone::systemTimeZoneId()) : id;
}

void TimeZoneComboBox::setTimeZoneId(const QString &timeZoneId)
{
    const QString selected =
        timeZoneId.trimmed().isEmpty() ? QString::fromUtf8(QTimeZone::systemTimeZoneId()) : timeZoneId.trimmed();

    const QSignalBlocker blocker(this);
    clear();
    addItem(selected);

    QList<QByteArray> zoneIds = QTimeZone::availableTimeZoneIds();
    std::sort(zoneIds.begin(), zoneIds.end());
    for (const QByteArray &zoneId : zoneIds)
    {
        const QString text = QString::fromUtf8(zoneId);
        if (text != selected)
        {
            addItem(text);
        }
    }
    setCurrentText(selected);
}
