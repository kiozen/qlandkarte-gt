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
#include "CMouseColorPicker.h"
#include "CMapDB.h"
#include "IMap.h"
#include "CMainWindow.h"

#include <QtGui>

CMouseColorPicker::CMouseColorPicker(CCanvas * canvas)
: IMouse(canvas)
, color(Qt::NoPen)
, selected(Qt::NoPen)
{
    cursor = QCursor(QPixmap(":/cursors/cursorColorChooser.png"),3,30);
}


CMouseColorPicker::~CMouseColorPicker()
{

}


void CMouseColorPicker::draw(QPainter& p)
{
    p.setPen(Qt::black);

    if(selected == Qt::NoPen)
    {
        p.setBrush(color);
        p.drawRect(50,50,100,100);
    }
    else
    {
        p.setBrush(color);
        p.drawRect(50,50,50,100);
        p.setBrush(selected);
        p.drawRect(100,50,50,100);
    }
}


void CMouseColorPicker::mouseMoveEvent(QMouseEvent * e)
{
    IMap& map           = CMapDB::self().getMap();
    const QImage& img   = map.getBuffer();

    QColor c = img.pixel(e->pos());

    if(c != color)
    {
        color = c;
        theMainWindow->getCanvas()->update();
    }
}


void CMouseColorPicker::mousePressEvent(QMouseEvent * e)
{
    selected = color;
    theMainWindow->getCanvas()->update();
}


void CMouseColorPicker::mouseReleaseEvent(QMouseEvent * e)
{

}
