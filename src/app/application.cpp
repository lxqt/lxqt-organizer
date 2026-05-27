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

#include "application.h"
#include "lxqtorganizerapplicationadaptor.h"
#include "applicationcontext.h"
#include "mainwindow.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusInterface>

namespace {

const char serviceName[] = "org.lxqt.lxqt-organizer";
const char ifaceName[] = "org.lxqt.LXQtOrganizer.Application";
const char objectPath[] = "/Application";

} // namespace

LXQtOrganizerApplication::LXQtOrganizerApplication(int &argc, char **argv)
    : LXQt::Application(argc, argv)
    , isPrimaryInstance_(false)
    , context_(new ApplicationContext)
{
    setApplicationName(QStringLiteral("lxqt-organizer"));
    setDesktopFileName(QStringLiteral("lxqt-organizer"));

    const QString versionInfo =
        QStringLiteral(LXQT_ORGANIZER_VERSION "\nliblxqt   " LXQT_VERSION "\nQt        " QT_VERSION_STR);
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
    if (dbus.isConnected() && dbus.registerService(QLatin1String(serviceName)))
    {
        isPrimaryInstance_ = true;
        new LXQtOrganizerApplicationAdaptor(this);
        dbus.registerObject(QLatin1String(objectPath), this);
    }
    else if (!dbus.isConnected())
    {
        isPrimaryInstance_ = true;
    }

    return parseCommandLine();
}

bool LXQtOrganizerApplication::parseCommandLine()
{
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("LXQt Organizer"));
    parser.addVersionOption();
    parser.addHelpOption();

    QCommandLineOption newWindowOption(QStringLiteral("win"), tr("Open new window"));
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
    auto *window = new MainWindow(*context_);
    window->setAttribute(Qt::WA_DeleteOnClose);
    connect(window, &QObject::destroyed, this, [this, window] { windows_.removeAll(window); });
    windows_.append(window);
    showWindow(window);
}

void LXQtOrganizerApplication::activateWindow()
{
    showWindow(activeWindowTarget());
}

bool LXQtOrganizerApplication::hasVisibleWindows() const
{
    for (const QPointer<MainWindow> &window : windows_)
    {
        if (!window.isNull() && window->isVisible())
        {
            return true;
        }
    }
    return false;
}

MainWindow *LXQtOrganizerApplication::activeWindowTarget()
{
    windows_.removeAll(nullptr);
    if (windows_.isEmpty())
    {
        newWindow();
    }

    windows_.removeAll(nullptr);
    return windows_.isEmpty() ? nullptr : windows_.constLast();
}

void LXQtOrganizerApplication::showWindow(MainWindow *window)
{
    if (!window)
    {
        return;
    }

    window->show();
    window->raise();
    window->activateWindow();
}
