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

#include "CMouseSelTrack.h"
#include "CCanvas.h"
#include "CTrackDB.h"

#include <QtGui>

CMouseSelTrack::CMouseSelTrack(CCanvas * canvas)
: IMouse(canvas)
, selTrack(false), unselectTrack(false)
{
    cursor = QCursor(QPixmap(":/cursors/cursorArrow.png"),0,0);
}


CMouseSelTrack::~CMouseSelTrack()
{

}


void CMouseSelTrack::mouseMoveEvent(QMouseEvent * e)
{
    if(!(selTrack || unselectTrack)) return;
    resizeRect(e->pos());
}


void CMouseSelTrack::mousePressEvent(QMouseEvent * e)
{
    if(e->button() == Qt::LeftButton)
    {
        startRect(e->pos());
        selTrack = true;
    } else if (e->button() == Qt::RightButton)
    {
        startRect(e->pos());
        unselectTrack = true;
    }
}


void CMouseSelTrack::mouseReleaseEvent(QMouseEvent * e)
{
    if(e->button() == Qt::LeftButton)
    {
        selTrack = false;
        resizeRect(e->pos());
        CTrackDB::self().select(rect.normalized(),true);
        //         canvas->setMouseMode(CCanvas::eMouseMoveArea);
    }else if (e->button() == Qt::RightButton)
    {
        unselectTrack = false;
        resizeRect(e->pos());
        CTrackDB::self().select(rect.normalized(),false);
    }
}


void CMouseSelTrack::draw(QPainter& p)
{
    if(!(selTrack || unselectTrack)) return;
    drawRect(p);
}
