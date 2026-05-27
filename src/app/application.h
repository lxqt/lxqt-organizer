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

#ifndef APPLICATION_H
#define APPLICATION_H

#include <LXQt/Application>

#include <QPointer>
#include <QScopedPointer>
#include <QVector>

class MainWindow;
class ApplicationContext;

class LXQtOrganizerApplication : public LXQt::Application
{
    Q_OBJECT

public:
    LXQtOrganizerApplication(int &argc, char **argv);
    ~LXQtOrganizerApplication() override;

    bool init();

public Q_SLOTS:
    void newWindow();
    void activateWindow();
    bool hasVisibleWindows() const;

private:
    bool parseCommandLine();
    MainWindow *activeWindowTarget();
    void showWindow(MainWindow *window);

    bool isPrimaryInstance_;
    QScopedPointer<ApplicationContext> context_;
    QVector<QPointer<MainWindow>> windows_;
};

#endif // APPLICATION_H
