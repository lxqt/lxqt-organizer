/***************************************************************************
 *   Author Basil Crow                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/
#ifndef ORGANIZERAPPLICATION_H
#define ORGANIZERAPPLICATION_H

#include <LXQt/Application>

#include <QPointer>
#include <QVector>

#include "dbmanager.h"

class MainWindow;

class OrganizerApplication : public LXQt::Application
{
    Q_OBJECT

public:
    OrganizerApplication(int &argc, char **argv);
    ~OrganizerApplication() override;

    bool init();

public Q_SLOTS:
    void newWindow();
    void activateWindow();

private:
    bool parseCommandLine();

    bool isPrimaryInstance_;
    DbManager dbm_;
    QVector<QPointer<MainWindow>> windows_;
};

#endif // ORGANIZERAPPLICATION_H
