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

#include "CMouseRefPoint.h"
#include "CCanvas.h"
#include "CMapDB.h"
#include "CCreateMapGridTool.h"

#include <QtGui>
#include <QMenu>

CMouseRefPoint::CMouseRefPoint(CCanvas * canvas)
: IMouse(canvas)
, selRefPt(0)
, selAreaMode(eSelAreaNone)
{
    cursor = QCursor(QPixmap(":/cursors/cursorMoveMap.png"),0,0);

    state = &CMouseRefPoint::stateMove;
}


CMouseRefPoint::~CMouseRefPoint()
{

}


void CMouseRefPoint::draw(QPainter& p)
{
    if(selRefPt)
    {
        IMap& map = CMapDB::self().getMap();

        double x = selRefPt->x;
        double y = selRefPt->y;
        map.convertM2Pt(x,y);

        p.drawPixmap(x - 15,y - 31,QPixmap(":/icons/iconRefPointHL31x31.png"));

    }
}


void CMouseRefPoint::mouseMoveEvent(QMouseEvent * e)
{

    mousePos = e->pos();

    CCreateMapGeoTiff * dlg = CCreateMapGeoTiff::self();
    if(dlg == 0) return;

    if((this->*state)(mousePos, *dlg))
    {
        canvas->update();
    }
}


bool CMouseRefPoint::stateMove(QPoint& pos, CCreateMapGeoTiff &dlg)
{
    IMap& map = CMapDB::self().getMap();
    selRefPt = 0;

    QMap<quint32,CCreateMapGeoTiff::refpt_t>& refpts         = dlg.getRefPoints();
    QMap<quint32,CCreateMapGeoTiff::refpt_t>::iterator refpt = refpts.begin();
    while(refpt != refpts.end())
    {
        double x = refpt->x;
        double y = refpt->y;
        map.convertM2Pt(x,y);

        QPoint diff = pos - QPoint(x,y);
        if(diff.manhattanLength() < 100)
        {
            selRefPt = &(*refpt);
            cursor = QCursor(QPixmap(":/cursors/cursorMoveRefPoint.png"),0,0);
            QApplication::setOverrideCursor(cursor);

            state = &CMouseRefPoint::stateHighlightRefPoint;
            return true;
        }

        ++refpt;
    }

    if(CCreateMapGridTool::self())
    {
        QRect r1 = dlg.getSelArea();

        double x1 = r1.left();
        double y1 = r1.top();
        double x2 = r1.right();
        double y2 = r1.bottom();
        int l1, l2;

        map.convertM2Pt(x1,y1);
        map.convertM2Pt(x2,y2);

        r1 = QRect(QPoint(x1,y1), QPoint(x2,y2));

        l1 = r1.top() + 10;
        l2 = r1.top() - 10;

        if(mousePos.y() > l2 && mousePos.y() < l1)
        {
            selAreaMode = eSelAreaTop;
            cursor = QCursor(QPixmap(":/cursors/cursorMoveVert.png"),12,15);
            QApplication::setOverrideCursor(cursor);

            state = &CMouseRefPoint::stateHighlightSelArea;
            return true;
        }

        l1 = r1.bottom() + 10;
        l2 = r1.bottom() - 10;

        if(mousePos.y() > l2 && mousePos.y() < l1)
        {
            selAreaMode = eSelAreaBottom;
            cursor = QCursor(QPixmap(":/cursors/cursorMoveVert.png"),12,15);
            QApplication::setOverrideCursor(cursor);

            state = &CMouseRefPoint::stateHighlightSelArea;
            return true;
        }

        l1 = r1.left() + 10;
        l2 = r1.left() - 10;

        if(mousePos.x() > l2 && mousePos.x() < l1)
        {
            selAreaMode = eSelAreaLeft;
            cursor = QCursor(QPixmap(":/cursors/cursorMoveHorz.png"),13,12);
            QApplication::setOverrideCursor(cursor);

            state = &CMouseRefPoint::stateHighlightSelArea;
            return true;
        }

        l1 = r1.right() + 10;
        l2 = r1.right() - 10;

        if(mousePos.x() > l2 && mousePos.x() < l1)
        {
            selAreaMode = eSelAreaRight;
            cursor = QCursor(QPixmap(":/cursors/cursorMoveHorz.png"),13,12);
            QApplication::setOverrideCursor(cursor);

            state = &CMouseRefPoint::stateHighlightSelArea;
            return true;
        }
    }

    return false;

}


