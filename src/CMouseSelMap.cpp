/**********************************************************************************************
    Copyright (C) 2007 Oliver Eichler oliver.eichler@gmx.de

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

#include "CMouseSelMap.h"
#include "CCanvas.h"
#include "CMapDB.h"
#include "CMapSelectionRaster.h"

#include <QtGui>
#include <QMenu>

CMouseSelMap::CMouseSelMap(CCanvas * canvas)
: IMouse(canvas)
, selArea(false)
, moveMapSel(false)
{
    rectMoveMapSel  = QRect(0,0,64,64);
    cursor          = QCursor(QPixmap(":/cursors/cursorSelMap.png"),0,0);
}


CMouseSelMap::~CMouseSelMap()
{

}


void CMouseSelMap::draw(QPainter& p)
{
    drawSelMap(p);
    drawSelArea(p);
}


void CMouseSelMap::drawSelMap(QPainter& p)
{
    if(selMap.isNull())
    {
        return;
    }

    p.setPen(QPen(Qt::yellow,2));
    p.setBrush(Qt::NoBrush);
    QRect r1 = selMap->rect();   //u1, v1, u2 - u1, v2 - v1);
    p.drawRect(r1);

    rectMoveMapSel.moveTopLeft(r1.center() - QPoint(32,32));
    p.drawPixmap(rectMoveMapSel.topLeft(), QPixmap(":/icons/iconMove64x64.png"));
}


void CMouseSelMap::drawSelArea(QPainter& p)
{
    if(!selArea)
    {
        return;
    }
    IMap& map = CMapDB::self().getMap();
    qint32 gridspace = selMap.isNull() ? map.scalePixelGrid(TILESIZE) : 0;

    p.setBrush(QColor(150,150,255,100));
    p.setPen(QPen(Qt::darkBlue,2));

    if(map.maptype != IMap::eRaster && map.maptype != IMap::eWMS && map.maptype != IMap::eTMS)
    {
        p.drawRect(rect);
        return;
    }

    if(gridspace != 0)
    {
        // snap grid if parts are too small
        int w = rect.width() % gridspace;
        int h = rect.height() % gridspace;

        if(w) w = gridspace - w;
        if(h) h = gridspace - h;

        rect.adjust(0,0,w,h);
    }
    else
    {
        p.drawRect(rect);
        return;
    }

    selTiles.clear();
    int pxx,pxy, x, y;

    p.setBrush(QColor(150,150,255,100));
    p.setPen(QPen(Qt::darkBlue,2));

    for(pxx = rect.left(), x = 0; pxx < rect.right(); pxx += gridspace, x++)
    {
        for(pxy = rect.top(), y = 0; pxy < rect.bottom(); pxy += gridspace, y++)
        {
            int w = (rect.right() - pxx) > gridspace ? gridspace : (rect.right() - pxx);
            int h = (rect.bottom() - pxy) > gridspace ? gridspace : (rect.bottom() - pxy);
            QRect r(pxx,pxy, w, h);

            QPair<int,int> index(x,y);
            selTiles[index] = false;

            p.drawRect(r);
        }
    }
}


void CMouseSelMap::mouseMoveEvent(QMouseEvent * e)
{
    mousePos = e->pos();

    mouseMoveEventMapSel(e);

    if(moveMapSel && !selMap.isNull())
    {
        IMap& map = CMapDB::self().getMap();
        double u1 = oldPoint.x();
        double v1 = oldPoint.y();
        double u2 = mousePos.x();
        double v2 = mousePos.y();

        map.convertPt2Rad(u1,v1);
        map.convertPt2Rad(u2,v2);

        selMap->lon1 += u2 - u1;
        selMap->lon2 += u2 - u1;

        selMap->lat1 += v2 - v1;
        selMap->lat2 += v2 - v1;

        canvas->update();
    }
    else
    {
        moveMapSel = false;
    }

    if(selArea)
    {
        resizeRect(e->pos());
        canvas->update();
    }

    oldPoint = e->pos();
}


void CMouseSelMap::mousePressEvent(QMouseEvent * e)
{
    if(e->button() == Qt::LeftButton)
    {
        oldPoint = e->pos();

        if(!selMap.isNull())
        {
            if(rectMoveMapSel.contains(e->pos()) && (selMap->type == IMapSelection::eRaster))
            {
                moveMapSel = true;
            }
            else
            {
                mousePressEventMapsel(e);
            }
        }
        else
        {
            startRect(e->pos());
            selArea = true;
        }
    }
    else if(e->button() == Qt::RightButton)
    {
        oldPoint = e->pos();
        canvas->raiseContextMenu(e->pos());
    }
}


void CMouseSelMap::mousePressEventMapsel(QMouseEvent * e)
{
    startRect(e->pos());
    selArea = true;
}


void CMouseSelMap::mouseReleaseEvent(QMouseEvent * e)
{
    if(e->button() == Qt::LeftButton)
    {
        oldPoint = e->pos();

        resizeRect(e->pos());
        rect = rect.normalized();

        if(rect.width() < 2)
        {
            rect.setWidth(2);
        }
        if(rect.height() < 2)
        {
            rect.setHeight(2);
        }

        // snap grid if parts are too small
        IMap& map = CMapDB::self().getMap();
        qint32 gridspace = map.scalePixelGrid(TILESIZE);

        if(selMap.isNull())
        {
            if(gridspace != 0)
            {
                int w = rect.width() % gridspace;
                int h = rect.height() % gridspace;

                if(w) w = gridspace - w;
                if(h) h = gridspace - h;

                rect.adjust(0,0,w,h);
            }

            CMapDB::self().select(rect, selTiles);
            canvas->setMouseMode(CCanvas::eMouseMoveArea);
        }
        else if(selMap->type == IMapSelection::eRaster && !moveMapSel)
        {
            IMap& map = CMapDB::self().getMap();

            CMapSelectionRaster * selRasterMap = (CMapSelectionRaster*)selMap.data();

            double x1 = selMap->lon1;
            double y1 = selMap->lat1;
            double x2 = selMap->lon2;
            double y2 = selMap->lat2;

            map.convertRad2Pt(x1, y1);
            map.convertRad2Pt(x2, y2);

            QRect rectSel(x1,y1, x2 - x1, y2 - y1);
            int pxx, pxy, x , y;
            for(pxx = rectSel.left(), x = 0; pxx < rectSel.right(); pxx += gridspace, x++)
            {
                for(pxy = rectSel.top(), y = 0; pxy < rectSel.bottom(); pxy += gridspace, y++)
                {
                    int w = (rectSel.right() - pxx) > gridspace ? gridspace : (rectSel.right() - pxx);
                    int h = (rectSel.bottom() - pxy) > gridspace ? gridspace : (rectSel.bottom() - pxy);
                    QRect r(pxx,pxy, w, h);

                    QPair<int,int> index(x,y);

                    if(rect.intersects(r))
                    {
                        selRasterMap->selTiles[index] = !selRasterMap->selTiles[index];
                    }
                }
            }
            CMapDB::self().emitSigModified(selRasterMap->getKey());
            CMapDB::self().emitSigChanged();
        }

        moveMapSel  = false;
        selArea     = false;
    }
}


void CMouseSelMap::contextMenu(QMenu& menu)
{
    if(!selMap.isNull() && (selMap->type == IMapSelection::eRaster))
    {
        menu.addSeparator();
        menu.addAction(QPixmap(":/icons/iconOk16x16.png"),tr("Select all tiles"),this,SLOT(slotMapSelAll()));
        menu.addAction(QPixmap(":/icons/iconClear16x16.png"),tr("Select no tiles"),this,SLOT(slotMapSelNone()));
    }
}


void CMouseSelMap::slotMapSelAll()
{
    if(!selMap.isNull())
    {
        CMapSelectionRaster * selRasterMap = dynamic_cast<CMapSelectionRaster*>(selMap.data());
        if(selRasterMap)
        {
            QList< QPair<int, int> > keys = selRasterMap->selTiles.keys();
            QPair<int,int> key;
            foreach(key, keys)
            {
                selRasterMap->selTiles[key] = false;
            }

            CMapDB::self().emitSigChanged();
        }
    }
}


void CMouseSelMap::slotMapSelNone()
{
    if(!selMap.isNull())
    {
        CMapSelectionRaster * selRasterMap = dynamic_cast<CMapSelectionRaster*>(selMap.data());
        if(selRasterMap)
        {

            QList< QPair<int, int> > keys = selRasterMap->selTiles.keys();
            QPair<int,int> key;
            foreach(key, keys)
            {
                selRasterMap->selTiles[key] = true;
            }
            CMapDB::self().emitSigChanged();
        }
    }
}
