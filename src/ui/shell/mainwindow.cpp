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

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "applicationcontext.h"
#include "mainwindow_composition.h"
#include "mainwindowpresenter.h"

#include <QCloseEvent>

#include <utility>

MainWindow::MainWindow(ApplicationContext &context, QWidget *parent)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
{
    m_ui->setupUi(this);
    MainWindowComposition products = composeMainWindow(*this, *m_ui, context);
    WindowServices *windowServices = products.windowServices.get();
    products = completeMainWindowComposition(std::move(products), *this, *m_ui, context, *windowServices);
    m_presenter = std::make_unique<MainWindowPresenter>(*this, context, std::move(products));
}

ApplicationContext &MainWindow::applicationContext()
{
    return m_presenter->applicationContext();
}

PreferencesController *MainWindow::preferencesController() const
{
    return m_presenter ? m_presenter->preferencesController() : nullptr;
}

ReloadCoordinator *MainWindow::dataReloadCoordinator() const
{
    return m_presenter ? m_presenter->dataReloadCoordinator() : nullptr;
}

const CalendarPane *MainWindow::calendarPane() const
{
    return m_presenter ? m_presenter->calendarPane() : nullptr;
}

const ContactsPane *MainWindow::contactsPane() const
{
    return m_presenter ? m_presenter->contactsPane() : nullptr;
}

MainWindow::~MainWindow()
{
    m_presenter.reset();
    delete m_ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_presenter)
    {
        m_presenter->onCloseRequested(event);
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::applyApplicationFontSize(int fontSize)
{
    if (m_presenter)
    {
        m_presenter->applyApplicationFontSize(fontSize);
    }
}

void MainWindow::saveSettings()
{
    if (m_presenter)
    {
        m_presenter->saveSettings();
    }
}