bool CMouseRefPoint::stateMoveMap(QPoint& pos, CCreateMapGeoTiff& dlg)
{
    IMap& map = CMapDB::self().getMap();

    map.move(oldPoint, pos);
    oldPoint = pos;
    return true;
}


bool CMouseRefPoint::stateMoveRefPoint(QPoint& pos, CCreateMapGeoTiff &dlg)
{
    IMap& map = CMapDB::self().getMap();
    double x = pos.x();
    double y = pos.y();
    map.convertPt2M(x,y);
    selRefPt->x = x;
    selRefPt->y = y;
    selRefPt->item->setText(CCreateMapGeoTiff::eX,tr("%1").arg((int)x));
    selRefPt->item->setText(CCreateMapGeoTiff::eY,tr("%1").arg((int)y));
    dlg.selRefPointByKey(selRefPt->item->data(CCreateMapGeoTiff::eLabel,Qt::UserRole).toInt());
    return true;
}


bool CMouseRefPoint::stateMoveSelArea(QPoint& pos, CCreateMapGeoTiff& dlg)
{
    IMap& map = CMapDB::self().getMap();
    double x = pos.x();
    double y = pos.y();
    map.convertPt2M(x,y);

    QRect& r1 = dlg.getSelArea();

    switch(selAreaMode)
    {
        case eSelAreaTop:
            r1.setTop(y);
            break;
        case eSelAreaBottom:
            r1.setBottom(y);
            break;
        case eSelAreaLeft:
            r1.setLeft(x);
            break;
        case eSelAreaRight:
            r1.setRight(x);
            break;
        default:;
    }

    return true;
}


bool CMouseRefPoint::stateHighlightRefPoint(QPoint& pos, CCreateMapGeoTiff &dlg)
{
    IMap& map = CMapDB::self().getMap();

    double x = selRefPt->x;
    double y = selRefPt->y;
    map.convertM2Pt(x,y);

    QPoint diff = pos - QPoint(x,y);
    if(diff.manhattanLength() > 100)
    {
        cursor = QCursor(QPixmap(":/cursors/cursorMoveMap.png"),0,0);
        QApplication::restoreOverrideCursor();

        state = &CMouseRefPoint::stateMove;
        selRefPt = 0;
        return true;
    }

    return false;
}


bool CMouseRefPoint::stateHighlightSelArea(QPoint& pos, CCreateMapGeoTiff &dlg)
{
    IMap& map = CMapDB::self().getMap();

    QRect r1 = dlg.getSelArea();

    double x1 = r1.left();
    double y1 = r1.top();
    double x2 = r1.right();
    double y2 = r1.bottom();
    int l1, l2;

    map.convertM2Pt(x1,y1);
    map.convertM2Pt(x2,y2);

    r1 = QRect(QPoint(x1,y1), QPoint(x2,y2));

    switch(selAreaMode)
    {
        case eSelAreaTop:
            l1 = r1.top() + 10;
            l2 = r1.top() - 10;
            if(pos.y() > l2 && pos.y() < l1)
            {
                return false;
            }
            break;
        case eSelAreaBottom:
            l1 = r1.bottom() + 10;
            l2 = r1.bottom() - 10;
            if(pos.y() > l2 && pos.y() < l1)
            {
                return false;
            }
            break;
        case eSelAreaLeft:
            l1 = r1.left() + 10;
            l2 = r1.left() - 10;
            if(pos.x() > l2 && pos.x() < l1)
            {
                return false;
            }
            break;
        case eSelAreaRight:
            l1 = r1.right() + 10;
            l2 = r1.right() - 10;
            if(pos.x() > l2 && pos.x() < l1)
            {
                return false;
            }
            break;
        default:;
    }

    cursor = QCursor(QPixmap(":/cursors/cursorMoveMap.png"),0,0);
    QApplication::restoreOverrideCursor();
    state = &CMouseRefPoint::stateMove;

    return false;
}


