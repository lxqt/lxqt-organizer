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

#ifndef MAINWINDOWACTIONCONTROLLER_H
#define MAINWINDOWACTIONCONTROLLER_H

#include "iactivepane.h"

#include <QList>
#include <QObject>
#include <QPointer>

namespace Ui {
class MainWindow;
}

class CalendarPane;
class CollectionService;
class ContactsPane;
class QAction;
class MainWindowViewState;
class PreferencesController;

class MainWindowActionController : public QObject
{
    Q_OBJECT

public:
    MainWindowActionController(Ui::MainWindow *ui,
                               CollectionService &collectionService,
                               MainWindowViewState &viewState,
                               PreferencesController *preferences,
                               CalendarPane *calendarPane,
                               ContactsPane *contactsPane,
                               QObject *parent = nullptr);

    void setActivePane(IActivePane *pane);
    void setCommandPanes(CalendarPane *calendarPane, ContactsPane *contactsPane);

public Q_SLOTS:
    void refreshActionState();

Q_SIGNALS:
    void newWindowRequested();
    void settingsSaveRequested();

private:
    void scheduleRefreshActionState();
    void connectContextAlias(QAction *alias, const QString &contextActionId);
    void triggerContextAction(const QString &contextActionId);
    QAction *contextAction(const QString &contextActionId) const;
    bool contextActionAvailable(const QString &contextActionId) const;
    void refreshContextAlias(QAction *alias, const QString &contextActionId);
    void refreshContextAliases();
    void closeWindow();
    void showNextMonth();
    void showPreviousMonth();
    void showToday();
    void increaseFont();
    void decreaseFont();
    void resetFont();
    void toggleQuickFullView();
    void updateViewToggleAction();

    Ui::MainWindow *m_ui = nullptr;
    CollectionService &m_collectionService;
    MainWindowViewState &m_viewState;
    PreferencesController *m_preferences = nullptr;
    CalendarPane *m_calendarPane = nullptr;
    ContactsPane *m_contactsPane = nullptr;
    IActivePane *m_activePane = nullptr;
    QPointer<QObject> m_activePaneObject;
    QList<QMetaObject::Connection> m_contextActionConnections;
    bool m_refreshActionStateQueued = false;
};

#endif // MAINWINDOWACTIONCONTROLLER_H
