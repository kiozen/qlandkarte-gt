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
#ifndef CEXCHANGETWONAV_H
#define CEXCHANGETWONAV_H

#include <IExchange.h>

class QDBusObjectPath;

class CTwoNavTreeWidgetItem : public IDeviceTreeWidgetItem
{
    public:
        CTwoNavTreeWidgetItem(const QString& id, QTreeWidget *parent);

        void readDevice();
};

class CExchangeTwoNav : public IExchange
{
    Q_OBJECT
    public:
        CExchangeTwoNav(QTreeWidget *treeWidget, QObject * parent);
        virtual ~CExchangeTwoNav();

    private slots:
        void slotDeviceAdded(const QDBusObjectPath& path, const QVariantMap& map);

};



#endif //CEXCHANGETWONAV_H

