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
#ifndef CDBUS_H
#define CDBUS_H

#include <QtDBus>

class CDBus : public QDBusAbstractAdaptor
{
    Q_OBJECT;
    Q_CLASSINFO("D-Bus Interface", "org.QLandkarteGT.dbus")

        public:
        virtual ~CDBus();

    public slots:
        void addGeoData(const QString& filename);
        void loadGeoData(const QString& filename);
        void zoomToRect(const double lon1, const double lat1, const double lon2, const double lat2);
        void setWaypointIcon(const QString& name, const QByteArray& data);

    private:
        friend class CMainWindow;
        CDBus(QObject * parent);
};
#endif                           //CDBUS_H
