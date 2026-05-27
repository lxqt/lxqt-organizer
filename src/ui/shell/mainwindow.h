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

#pragma once

#include <QMainWindow>

#include <memory>

namespace Ui {
class MainWindow;
}

class ApplicationContext;
class CalendarPane;
class ContactsPane;
class MainWindowPresenter;
class PreferencesController;
class QCloseEvent;
class ReloadCoordinator;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(ApplicationContext &context, QWidget *parent = nullptr);
    ~MainWindow();
    ApplicationContext &applicationContext();
    PreferencesController *preferencesController() const;
    ReloadCoordinator *dataReloadCoordinator() const;
    const CalendarPane *calendarPane() const;
    const ContactsPane *contactsPane() const;
    void applyApplicationFontSize(int fontSize);
    void saveSettings();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::MainWindow *m_ui;
    std::unique_ptr<MainWindowPresenter> m_presenter;
};
