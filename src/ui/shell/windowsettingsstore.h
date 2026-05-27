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

#ifndef WINDOWSETTINGSSTORE_H
#define WINDOWSETTINGSSTORE_H

#include "preferences.h"

#include <QByteArray>
#include <QObject>

class QMainWindow;
class QSplitter;

class WindowSettingsStore : public QObject
{
    Q_OBJECT

public:
    struct Snapshot
    {
        Preferences preferences;
        QByteArray geometry;
        QByteArray calendarSplitterState;
        bool contactDetailsVisible = false;
        bool taskPaneVisible = true;
    };

    explicit WindowSettingsStore(QObject *parent = nullptr);

    Snapshot load(const Preferences &fallbackPreferences = Preferences()) const;
    void save(const Snapshot &snapshot) const;
    void restoreWindowGeometry(QMainWindow *window, const QByteArray &geometry) const;
    void saveWindowGeometry(QMainWindow *window, Snapshot *snapshot) const;
};

#endif // WINDOWSETTINGSSTORE_H
