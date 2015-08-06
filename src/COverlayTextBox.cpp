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

#include "COverlayTextBox.h"
#include "CMapDB.h"
#include "CMainWindow.h"
#include "CCanvas.h"
#include "COverlayDB.h"
#include "CDlgEditText.h"
#include "GeoMath.h"

#include <QtGui>

COverlayTextBox::COverlayTextBox(const QString& text, double lon, double lat, const QPoint& anchor, const QRect& r, QObject * parent)
: IOverlay(parent, "TextBox", ":/icons/iconTextBox16x16.png")
, lon(lon)
, lat(lat)
, rect(r)
, pt(anchor)
, doMove(false)
, doSize(false)
, doPos(false)
, doSpecialCursor(false)
{
    rect.translate(-anchor);
    pt = QPoint(0,0);

    polyline = makePolyline(pt, rect);

    rectMove    = QRect(rect.topLeft()     + QPoint(2,2)  , QSize(16, 16));
    rectEdit    = QRect(rect.topLeft()     + QPoint(20,2) , QSize(16, 16));
    rectDel     = QRect(rect.topRight()    - QPoint(18,-2), QSize(16, 16));
    rectSize    = QRect(rect.bottomRight() - QPoint(16,16), QSize(16, 16));
    rectDoc     = QRect(rect.topLeft()     + QPoint(5,5)  , rect.size() - QSize(10, 10));
    rectAnchor  = QRect(QPoint(-7,-7), QSize(16, 16));

    comment = text;

    doc = new QTextDocument(this);
    doc->setHtml(comment);
    doc->setPageSize(rectDoc.size());

}


COverlayTextBox::~COverlayTextBox()
{

}


QPolygonF COverlayTextBox::makePolyline(const QPoint& anchor, const QRect& r)
{
    QPolygonF poly1, poly2;
    poly1 << r.topLeft() << r.topRight() << r.bottomRight() << r.bottomLeft();

    if(!r.contains(anchor))
    {
        int w = (r.width()>>1) - 50;
        int h = (r.height()>>1) - 50;

        w = w > 100 ? 100 : w;
        h = h > 100 ? 100 : h;

        w = w < 20 ? 20 : w;
        h = h < 20 ? 20 : h;

        if(anchor.x() < r.left())
        {
            poly2 << anchor << (r.center() + QPoint(0,-h)) << (r.center() + QPoint(0,h)) << anchor;
        }
        else if(r.right() < anchor.x())
        {
            poly2 << anchor << (r.center() + QPoint(0,-h)) << (r.center() + QPoint(0,h)) << anchor;
        }
        else if(anchor.y() < r.top())
        {
            poly2 << anchor << (r.center() + QPoint(-w,0)) << (r.center() + QPoint(w,0)) << anchor;
        }
        else if(r.bottom() < anchor.y())
        {
            poly2 << anchor << (r.center() + QPoint(-w,0)) << (r.center() + QPoint(w,0)) << anchor;
        }

        QPainterPath path1;
        path1.addRoundedRect(r,5,5);
        QPainterPath path2;
        path2.addPolygon(poly2);

        path1 = path1.united(path2);

        poly1 = path1.toFillPolygon();
    }

    return poly1;
}


void COverlayTextBox::draw(QPainter& p, const QRect& viewport)
{
    double x = lon;
    double y = lat;

    CMapDB::self().getMap().convertRad2Pt(x,y);
    p.save();
    p.translate(x,y);

    if(selected == this)
    {
        p.setBrush(Qt::white);
        p.setPen(QPen(Qt::red,3));
        p.drawPolygon(polyline);

        p.drawPixmap(rectMove, QPixmap(":/icons/iconMoveMap16x16.png"));
        p.drawPixmap(rectSize, QPixmap(":/icons/iconSize16x16.png"));
        p.drawPixmap(rectDel, QPixmap(":/icons/iconClear16x16.png"));
        p.drawPixmap(rectEdit, QPixmap(":/icons/iconEdit16x16.png"));
        p.drawPixmap(rectAnchor, QPixmap(":/icons/iconMoveMap16x16.png"));
    }
    else
    {
        if(highlight)
        {
            p.setBrush(Qt::white);
            p.setPen(QPen(Qt::blue,3));
        }
        else
        {
            p.setBrush(Qt::white);
            p.setPen(QPen(Qt::darkGray,2));
        }
        p.drawPolygon(polyline);
    }

    p.save();
    p.setClipRect(rectDoc);
    p.translate(rectDoc.topLeft());
    doc->drawContents(&p);
    p.restore();

    p.restore();
}


bool COverlayTextBox::isCloseEnough(const QPoint& pt)
{
    QPolygonF box = polyline;

    double x = lon;
    double y = lat;

    CMapDB::self().getMap().convertRad2Pt(x,y);
    box.translate(x,y);

    QRectF r = box.boundingRect();
    r.setTopLeft(r.topLeft() - QPoint(8,8));
    r.setBottomRight(r.bottomRight() + QPoint(8,8));

    return r.contains(pt);
}


QString COverlayTextBox::getInfo()
{
    QString text;

    GPS_Math_Deg_To_Str(lon * RAD_TO_DEG, lat * RAD_TO_DEG, text);

    text += "\n" + doc->toPlainText();

    if(text.length() < 60)
    {
        return text;
    }
    else
    {
        return text.left(57) + "...";
    }
}


void COverlayTextBox::save(QDataStream& s)
{
    s << lon << lat << pt << rect << comment << getKey();
}


void COverlayTextBox::load(QDataStream& s)
{
    QString key;
    s >> lon >> lat >> pt >> rect >> comment >> key;
    setKey(key);
}


