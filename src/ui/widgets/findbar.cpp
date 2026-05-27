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

#include "findbar.h"
#include "ui_findbar.h"

#include <QIcon>
#include <QLineEdit>
#include <QShortcut>
#include <QToolButton>

FindBar::FindBar(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FindBar)
{
    ui->setupUi(this);
    hide();

    ui->toolButtonCloseFind->setIcon(QIcon::fromTheme(QStringLiteral("window-close")));
    ui->lineEditFind->setPlaceholderText(tr("Find"));
    ui->lineEditFind->addAction(QIcon::fromTheme(QStringLiteral("edit-find")), QLineEdit::LeadingPosition);
    ui->toolButtonFindPrev->setIcon(QIcon::fromTheme(QStringLiteral("go-up")));
    ui->toolButtonFindNext->setIcon(QIcon::fromTheme(QStringLiteral("go-down")));

    connect(ui->toolButtonCloseFind, &QToolButton::clicked, this, [this]() {
        hideFind();
        Q_EMIT closeRequested();
    });
    QShortcut *closeFindShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    closeFindShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(closeFindShortcut, &QShortcut::activated, this, [this]() {
        hideFind();
        Q_EMIT closeRequested();
    });
    connect(ui->toolButtonFindNext, &QToolButton::clicked, this, [this]() { requestFind(true); });
    connect(ui->toolButtonFindPrev, &QToolButton::clicked, this, [this]() { requestFind(false); });
    connect(ui->lineEditFind, &QLineEdit::returnPressed, this, [this]() { requestFind(true); });
    connect(ui->lineEditFind, &QLineEdit::textChanged, this, &FindBar::clearStatus);
}

FindBar::~FindBar()
{
    delete ui;
}

bool FindBar::isFindActive() const
{
    return m_findActive;
}

void FindBar::showFind()
{
    m_findActive = true;
    show();
    ui->lineEditFind->setFocus(Qt::ShortcutFocusReason);
    ui->lineEditFind->selectAll();
}

void FindBar::hideFind()
{
    m_findActive = false;
    clearStatus();
    hide();
}

bool FindBar::requestFind(bool forward)
{
    if (!m_findActive)
    {
        showFind();
    }

    const QString needle = ui->lineEditFind->text();
    if (needle.isEmpty())
    {
        ui->lineEditFind->setFocus(Qt::ShortcutFocusReason);
        return false;
    }

    Q_EMIT findRequested(needle, forward);
    return true;
}

void FindBar::clearStatus()
{
    ui->labelFindStatus->clear();
}

void FindBar::showNoMatch()
{
    ui->labelFindStatus->setText(tr("No more items"));
}
