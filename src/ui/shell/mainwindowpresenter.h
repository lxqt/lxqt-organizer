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

#ifndef MAINWINDOWPRESENTER_H
#define MAINWINDOWPRESENTER_H

#include "windowsettingsstore.h"

#include <QObject>
#include <QTimer>

#include <memory>

class ApplicationContext;
class CalendarPane;
class CalendarPaneController;
class ContactsPane;
class ContactsPaneController;
class MainWindow;
class MainWindowActionController;
struct MainWindowComposition;
class MainWindowViewState;
class PreferencesController;
class QCloseEvent;
class ReloadCoordinator;
class StatusBarPresenter;
struct WindowServices;

class MainWindowPresenter : public QObject
{
public:
    MainWindowPresenter(MainWindow &window, ApplicationContext &context, MainWindowComposition &&composition);
    ~MainWindowPresenter() override;

    ApplicationContext &applicationContext() const;
    PreferencesController *preferencesController() const;
    ReloadCoordinator *dataReloadCoordinator() const;
    const CalendarPane *calendarPane() const;
    const ContactsPane *contactsPane() const;

    void onCloseRequested(QCloseEvent *event);
    void applyApplicationFontSize(int fontSize);
    void saveSettings();

private:
    void initializeSettingsSaveOwner();
    void scheduleSettingsSave();
    void flushSettingsSave();

    MainWindow &m_window;
    ApplicationContext &m_applicationContext;
    std::unique_ptr<WindowSettingsStore> m_windowSettingsStore;
    std::unique_ptr<PreferencesController> m_preferencesController;
    std::unique_ptr<ReloadCoordinator> m_reloadCoordinator;
    std::unique_ptr<WindowServices> m_windowServices;
    std::unique_ptr<MainWindowActionController> m_actionController;
    std::unique_ptr<StatusBarPresenter> m_statusPresenter;
    CalendarPane *m_calendarPane = nullptr;
    std::unique_ptr<CalendarPaneController> m_calendarPaneController;
    ContactsPane *m_contactsPane = nullptr;
    std::unique_ptr<ContactsPaneController> m_contactsPaneController;
    std::unique_ptr<MainWindowViewState> m_viewState;
    WindowSettingsStore::Snapshot m_settingsSnapshot;
    QTimer m_settingsSaveTimer;
};

#endif // MAINWINDOWPRESENTER_H
