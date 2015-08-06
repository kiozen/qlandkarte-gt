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

#include "COverlayArea.h"
#include "CMapDB.h"
#include "CMainWindow.h"
#include "GeoMath.h"
#include "COverlayDB.h"
#include "IUnit.h"
#include "CResources.h"
#include "COverlayAreaEditWidget.h"

#include <QtGui>
#include <QMenu>

static bool operator==(const projXY& p1, const projXY& p2)
{
    return (p1.u == p2.u) && (p1.v == p2.v);
}


QPointer<COverlayAreaEditWidget> overlayAreaEditWidget;

COverlayArea::COverlayArea(const QString &name, const QString &comment, const QColor& color, const Qt::BrushStyle style, const QList<pt_t> &pts, QObject *parent)
: IOverlay(parent, "Area", ":/icons/iconArea16x16.png")
, points(pts)
, color(color)
, style(style)
, width(5)
, opacity(255)
, thePoint(0)
, thePointBefor(0)
, thePointAfter(0)
, doSpecialCursor(false)
, doMove(false)
, doFuncWheel(false)
, addType(eNone)
, isEdit(false)
{
    if(name.isEmpty())
    {
        setName(tr("Area %1").arg(keycnt));
    }
    else
    {
        setName(name);
    }

    setComment(comment);

    rectDel  = QRect(0,0,16,16);
    rectMove = QRect(32,0,16,16);
    rectAdd1 = QRect(0,32,16,16);
    rectAdd2 = QRect(32,32,16,16);

    calc();

    if(pts.size() == 1)
    {
        points.append(points[0]);
        thePointBefor   = &points[0];
        thePoint        = &points[1];
        doMove          = true;
        addType         = eAtEnd;
        doFuncWheel     = false;
    }
}


void COverlayArea::save(QDataStream& s)
{
    s << name << comment << points.size();
    projXY pt;
    foreach(pt, points)
    {
        s << pt.u << pt.v;
    }
    s << color;
    s << getKey();
    s << getParentWpt();
    s << qint32(style);
    s << width;
    s << opacity;
}


void COverlayArea::load(QDataStream& s)
{
    pt_t pt;
    int size;
    QString key;
    QString parentWpt;
    qint32 tmp32;

    points.clear();

    s >> name >> comment >> size;
    for(int i = 0; i < size; ++i)
    {
        s >> pt.u >> pt.v;
        pt.idx = i;
        points << pt;
    }
    s >> color;
    s >> key;
    s >> parentWpt;
    s >> tmp32;
    s >> width;
    s >> opacity;

    style = (Qt::BrushStyle)tmp32;

    setKey(key);

}


QString COverlayArea::getInfo()
{
    QString info;
    QString val, unit;

    if(!name.isEmpty())
    {
        info += name + "\n";
    }
    if(!comment.isEmpty())
    {
        if(comment.length() < 60)
        {
            info += comment + "";
        }
        else
        {
            info += comment.left(57) + "...";
        }
    }

    return info;
}


