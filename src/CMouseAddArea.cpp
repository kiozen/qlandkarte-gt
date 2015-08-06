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

#include "CMouseAddArea.h"
#include "CCanvas.h"
#include "CMapDB.h"
#include "COverlayDB.h"
#include "COverlayArea.h"

#include <QtGui>

CMouseAddArea::CMouseAddArea(CCanvas *canvas)
: IMouse(canvas)
{
    cursor = QCursor(QPixmap(":/cursors/cursorArea.png"),0,0);
}


void CMouseAddArea::mouseMoveEvent(QMouseEvent * e)
{

}


void CMouseAddArea::mousePressEvent(QMouseEvent * e)
{
    if(e->button() == Qt::LeftButton)
    {
        canvas->setMouseMode(CCanvas::eMouseOverlay);

        pos      = e->pos();
        double x = e->pos().x();
        double y = e->pos().y();
        CMapDB::self().getMap().convertPt2Rad(x,y);
        COverlayArea::pt_t pt;
        pt.u    = x;
        pt.v    = y;
        pt.idx  = 0;
        QList<COverlayArea::pt_t> pts;
        pts << pt;
        selOverlay = COverlayDB::self().addArea("", "", Qt::blue, Qt::NoBrush, pts);
    }
}


void CMouseAddArea::mouseReleaseEvent(QMouseEvent * e)
{

}
