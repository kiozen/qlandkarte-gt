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
#ifndef CGARMINTYPNT_H
#define CGARMINTYPNT_H

#include "IGarminTyp.h"

class CGarminTypNT : public IGarminTyp
{
    Q_OBJECT;
    public:
        CGarminTypNT(QObject * parent);
        virtual ~CGarminTypNT();

        bool decode(QDataStream& in, QMap<quint32, polygon_property>& polygons, QMap<quint32, polyline_property>& polylines, QList<quint32>& drawOrder, QMap<quint32, point_property>& points);

    protected:
        typ_section_t sectNT;
};
#endif                           //CGARMINTYPNT_H
