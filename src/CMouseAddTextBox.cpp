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

#include "CMouseAddTextBox.h"
#include "CCanvas.h"
#include "CMapDB.h"
#include "COverlayDB.h"
#include "COverlayTextBox.h"
#include <QtGui>

CMouseAddTextBox::CMouseAddTextBox(CCanvas * canvas)
: IMouse(canvas)
, selArea(false)
, selAnchor(false)
{
    cursor = QCursor(QPixmap(":/cursors/cursorTextBox.png"),0,0);

}


CMouseAddTextBox::~CMouseAddTextBox()
{

}


void CMouseAddTextBox::draw(QPainter& p)
{

    if(selArea)
    {
        p.setBrush(Qt::white);
        p.setPen(Qt::black);
        p.drawRect(rect);
    }
    if(selAnchor)
    {
        p.setBrush(Qt::white);
        p.setPen(Qt::black);
        p.drawPolygon(COverlayTextBox::makePolyline(anchor, rect));
    }
}


void CMouseAddTextBox::mouseMoveEvent(QMouseEvent * e)
{
    if(selArea)
    {
        resizeRect(e->pos());
    }
    else if(selAnchor)
    {
        anchor = e->pos();
        canvas->update();
    }
}


void CMouseAddTextBox::mousePressEvent(QMouseEvent * e)
{
    if(e->button() == Qt::LeftButton)
    {
        if(!selArea && !selAnchor)
        {
            startRect(e->pos());
            selArea = true;
        }
        else if(selAnchor)
        {
            selAnchor = false;

            double u = anchor.x();
            double v = anchor.y();
            CMapDB::self().getMap().convertPt2Rad(u,v);

            COverlayDB::self().addTextBox("", u, v, anchor, rect);
            canvas->setMouseMode(CCanvas::eMouseMoveArea);
        }
    }
}


void CMouseAddTextBox::mouseReleaseEvent(QMouseEvent * e)
{
    if(e->button() == Qt::LeftButton)
    {
        if(selArea)
        {
            resizeRect(e->pos(), true);
            selArea     = false;
            selAnchor   = true;
            anchor      = e->pos();
        }
    }
}
