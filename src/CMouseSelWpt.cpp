/**********************************************************************************************
    Copyright (C) 2012 Oliver Eichler oliver.eichler@gmx.de

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

#include "CMouseSelWpt.h"
#include "CMapDB.h"
#include "GeoMath.h"
#include "IUnit.h"
#include "CMainWindow.h"
#include "CResources.h"
#include "CWptDB.h"

#include <QtGui>
#include <QMouseEvent>

CMouseSelWpt::CMouseSelWpt(CCanvas * canvas)
: IMouse(canvas)
, mousePressed(false)
{
    cursor = QCursor(QPixmap(":/cursors/cursorArrow.png"),0,0);
}


CMouseSelWpt::~CMouseSelWpt()
{

}


void CMouseSelWpt::mouseMoveEvent(QMouseEvent * e)
{
    if(!mousePressed) return;

    IMap& map = CMapDB::self().getMap();
#ifdef QK_QT5_PORT
    double u = e->localPos().x();
    double v = e->localPos().y();
#else
    double u = e->posF().x();
    double v = e->posF().y();
#endif
    map.convertPt2Rad(u,v);

    point1 = QPointF(u,v);

    QRect r = canvas->rect();

    int w = r.width() / 10;
    int h = r.height() / 10;

    if(e->pos().x() < (r.left() + w))
    {
        canvas->move(CCanvas::eMoveLeftSmall);
    }
    else if(e->pos().x() > (r.right() - w))
    {
        canvas->move(CCanvas::eMoveRightSmall);
    }

    if(e->pos().y() < (r.top() + h))
    {
        canvas->move(CCanvas::eMoveUpSmall);
    }
    else if(e->pos().y() > (r.bottom() - h))
    {
        canvas->move(CCanvas::eMoveDownSmall);
    }

    canvas->update();
}


void CMouseSelWpt::mousePressEvent(QMouseEvent * e)
{
    IMap& map = CMapDB::self().getMap();
#ifdef QK_QT5_PORT
    double u = e->localPos().x();
    double v = e->localPos().y();
#else
    double u = e->posF().x();
    double v = e->posF().y();
#endif
    map.convertPt2Rad(u,v);

    center = QPointF(u,v);

    mousePressed = true;
}


void CMouseSelWpt::mouseReleaseEvent(QMouseEvent * e)
{

    projXY p1, p2;
    double a1,a2;
    p1.u        = center.x();
    p1.v        = center.y();
    p2.u        = point1.x();
    p2.v        = point1.y();
    double d    = distance(p1, p2, a1, a2);

    CWptDB::self().selWptInRange(center, d);

    center = QPoint(0,0);
    point1 = QPoint(0,0);
    mousePressed = false;
    canvas->setMouseMode(CCanvas::eMouseMoveArea);
}


void CMouseSelWpt::draw(QPainter& p)
{
    qDebug() << center << point1 << mousePressed;
    if(center == point1 || !mousePressed) return;

    IMap& map   = CMapDB::self().getMap();
    double u0   = center.x();
    double v0   = center.y();
    double u1   = point1.x();
    double v1   = point1.y();
    map.convertRad2Pt(u0,v0);
    map.convertRad2Pt(u1,v1);
    double r    = sqrt((u0-u1)*(u0-u1) + (v0-v1)*(v0-v1));
    double a1   = 0, a2 = 0;

    projXY p1, p2;
    p1.u        = center.x();
    p1.v        = center.y();
    p2.u        = point1.x();
    p2.v        = point1.y();
    double d    = distance(p1, p2, a1, a2);

    p.save();
    p.setBrush(CCanvas::brushBackWhite);
    p.setPen(CCanvas::penBorderBlue);
    p.drawEllipse(QPointF(u0,v0), r, r);
    p.drawLine(QPointF(u0,v0), QPointF(u1,v1));

    QFontMetrics fm(CResources::self().getMapFont());
    QString unit, val;
    IUnit::self().meter2distance(d, val, unit);
    QString str = QString("%1 %2").arg(val).arg(unit);

    if(a1 > 0)
    {
        p.translate(u1 + fm.width(str)/2 + 4,v1);
    }
    else
    {
        p.translate(u1 - fm.width(str)/2 - 4,v1);
    }
    CCanvas::drawText(str,p,QPoint(0,0));
    p.restore();
}
