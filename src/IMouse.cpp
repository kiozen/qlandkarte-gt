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

#include "IMouse.h"
#include "CCanvas.h"
#include "CWptDB.h"
#include "CMapDB.h"
#include "CTrackDB.h"
#include "CRouteDB.h"
#include "COverlayDB.h"
#include "CWpt.h"
#include "CMainWindow.h"
#include "CResources.h"
#include "IOverlay.h"
#include "IUnit.h"
#include "IMap.h"
#include "CDlgEditWpt.h"
#include "GeoMath.h"
#include "CSearch.h"
#include "CSearchDB.h"
#include "IMapSelection.h"
#include "CImageViewer.h"
#include <QtGui>

QPointer<IOverlay> IMouse::selOverlay;

QPointF IMouse::pos1Pixel(-1,-1);
QPointF IMouse::pos1LonLat(-1,-1);

IMouse::IMouse(CCanvas * canvas)
: QObject(canvas)
, cursor(QPixmap(":/cursors/cursorArrow.png"))
, canvas(canvas)
, selTrkPt(0)
, selRtePt(0)
, doSpecialCursorWpt(false)
, doSpecialCursorSearch(false)
, doShowWptBuddies(false)
, lockWptCircles(false)
{
    rectIcon            = QRect(-8,-8,16,16);
    rectMarkWpt         = QRect(16,-8,16,16);
    rectDelWpt          = QRect(-5, 3,16,16);
    rectMoveWpt         = QRect(37, 3,16,16);
    rectEditWpt         = QRect(-5,29,16,16);
    rectCopyWpt         = QRect(37,29,16,16);
    rectViewWpt         = QRect(16,40,16,16);

    rectDelSearch       = QRect(0,0,16,16);
    rectConvertSearch   = QRect(0,32,16,16);
    rectCopySearch      = QRect(32,32,16,16);

}


IMouse::~IMouse()
{

}


void IMouse::startRect(const QPoint& p)
{
    rect.setTopLeft(p);
    rect.setSize(QSize(0,0));
}


void IMouse::resizeRect(const QPoint& p, bool normalize)
{
    if (normalize)
    {
        QPoint p0(rect.topLeft());
        rect.setCoords(qMin(p.x(), p0.x()), qMin(p.y(), p0.y()),
            qMax(p.x(), p0.x()), qMax(p.y(), p0.y()));
    }
    else
    {
        rect.setBottomRight(p);
    }
    canvas->update();
}


void IMouse::drawRect(QPainter& p)
{
    p.setBrush(QBrush( QColor(230,230,255,100) ));
    p.setPen(QPen(QColor(255,255,0),3));
    p.drawRect(rect);
}


#define LENGTH 100
void IMouse::drawPos1(QPainter& p)
{
    if(pos1Pixel.x() == -1 || pos1Pixel.y() == -1)
    {
        return;
    }

    IMap& map = CMapDB::self().getMap();
    double u = pos1LonLat.x();
    double v = pos1LonLat.y();
    map.convertRad2Pt(u,v);

    p.setPen(QPen(Qt::white, 5));
    p.drawLine(u - 10, v, u + LENGTH, v);
    p.drawLine(u, v - 10, u, v + LENGTH);

    p.setPen(QPen(Qt::blue, 3));
    p.drawLine(u - 10, v, u + LENGTH, v);
    p.drawLine(u, v - 10, u, v + LENGTH);

    CCanvas::drawText("Pos. 1",p, QRect(u,v, LENGTH, - 20));

    QString fn = map.getFilename(u, v);
    QFileInfo fi(fn);
    CCanvas::drawText(fi.fileName(), p, QRect(u,v, LENGTH, 20));
}


void IMouse::clearSelWpts()
{
    lockWptCircles = false;
    foreach(const wpt_t& wpt, selWpts)
    {
        if(!wpt.wpt.isNull())
        {
            wpt.wpt->showBuddies(false);
        }
    }
    doShowWptBuddies = false;
    selWpts.clear();
}


