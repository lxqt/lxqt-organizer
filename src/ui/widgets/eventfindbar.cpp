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

#include "eventfindbar.h"
#include "ui_eventfindbar.h"

#include <QIcon>
#include <QLineEdit>
#include <QShortcut>
#include <QToolButton>

EventFindBar::EventFindBar(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::EventFindBar)
{
    ui->setupUi(this);
    hide();

    ui->toolButtonCloseEventFind->setIcon(QIcon::fromTheme(QStringLiteral("window-close")));
    ui->lineEditEventFind->setPlaceholderText(tr("Find"));
    ui->lineEditEventFind->addAction(QIcon::fromTheme(QStringLiteral("edit-find")), QLineEdit::LeadingPosition);
    ui->toolButtonEventFindPrev->setIcon(QIcon::fromTheme(QStringLiteral("go-up")));
    ui->toolButtonEventFindNext->setIcon(QIcon::fromTheme(QStringLiteral("go-down")));

    connect(ui->toolButtonCloseEventFind, &QToolButton::clicked, this, [this]() {
        hideFind();
        Q_EMIT closeRequested();
    });
    QShortcut *closeEventFindShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    closeEventFindShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(closeEventFindShortcut, &QShortcut::activated, this, [this]() {
        hideFind();
        Q_EMIT closeRequested();
    });
    connect(ui->toolButtonEventFindNext, &QToolButton::clicked, this, [this]() { requestFind(true); });
    connect(ui->toolButtonEventFindPrev, &QToolButton::clicked, this, [this]() { requestFind(false); });
    connect(ui->lineEditEventFind, &QLineEdit::returnPressed, this, [this]() { requestFind(true); });
    connect(ui->lineEditEventFind, &QLineEdit::textChanged, this, &EventFindBar::clearStatus);
}

EventFindBar::~EventFindBar()
{
    delete ui;
}

bool EventFindBar::isFindActive() const
{
    return m_findActive;
}

void EventFindBar::showFind()
{
    m_findActive = true;
    show();
    ui->lineEditEventFind->setFocus(Qt::ShortcutFocusReason);
    ui->lineEditEventFind->selectAll();
}

void EventFindBar::hideFind()
{
    m_findActive = false;
    clearStatus();
    hide();
}

bool EventFindBar::requestFind(bool forward)
{
    if (!m_findActive)
    {
        showFind();
    }

    const QString needle = ui->lineEditEventFind->text();
    if (needle.isEmpty())
    {
        ui->lineEditEventFind->setFocus(Qt::ShortcutFocusReason);
        return false;
    }

    Q_EMIT findRequested(needle, forward);
    return true;
}

void EventFindBar::clearStatus()
{
    ui->labelEventFindStatus->clear();
}

void EventFindBar::showNoMatch()
{
    ui->labelEventFindStatus->setText(tr("No more items"));
}
