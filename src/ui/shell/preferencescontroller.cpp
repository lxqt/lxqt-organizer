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

#include "preferencescontroller.h"

namespace {

int boundedApplicationFontSize(int fontSize)
{
    return qBound(8, fontSize, 36);
}

bool equalPreferences(const Preferences &first, const Preferences &second)
{
    return first.applicationFontSize == second.applicationFontSize &&
           first.dayViewTimeStart == second.dayViewTimeStart && first.dayViewTimeFinish == second.dayViewTimeFinish;
}

} // namespace

PreferencesController::PreferencesController(QObject *parent)
    : QObject(parent)
{}

PreferencesController::~PreferencesController() = default;

const Preferences &PreferencesController::preferences() const
{
    return m_preferences;
}

QLocale PreferencesController::locale() const
{
    return m_locale;
}

void PreferencesController::setLocale(const QLocale &locale)
{
    if (m_locale == locale)
    {
        return;
    }
    m_locale = locale;
    Q_EMIT localeChanged(m_locale);
}

void PreferencesController::setPreferences(const Preferences &preferences)
{
    applyPreferences(preferences, true);
}

bool PreferencesController::applyPreferences(const Preferences &preferences, bool emitSignals)
{
    Preferences next = preferences;
    next.applicationFontSize = boundedApplicationFontSize(next.applicationFontSize);

    const Preferences previous = m_preferences;
    if (equalPreferences(previous, next))
    {
        return false;
    }

    m_preferences = next;
    if (!emitSignals)
    {
        return true;
    }

    Q_EMIT preferencesChanged(m_preferences);
    if (previous.dayViewTimeStart != m_preferences.dayViewTimeStart ||
        previous.dayViewTimeFinish != m_preferences.dayViewTimeFinish)
    {
        Q_EMIT dayViewRangeChanged(m_preferences.dayViewTimeStart, m_preferences.dayViewTimeFinish);
    }
    if (previous.applicationFontSize != m_preferences.applicationFontSize)
    {
        Q_EMIT applicationFontSizeChanged(m_preferences.applicationFontSize);
    }
    return true;
}

int PreferencesController::applicationFontSize() const
{
    return m_preferences.applicationFontSize;
}

void PreferencesController::setApplicationFontSize(int fontSize)
{
    const int bounded = boundedApplicationFontSize(fontSize);
    if (m_preferences.applicationFontSize == bounded)
    {
        return;
    }

    Preferences updated = m_preferences;
    updated.applicationFontSize = bounded;
    setPreferences(updated);
}

void PreferencesController::resetApplicationFontSize()
{
    setApplicationFontSize(Preferences::defaultApplicationFontSize);
}