#define RADIUS_CIRCLES      130
#define RADIUS_CIRCLE       35
#define ANGLE_START         -45
void IMouse::sortSelWpts(QList<wpt_t>& list)
{
    struct d_t{int a[8];};
    struct p_t{int x; int y;};
    p_t pts[8];

    int nWpts       = list.size() > 8 ? 8 : list.size();
    d_t * dist      = new d_t[nWpts];
    QPoint ref      = lockWptCircles ? mousePosWptCircle : mousePos;
    int order[8]    = {-1,-1,-1,-1,-1,-1,-1,-1};

    double deg = ANGLE_START;
    for(int i = 0; i < 8; i++)
    {
        pts[i].x = cos(deg * DEG_TO_RAD) * RADIUS_CIRCLES;
        pts[i].y = sin(deg * DEG_TO_RAD) * RADIUS_CIRCLES;
        deg += 45;
    }

    // sort waypoints to be closest to the reference point
    for(int i = 0; i < list.size(); i++)
    {
        QPoint p0       = QPoint(list[i].xReal, list[i].yReal) - ref;
        list[i].order   = 0x7FFFFFFF;
        list[i].dist    = p0.x()*p0.x() + p0.y()*p0.y();
    }
    qSort(list);

    // calculate distance matrix for the first 8 waypoints
    for(int i = 0; i < nWpts; i++)
    {
        QPoint p0       = QPoint(list[i].xReal, list[i].yReal) - ref;
        list[i].dist    = 0x7FFFFFFF;
        for(int j = 0; j < 8; j++)
        {
            dist[i].a[j] = (pts[j].x - p0.x()) * (pts[j].x - p0.x()) + (pts[j].y - p0.y()) * (pts[j].y - p0.y());
        }
    }

    // now start the sorting hell
    for(int i = 0; i < nWpts; i++)
    {
        // basically we try to find the position in the circle with the shortest distance to the waypoint
        int minDist = 0x7FFFFFFF;
        int minIdx  = -1;
        for(int j = 0; j < 8; j++)
        {
            // if the distance is less than any seen sofar:
            if(dist[i].a[j]  < minDist)
            {
                // test if this position is occupied by any other waypoint than the current one
                if(order[j] != -1 && order[j] != i)
                {
                    // position conflict! who wins?
                    if(list[order[j]].dist > dist[i].a[j])
                    {
                        // the current waypoints has a shorter distance than the one right now on this position
                        minDist = dist[i].a[j];
                        minIdx  = j;

                        // we can't actually replace the waypoints now, as this is still an
                        // intermediate result. maybe a later position is better than this
                        // on. In this case we wouldn't like to replace the waypoint.
                    }
                }
                else
                {
                    // trivial: position is vacant or allready occupied by current waypoint
                    minDist = dist[i].a[j];
                    minIdx  = j;
                }
            }
        }

        // finally, we have a result
        wpt_t& wpt      = list[i];
        wpt.dist        = minDist;
        wpt.order       = minIdx;

        // now we replace the waypoint in the order list.
        int tmp = order[minIdx];
        order[minIdx] = i;

        // repeat the search for the waypoint that has been kicked out
        // @todo manipulating the counter is not a good idea. better to use recursive functions
        if(tmp != -1 && tmp != i) i = tmp - 1;

    }

    // convert positions to coordinates
    for(int i = 0; i < nWpts; i++)
    {
        wpt_t& wpt = list[i];
        wpt.x = pts[wpt.order].x;
        wpt.y = pts[wpt.order].y;
    }

    qSort(list);

    delete [] dist;
}


