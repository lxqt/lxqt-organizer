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

#ifndef PREFERENCESCONTROLLER_H
#define PREFERENCESCONTROLLER_H

#include "preferences.h"

#include <QObject>
#include <QLocale>

// Why two classes? PreferencesController owns user-configurable preferences:
// font size, day-view range, and locale. Persistence is owned by the main
// window settings save path so preferences cannot overwrite unrelated window
// state.
class PreferencesController : public QObject
{
    Q_OBJECT

public:
    explicit PreferencesController(QObject *parent = nullptr);
    ~PreferencesController() override;

    const Preferences &preferences() const;
    void setPreferences(const Preferences &preferences);
    QLocale locale() const;
    void setLocale(const QLocale &locale);
    int applicationFontSize() const;
    void setApplicationFontSize(int fontSize);
    void resetApplicationFontSize();

Q_SIGNALS:
    void preferencesChanged(const Preferences &preferences);
    void dayViewRangeChanged(int startHour, int finishHour);
    void applicationFontSizeChanged(int fontSize);
    void localeChanged(const QLocale &locale);

private:
    bool applyPreferences(const Preferences &preferences, bool emitSignals);

    Preferences m_preferences;
    QLocale m_locale = QLocale::system();
};

#endif // PREFERENCESCONTROLLER_H
