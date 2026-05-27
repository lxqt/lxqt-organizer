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

#ifndef STATUSBARPRESENTER_H
#define STATUSBARPRESENTER_H

#include "storageresult.h"

#include <QObject>
#include <QPointer>
#include <functional>

class QDate;
class IActivePane;
class QLabel;
class QLocale;
class QStatusBar;

class StatusBarPresenter : public QObject
{
    Q_OBJECT

public:
    explicit StatusBarPresenter(QStatusBar *statusBar, QLabel *itemCountLabel = nullptr, QObject *parent = nullptr);

    void setLocaleProvider(std::function<QLocale()> localeProvider);
    void setActivePane(IActivePane *pane);
    void setSelectedDate(const QDate &date, const QLocale &locale);

public Q_SLOTS:
    void setSelectedDate(const QDate &date);
    void showReadFailures(const QList<ReadFailure> &readFailures);
    void refresh();

private:
    QStatusBar *m_statusBar = nullptr;
    QLabel *m_itemCountLabel = nullptr;
    QLabel *m_selectedDateLabel = nullptr;
    IActivePane *m_activePane = nullptr;
    QPointer<QObject> m_activePaneObject;
    std::function<QLocale()> m_localeProvider;
};

#endif // STATUSBARPRESENTER_H