void IMouse::drawSelWpt(QPainter& p)
{
    if((selWpts.size() == 1) && !selWpts.first().wpt.isNull())
    {
        wpt_t& wptInfo = selWpts.first();
        drawSelWpt(p, wptInfo, eFeatAll /*& (~eFeatName)*/);
    }
    else if(selWpts.size() > 1)
    {
        int idxInfoBox = -1;
        double deg = ANGLE_START;

        QPointF ref = lockWptCircles ? mousePosWptCircle : mousePos;

        p.save();
        p.translate(ref);

        // draw function circles for waypoints
        int count = selWpts.size() < 9 ? selWpts.size() : 7;
        for(int i = 0; i < count; i++)
        {
            wpt_t& wptInfo = selWpts[i];
            if(wptInfo.wpt.isNull())
            {
                continue;
            }

            QPoint pt = mousePos - QPoint(wptInfo.x, wptInfo.y) - mousePosWptCircle;
            if((pt.x()*pt.x() + pt.y()*pt.y()) < (RADIUS_CIRCLE*RADIUS_CIRCLE))
            {
                idxInfoBox = i;
            }

            p.setPen(CCanvas::penBorderBlue);
            QPointF p0 = QPointF(wptInfo.xReal, wptInfo.yReal) - ref;
            QPointF p1 = QPointF(wptInfo.x, wptInfo.y);

            double d = sqrt((p0.x() - p1.x())*(p0.x() - p1.x()) + (p0.y() - p1.y())*(p0.y() - p1.y()));
            double r = 40 / d;
            double x = r * p0.x() + (1 - r) * p1.x();
            double y = r * p0.y() + (1 - r) * p1.y();
            p.drawLine(p0, QPointF(x,y));
            p.drawEllipse(p0,3,3);

            drawSelWpt(p, wptInfo, eFeatWheel|eFeatName);

            deg += 45;
        }

        // if more than 7 waypoints display "more" message
        if(selWpts.size() > 8)
        {
            int x = cos(deg * DEG_TO_RAD) * RADIUS_CIRCLES;
            int y = sin(deg * DEG_TO_RAD) * RADIUS_CIRCLES;

            p.save();
            p.translate(x, y);

            QString str = tr("too many...");
            CCanvas::drawText(str,p,QPoint(0,0));

            p.restore();
        }

        // display help box
        {
            QString str = tr("Left click to lock circles.\nThen select function from circle.\nLeft click on canvas to un-lock circles.");
            QFont           f       = CResources::self().getMapFont();
            QFontMetrics    fm(f);
            QRect           rText   = fm.boundingRect(QRect(0,0,300,0), Qt::AlignCenter|Qt::TextWordWrap, str);
            QRect           rFrame(0, 0, 1,1);

            rFrame.setWidth(rText.width() + 20);
            rFrame.setHeight(rText.height() + 20);
            rText.moveTopLeft(QPoint(10, 10));

            p.save();
            p.translate(-rFrame.width()/2, RADIUS_CIRCLES + RADIUS_CIRCLE + 15);

            p.setPen(CCanvas::penBorderBlue);
            p.setBrush(CCanvas::brushBackWhite);
            PAINT_ROUNDED_RECT(p,rFrame);

            p.setFont(CResources::self().getMapFont());
            p.setPen(Qt::darkBlue);
            p.drawText(rText, Qt::AlignJustify|Qt::AlignCenter|Qt::TextWordWrap,str);
            p.restore();
        }

        // display infobox for waypoint under mouse cursor, if any
        if(idxInfoBox >= 0)
        {
            drawSelWpt(p, selWpts[idxInfoBox], eFeatInfoBox|eFeatArrow);
        }

        p.restore();             // mousePos
    }
}