void COverlayArea::draw(QPainter& p, const QRect& viewport)
{
    if(points.isEmpty() || !isVisible) return;

    IMap& map = CMapDB::self().getMap();

    QPen pen1, pen2;
    QPixmap icon(":/icons/small_bullet_darkgray.png");
    QPixmap icon_red(":/icons/small_bullet_red.png");
    QPixmap icon_BigRed(":/icons/bullet_red.png");
    projXY pt1, pt2;
    QPoint pt;
    QPoint pt3;

    int i;
    int start   = 0;
    int stop    = points.count();
    int skip    = -1;

    // if there is an active subline fine tune start and stop index
    // to make the subline replace the first of last line segment
    if(thePoint && !subline.isEmpty())
    {
        if(*thePoint == points.last() )
        {
            stop -= 1;
        }
        else if(*thePoint == points.first())
        {
            start += 1;
        }

        int idx = points.indexOf(*thePoint);

        if(addType == eAfter)
        {
            skip = idx;
        }
        else
        {
            skip = idx + 1;
        }
    }

    pt1 = points[start];
    map.convertRad2Pt(pt1.u, pt1.v);

    QPolygon polyline;

    polyline << QPoint(pt1.u, pt1.v);

    // draw the lines
    for(i = start + 1; i < stop; i++)
    {

        pt2 = points[i];
        map.convertRad2Pt(pt2.u, pt2.v);

        int d = abs(pt1.u - pt2.u) + abs(pt1.v - pt2.v);
        if(d < 10)
        {
            continue;
        }

        if(i != skip)
        {
            polyline << QPoint(pt2.u, pt2.v);
        }
        pt1 = pt2;
    }

    p.save();
    if(highlight)
    {
        color.setAlpha(opacity);

        pen1 = QPen(QColor(255,255,255,opacity),width + 2);
        pen1.setCapStyle(Qt::RoundCap);
        pen1.setJoinStyle(Qt::RoundJoin);

        pen2 = QPen(color,width);
        pen2.setCapStyle(Qt::RoundCap);
        pen2.setJoinStyle(Qt::RoundJoin);
    }
    else
    {
        color.setAlpha(opacity);

        pen1 = QPen(QColor(255,255,255,opacity),width);
        pen1.setCapStyle(Qt::RoundCap);
        pen1.setJoinStyle(Qt::RoundJoin);

        pen2 = QPen(color,width - 2);
        pen2.setCapStyle(Qt::RoundCap);
        pen2.setJoinStyle(Qt::RoundJoin);
    }

    p.setBrush(Qt::NoBrush);
    p.setPen(pen1);
    p.drawPolygon(polyline);

    p.setPen(pen2);
    p.setBrush(QBrush(color, style));
    p.drawPolygon(polyline);

    p.restore();

    pt3 = getPolygonCentroid(polyline);
    CCanvas::drawText(getName(), p, pt3);

    // overlay _the_ point with a red bullet
    if(thePoint)
    {
        pt2 = *thePoint;
        map.convertRad2Pt(pt2.u, pt2.v);
        p.drawPixmap(pt2.u - 4, pt2.v - 4, icon_red);
    }

    // if there is a subline draw it
    if(!subline.isEmpty())
    {

        QPen pen;
        pen.setBrush(QBrush(QColor(255,0,255,150)));
        pen.setWidth(20);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);

        p.setPen(pen);
        p.drawPolyline(leadline);

        p.setPen(QPen(Qt::white, 7));
        p.drawPolyline(subline);
        p.setPen(QPen(Qt::red, 5));
        p.drawPolyline(subline);
        p.setPen(QPen(Qt::white, 1));
        p.drawPolyline(subline);

        p.setPen(Qt::black);
        for(i = 1; i < (subline.size() - 1); i++)
        {
            p.drawPixmap(subline[i] - QPoint(4,4), icon_red);
        }

    }

    if(thePoint && !doMove)
    {
        pt2 = *thePoint;
        map.convertRad2Pt(pt2.u, pt2.v);

        if(doFuncWheel)
        {

            p.setPen(CCanvas::penBorderBlue);
            p.setBrush(CCanvas::brushBackWhite);
            p.drawEllipse(pt2.u - 35, pt2.v - 35, 70, 70);

            p.save();
            p.translate(pt2.u - 24, pt2.v - 24);
            p.drawPixmap(rectDel, QPixmap(":/icons/iconClear16x16.png"));
            p.drawPixmap(rectMove, QPixmap(":/icons/iconMove16x16.png"));
            if(anglePrev < 360)
            {
                p.save();
                p.setRenderHint(QPainter::SmoothPixmapTransform, true);
                p.translate(rectAdd1.center());
                p.rotate(-anglePrev);
                p.drawPixmap(QPoint(-8, -8), QPixmap(":/icons/iconAddPoint16x16.png"));
                p.restore();
            }
            else
            {
                p.save();
                p.setRenderHint(QPainter::SmoothPixmapTransform, true);
                p.translate(rectAdd1.center());
                p.rotate(-angleNext + 180);
                p.drawPixmap(QPoint(-8, -8), QPixmap(":/icons/iconAddPointEnd16x16.png"));
                p.restore();
            }

            if(angleNext < 360)
            {
                p.save();
                p.setRenderHint(QPainter::SmoothPixmapTransform, true);
                p.translate(rectAdd2.center());
                p.rotate(-angleNext);
                p.drawPixmap(QPoint(-8, -8), QPixmap(":/icons/iconAddPoint16x16.png"));
                p.restore();
            }
            else
            {
                p.save();
                p.setRenderHint(QPainter::SmoothPixmapTransform, true);
                p.translate(rectAdd2.center());
                p.rotate(-anglePrev + 180);
                p.drawPixmap(QPoint(-8, -8), QPixmap(":/icons/iconAddPointEnd16x16.png"));
                p.restore();
            }

            p.restore();
        }
        else
        {
            p.setPen(CCanvas::penBorderBlue);
            p.setBrush(CCanvas::brushBackWhite);
            p.drawEllipse(pt2.u - 8, pt2.v - 8, 16, 16);
        }
    }

    // overlay points with the selected point icon
    foreach(i, selectedPoints)
    {
        if(i < points.size())
        {
            projXY pt = points[i];
            map.convertRad2Pt(pt.u, pt.v);
            p.drawPixmap(pt.u - 6, pt.v - 6, icon_BigRed);
        }
    }

    // draw distance information to neighbour points
    if(thePointBefor && subline.isEmpty())
    {
        drawDistanceInfo(*thePointBefor, *thePoint, p, map);
    }

    if(thePointAfter && subline.isEmpty())
    {
        drawDistanceInfo(*thePoint, *thePointAfter, p, map);
    }

}


