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

#include "windowsettingsstore.h"

#include <LXQt/Settings>

#include <QMainWindow>

namespace {
constexpr auto kSettingsGroup = "lxqt-organizer";
constexpr auto kTaskPaneVisibleKey = "TaskPaneVisible";
} // namespace

WindowSettingsStore::WindowSettingsStore(QObject *parent)
    : QObject(parent)
{}

WindowSettingsStore::Snapshot WindowSettingsStore::load(const Preferences &fallbackPreferences) const
{
    LXQt::Settings settings(QString::fromLatin1(kSettingsGroup));
    settings.beginGroup(QStringLiteral("CoreSettings"));

    Snapshot snapshot;
    snapshot.preferences = fallbackPreferences;
    snapshot.preferences.applicationFontSize =
        settings.value(QStringLiteral("ApplicationFontSize"), snapshot.preferences.applicationFontSize).toInt();
    snapshot.preferences.dayViewTimeStart = qBound(
        0, settings.value(QStringLiteral("DayviewTimeStart"), snapshot.preferences.dayViewTimeStart).toInt(), 23);
    snapshot.preferences.dayViewTimeFinish =
        qBound(snapshot.preferences.dayViewTimeStart + 1,
               settings.value(QStringLiteral("DayviewTimeFinish"), snapshot.preferences.dayViewTimeFinish).toInt(),
               24);
    snapshot.geometry = settings.value(QStringLiteral("geometry")).toByteArray();
    snapshot.contactDetailsVisible =
        settings.value(QStringLiteral("ContactDetailsVisible"), snapshot.contactDetailsVisible).toBool();
    snapshot.taskPaneVisible =
        settings.value(QString::fromLatin1(kTaskPaneVisibleKey), snapshot.taskPaneVisible).toBool();
    snapshot.calendarSplitterState = settings.value(QStringLiteral("CalendarSplitterState")).toByteArray();
    settings.endGroup();
    return snapshot;
}

void WindowSettingsStore::save(const Snapshot &snapshot) const
{
    LXQt::Settings settings(QString::fromLatin1(kSettingsGroup));
    settings.beginGroup(QStringLiteral("CoreSettings"));
    settings.setValue(QStringLiteral("ApplicationFontSize"), snapshot.preferences.applicationFontSize);
    settings.setValue(QStringLiteral("DayviewTimeStart"), snapshot.preferences.dayViewTimeStart);
    settings.setValue(QStringLiteral("DayviewTimeFinish"), snapshot.preferences.dayViewTimeFinish);
    if (!snapshot.geometry.isEmpty())
    {
        settings.setValue(QStringLiteral("geometry"), snapshot.geometry);
    }
    settings.setValue(QStringLiteral("ContactDetailsVisible"), snapshot.contactDetailsVisible);
    settings.setValue(QString::fromLatin1(kTaskPaneVisibleKey), snapshot.taskPaneVisible);
    settings.setValue(QStringLiteral("CalendarSplitterState"), snapshot.calendarSplitterState);
    settings.endGroup();
}

void WindowSettingsStore::restoreWindowGeometry(QMainWindow *window, const QByteArray &geometry) const
{
    if (window && !geometry.isEmpty())
    {
        window->restoreGeometry(geometry);
    }
}

void WindowSettingsStore::saveWindowGeometry(QMainWindow *window, Snapshot *snapshot) const
{
    if (window && snapshot)
    {
        snapshot->geometry = window->saveGeometry();
    }
}
