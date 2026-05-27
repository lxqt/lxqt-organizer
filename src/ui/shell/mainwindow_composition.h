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

#ifndef MAINWINDOW_COMPOSITION_H
#define MAINWINDOW_COMPOSITION_H

#include "windowsettingsstore.h"

#include <memory>

namespace Ui {
class MainWindow;
}

class ApplicationContext;
class CalendarPane;
class CalendarPaneController;
class ContactsPane;
class ContactsPaneController;
class MainWindow;
class MainWindowActionController;
class MainWindowViewState;
class PreferencesController;
class ReloadCoordinator;
class StatusBarPresenter;
struct WindowServices;

struct MainWindowComposition
{
    MainWindowComposition();
    ~MainWindowComposition();
    MainWindowComposition(MainWindowComposition &&) noexcept;
    MainWindowComposition &operator=(MainWindowComposition &&) noexcept;
    MainWindowComposition(const MainWindowComposition &) = delete;
    MainWindowComposition &operator=(const MainWindowComposition &) = delete;

    std::unique_ptr<WindowSettingsStore> windowSettingsStore;
    std::unique_ptr<PreferencesController> preferencesController;
    std::unique_ptr<MainWindowViewState> viewState;
    std::unique_ptr<ReloadCoordinator> reloadCoordinator;
    std::unique_ptr<WindowServices> windowServices;
    std::unique_ptr<MainWindowActionController> actionController;
    std::unique_ptr<StatusBarPresenter> statusPresenter;
    std::unique_ptr<CalendarPaneController> calendarPaneController;
    std::unique_ptr<ContactsPaneController> contactsPaneController;
    WindowSettingsStore::Snapshot settingsSnapshot;
    CalendarPane *calendarPane = nullptr;
    ContactsPane *contactsPane = nullptr;
};

MainWindowComposition composeMainWindow(MainWindow &window, Ui::MainWindow &ui, ApplicationContext &context);
MainWindowComposition completeMainWindowComposition(MainWindowComposition &&composition,
                                                    MainWindow &window,
                                                    Ui::MainWindow &ui,
                                                    ApplicationContext &context,
                                                    const WindowServices &windowServices);

#endif // MAINWINDOW_COMPOSITION_H
