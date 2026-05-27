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

#include "mainwindowactioncontroller.h"
#include "ui_mainwindow.h"

#include "calendarpane.h"
#include "collectionservice.h"
#include "contactspane.h"
#include "mainwindowviewstate.h"
#include "preferencescontroller.h"

#include <QAction>
#include <QApplication>
#include <QMetaObject>
#include <QSignalBlocker>
#include <QStringList>
#include <QTabWidget>
#include <QTimer>
#include <QWidget>

#include <utility>

namespace {

QStringList contextActionIds(const QAction *action)
{
    if (!action)
    {
        return {};
    }

    QStringList ids = action->property("contextActionIds").toStringList();
    if (ids.isEmpty() && !action->objectName().isEmpty())
    {
        ids.append(action->objectName());
    }
    return ids;
}

QAction *contextActionInPane(IActivePane *pane, const QString &contextActionId)
{
    if (!pane)
    {
        return nullptr;
    }

    for (QAction *action : pane->contextActions())
    {
        if (contextActionIds(action).contains(contextActionId))
        {
            return action;
        }
    }
    return nullptr;
}

bool anyWritable(const QList<Collection> &collections)
{
    for (const Collection &collection : collections)
    {
        if (collection.isWritable())
        {
            return true;
        }
    }
    return false;
}

} // namespace

MainWindowActionController::MainWindowActionController(Ui::MainWindow *ui,
                                                       CollectionService &collectionService,
                                                       MainWindowViewState &viewState,
                                                       PreferencesController *preferences,
                                                       CalendarPane *calendarPane,
                                                       ContactsPane *contactsPane,
                                                       QObject *parent)
    : QObject(parent)
    , m_ui(ui)
    , m_collectionService(collectionService)
    , m_viewState(viewState)
    , m_preferences(preferences)
    , m_calendarPane(calendarPane)
    , m_contactsPane(contactsPane)
{
    Q_ASSERT(m_ui && m_preferences && m_calendarPane && m_contactsPane);

    connect(&m_collectionService,
            &CollectionService::collectionsReloaded,
            this,
            &MainWindowActionController::scheduleRefreshActionState);
    connect(&m_viewState,
            &MainWindowViewState::contactDetailsVisibleChanged,
            this,
            &MainWindowActionController::scheduleRefreshActionState);
    connect(&m_viewState,
            &MainWindowViewState::taskPaneVisibleChanged,
            this,
            &MainWindowActionController::scheduleRefreshActionState);

    connect(m_ui->actionExit, &QAction::triggered, qApp, &QApplication::quit);
    connect(m_ui->actionNew_Window, &QAction::triggered, this, &MainWindowActionController::newWindowRequested);
    connect(m_ui->actionClose_Window, &QAction::triggered, this, &MainWindowActionController::closeWindow);
    connect(m_ui->actionNext_Month, &QAction::triggered, this, &MainWindowActionController::showNextMonth);
    connect(m_ui->actionPrevious_Month, &QAction::triggered, this, &MainWindowActionController::showPreviousMonth);
    connect(m_ui->actionToday, &QAction::triggered, this, &MainWindowActionController::showToday);
    connect(m_ui->actionIncrease_Font, &QAction::triggered, this, &MainWindowActionController::increaseFont);
    connect(m_ui->actionDecrease_Font, &QAction::triggered, this, &MainWindowActionController::decreaseFont);
    connect(m_ui->actionReset_Font, &QAction::triggered, this, &MainWindowActionController::resetFont);
    connect(m_ui->actionQuick_Full_View, &QAction::triggered, this, &MainWindowActionController::toggleQuickFullView);
    connectContextAlias(m_ui->actionNew_Event, QStringLiteral("actionNew_Event"));
    connectContextAlias(m_ui->actionNew_Task, QStringLiteral("actionNew_Task"));
    connectContextAlias(m_ui->actionNew_Contact, QStringLiteral("actionNew_Contact"));
    connectContextAlias(m_ui->actionEdit, QStringLiteral("actionEdit"));
    connectContextAlias(m_ui->actionDelete, QStringLiteral("actionDelete"));
    connectContextAlias(m_ui->actionFind, QStringLiteral("actionFind"));
    connectContextAlias(m_ui->actionFindNext, QStringLiteral("actionFindNext"));
    connectContextAlias(m_ui->actionFindPrev, QStringLiteral("actionFindPrev"));
    connectContextAlias(m_ui->actionMail_To, QStringLiteral("actionMail_To"));
}

