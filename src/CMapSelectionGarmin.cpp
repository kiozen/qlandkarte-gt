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
#include "CMapSelectionGarmin.h"
#include "CMapDB.h"
#include "IMap.h"

#include <QtGui>
#include <proj_api.h>
#ifdef __MINGW32__
#undef LP
#endif

CMapSelectionGarmin::CMapSelectionGarmin(QObject * parent)
: IMapSelection(eVector, eGarmin, parent)
, tilecnt(0)
{
    iconPixmap = QPixmap(":/icons/iconMap16x16.png");
    setName("Garmin");
}


CMapSelectionGarmin::~CMapSelectionGarmin()
{

}


QDataStream& CMapSelectionGarmin::operator>>(QDataStream& s)
{
    QList<sel_head_entry_t> entries;

    //---------------------------------------
    // prepare base data
    //---------------------------------------
    sel_head_entry_t entryBase;
    entryBase.type = eHeadBase;
    QDataStream s1(&entryBase.data, QIODevice::WriteOnly);
    s1.setVersion(QDataStream::Qt_4_5);

    s1 << (qint32)type;
    s1 << getKey();
    s1 << mapkey;
    s1 << timestamp;
    s1 << name;
    s1 << comment;
    s1 << description;
    s1 << lon1;                  ///< top left longitude [rad]
    s1 << lat1;                  ///< top left latitude [rad]
    s1 << lon2;                  ///< bottom right longitude [rad]
    s1 << lat2;                  ///< bottom right latitude [rad]

    entries << entryBase;
    //---------------------------------------
    // prepare raster specific data
    //---------------------------------------
    sel_head_entry_t entryGarmin;
    entryGarmin.type = eHeadGarmin;
    QDataStream s2(&entryGarmin.data, QIODevice::WriteOnly);
    s2.setVersion(QDataStream::Qt_4_5);

    s2 << maps.count();
    QString key1;
    foreach(key1, maps.keys())
    {
        map_t& map = maps[key1];

        s2 << key1;
        s2 << map.unlockKey;
        s2 << map.name;
        s2 << map.typfile;
        s2 << map.mdrfile;
        s2 << map.fid;
        s2 << map.pid;

        s2 << map.tiles.count();
        QString key2;
        foreach(key2, map.tiles.keys())
        {
            tile_t& tile = map.tiles[key2];

            s2 << key2;
            s2 << tile.id;
            s2 << tile.name;
            s2 << tile.filename;
            s2 << tile.u;
            s2 << tile.v;
            s2 << tile.memSize;
            s2 << tile.area;
            s2 << tile.fid;
            s2 << tile.pid;
        }
    }

    entries << entryGarmin;
    //---------------------------------------
    // prepare terminator
    //---------------------------------------
    sel_head_entry_t entryEnd;
    entryEnd.type = eHeadEnd;
    entries << entryEnd;

    //---------------------------------------
    //---------------------------------------
    // now start to actually write data;
    //---------------------------------------
    //---------------------------------------
    // write magic key
    s.writeRawData("QLMapSel",9);

    // calculate offset table
    quint32 offset = entries.count() * 8 + 9;

    QList<sel_head_entry_t>::iterator entry = entries.begin();
    while(entry != entries.end())
    {
        entry->offset = offset;
        offset += entry->data.size() + sizeof(quint32);
        ++entry;
    }

    // write offset table
    entry = entries.begin();
    while(entry != entries.end())
    {
        s << entry->type << entry->offset;
        ++entry;
    }

    // write entry data
    entry = entries.begin();
    while(entry != entries.end())
    {
        s << entry->data;
        ++entry;
    }

    return s;
}


void CMapSelectionGarmin::draw(QPainter& p, const QRect& rect)
{
    int n;
    IMap& theMap = CMapDB::self().getMap();

    p.setBrush(QColor(150,150,255,100));

    QMap<QString,map_t>::const_iterator map = maps.begin();
    while(map != maps.end())
    {

        if(focusedMap == "gmapsupp")
        {
            p.setPen(QPen(Qt::red,2));
        }
        else if(map.key() == theMap.getKey())
        {
            p.setPen(QPen(Qt::darkBlue,2));
        }
        else
        {
            p.setPen(QPen(Qt::gray,2));
        }

        QMap<QString, tile_t>::const_iterator tile = map->tiles.begin();
        while(tile != map->tiles.end())
        {

            const QVector<double>& u = tile->u;
            const QVector<double>& v = tile->v;

            QPolygonF line;
            int N = u.size();
            for(n = 0; n < N; ++n)
            {
                double x = u[n];
                double y = v[n];
                theMap.convertRad2Pt(x,y);
                line << QPointF(x,y);
            }

            if(rect.intersects(line.boundingRect().toRect()))
            {
                p.drawPolygon(line);
            }

            ++tile;
        }
        ++map;
    }
}


quint32 CMapSelectionGarmin::getMemSize()
{
    quint32 memSize = 0;

    QMap<QString,map_t>::const_iterator map = maps.begin();
    while(map != maps.end())
    {

        QMap<QString, tile_t>::const_iterator tile = map->tiles.begin();
        while(tile != map->tiles.end())
        {
            memSize += tile->memSize;
            ++tile;
        }
        ++map;
    }
    return memSize;
}


void CMapSelectionGarmin::calcArea()
{
    tilecnt = 0;

    lat1 =  -90.0 * DEG_TO_RAD;
    lon2 = -180.0 * DEG_TO_RAD;
    lat2 =   90.0 * DEG_TO_RAD;
    lon1 =  180.0 * DEG_TO_RAD;

    QMap<QString,map_t>::const_iterator map = maps.begin();
    while(map != maps.end())
    {

        QMap<QString, tile_t>::const_iterator tile = map->tiles.begin();
        while(tile != map->tiles.end())
        {
            const QRectF& r = tile->area;
            if(lat1 < r.top())      lat1 = r.top();
            if(lon2 < r.right())    lon2 = r.right();
            if(lat2 > r.bottom())   lat2 = r.bottom();
            if(lon1 > r.left())     lon1 = r.left();

            ++tile; ++tilecnt;
        }
        ++map;
    }
}
