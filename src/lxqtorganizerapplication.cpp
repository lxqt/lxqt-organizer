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
#include "lxqtorganizerapplication.h"
#include "mainwindow.h"
#include "lxqtorganizerapplicationadaptor.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusInterface>

namespace {

const char serviceName[] = "org.lxqt.lxqt-organizer";
const char ifaceName[] = "org.lxqt.LXQtOrganizer.Application";
const char objectPath[] = "/Application";

} // namespace

LXQtOrganizerApplication::LXQtOrganizerApplication(int &argc, char **argv) :
    LXQt::Application(argc, argv),
    isPrimaryInstance_(false)
{
    setApplicationName(QStringLiteral("lxqt-organizer"));
    setDesktopFileName(QStringLiteral("lxqt-organizer"));

    const QString versionInfo = QStringLiteral(LXQT_ORGANIZER_VERSION
                                               "\nliblxqt   " LXQT_VERSION
                                               "\nQt        " QT_VERSION_STR);
    setApplicationVersion(versionInfo);
}

LXQtOrganizerApplication::~LXQtOrganizerApplication()
{
    while (!windows_.isEmpty())
    {
        delete windows_.takeLast();
    }
}

bool LXQtOrganizerApplication::init()
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    if (!dbus.isConnected() || dbus.registerService(QLatin1String(serviceName)))
    {
        isPrimaryInstance_ = true;
        dbm_.openDatabase();
        dbm_.createDatebaseTables();
        if (dbus.isConnected())
        {
            new LXQtOrganizerApplicationAdaptor(this);
            dbus.registerObject(QLatin1String(objectPath), this);
        }
    }

    return parseCommandLine();
}

bool LXQtOrganizerApplication::parseCommandLine()
{
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("LXQt Organizer"));
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

void LXQtOrganizerApplication::newWindow()
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

void LXQtOrganizerApplication::activateWindow()
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