bool COverlayArea::isCloseEnough(const QPoint& pt)
{
    IMap& map = CMapDB::self().getMap();
    QList<pt_t>::iterator p = points.begin();

    if(doFuncWheel)
    {
        return true;
    }

    thePoint        = 0;
    thePointBefor   = 0;
    thePointAfter   = 0;

    double ref  = doFuncWheel ? (35.0 * 35.0) : (8.0 * 8.0);
    double dist = ref;
    while(p != points.end())
    {
        projXY pt1 = *p;
        map.convertRad2Pt(pt1.u, pt1.v);

        double d = (pt.x() - pt1.u) * (pt.x() - pt1.u) + (pt.y() - pt1.v) * (pt.y() - pt1.v);
        if(d < dist)
        {
            thePoint = &(*p);

            if(p != points.begin())
            {
                projXY p1 = *p;
                projXY p2 = *(p - 1);

                map.convertRad2M(p1.u, p1.v);
                map.convertRad2M(p2.u, p2.v);

                anglePrev = atan2((p2.v - p1.v) , (p2.u - p1.u)) * 180/M_PI;

                thePointBefor = &(*(p-1));
            }
            else
            {
                anglePrev = 1000;
            }

            if((p + 1) != points.end())
            {
                projXY p1 = *p;
                projXY p2 = *(p + 1);

                map.convertRad2M(p1.u, p1.v);
                map.convertRad2M(p2.u, p2.v);

                angleNext = atan2((p2.v - p1.v) , (p2.u - p1.u)) * 180/M_PI;

                thePointAfter = &(*(p+1));

            }
            else
            {
                angleNext = 1000;
            }

            dist = d;
        }
        ++p;
    }

    return (dist != ref);
}