void COverlayTextBox::mouseMoveEvent(QMouseEvent * e)
{
    double x = lon;
    double y = lat;
    CMapDB::self().getMap().convertRad2Pt(x,y);
    QPoint pos = e->pos() - QPoint(x,y);

    if(doMove)
    {
        rect.moveTopLeft(pos);
        rectMove = QRect(rect.topLeft()     + QPoint(2,2)  , QSize(16, 16));
        rectEdit = QRect(rect.topLeft()     + QPoint(20,2) , QSize(16, 16));
        rectDel  = QRect(rect.topRight()    - QPoint(18,-2), QSize(16, 16));
        rectSize = QRect(rect.bottomRight() - QPoint(16,16), QSize(16, 16));

        rectDoc     = QRect(rect.topLeft()     + QPoint(5,5)  , rect.size() - QSize(10, 10));
        doc->setPageSize(rectDoc.size());

        polyline = makePolyline(QPoint(0,0), rect);

        theMainWindow->getCanvas()->update();
    }
    else if(doSize)
    {
        rect.setBottom(qMax(rect.top()+0+18*2, pos.y()));
        rect.setRight(qMax(rect.left()+0+18*3, pos.x()));
        rectMove = QRect(rect.topLeft()     + QPoint(2,2)  , QSize(16, 16));
        rectEdit = QRect(rect.topLeft()     + QPoint(20,2) , QSize(16, 16));
        rectDel  = QRect(rect.topRight()    - QPoint(18,-2), QSize(16, 16));
        rectSize = QRect(rect.bottomRight() - QPoint(16,16), QSize(16, 16));

        rectDoc     = QRect(rect.topLeft()     + QPoint(5,5)  , rect.size() - QSize(10, 10));
        doc->setPageSize(rectDoc.size());

        polyline = makePolyline(QPoint(0,0), rect);

        theMainWindow->getCanvas()->update();
    }
    else if(doPos)
    {
        rectAnchor  = QRect(pos - QPoint(8,8), QSize(16, 16));
        rectMove    = QRect(rect.topLeft()     + QPoint(2,2)  , QSize(16, 16));
        rectEdit    = QRect(rect.topLeft()     + QPoint(20,2) , QSize(16, 16));
        rectDel     = QRect(rect.topRight()    - QPoint(18,-2), QSize(16, 16));
        rectSize    = QRect(rect.bottomRight() - QPoint(16,16), QSize(16, 16));
        rectDoc     = QRect(rect.topLeft()     + QPoint(5,5)  , rect.size() - QSize(10, 10));

        doc->setPageSize(rectDoc.size());

        polyline = makePolyline(pos, rect);

        theMainWindow->getCanvas()->update();
    }
    else if(rectMove.contains(pos) || rectSize.contains(pos) || rectEdit.contains(pos) || rectDel.contains(pos) || rectAnchor.contains(pos))
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
}


void COverlayTextBox::mousePressEvent(QMouseEvent * e)
{
    double x = lon;
    double y = lat;
    CMapDB::self().getMap().convertRad2Pt(x,y);
    QPoint pos = e->pos() - QPoint(x,y);

    if(rectMove.contains(pos))
    {
        doMove = true;
    }
    else if(rectSize.contains(pos))
    {
        doSize = true;
    }
    else if(rectAnchor.contains(pos))
    {
        doPos = true;
    }
    else if(rectEdit.contains(pos))
    {
        CDlgEditText dlg(comment, theMainWindow->getCanvas());
        dlg.exec();
        doc->setHtml(comment);
        theMainWindow->getCanvas()->update();
        emit sigChanged();
    }
    else if(rectDel.contains(pos))
    {
        QStringList keys(getKey());
        COverlayDB::self().delOverlays(keys);
        QApplication::restoreOverrideCursor();
        doSpecialCursor = false;
        theMainWindow->getCanvas()->setMouseMode(CCanvas::eMouseMoveArea);
    }
}


void COverlayTextBox::mouseReleaseEvent(QMouseEvent * e)
{
    double x = lon;
    double y = lat;
    CMapDB::self().getMap().convertRad2Pt(x,y);
    QPoint pos = e->pos() - QPoint(x,y);

    if(doPos)
    {
        rect.translate(-pos);

        polyline    = makePolyline(QPoint(0,0), rect);

        rectMove    = QRect(rect.topLeft()     + QPoint(2,2)  , QSize(16, 16));
        rectEdit    = QRect(rect.topLeft()     + QPoint(20,2) , QSize(16, 16));
        rectDel     = QRect(rect.topRight()    - QPoint(18,-2), QSize(16, 16));
        rectSize    = QRect(rect.bottomRight() - QPoint(16,16), QSize(16, 16));
        rectDoc     = QRect(rect.topLeft()     + QPoint(5,5)  , rect.size() - QSize(10, 10));
        rectAnchor  = QRect(QPoint(-9,-9), QSize(16, 16));

        lon = e->pos().x();
        lat = e->pos().y();
        CMapDB::self().getMap().convertPt2Rad(lon, lat);
    }

    if(doSize || doMove || doPos)
    {
        emit sigChanged();
    }

    doSize = doMove = doPos = false;
}


void COverlayTextBox::makeVisible()
{
    theMainWindow->getCanvas()->move(lon * RAD_TO_DEG, lat * RAD_TO_DEG);
}


QRectF COverlayTextBox::getBoundingRectF()
{
    return QRectF(lon * RAD_TO_DEG, lat * RAD_TO_DEG, 0.00001, 0.00001);
}
