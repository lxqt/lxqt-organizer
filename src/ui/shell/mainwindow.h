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

#include "iactivepane.h"
#include "settings_io.h"
#include "storageresult.h"

#include <QByteArray>
#include <QDate>
#include <QList>
#include <QMainWindow>
#include <QMetaObject>
#include <QPointer>
#include <QTimer>

#include <memory>

namespace Ui {
class MainWindow;
}

class CalendarPane;
class CalendarPaneController;
class ContactsPane;
class ContactsPaneController;
class MainWindowServices;
class PreferencesController;
class QAction;
class QCloseEvent;
class QLabel;
class ReloadCoordinator;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(MainWindowServices &services, QWidget *parent = nullptr);
    ~MainWindow() override;

    MainWindowServices &services() { return m_services; }
    PreferencesController *preferencesController() const { return m_prefs.get(); }
    ReloadCoordinator *dataReloadCoordinator() const { return m_reload.get(); }
    const CalendarPane *calendarPane() const { return m_calendarPane; }
    const ContactsPane *contactsPane() const { return m_contactsPane; }

    void applyApplicationFontSize(int fontSize);
    void saveSettings();

    QDate selectedDate() const { return m_selectedDate.isValid() ? m_selectedDate : QDate::currentDate(); }
    QByteArray calendarSplitterState() const { return m_calendarSplitterState; }
    bool contactDetailsVisible() const { return m_contactDetailsVisible; }
    bool taskPaneVisible() const { return m_taskPaneVisible; }

public Q_SLOTS:
    void setSelectedDate(const QDate &date);
    void setCalendarSplitterState(const QByteArray &state);
    void setContactDetailsVisible(bool visible);
    void setTaskPaneVisible(bool visible);

Q_SIGNALS:
    void selectedDateChanged(const QDate &date);
    void calendarSplitterStateChanged(const QByteArray &state);
    void contactDetailsVisibleChanged(bool visible);
    void taskPaneVisibleChanged(bool visible);

protected:
    void closeEvent(QCloseEvent *event) override;

private Q_SLOTS:
    void refreshActionState();
    void refreshStatusBar();
    void showReadFailures(const QList<ReadFailure> &readFailures);

private:
    void configurePanes();
    void wirePaneSignals();
    void setupActionWiring();
    void loadStorageAndSettings();
    void loadSettings();
    void setupInitialCalendarState();

    void setActivePane(IActivePane *pane);

    void scheduleRefreshActionState();
    void triggerContextAction(const QString &contextActionId);
    void refreshContextAliases();

    void scheduleSettingsSave();
    void flushSettingsSave();

    Ui::MainWindow *m_ui;
    MainWindowServices &m_services;
    std::unique_ptr<PreferencesController> m_prefs;
    std::unique_ptr<ReloadCoordinator> m_reload;
    std::unique_ptr<CalendarPaneController> m_calendarPaneController;
    std::unique_ptr<ContactsPaneController> m_contactsPaneController;
    CalendarPane *m_calendarPane = nullptr;
    ContactsPane *m_contactsPane = nullptr;

    QLabel *m_itemCountLabel = nullptr;
    QLabel *m_selectedDateLabel = nullptr;
    IActivePane *m_activePane = nullptr;
    QPointer<QObject> m_activePaneObject;
    QList<QMetaObject::Connection> m_activePaneConnections;
    QList<QMetaObject::Connection> m_contextActionConnections;
    bool m_refreshActionStateQueued = false;

    QDate m_selectedDate;
    QByteArray m_calendarSplitterState;
    bool m_contactDetailsVisible = false;
    bool m_taskPaneVisible = true;

    settings_io::Snapshot m_settingsSnapshot;
    QTimer m_settingsSaveTimer;
};