void CMouseRefPoint::mousePressEvent(QMouseEvent * e)
{
    if(e->button() == Qt::LeftButton)
    {
        if(state == &CMouseRefPoint::stateHighlightRefPoint)
        {
            state = &CMouseRefPoint::stateMoveRefPoint;
            mouseMoveEvent(e);
        }
        else if(state == &CMouseRefPoint::stateHighlightSelArea)
        {
            state = &CMouseRefPoint::stateMoveSelArea;
            mouseMoveEvent(e);
        }
        else
        {
            cursor = QCursor(QPixmap(":/cursors/cursorMove.png"));
            QApplication::setOverrideCursor(cursor);
            oldPoint    = e->pos();

            state = &CMouseRefPoint::stateMoveMap;
            mouseMoveEvent(e);
        }
    }
    else if(e->button() == Qt::RightButton)
    {
        mousePos = e->pos();
        canvas->raiseContextMenu(e->pos());
    }

    canvas->update();
}


void CMouseRefPoint::mouseReleaseEvent(QMouseEvent * e)
{
    if(e->button() == Qt::LeftButton)
    {
        if(state == &CMouseRefPoint::stateMoveMap)
        {
            cursor = QCursor(QPixmap(":/cursors/cursorMoveMap.png"),0,0);
            QApplication::restoreOverrideCursor();

            //mouseMoveEvent(e);
            state = &CMouseRefPoint::stateMove;
        }
        else if(state == &CMouseRefPoint::stateMoveRefPoint)
        {
            cursor = QCursor(QPixmap(":/cursors/cursorMoveMap.png"),0,0);
            QApplication::restoreOverrideCursor();

            //mouseMoveEvent(e);
            selRefPt = 0;
            state = &CMouseRefPoint::stateMove;

        }
        else if(state == &CMouseRefPoint::stateMoveSelArea)
        {
            cursor = QCursor(QPixmap(":/cursors/cursorMoveMap.png"),0,0);
            QApplication::restoreOverrideCursor();

            //mouseMoveEvent(e);
            selAreaMode = eSelAreaNone;
            state = &CMouseRefPoint::stateMove;
        }
    }
}


void CMouseRefPoint::contextMenu(QMenu& menu)
{
    IMap& map = CMapDB::self().getMap();

    double u = mousePos.x();
    double v = mousePos.y();
    map.convertPt2Pixel(u,v);

    if(u >= 0 && v >= 0)
    {
        QString posPixel = tr("Pixel %1x%2").arg(u, 0,'f',0).arg(v,0,'f',0);
        menu.addAction(QIcon(":/icons/iconClipboard16x16.png"), posPixel, this, SLOT(slotCopyPosPixel()));

        if(pos1Pixel.x() >= 0 && pos1Pixel.y() >= 0)
        {
            double u1 = pos1Pixel.x();
            double v1 = pos1Pixel.y();

            QString posPixelSize = tr("Pos1 -> Pos %1x%2 w:%3 h:%4").arg(u1, 0,'f',0).arg(v1,0,'f',0).arg(u - u1,0,'f',0).arg(v - v1, 0,'f',0);
            menu.addAction(QIcon(":/icons/iconClipboard16x16.png"), posPixelSize, this, SLOT(slotCopyPosPixelSize()));
        }

        menu.addAction(QIcon(":/icons/wpt/flag_pin_red15x15.png"), tr("Set as Pos1"), this, SLOT(slotSetPos1()));

    }

}


void CMouseRefPoint::slotCopyPosPixel()
{
    IMap& map = CMapDB::self().getMap();

    double u = mousePos.x();
    double v = mousePos.y();

    map.convertPt2Pixel(u,v);
    QString position = QString("%1 %2").arg(u, 0,'f',0).arg(v,0,'f',0);

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(position);

}


void CMouseRefPoint::slotCopyPosPixelSize()
{
    IMap& map = CMapDB::self().getMap();

    double u1 = pos1Pixel.x();
    double v1 = pos1Pixel.y();
    double u2 = mousePos.x();
    double v2 = mousePos.y();

    map.convertPt2Pixel(u2,v2);
    QString position = QString("%1 %2 %3 %4").arg(u1, 0,'f',0).arg(v1,0,'f',0).arg(u2 - u1, 0,'f',0).arg(v2 - v1,0,'f',0);

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(position);

}