void COverlayArea::keyPressEvent(QKeyEvent * e)
{
    if (e->key() == Qt::Key_Backspace)
    {
        if (!doFuncWheel && addType != eNone && thePoint && points.size() > 1)
        {
            int idx;
            switch (addType)
            {
                case eBefore:
                    idx = points.indexOf(*thePoint);
                    if (idx < points.size())
                    {
                        points.removeAt(idx+1);
                        thePointAfter = (idx+1 < points.size()) ? &(points[idx+1]) : 0;
                    }
                    break;
                case eAfter:
                    idx = points.indexOf(*thePoint);
                    if (idx > 0)
                    {
                        points.removeAt(idx-1);
                        thePointBefor = (idx > 1) ? &(points[idx-2]) : 0;
                    }
                    break;
                case eAtEnd:
                    points.removeLast();
                    thePointBefor = (points.size() > 1) ? &(*(points.end() - 2)) : 0;
                    thePoint      = &points.last();
                    break;
                default:
                    break;
            }

            calc();
            emit sigChanged();
            QPoint pos = theMainWindow->getCanvas()->mapFromGlobal(QCursor::pos());
            QMouseEvent * ev = new QMouseEvent(QEvent::MouseMove, pos, Qt::NoButton, QApplication::mouseButtons(), QApplication::keyboardModifiers());
            QCoreApplication::postEvent(theMainWindow->getCanvas(), ev);
        }
    }
}


void COverlayArea::mouseMoveEvent(QMouseEvent * e)
{

    IMap& map   = CMapDB::self().getMap();
    QPoint pos  = e->pos();
    QPoint pos1 = e->pos();

    subline.clear();

    if(thePoint)
    {
        projXY pt = *thePoint;
        map.convertRad2Pt(pt.u, pt.v);
        pos1 -= QPoint(pt.u - 24, pt.v - 24);

        if(doMove)
        {
            pt_t pt;
            pt.idx  = thePoint->idx;
            pt.u    = pos.x();
            pt.v    = pos.y();
            map.convertPt2Rad(pt.u, pt.v);

            *thePoint = pt;
            theMainWindow->getCanvas()->update();
        }
        else if(rectMove.contains(pos1) || rectDel.contains(pos1) || rectAdd1.contains(pos1) || rectAdd2.contains(pos1))
        {
            if(!doSpecialCursor)
            {
                QApplication::setOverrideCursor(Qt::PointingHandCursor);
                doSpecialCursor = true;
            }
        }
        else
        {
            if(doSpecialCursor)
            {
                QApplication::restoreOverrideCursor();
                doSpecialCursor = false;
            }
        }

        if(addType != eNone)
        {
            // find subline between last steady point and current point
            double u1, v1, u2, v2;
            IMap& map = CMapDB::self().getMap();

            if(points.size() > 1 && *thePoint == points.last())
            {
                u1 = (points.end() - 2)->u;
                v1 = (points.end() - 2)->v;
            }
            else if(points.size() > 1 && *thePoint == points.first())
            {
                u1 = (points.begin() + 1)->u;
                v1 = (points.begin() + 1)->v;
            }
            else
            {
                int idx = points.indexOf(*thePoint);

                if(addType == eAfter)
                {
                    idx--;
                }
                else
                {
                    idx++;
                }

                u1 = (points.begin() + idx)->u;
                v1 = (points.begin() + idx)->v;
            }

            map.convertRad2Pt(u1,v1);
            QPoint pt1(u1, v1);

            u2 = thePoint->u;
            v2 = thePoint->v;

            map.convertRad2Pt(u2,v2);
            QPoint pt2(u2, v2);

            CMapDB::self().getMap().getClosePolyline(pt1, pt2, 10, leadline);

            if(!leadline.isEmpty())
            {
                GPS_Math_SubPolyline(pt1, pt2, 10, leadline, subline);
            }

            QRect r = theMainWindow->getCanvas()->rect();

            int w = r.width() / 10;
            int h = r.height() / 10;

            if(e->pos().x() < (r.left() + w))
            {
                theMainWindow->getCanvas()->move(CCanvas::eMoveLeftSmall);
            }
            else if(e->pos().x() > (r.right() - w))
            {
                theMainWindow->getCanvas()->move(CCanvas::eMoveRightSmall);
            }

            if(e->pos().y() < (r.top() + h))
            {
                theMainWindow->getCanvas()->move(CCanvas::eMoveUpSmall);
            }
            else if(e->pos().y() > (r.bottom() - h))
            {
                theMainWindow->getCanvas()->move(CCanvas::eMoveDownSmall);
            }
        }
    }

}


