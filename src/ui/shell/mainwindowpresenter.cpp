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

#include "mainwindowpresenter.h"

#include "mainwindow.h"
#include "mainwindow_composition.h"

#include "calendarpane.h"
#include "calendarpanecontroller.h"
#include "contactspane.h"
#include "contactspanecontroller.h"
#include "mainwindowactioncontroller.h"
#include "mainwindowviewstate.h"
#include "preferences.h"
#include "preferencescontroller.h"
#include "reloadcoordinator.h"
#include "statusbarpresenter.h"
#include "windowsettingsstore.h"
#include "windowservices.h"

#include <QCloseEvent>
#include <QFont>
#include <QSplitter>

#include <utility>

MainWindowPresenter::MainWindowPresenter(MainWindow &window,
                                         ApplicationContext &context,
                                         MainWindowComposition &&products)
    : QObject(&window)
    , m_window(window)
    , m_applicationContext(context)
{
    m_windowServices = std::move(products.windowServices);

    m_windowSettingsStore = std::move(products.windowSettingsStore);
    m_preferencesController = std::move(products.preferencesController);
    m_viewState = std::move(products.viewState);
    m_reloadCoordinator = std::move(products.reloadCoordinator);
    m_actionController = std::move(products.actionController);
    m_statusPresenter = std::move(products.statusPresenter);
    m_calendarPaneController = std::move(products.calendarPaneController);
    m_contactsPaneController = std::move(products.contactsPaneController);
    m_calendarPane = products.calendarPane;
    m_contactsPane = products.contactsPane;
    m_settingsSnapshot = products.settingsSnapshot;

    Q_ASSERT(m_windowSettingsStore && m_preferencesController && m_viewState && m_reloadCoordinator &&
             m_calendarPaneController && m_contactsPaneController && m_calendarPane && m_contactsPane);

    initializeSettingsSaveOwner();
    applyApplicationFontSize(m_preferencesController->applicationFontSize());
}

MainWindowPresenter::~MainWindowPresenter()
{
    if (m_settingsSaveTimer.isActive())
    {
        flushSettingsSave();
    }
}

ApplicationContext &MainWindowPresenter::applicationContext() const
{
    return m_applicationContext;
}

PreferencesController *MainWindowPresenter::preferencesController() const
{
    return m_preferencesController.get();
}

ReloadCoordinator *MainWindowPresenter::dataReloadCoordinator() const
{
    return m_reloadCoordinator.get();
}

const CalendarPane *MainWindowPresenter::calendarPane() const
{
    return m_calendarPane;
}

const ContactsPane *MainWindowPresenter::contactsPane() const
{
    return m_contactsPane;
}

void MainWindowPresenter::onCloseRequested(QCloseEvent *event)
{
    Q_UNUSED(event)
    flushSettingsSave();
}

void MainWindowPresenter::applyApplicationFontSize(int fontSize)
{
    Q_ASSERT(m_calendarPane);

    QFont font = m_window.font();
    font.setPointSize(qBound(8, fontSize, 36));
    m_window.setFont(font);
    m_calendarPane->setCalendarFontSize(fontSize);
}

void MainWindowPresenter::saveSettings()
{
    Q_ASSERT(m_windowSettingsStore && m_preferencesController && m_viewState && m_calendarPane && m_contactsPane);

    flushSettingsSave();
}

void MainWindowPresenter::initializeSettingsSaveOwner()
{
    m_settingsSaveTimer.setSingleShot(true);
    m_settingsSaveTimer.setInterval(200);
    connect(&m_settingsSaveTimer, &QTimer::timeout, this, &MainWindowPresenter::flushSettingsSave);

    connect(m_preferencesController.get(),
            &PreferencesController::preferencesChanged,
            this,
            [this](const Preferences &preferences) {
                m_settingsSnapshot.preferences = preferences;
                scheduleSettingsSave();
            });
    connect(
        m_viewState.get(), &MainWindowViewState::calendarSplitterStateChanged, this, [this](const QByteArray &state) {
            m_settingsSnapshot.calendarSplitterState = state;
            scheduleSettingsSave();
        });
    connect(m_viewState.get(), &MainWindowViewState::contactDetailsVisibleChanged, this, [this](bool visible) {
        m_settingsSnapshot.contactDetailsVisible = visible;
        scheduleSettingsSave();
    });
    connect(m_viewState.get(), &MainWindowViewState::taskPaneVisibleChanged, this, [this](bool visible) {
        m_settingsSnapshot.taskPaneVisible = visible;
        scheduleSettingsSave();
    });
}

void MainWindowPresenter::scheduleSettingsSave()
{
    m_settingsSaveTimer.start();
}

void MainWindowPresenter::flushSettingsSave()
{
    if (m_settingsSaveTimer.isActive())
    {
        m_settingsSaveTimer.stop();
    }

    m_settingsSnapshot.preferences = m_preferencesController->preferences();
    m_settingsSnapshot.contactDetailsVisible = m_viewState->contactDetailsVisible();
    m_settingsSnapshot.taskPaneVisible = m_viewState->taskPaneVisible();
    m_settingsSnapshot.calendarSplitterState = m_viewState->calendarSplitterState();
    m_windowSettingsStore->saveWindowGeometry(&m_window, &m_settingsSnapshot);
    m_windowSettingsStore->save(m_settingsSnapshot);
}
