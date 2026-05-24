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
#include "organizerapplication.h"
#include "mainwindow.h"
#include "organizerapplicationadaptor.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusInterface>

namespace {

const char serviceName[] = "org.lxqt.Organizer";
const char ifaceName[] = "org.lxqt.Organizer.Application";
const char objectPath[] = "/Application";

} // namespace

OrganizerApplication::OrganizerApplication(int &argc, char **argv) :
    LXQt::Application(argc, argv),
    isPrimaryInstance_(false)
{
    const QString versionInfo = QStringLiteral(ORGANIZER_VERSION
                                               "\nliblxqt   " LXQT_VERSION
                                               "\nQt        " QT_VERSION_STR);
    setApplicationVersion(versionInfo);
}

OrganizerApplication::~OrganizerApplication()
{
    while (!windows_.isEmpty())
    {
        delete windows_.takeLast();
    }
}

bool OrganizerApplication::init()
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    if (!dbus.isConnected() || dbus.registerService(QLatin1String(serviceName)))
    {
        isPrimaryInstance_ = true;
        dbm_.openDatabase();
        dbm_.createDatebaseTables();
        if (dbus.isConnected())
        {
            new OrganizerApplicationAdaptor(this);
            dbus.registerObject(QLatin1String(objectPath), this);
        }
    }

    return parseCommandLine();
}

bool OrganizerApplication::parseCommandLine()
{
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Organizer"));
    parser.addVersionOption();
    parser.addHelpOption();

    QCommandLineOption newWindowOption(QStringLiteral("win"),
                                       tr("Open new window"));
    parser.addOption(newWindowOption);

    parser.process(*this);

    if (isPrimaryInstance_)
    {
        newWindow();
        return true;
    }

    QDBusInterface iface(QLatin1String(serviceName),
                         QLatin1String(objectPath),
                         QLatin1String(ifaceName),
                         QDBusConnection::sessionBus(),
                         this);
    if (parser.isSet(newWindowOption))
        iface.call(QStringLiteral("newWindow"));
    else
        iface.call(QStringLiteral("activateWindow"));

    return false;
}

void OrganizerApplication::newWindow()
{
    auto *window = new MainWindow(dbm_);
    window->setAttribute(Qt::WA_DeleteOnClose);
    connect(window, &QObject::destroyed, this, [this, window] {
        windows_.removeAll(window);
    });
    windows_.append(window);
    window->show();
    window->raise();
    window->activateWindow();
}

void OrganizerApplication::activateWindow()
{
    windows_.removeAll(nullptr);
    if (windows_.isEmpty())
    {
        newWindow();
        return;
    }

    MainWindow *window = windows_.constLast();
    window->show();
    window->raise();
    window->activateWindow();
}