void COverlayArea::mousePressEvent(QMouseEvent * e)
{
    if(thePoint == 0) return;
    IMap& map   = CMapDB::self().getMap();

    if(e->button() == Qt::LeftButton)
    {
        if(doMove)
        {
            if(*thePoint == points.last() && addType == eAtEnd)
            {
                const int size = subline.size();
                if(size < 2)
                {
                    pt_t pt;
                    pt.u    = e->pos().x();
                    pt.v    = e->pos().y();
                    map.convertPt2Rad(pt.u, pt.v);

                    points.push_back(pt);
                }
                else
                {
                    pt_t pt;
                    pt.u = subline[1].x();
                    pt.v = subline[1].y();
                    map.convertPt2Rad(pt.u, pt.v);

                    *thePoint = pt;

                    for(int i = 2; i < size; i++)
                    {
                        pt.u = subline[i].x();
                        pt.v = subline[i].y();
                        map.convertPt2Rad(pt.u, pt.v);

                        points.push_back(pt);
                    }

                    points.push_back(pt);

                }
                thePointBefor   = &(*(points.end() - 2));
                thePoint        = &points.last();
                thePointAfter   = 0;
            }
            else if(*thePoint == points.first() && addType == eAtEnd)
            {
                const int size = subline.size();
                if(size < 2)
                {

                    pt_t pt;
                    pt.u = e->pos().x();
                    pt.v = e->pos().y();
                    map.convertPt2Rad(pt.u, pt.v);

                    points.push_front(pt);
                }
                else
                {
                    pt_t pt;
                    pt.u = subline[1].x();
                    pt.v = subline[1].y();
                    map.convertPt2Rad(pt.u, pt.v);

                    *thePoint = pt;

                    for(int i = 2; i < size; i++)
                    {
                        pt.u = subline[i].x();
                        pt.v = subline[i].y();
                        map.convertPt2Rad(pt.u, pt.v);

                        points.push_front(pt);
                    }

                    points.push_front(pt);
                }
                thePointBefor   = 0;
                thePoint        = &points.first();
                thePointAfter   = &(*(points.begin() + 1));
            }
            else if(addType != eNone)
            {
                pt_t pt;
                const int size = subline.size();
                int idx = points.indexOf(*thePoint);

                if(size > 2)
                {
                    if(addType == eAfter)
                    {
                        pt.u = subline[0].x();
                        pt.v = subline[0].y();
                        map.convertPt2Rad(pt.u, pt.v);

                        points[idx - 1] = pt;

                        pt.u = subline[size - 1].x();
                        pt.v = subline[size - 1].y();
                        map.convertPt2Rad(pt.u, pt.v);

                        points[idx] = pt;

                        for(int i = 1; i < size - 1; i++)
                        {
                            pt.u = subline[i].x();
                            pt.v = subline[i].y();
                            map.convertPt2Rad(pt.u, pt.v);

                            points.insert(idx - 1 + i,pt);
                        }
                        idx += size - 2;
                    }
                    else
                    {
                        pt.u = subline[0].x();
                        pt.v = subline[0].y();
                        map.convertPt2Rad(pt.u, pt.v);

                        points[idx + 1] = pt;

                        pt.u = subline[size - 1].x();
                        pt.v = subline[size - 1].y();
                        map.convertPt2Rad(pt.u, pt.v);

                        points[idx] = pt;

                        for(int i = 1; i < size - 1; i++)
                        {
                            pt.u = subline[i].x();
                            pt.v = subline[i].y();
                            map.convertPt2Rad(pt.u, pt.v);

                            points.insert(idx+1,pt);
                        }
                    }
                }

                if(addType == eAfter)
                {
                    idx++;
                }

                pt.u = e->pos().x();
                pt.v = e->pos().y();
                map.convertPt2Rad(pt.u, pt.v);
                points.insert(idx,pt);

                thePointBefor   = idx ? &points[idx - 1] : 0;
                thePoint        = &points[idx];
                thePointAfter   = (idx + 1) == points.count() ? 0 : &points[idx + 1];
            }
            else
            {
                pt_t pt;
                pt.u = e->pos().x();
                pt.v = e->pos().y();
                map.convertPt2Rad(pt.u, pt.v);

                *thePoint = pt;

                doMove          = false;
                addType         = eNone;
                thePoint        = 0;
                thePointBefor   = 0;
                thePointAfter   = 0;

            }

            subline.clear();

            calc();
            theMainWindow->getCanvas()->update();

            //if(addType == eNone)   // why?
            //{
            emit sigChanged();
            //}
            return;
        }

        if(!doFuncWheel)
        {
            selectedPoints.clear();
            selectedPoints << points.indexOf(*thePoint);
            emit sigSelectionChanged();

            doFuncWheel = true;
            theMainWindow->getCanvas()->update();
            return;
        }

        QPoint pos1 = e->pos();

        projXY pt = *thePoint;
        map.convertRad2Pt(pt.u, pt.v);
        pos1 -= QPoint(pt.u - 24, pt.v - 24);

        if(rectDel.contains(pos1))
        {
            int idx = points.indexOf(*thePoint);

            if(idx == -1) return;

            points.takeAt(idx);

            if(points.isEmpty())
            {
                QStringList keys(getKey());
                COverlayDB::self().delOverlays(keys);
            }

            calc();
            doFuncWheel     = false;
            thePoint        = 0;
            thePointBefor   = 0;
            thePointAfter   = 0;

            emit sigChanged();
        }
        else if(rectMove.contains(pos1))
        {
            QApplication::setOverrideCursor(QCursor(QPixmap(":/cursors/cursorMoveWpt.png"),0,0));
            doMove      = true;
            doFuncWheel = false;

            savePoint = *thePoint;
        }
        else if(rectAdd1.contains(pos1))
        {
            int idx = points.indexOf(*thePoint);

            if(idx == -1) return;

            pt_t pt;
            pt.u = e->pos().x();
            pt.v = e->pos().y();
            map.convertPt2Rad(pt.u, pt.v);
            points.insert(idx,pt);

            thePointBefor   = idx ? &points[idx - 1] : 0;
            thePoint        = &points[idx];
            thePointAfter   = (idx + 1) == points.count() ? 0 : &points[idx + 1];

            doMove          = true;
            addType         = eBefore;
            doFuncWheel     = false;

            theMainWindow->getCanvas()->update();
        }
        else if(rectAdd2.contains(pos1))
        {
            int idx = points.indexOf(*thePoint);

            if(idx == -1) return;

            idx++;

            pt_t pt;
            pt.u = e->pos().x();
            pt.v = e->pos().y();
            map.convertPt2Rad(pt.u, pt.v);
            points.insert(idx,pt);

            thePointBefor   = idx ? &points[idx - 1] : 0;
            thePoint        = &points[idx];
            thePointAfter   = (idx + 1) == points.count() ? 0 : &points[idx + 1];

            doMove          = true;
            addType         = eAfter;
            doFuncWheel     = false;

            theMainWindow->getCanvas()->update();

        }
        else
        {
            doFuncWheel     = false;
            thePoint        = 0;
            thePointBefor   = 0;
            thePointAfter   = 0;

        }
    }
    else if(e->button() == Qt::RightButton)
    {
        looseFocus();
        return;
    }

    selectedPoints.clear();
    if(addType == eNone)
    {
        emit sigSelectionChanged();
    }
}


