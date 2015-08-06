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
#include "CMapSelectionRaster.h"

#include <QtGui>
#include <proj_api.h>
#ifdef __MINGW32__
#undef LP
#endif
#include "GeoMath.h"
#include "CMapDB.h"
#include "IMap.h"

CMapSelectionRaster::CMapSelectionRaster(subtype_e subtype, QObject * parent)
: IMapSelection(eRaster, subtype, parent)
{
    type = eRaster;

    iconPixmap = QPixmap(":/icons/iconRaster16x16.png");
}


CMapSelectionRaster::~CMapSelectionRaster()
{

}


QDataStream& CMapSelectionRaster::operator>>(QDataStream& s)
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
    s1 << subtype;

    entries << entryBase;
    //---------------------------------------
    // prepare raster specific data
    //---------------------------------------
    sel_head_entry_t entryRaster;
    entryRaster.type = eHeadRaster;
    QDataStream s2(&entryRaster.data, QIODevice::WriteOnly);
    s2.setVersion(QDataStream::Qt_4_5);

    s2 << selTiles;

    entries << entryRaster;
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


void CMapSelectionRaster::draw(QPainter& p, const QRect& rect)
{
    IMap& map = CMapDB::self().getMap();

    double u1 = lon1;
    double v1 = lat1;
    double u2 = lon2;
    double v2 = lat2;

    map.convertRad2Pt(u1, v1);
    map.convertRad2Pt(u2, v2);

    p.setBrush(Qt::NoBrush);

    if(focusedMap == getKey())
    {
        p.setPen(QPen(Qt::red,2));
    }
    else if(mapkey == map.getKey())
    {
        p.setPen(QPen(Qt::darkBlue,2));
    }
    else
    {
        p.setPen(QPen(Qt::gray,2));
    }

    QRect r(u1, v1, u2 - u1, v2 - v1);
    if(rect.intersects(r))
    {

        qint32 gridspace = map.scalePixelGrid(1024);

        if(gridspace != 0)
        {
            int pxx,pxy, x, y;
            for(pxx = r.left(), x = 0; pxx < r.right(); pxx += gridspace, x++)
            {
                for(pxy = r.top(), y = 0; pxy < r.bottom(); pxy += gridspace, y++)
                {
                    int w = (r.right() - pxx) > gridspace ? gridspace : (r.right() - pxx);
                    int h = (r.bottom() - pxy) > gridspace ? gridspace : (r.bottom() - pxy);
                    QRect rect(pxx,pxy, w, h);

                    QPair<int,int> index(x,y);

                    if(!selTiles.contains(index))
                    {
                        selTiles[index] = false;
                    }

                    if(selTiles[index])
                    {
                        p.setBrush(Qt::NoBrush);
                    }
                    else
                    {
                        p.setBrush(QColor(150,150,255,100));
                    }

                    p.drawRect(rect);

                }
            }
        }
        else
        {
            p.setBrush(QColor(150,150,255,100));
            p.drawRect(r);
        }
        CCanvas::drawText(getDescription(),p,r);
    }
}


QString CMapSelectionRaster::getDescription() const
{
    QString pos1, pos2, str;
    GPS_Math_Deg_To_Str(lon1 * RAD_TO_DEG, lat1 * RAD_TO_DEG, pos1);
    GPS_Math_Deg_To_Str(lon2 * RAD_TO_DEG, lat2 * RAD_TO_DEG, pos2);

    double a1, a2;
    projXY p1, p2;

    p1.u = lon1;
    p1.v = lat1;

    p2.u = lon2;
    p2.v = lat1;

    distance(p1, p2, a1, a2) / 1000.0;

    p1.u = lon1;
    p1.v = lat1;

    p2.u = lon1;
    p2.v = lat2;

    distance(p1, p2, a1, a2) / 1000.0;

    int tileCount = 0, i;
    foreach(i, selTiles)
    {
        tileCount += i ? 0 : 1;
    }

    str  = name;
    str += "\n" + tr("Tiles: #%1").arg(tileCount);
    return str;

}
