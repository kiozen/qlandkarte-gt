/**********************************************************************************************
    Copyright (C) 2008 Oliver Eichler oliver.eichler@gmx.de

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

#include "IMapSelection.h"
#include "CMapDB.h"
#include "IMap.h"

#include <QtCore>

QString IMapSelection::focusedMap;

QRect IMapSelection::rect()
{
    IMap& map = CMapDB::self().getMap();

    double x1 = lon1;
    double y1 = lat1;
    double x2 = lon2;
    double y2 = lat2;

    map.convertRad2Pt(x1, y1);
    map.convertRad2Pt(x2, y2);

    return QRect(x1, y1, abs(x1 - x2), abs(y1 - y2));
}