void COverlayArea::mouseReleaseEvent(QMouseEvent * e)
{

}


void COverlayArea::drawDistanceInfo(projXY p1, projXY p2, QPainter& p, IMap& map)
{
    QString val, unit, str;
    double a1, a2, dist;

    dist = ::distance(p1, p2, a1, a2);
    IUnit::self().meter2distance(dist, val, unit);
    str = QString("%1 %2 %3%4").arg(val).arg(unit).arg(a2,0,'f',0).arg(QChar(0260));

    map.convertRad2Pt(p1.u, p1.v);
    map.convertRad2Pt(p2.u, p2.v);

    QFontMetrics fm(CResources::self().getMapFont());
    qint32 pixel = sqrt((p2.u - p1.u)*(p2.u - p1.u) + (p2.v - p1.v)*(p2.v - p1.v));

    if(fm.width(str) > pixel)
    {
        return;
    }

    p.save();
    p.translate(p1.u + (p2.u - p1.u) / 2, p1.v + (p2.v - p1.v) / 2);
    if(a2 > 0)
    {
        p.rotate(a2 - 90);
    }
    else
    {
        p.rotate(a2 + 90);
    }

    CCanvas::drawText(str, p, QPoint(0,0));

    p.restore();

}


void COverlayArea::slotShow()
{
    isVisible = !isVisible;
    emit sigChanged();
}


