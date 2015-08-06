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

#include "CMouseCutTrack.h"
#include "CTrackDB.h"
#include "CTrack.h"
#include "CMapDB.h"
#include "IMap.h"

#include <QtGui>

CMouseCutTrack::CMouseCutTrack(CCanvas * canvas)
: IMouse(canvas)
, nextTrkPt(0)
{
    cursor = QCursor(QPixmap(":/cursors/cursorCutTrack.png"),0,0);
}


CMouseCutTrack::~CMouseCutTrack()
{

}


void CMouseCutTrack::draw(QPainter& p)
{
    drawSelTrkPt(p);

    IMap& map = CMapDB::self().getMap();
    if(nextTrkPt)
    {
        double u1 = nextTrkPt->lon * DEG_TO_RAD;
        double v1 = nextTrkPt->lat * DEG_TO_RAD;
        map.convertRad2Pt(u1,v1);

        double u2 = selTrkPt->lon * DEG_TO_RAD;
        double v2 = selTrkPt->lat * DEG_TO_RAD;
        map.convertRad2Pt(u2,v2);

        p.setPen(QPen(Qt::black, 5));
        p.drawLine(u1, v1, u2, v2);
        p.setPen(QPen(Qt::red, 3));
        p.drawLine(u1, v1, u2, v2);

        p.setPen(Qt::black);
        p.setBrush(Qt::red);
        p.drawEllipse(QRect(u1 - 5,  v1 - 5, 11, 11));

        p.setPen(Qt::black);
        p.setBrush(Qt::red);
        p.drawEllipse(QRect(u2 - 5,  v2 - 5, 11, 11));

    }
}


void CMouseCutTrack::mouseMoveEvent(QMouseEvent * e)
{
    nextTrkPt = 0;
    mouseMoveEventTrack(e);

    if(selTrkPt == 0) return;

    CTrack * track = CTrackDB::self().highlightedTrack();
    if(track == 0) return;

    int idx = 0;
    if(selTrkPt)
    {
        QList<CTrack::pt_t>& trkpts = track->getTrackPoints();
        idx = trkpts.indexOf(*selTrkPt);
        while(idx < trkpts.size())
        {
            if(&trkpts[idx] != selTrkPt && !(trkpts[idx].flags & CTrack::pt_t::eDeleted))
            {
                break;
            }

            ++idx;
        }
        if(idx < trkpts.size())
        {
            nextTrkPt = &trkpts[idx];
        }
    }
    //     qDebug() << nextTrkPt->lon << nextTrkPt->lat;
}


void CMouseCutTrack::mousePressEvent(QMouseEvent * e)
{
    if(selTrkPt && nextTrkPt && selTrkPt != nextTrkPt)
    {
        CTrackDB::self().splitTrack(selTrkPt->idx);
    }
}


void CMouseCutTrack::mouseReleaseEvent(QMouseEvent * e)
{

}