void MainWindowActionController::setActivePane(IActivePane *pane)
{
    Q_ASSERT(m_ui && m_preferences && m_calendarPane && m_contactsPane);

    if (m_activePaneObject)
    {
        disconnect(m_activePaneObject, nullptr, this, nullptr);
    }
    for (const QMetaObject::Connection &connection : std::as_const(m_contextActionConnections))
    {
        disconnect(connection);
    }
    m_contextActionConnections.clear();

    m_activePane = pane;
    m_activePaneObject = dynamic_cast<QObject *>(pane);
    if (m_activePaneObject)
    {
        connect(m_activePaneObject, SIGNAL(actionStateChanged()), this, SLOT(refreshActionState()));
        for (QAction *action : m_activePane->contextActions())
        {
            if (action)
            {
                m_contextActionConnections.append(
                    connect(action, &QAction::changed, this, &MainWindowActionController::scheduleRefreshActionState));
            }
        }
    }
    refreshActionState();
}

void MainWindowActionController::setCommandPanes(CalendarPane *calendarPane, ContactsPane *contactsPane)
{
    Q_ASSERT(m_ui && m_preferences && calendarPane && contactsPane);

    m_calendarPane = calendarPane;
    m_contactsPane = contactsPane;
}

void MainWindowActionController::scheduleRefreshActionState()
{
    Q_ASSERT(m_ui && m_preferences && m_calendarPane && m_contactsPane);

    if (m_refreshActionStateQueued)
    {
        return;
    }

    m_refreshActionStateQueued = true;
    QTimer::singleShot(0, this, [this]() {
        m_refreshActionStateQueued = false;
        refreshActionState();
    });
}

void MainWindowActionController::refreshActionState()
{
    Q_ASSERT(m_ui && m_preferences && m_calendarPane && m_contactsPane);

    refreshContextAliases();
    updateViewToggleAction();
}

void MainWindowActionController::connectContextAlias(QAction *alias, const QString &contextActionId)
{
    if (!alias)
    {
        return;
    }
    connect(alias, &QAction::triggered, this, [this, contextActionId]() { triggerContextAction(contextActionId); });
}

void MainWindowActionController::triggerContextAction(const QString &contextActionId)
{
    Q_ASSERT(m_ui && m_preferences && m_calendarPane && m_contactsPane);

    QAction *action = contextAction(contextActionId);
    if (action && contextActionAvailable(contextActionId))
    {
        action->trigger();
    }
}

QAction *MainWindowActionController::contextAction(const QString &contextActionId) const
{
    if (contextActionId == QLatin1String("actionNew_Event") || contextActionId == QLatin1String("actionNew_Task"))
    {
        return contextActionInPane(m_calendarPane, contextActionId);
    }
    if (contextActionId == QLatin1String("actionNew_Contact"))
    {
        return contextActionInPane(m_contactsPane, contextActionId);
    }

    return contextActionInPane(m_activePane, contextActionId);
}

bool MainWindowActionController::contextActionAvailable(const QString &contextActionId) const
{
    const QAction *action = contextAction(contextActionId);
    if (!action || !action->isEnabled())
    {
        return false;
    }

    const QString objectName = action->objectName();
    if (objectName == QLatin1String("actionNew_Event") || objectName == QLatin1String("actionNew_Task"))
    {
        return anyWritable(m_collectionService.calendarList());
    }
    if (objectName == QLatin1String("actionNew_Contact"))
    {
        return anyWritable(m_collectionService.addressBookList());
    }
    return true;
}

void MainWindowActionController::refreshContextAlias(QAction *alias, const QString &contextActionId)
{
    if (!alias)
    {
        return;
    }

    QAction *source = contextAction(contextActionId);
    const bool available = contextActionAvailable(contextActionId);
    QSignalBlocker blocker(alias);
    alias->setEnabled(available);
    if (!source)
    {
        return;
    }

    alias->setText(source->text());
    alias->setIconText(source->iconText());
    alias->setIcon(source->icon());
    alias->setToolTip(source->toolTip());
    alias->setStatusTip(source->statusTip());
    alias->setWhatsThis(source->whatsThis());
}