void IMouse::drawSelWpt(QPainter& p, wpt_t& wptInfo, quint32 features)
{
    CWpt * selWpt  = wptInfo.wpt;

    double u = wptInfo.x;
    double v = wptInfo.y;

    if(features & eFeatWheel)
    {
        QPixmap icon = selWpt->getIcon().scaled(16,16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        p.setPen(CCanvas::penBorderBlue);
        p.setBrush(CCanvas::brushBackWhite);
        p.drawEllipse(u - RADIUS_CIRCLE, v - RADIUS_CIRCLE, RADIUS_CIRCLE*2, RADIUS_CIRCLE*2);
        p.drawPixmap(u - 8 , v - 8, icon);

        p.save();
        p.translate(u - 24, v - 24);

        if(selWpt->isDeletable())
        {
            p.drawPixmap(rectDelWpt, QPixmap(":/icons/iconClear16x16.png"));
        }

        if(selWpt->isMovable() && (selWpts.size() == 1))
        {
            p.drawPixmap(rectMoveWpt, QPixmap(":/icons/iconMove16x16.png"));
        }
        else if(selWpt->isGeoCache() && selWpt->hasBuddies())
        {
            p.drawPixmap(rectMoveWpt, QPixmap(":/icons/iconWaypoint16x16.png"));
        }

        p.drawPixmap(rectEditWpt, QPixmap(":/icons/iconEdit16x16.png"));
        p.drawPixmap(rectCopyWpt, QPixmap(":/icons/iconClipboard16x16.png"));
        if(!selWpt->images.isEmpty() /*&& !selWpt->images[0].filePath.isEmpty()*/)
        {
            p.drawPixmap(rectViewWpt, QPixmap(":/icons/iconImage16x16.png"));
        }

        if(selWpt->selected)
        {
            p.drawPixmap(rectMarkWpt, QPixmap(":/icons/iconCheckbox16x16.png"));
        }

        p.restore();
    }

    if(doShowWptBuddies)
    {
        return;
    }

    if(features & eFeatInfoBox)
    {
        QPixmap pic;
        int wPic = 0;
        int hPic = 0;
        if(!selWpt->images.isEmpty())
        {
            pic = selWpt->images[0].pixmap.scaledToWidth(80,Qt::SmoothTransformation);
            wPic = pic.width();
            hPic = pic.height();
        }

        QString         str     = selWpt->getInfo();
        QFont           f       = CResources::self().getMapFont();
        QFontMetrics    fm(f);
        QRect           rText   = fm.boundingRect(QRect(0,0,300,0), Qt::AlignLeft|Qt::AlignTop|Qt::TextWordWrap, str);

        QRect           rFrame(0, 0, 1,1);

        if(!pic.isNull())
        {
            rFrame.setWidth(rText.width() + 30 + wPic);
            rFrame.setHeight((hPic > rText.height() ? hPic : rText.height()) + 20);
            rText.moveTopLeft(QPoint(20 + wPic, 10));
        }
        else
        {
            rFrame.setWidth(rText.width() + 20);
            rFrame.setHeight(rText.height() + 20);
            rText.moveTopLeft(QPoint(10, 10));
        }

        p.save();
        p.translate(u + RADIUS_CIRCLE + 15, v);

        p.setPen(CCanvas::penBorderBlue);
        p.setBrush(CCanvas::brushBackWhite);
        PAINT_ROUNDED_RECT(p,rFrame);

        p.setFont(CResources::self().getMapFont());
        p.setPen(Qt::darkBlue);
        p.drawText(rText, Qt::AlignJustify|Qt::AlignTop|Qt::TextWordWrap,str);

        if(!pic.isNull())
        {
            p.drawPixmap(10,10, pic);
            if(selWpt->images.size() > 1)
            {
                p.drawPixmap(pic.width() + 10 - 16, pic.height() + 10 - 16, QPixmap("://icons/iconMultipleImages16x16.png"));
            }
        }

        p.restore();
    }

    if((selWpt->dir != WPT_NOFLOAT) && (features & eFeatArrow))
    {
#define ARROWDIR_H 15
#define ARROWDIR_W 20

        p.save();
        p.setPen(CCanvas::penBorderBlue);
        p.setBrush(CCanvas::brushBackWhite);

        p.translate(u, v);
        p.rotate(selWpt->dir);
        p.translate(0,  - RADIUS_CIRCLE - 2 - ARROWDIR_H/2);

        QPolygon arrow;
        arrow << QPoint(0, -ARROWDIR_H/2) << QPoint(-ARROWDIR_W/2, ARROWDIR_H/2) << QPoint(0, ARROWDIR_H/3) << QPoint(ARROWDIR_W/2, ARROWDIR_H/2);

        p.drawPolygon(arrow);

        p.restore();
    }

    if(features & eFeatName)
    {
        int off;
        p.save();
        p.translate(u,v);

        QString name = selWpt->getName();
        QFontMetricsF fm(p.font());

        off = RADIUS_CIRCLE + 4;
        QPainterPath path(QPoint(0,-off));
        path.arcTo(QRectF(-off,-off,off*2,off*2),90,-360);

        off = RADIUS_CIRCLE + 3 + floor(fm.height()/2);
        p.setPen(QPen(CCanvas::brushBackWhite,fm.height()));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(-off,-off,2*off,2*off);

        int l = 1;
        for ( int i = 0; i < name.size(); i++ )
        {
            QString str = name.mid(i,1);

            qreal percent = path.percentAtLength(l);
            QPointF point = path.pointAtPercent(percent);
            qreal angle   = path.angleAtPercent(percent);

            p.save();
            p.translate(point);
            p.rotate(-angle);

            p.setPen(CResources::self().wptTextColor());
            p.drawText( 0, 0,str);

            p.restore();

            l += fm.width(str) + 2;
        }

        p.restore();
    }
}


void IMouse::drawSelSearch(QPainter& p)
{
    IMap& map = CMapDB::self().getMap();
    if(!selSearch.isNull())
    {
        double u = selSearch->lon * DEG_TO_RAD;
        double v = selSearch->lat * DEG_TO_RAD;
        map.convertRad2Pt(u,v);

        p.setPen(CCanvas::penBorderBlue);
        p.setBrush(CCanvas::brushBackWhite);
        p.drawEllipse(u - RADIUS_CIRCLE, v - RADIUS_CIRCLE, RADIUS_CIRCLE*2, RADIUS_CIRCLE*2);
        p.drawPixmap(u-8 , v-8, QPixmap(":/icons/iconBullseye16x16"));

        p.save();
        p.translate(u - 24, v - 24);
        p.drawPixmap(rectDelSearch, QPixmap(":/icons/iconClear16x16.png"));
        p.drawPixmap(rectConvertSearch, QPixmap(":/icons/iconAdd16x16.png"));
        p.drawPixmap(rectCopySearch, QPixmap(":/icons/iconClipboard16x16.png"));
        p.restore();

    }
}


void IMouse::drawSelTrkPt(QPainter& p)
{
    IMap& map       = CMapDB::self().getMap();

    if(selTrkPt && selWpts.isEmpty())
    {
        CTrack * track = CTrackDB::self().highlightedTrack();
        if(track == 0)
        {
            return;
        }

        QString str;
        double u = selTrkPt->lon * DEG_TO_RAD;
        double v = selTrkPt->lat * DEG_TO_RAD;
        map.convertRad2Pt(u,v);

        p.setPen(CCanvas::penBorderBlue);
        p.setBrush(CCanvas::brushBackWhite);
        p.drawEllipse(QRect(u - 5,  v - 5, 11, 11));

        str = track->getTrkPtInfo1(*selTrkPt);
        //-----------------------------------------------------------------------------------------------------------
        if (str != "")
        {
            QFont           f = CResources::self().getMapFont();
            QFontMetrics    fm(f);
            QRect           r1 = fm.boundingRect(QRect(0,0,300,0), Qt::AlignLeft|Qt::AlignTop|Qt::TextWordWrap, str);
            r1.moveTopLeft(QPoint(u + 45, v));

            QRect           r2 = r1;
            r2.setWidth(r1.width() + 20);
            r2.moveLeft(r1.left() - 10);
            r2.setHeight(r1.height() + 20);
            r2.moveTop(r1.top() - 10);

            p.setPen(QPen(CCanvas::penBorderBlue));
            p.setBrush(CCanvas::brushBackWhite);
            PAINT_ROUNDED_RECT(p,r2);

            p.setFont(CResources::self().getMapFont());
            p.setPen(Qt::darkBlue);
            p.drawText(r1, Qt::AlignLeft|Qt::AlignTop|Qt::TextWordWrap,str);
        }

        str = track->getTrkPtInfo2(*selTrkPt);
        //-----------------------------------------------------------------------------------------------------------
        if (str != "")
        {
            QFont           f = CResources::self().getMapFont();
            QFontMetrics    fm(f);
            QRect           r1 = fm.boundingRect(QRect(0,0,300,0), Qt::AlignLeft|Qt::AlignTop, str);

            QRect r = theMainWindow->getCanvas()->rect();
            r1.moveTopLeft(QPoint((r.width() - r1.width())/2, 20));

            QRect           r2 = r1;
            r2.setWidth(r1.width() + 20);
            r2.moveLeft(r1.left() - 10);
            r2.setHeight(r1.height() + 10);
            r2.moveTop(r1.top() - 5);

            p.setPen(QPen(CCanvas::penBorderBlue));
            p.setBrush(CCanvas::brushBackWhite);
            PAINT_ROUNDED_RECT(p,r2);

            p.setFont(CResources::self().getMapFont());
            p.setPen(Qt::darkBlue);
            p.drawText(r1, Qt::AlignLeft|Qt::AlignTop,str);
        }
    }
}


void IMouse::drawSelRtePt(QPainter& p)
{
    IMap& map = CMapDB::self().getMap();
    if(selRtePt && selWpts.isEmpty())
    {
        double u = selRtePt->lon * DEG_TO_RAD;
        double v = selRtePt->lat * DEG_TO_RAD;
        map.convertRad2Pt(u,v);

        p.setPen(CCanvas::penBorderBlue);
        p.setBrush(CCanvas::brushBackWhite);
        p.drawEllipse(QRect(u - 5,  v - 5, 11, 11));

        QString         str = selRtePt->action;
        QFont           f = CResources::self().getMapFont();
        QFontMetrics    fm(f);
        QRect           r1 = fm.boundingRect(QRect(0,0,300,0), Qt::AlignLeft|Qt::AlignTop|Qt::TextWordWrap, str);
        r1.moveTopLeft(QPoint(u + 45, v));

        QRect           r2 = r1;
        r2.setWidth(r1.width() + 20);
        r2.moveLeft(r1.left() - 10);
        r2.setHeight(r1.height() + 20);
        r2.moveTop(r1.top() - 10);

        p.setPen(QPen(CCanvas::penBorderBlue));
        p.setBrush(CCanvas::brushBackWhite);
        PAINT_ROUNDED_RECT(p,r2);

        p.setFont(CResources::self().getMapFont());
        p.setPen(Qt::darkBlue);
        p.drawText(r1, Qt::AlignLeft|Qt::AlignTop|Qt::TextWordWrap,str);
    }
}


void IMouse::mouseMoveEventWpt(QMouseEvent * e)
{

    QPoint pos      = e->pos();
    IMap& map       = CMapDB::self().getMap();
    int oldSize     = selWpts.size();

    if(!lockWptCircles)
    {
        clearSelWpts();

        // find the waypoint close to the cursor
        QMap<QString,CWpt*>::const_iterator wpt = CWptDB::self().begin();
        while(wpt != CWptDB::self().end())
        {
            if((*wpt) == 0)
            {
                wpt++;
                continue;
            }

            double u = (*wpt)->lon * DEG_TO_RAD;
            double v = (*wpt)->lat * DEG_TO_RAD;
            map.convertRad2Pt(u,v);

            if(((pos.x() - u) * (pos.x() - u) + (pos.y() - v) * (pos.y() - v)) < 1225)
            {
                selWpts << wpt_t();

                wpt_t& wptInfo = selWpts.last();
                wptInfo.wpt = *wpt;

                double u = wptInfo.wpt->lon * DEG_TO_RAD;
                double v = wptInfo.wpt->lat * DEG_TO_RAD;
                map.convertRad2Pt(u,v);

                wptInfo.x = wptInfo.xReal = u;
                wptInfo.y = wptInfo.yReal = v;

            }

            ++wpt;
        }

        if(selWpts.size() > 1)
        {
            sortSelWpts(selWpts);
        }

    }

    // check for cursor-over-function
    for(int i = 0; i < selWpts.size(); i++)
    {
        QPoint pt;
        wpt_t& wptInfo  = selWpts[i];
        CWpt * selWpt   = wptInfo.wpt;

        if(selWpt == 0)
        {
            continue;
        }

        // mind the different offset values in wptInfo for single or multiple waypoints
        // @todo there must be a better way
        if(lockWptCircles)
        {
            pt = pos - QPoint(wptInfo.x - 24, wptInfo.y - 24) - mousePosWptCircle;
        }
        else
        {
            pt = pos - QPoint(wptInfo.x - 24, wptInfo.y - 24);
        }

        if(selWpt->isGeoCache() && ((selWpts.size() == 1) || lockWptCircles))
        {
            if(!doShowWptBuddies && rectMoveWpt.contains((pt)))
            {
                selWpt->showBuddies(true);
                doShowWptBuddies = true;
                canvas->update();
            }
            else if(!rectMoveWpt.contains(pt))
            {
                selWpt->showBuddies(false);
                doShowWptBuddies = false;
                canvas->update();
            }
        }
        else
        {
            selWpt->showBuddies(false);
            doShowWptBuddies = false;
        }

        // this has to be the last thing in the loop to do, as we break if a waypoint matches
        if(rectDelWpt.contains(pt) || rectCopyWpt.contains(pt) || rectMoveWpt.contains(pt) || rectEditWpt.contains(pt) || rectViewWpt.contains(pt))
        {
            if(!doSpecialCursorWpt)
            {
                QApplication::setOverrideCursor(Qt::PointingHandCursor);
                doSpecialCursorWpt = true;
            }
            break;
        }
        else
        {
            if(doSpecialCursorWpt)
            {
                QApplication::restoreOverrideCursor();
                doSpecialCursorWpt = false;
            }
        }

    }

    if(selWpts.isEmpty())
    {
        if(doSpecialCursorWpt)
        {
            QApplication::restoreOverrideCursor();
            doSpecialCursorWpt = false;
        }
    }

    if(oldSize != selWpts.size() || selWpts.size() > 1)
    {
        canvas->update();
    }

}


void IMouse::mouseMoveEventSearch(QMouseEvent * e)
{
    QPoint pos          = e->pos();
    IMap& map           = CMapDB::self().getMap();
    CSearch * oldSearch = selSearch; selSearch = 0;

    // find the search close to the cursor
    QMap<QString,CSearch*>::const_iterator search = CSearchDB::self().begin();
    while(search != CSearchDB::self().end())
    {
        double u = (*search)->lon * DEG_TO_RAD;
        double v = (*search)->lat * DEG_TO_RAD;
        map.convertRad2Pt(u,v);

        if(((pos.x() - u) * (pos.x() - u) + (pos.y() - v) * (pos.y() - v)) < 1225)
        {
            selSearch = *search;
            break;
        }

        ++search;
    }

    // check for cursor-over-function
    if(selSearch)
    {
        double u = selSearch->lon * DEG_TO_RAD;
        double v = selSearch->lat * DEG_TO_RAD;
        map.convertRad2Pt(u,v);

        QPoint pt = pos - QPoint(u - 24, v - 24);

        if(rectDelSearch.contains(pt) || rectCopySearch.contains(pt) ||  rectConvertSearch.contains(pt))
        {
            if(!doSpecialCursorSearch)
            {
                QApplication::setOverrideCursor(Qt::PointingHandCursor);
                doSpecialCursorSearch = true;
            }
        }
        else
        {
            if(doSpecialCursorSearch)
            {
                QApplication::restoreOverrideCursor();
                doSpecialCursorSearch = false;
            }
        }
    }
    else
    {
        if(doSpecialCursorSearch)
        {
            QApplication::restoreOverrideCursor();
            doSpecialCursorSearch = false;
        }
    }

    // do a canvas update on a change only
    if(oldSearch != selSearch)
    {
        canvas->update();
    }
}


void IMouse::mousePressEventWpt(QMouseEvent * e)
{
    if(selWpts.isEmpty()) return;

    if(selWpts.size() > 1 && !lockWptCircles)
    {
        lockWptCircles = true;
        mousePosWptCircle = e->pos();
        return;
    }

    bool doBreak = false;
    for(int i = 0; i < selWpts.size(); i++)
    {
        QPoint pt;
        wpt_t& wptInfo  = selWpts[i];
        CWpt * selWpt   = wptInfo.wpt;
        QPoint pos      = e->pos();

        if(selWpt == 0)
        {
            continue;
        }

        // mind the different offset values in wptInfo for single or multiple waypoints
        // @todo there must be a better way
        if(lockWptCircles)
        {
            pt = pos - QPoint(wptInfo.x - 24, wptInfo.y - 24) - mousePosWptCircle;
        }
        else
        {
            pt = pos - QPoint(wptInfo.x - 24, wptInfo.y - 24);
        }

        if(rectDelWpt.contains(pt) && !selWpt->sticky)
        {
            selWpts.removeAt(i);
            if(selWpts.size() < 2)
            {
                lockWptCircles = false;
            }
            sortSelWpts(selWpts);

            CWptDB::self().delWpt(selWpt->getKey(), false, true);

            canvas->update();
            doBreak = true;
        }
        else if(rectMoveWpt.contains(pt) && selWpt->isMovable() && (selWpts.size() == 1))
        {
            canvas->setMouseMode(CCanvas::eMouseMoveWpt);

            QMouseEvent event1(QEvent::MouseMove, QPoint(wptInfo.x,wptInfo.y), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
            QCoreApplication::sendEvent(canvas,&event1);

            QMouseEvent event2(QEvent::MouseButtonPress, QPoint(wptInfo.x,wptInfo.y), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            QCoreApplication::sendEvent(canvas,&event2);
            doBreak = true;
        }
        else if(rectEditWpt.contains(pt))
        {
            CDlgEditWpt dlg(*selWpt,canvas);
            dlg.exec();
            doBreak = true;
        }
        else if(rectCopyWpt.contains(pt))
        {
            QString position;
            GPS_Math_Deg_To_Str(selWpt->lon, selWpt->lat, position);

            QClipboard *clipboard = QApplication::clipboard();
            clipboard->setText(position);

            selWpt = 0;
            canvas->update();
            doBreak = true;
        }
        else if(rectViewWpt.contains(pt) && !selWpt->images.isEmpty())
        {
            CImageViewer view(selWpt->images, 0, theMainWindow);
            view.exec();
            doBreak = true;
        }
        else if(rectMarkWpt.contains(pt))
        {
            CWptDB::self().selWptByKey(selWpt->getKey(), true);
            canvas->update();
            doBreak = true;
        }

        // has a function been triggered? if yes we can stop polling waypoints
        if(doBreak)
        {
            break;
        }
    }

    // if the click has been without triggering a function
    // the circle for multiple waypoints is reset.
    if(lockWptCircles && !doBreak)
    {
        clearSelWpts();
        canvas->update();
    }

}


void IMouse::mousePressEventSearch(QMouseEvent * e)
{
    if(selSearch.isNull()) return;

    IMap& map   = CMapDB::self().getMap();
    QPoint pos  = e->pos();
    double u    = selSearch->lon * DEG_TO_RAD;
    double v    = selSearch->lat * DEG_TO_RAD;
    map.convertRad2Pt(u,v);

    QPoint pt = pos - QPoint(u - 24, v - 24);
    if(rectDelSearch.contains(pt))
    {
        QStringList keys;
        keys << selSearch->getKey();
        CSearchDB::self().delResults(keys);
        selSearch = 0;
        canvas->update();
    }
    else if(rectConvertSearch.contains(pt))
    {
        QString key = selSearch->getKey();
        float ele = CMapDB::self().getDEM().getElevation(selSearch->lon * DEG_TO_RAD, selSearch->lat * DEG_TO_RAD);
        CWpt * wpt = CWptDB::self().newWpt(selSearch->lon * DEG_TO_RAD, selSearch->lat * DEG_TO_RAD, ele, selSearch->getName());
        if(wpt)
        {
            selWpts << wpt_t();
            selWpts.last().wpt = wpt;

            QStringList keys;

            keys << key;
            CSearchDB::self().delResults(keys);
            canvas->update();
        }

    }
    else if(rectCopyWpt.contains(pt))
    {
        QString position;
        GPS_Math_Deg_To_Str(selSearch->lon, selSearch->lat, position);

        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(position);

        selSearch = 0;
        canvas->update();
    }
}


void IMouse::mouseMoveEventTrack(QMouseEvent * e)
{
    CTrack * track = CTrackDB::self().highlightedTrack();
    if(track == 0) return;

    CTrack::pt_t * oldTrackPt = selTrkPt;
    int d1      = 20;
    selTrkPt    = 0;

    QList<CTrack::pt_t>& pts          = track->getTrackPoints();
    QList<CTrack::pt_t>::iterator pt  = pts.begin();
    while(pt != pts.end())
    {
        if(pt->flags & CTrack::pt_t::eDeleted)
        {
            ++pt; continue;
        }

        int d2 = abs(e->pos().x() - pt->px.x()) + abs(e->pos().y() - pt->px.y());

        if(d2 < d1)
        {
            selTrkPt = &(*pt);
            d1 = d2;
        }

        ++pt;
    }

    if(oldTrackPt != selTrkPt)
    {
        if(selTrkPt != 0)
        {
            CTrackDB::self().setPointOfFocusByIdx(selTrkPt->idx);
        }
        else
        {
            CTrackDB::self().setPointOfFocusByIdx(-1);
        }
        canvas->update();
    }
}


void IMouse::mouseMoveEventRoute(QMouseEvent * e)
{
    CRoute * route = CRouteDB::self().highlightedRoute();
    if(route == 0) return;

    IMap& map = CMapDB::self().getMap();
    double u,v;

    CRoute::pt_t * oldRoutePt = selRtePt;
    int d1      = 20;
    selRtePt    = 0;

    QVector<CRoute::pt_t>& pts          = route->getSecRtePoints().isEmpty() ? route->getPriRtePoints() : route->getSecRtePoints();
    QVector<CRoute::pt_t>::iterator pt  = pts.begin();
    while(pt != pts.end())
    {
        if(pt->action.isEmpty())
        {
            ++pt; continue;
        }

        u = pt->lon * DEG_TO_RAD;
        v = pt->lat * DEG_TO_RAD;
        map.convertRad2Pt(u,v);

        int d2 = abs(e->pos().x() - u) + abs(e->pos().y() - v);

        if(d2 < d1)
        {
            selRtePt = &(*pt);
            d1 = d2;
        }

        ++pt;
    }

    if(oldRoutePt != selRtePt)
    {
        canvas->update();
    }

}


void IMouse::mouseMoveEventOverlay(QMouseEvent * e)
{
    IOverlay * oldOverlay = selOverlay;

    if(!selOverlay || !selOverlay->mouseActionInProgress())
    {

        QMap<QString, IOverlay*>::const_iterator overlay = COverlayDB::self().begin();
        while(overlay != COverlayDB::self().end())
        {
            if((*overlay)->isCloseEnough(e->pos())) break;
            ++overlay;
        }

        if(overlay != COverlayDB::self().end())
        {
            (*overlay)->select(*overlay);
            selOverlay = *overlay;
        }
        else
        {
            IOverlay::select(0);
            selOverlay = 0;
        }

    }

    if(oldOverlay != selOverlay)
    {
        if(selOverlay && selOverlay->visible())
        {
            canvas->setMouseMode(CCanvas::eMouseOverlay);
        }
        else
        {
            canvas->setMouseMode(CCanvas::eMouseMoveArea);
        }
        canvas->update();
    }
}


void IMouse::mouseMoveEventMapSel(QMouseEvent * e)
{

    IMapSelection * oldSel = selMap;

    IMap& map = CMapDB::self().getMap();
    double u = e->pos().x();
    double v = e->pos().y();

    map.convertPt2Rad(u,v);

    selMap = CMapDB::self().getSelectedMap(u,v);

    if(selMap && !oldSel)
    {
        theMainWindow->getCanvas()->setMouseMode(CCanvas::eMouseSelectArea);
    }
    else if(!selMap && oldSel)
    {
        theMainWindow->getCanvas()->setMouseMode(CCanvas::eMouseMoveArea);
    }

    if(selMap != oldSel)
    {
        canvas->update();
    }

}


void IMouse::slotMapChanged()
{
    clearSelWpts();
}


void IMouse::slotSetPos1()
{
    IMap& map = CMapDB::self().getMap();

    double u,v;
    u = mousePos.x();
    v = mousePos.y();
    map.convertPt2Pixel(u,v);
    pos1Pixel = QPointF(u,v);

    u = mousePos.x();
    v = mousePos.y();
    map.convertPt2Rad(u,v);
    pos1LonLat = QPointF(u,v);
}


void IMouse::setSelTrackPt(CTrack::pt_t * pt)
{
    selTrkPt = pt;
    canvas->update();
}