void COverlayArea::slotEdit()
{
    if(!overlayAreaEditWidget.isNull()) delete overlayAreaEditWidget;
    overlayAreaEditWidget = new COverlayAreaEditWidget(theMainWindow->getCanvas(), this);
    theMainWindow->setTempWidget(overlayAreaEditWidget, tr("Overlay"));
}


void COverlayArea::customMenu(QMenu& menu)
{
    menu.addAction(QPixmap(":/icons/iconEdit16x16.png"),tr("Edit..."),this,SLOT(slotEdit()));
    menu.addSeparator();
    QAction * actShow = menu.addAction(tr("Show"),this,SLOT(slotShow()));
    actShow->setCheckable(true);
    actShow->setChecked(isVisible);
}


void COverlayArea::makeVisible()
{
    double north =  -90.0 * DEG_TO_RAD;
    double south =  +90.0 * DEG_TO_RAD;
    double west  = +180.0 * DEG_TO_RAD;
    double east  = -180.0 * DEG_TO_RAD;

    projXY pt;
    foreach(pt, points)
    {
        if(pt.u < west)  west  = pt.u;
        if(pt.u > east)  east  = pt.u;
        if(pt.v < south) south = pt.v;
        if(pt.v > north) north = pt.v;
    }
    CMapDB::self().getMap().zoom(west, north, east, south);

    isVisible = true;
    emit sigChanged();
}


void COverlayArea::looseFocus()
{
    if(thePoint && doMove)
    {
        if(addType != eNone)
        {
            points.removeOne(*thePoint);
        }
        else
        {
            *thePoint = savePoint;
        }
    }

    if(doSpecialCursor)
    {
        QApplication::restoreOverrideCursor();
        doSpecialCursor = false;
    }

    doMove          = false;
    addType         = eNone;
    doFuncWheel     = false;

    subline.clear();
    calc();
}


QRectF COverlayArea::getBoundingRectF()
{
    double north =  -90.0 * DEG_TO_RAD;
    double south =  +90.0 * DEG_TO_RAD;
    double west  = +180.0 * DEG_TO_RAD;
    double east  = -180.0 * DEG_TO_RAD;

    projXY pt;
    foreach(pt, points)
    {
        if(pt.u < west)  west  = pt.u;
        if(pt.u > east)  east  = pt.u;
        if(pt.v < south) south = pt.v;
        if(pt.v > north) north = pt.v;
    }

    return QRectF(QPointF(west * RAD_TO_DEG,north * RAD_TO_DEG),QPointF(east * RAD_TO_DEG,south * RAD_TO_DEG));

}


void COverlayArea::delPointsByIdx(const QList<int>& idx)
{
    int i;

    foreach(i, idx)
    {
        QList<pt_t>::iterator pt = points.begin();
        while(pt != points.end())
        {
            if(pt->idx == i)
            {
                pt = points.erase(pt);
            }
            else
            {
                pt++;
            }
        }
    }

    thePoint        = 0;
    thePointBefor   = 0;
    thePointAfter   = 0;
    doMove          = false;
    addType         = eNone;
    doFuncWheel     = false;

    selectedPoints.clear();

    calc();
    emit sigChanged();
}


void COverlayArea::calc()
{
    for(int i = 1; i < points.count(); i++)
    {
        points[i].idx = i;
    }
}