void MainWindowActionController::refreshContextAliases()
{
    refreshContextAlias(m_ui->actionNew_Event, QStringLiteral("actionNew_Event"));
    refreshContextAlias(m_ui->actionNew_Task, QStringLiteral("actionNew_Task"));
    refreshContextAlias(m_ui->actionNew_Contact, QStringLiteral("actionNew_Contact"));
    refreshContextAlias(m_ui->actionEdit, QStringLiteral("actionEdit"));
    refreshContextAlias(m_ui->actionDelete, QStringLiteral("actionDelete"));
    refreshContextAlias(m_ui->actionFind, QStringLiteral("actionFind"));
    refreshContextAlias(m_ui->actionFindNext, QStringLiteral("actionFindNext"));
    refreshContextAlias(m_ui->actionFindPrev, QStringLiteral("actionFindPrev"));
    refreshContextAlias(m_ui->actionMail_To, QStringLiteral("actionMail_To"));
}

void MainWindowActionController::closeWindow()
{
    Q_ASSERT(m_ui && m_preferences && m_calendarPane && m_contactsPane);

    if (QWidget *window = qobject_cast<QWidget *>(parent()))
    {
        window->close();
    }
}

void MainWindowActionController::showNextMonth()
{
    Q_ASSERT(m_ui && m_preferences && m_calendarPane && m_contactsPane);

    m_calendarPane->gotoNextMonth();
}

void MainWindowActionController::showPreviousMonth()
{
    Q_ASSERT(m_ui && m_preferences && m_calendarPane && m_contactsPane);

    m_calendarPane->gotoPreviousMonth();
}

void MainWindowActionController::showToday()
{
    Q_ASSERT(m_ui && m_preferences && m_calendarPane && m_contactsPane);

    m_calendarPane->gotoToday();
}

void MainWindowActionController::increaseFont()
{
    Q_ASSERT(m_ui && m_preferences && m_calendarPane && m_contactsPane);

    m_preferences->setApplicationFontSize(m_preferences->applicationFontSize() + 1);
}

void MainWindowActionController::decreaseFont()
{
    Q_ASSERT(m_ui && m_preferences && m_calendarPane && m_contactsPane);

    m_preferences->setApplicationFontSize(m_preferences->applicationFontSize() - 1);
}

void MainWindowActionController::resetFont()
{
    Q_ASSERT(m_ui && m_preferences && m_calendarPane && m_contactsPane);

    m_preferences->resetApplicationFontSize();
}

void MainWindowActionController::toggleQuickFullView()
{
    Q_ASSERT(m_ui && m_preferences && m_calendarPane && m_contactsPane);

    if (m_ui->tabWidget->currentWidget() == m_contactsPane)
    {
        m_viewState.setContactDetailsVisible(!m_contactsPane->contactDetailsVisible());
    }
    else
    {
        m_viewState.setTaskPaneVisible(!m_viewState.taskPaneVisible());
    }
    Q_EMIT settingsSaveRequested();
}

void MainWindowActionController::updateViewToggleAction()
{
    Q_ASSERT(m_ui && m_preferences && m_calendarPane && m_contactsPane);

    if (m_ui->tabWidget->currentWidget() == m_contactsPane)
    {
        const bool detailsVisible = m_contactsPane->contactDetailsVisible();
        m_ui->actionQuick_Full_View->setChecked(detailsVisible);
        m_ui->actionQuick_Full_View->setText(tr("Full View"));
        m_ui->actionQuick_Full_View->setToolTip(detailsVisible ? tr("Show full contact view")
                                                               : tr("Show quick contact view"));
        return;
    }

    const bool taskPaneVisible = m_viewState.taskPaneVisible();
    m_ui->actionQuick_Full_View->setChecked(taskPaneVisible);
    m_ui->actionQuick_Full_View->setText(tr("Tasks"));
    m_ui->actionQuick_Full_View->setToolTip(taskPaneVisible ? tr("Hide tasks") : tr("Show tasks"));
}
