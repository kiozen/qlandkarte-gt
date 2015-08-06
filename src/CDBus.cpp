/**********************************************************************************************
    Copyright (C) 2009 Oliver Eichler oliver.eichler@gmx.de

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

**********************************************************************************************/
#include "CDBus.h"
#include "CMapDB.h"
#include "CWptDB.h"
#include "CWpt.h"
#include "CTrackDB.h"
#include "CTrack.h"
#include "CRouteDB.h"
#include "CRoute.h"
#include "CDiaryDB.h"
#include "COverlayDB.h"
#include "IOverlay.h"
#include "CMainWindow.h"
#include "WptIcons.h"
#include "config.h"

CDBus::CDBus(QObject * parent)
: QDBusAbstractAdaptor(parent)
{
    qDebug() << "QDBusConnection::sessionBus().registerService(\"org.qlandkarte.dbus\")";
    if(!QDBusConnection::sessionBus().registerService("org.qlandkarte.dbus"))
    {
        qWarning() << "failed;";
        return;
    }

    qDebug() << "QDBusConnection::sessionBus().registerObject(\"/dbus\", this)";
    if(!QDBusConnection::sessionBus().registerObject("/", qApp))
    {
        qWarning() << "failed;";
        return;
    }
}


CDBus::~CDBus()
{

}


void CDBus::addGeoData(const QString& filename)
{
    QString filter;
    theMainWindow->loadData(filename, filter);
}


void CDBus::loadGeoData(const QString& filename)
{
    QString filter;

    CMapDB::self().clear();
    CWptDB::self().clear();
    CTrackDB::self().clear();
    CRouteDB::self().clear();
    CDiaryDB::self().clear();
    COverlayDB::self().clear();

    theMainWindow->loadData(filename, filter);
}


void CDBus::zoomToRect(const double lon1, const double lat1, const double lon2, const double lat2)
{
    CMapDB::self().getMap().zoom(lon1 * DEG_TO_RAD, lat1 * DEG_TO_RAD, lon2 * DEG_TO_RAD, lat2 * DEG_TO_RAD);
}


void CDBus::setWaypointIcon(const QString& name, const QByteArray& data)
{
    QPixmap icon;
    icon.loadFromData(data);
    setWptIconByName(name, icon);
}


//void CDBus::setWaypointIconFile(const QString& name, const QString& filename1)
//{
//    QDir dirIcon(QDir::home().filePath(CONFIGDIR "WaypointIcons"));

//    QString filename2 = dirIcon.filePath(name + ".png");

//    QPixmap icon(filename1);
//    icon.save(filename2);

//    setWptIconByName(name, filename2);
//}
