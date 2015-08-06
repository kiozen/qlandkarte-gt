/**********************************************************************************************
    Copyright (C) 2014 Oliver Eichler oliver.eichler@gmx.de

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
#include "CExchangeTwoNav.h"

#include <QtDBus>

CTwoNavTreeWidgetItem::CTwoNavTreeWidgetItem(const QString& id, QTreeWidget *parent)
    : IDeviceTreeWidgetItem(id,parent)
{
    setIcon(0, QIcon("://icons/iconDeviceTwoNav16x16.png"));
}

void CTwoNavTreeWidgetItem::readDevice()
{

}

// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------
CExchangeTwoNav::CExchangeTwoNav(QTreeWidget * treeWidget, QObject * parent)
    : IExchange("General", treeWidget,parent)
{

}

CExchangeTwoNav::~CExchangeTwoNav()
{

}

void CExchangeTwoNav::slotDeviceAdded(const QDBusObjectPath& path, const QVariantMap& map)
{
#ifdef Q_OS_LINUX
    qDebug() << "-----------CExchangeTwoNav::slotDeviceAdded----------";
    qDebug() << path.path() << map;

    QString device = checkForDevice(path);
    if(!device.isEmpty())
    {
        CTwoNavTreeWidgetItem * item = new CTwoNavTreeWidgetItem(path.path(), treeWidget);
        item->setText(0, "TwoNav " + device);
    }
#endif //Q_OS_LINUX
}


