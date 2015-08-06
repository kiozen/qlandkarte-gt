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

#include "COverlayText.h"
#include "CMainWindow.h"
#include "CCanvas.h"
#include "COverlayDB.h"
#include "CDlgEditText.h"

#include <QtGui>

COverlayText::COverlayText(const QString& text, const QRect& rect, QObject * parent)
: IOverlay(parent, "Text", ":/icons/iconText16x16.png")
, rect(rect)
, doMove(false)
, doSize(false)
, doSpecialCursor(false)
{
    rectMove = QRect(rect.topLeft()     + QPoint(2,2)  , QSize(16, 16));
    rectEdit = QRect(rect.topLeft()     + QPoint(20,2) , QSize(16, 16));
    rectDel  = QRect(rect.topRight()    - QPoint(18,-2), QSize(16, 16));
    rectSize = QRect(rect.bottomRight() - QPoint(16,16), QSize(16, 16));

    rectDoc  = QRect(rect.topLeft()     + QPoint(5,5)  , rect.size() - QSize(10, 10));

    rectMouse = rect;
    rectMouse.setTopLeft(rectMouse.topLeft() - QPoint(8,8));
    rectMouse.setBottomRight(rectMouse.bottomRight() + QPoint(8,8));

    comment = text;

    doc = new QTextDocument(this);
    doc->setHtml(comment);
    doc->setPageSize(rectDoc.size());

}


COverlayText::~COverlayText()
{

}


bool COverlayText::isCloseEnough(const QPoint& pt)
{
    return rectMouse.contains(pt);
}


QString COverlayText::getInfo()
{
    QString text = doc->toPlainText();
    if(text.isEmpty())
    {
        return tr("no text");
    }
    else if(text.length() < 40)
    {
        return text;
    }
    else
    {
        return text.left(37) + "...";
    }
}


void COverlayText::draw(QPainter& p, const QRect& viewport)
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
    PAINT_ROUNDED_RECT(p,rect);

    if(selected == this)
    {
        p.setBrush(Qt::white);
        p.setPen(QPen(Qt::red, 3));
        PAINT_ROUNDED_RECT(p,rect);

        p.drawPixmap(rectMove, QPixmap(":/icons/iconMoveMap16x16.png"));
        p.drawPixmap(rectSize, QPixmap(":/icons/iconSize16x16.png"));
        p.drawPixmap(rectDel, QPixmap(":/icons/iconClear16x16.png"));
        p.drawPixmap(rectEdit, QPixmap(":/icons/iconEdit16x16.png"));
    }
    p.save();
    p.setClipRect(rectDoc);
    p.translate(rectDoc.topLeft());
    doc->drawContents(&p);
    p.restore();
}


void COverlayText::mouseMoveEvent(QMouseEvent * e)
{
    QPoint pos = e->pos();
    if(doMove)
    {
        rect.moveTopLeft(pos);
        rectMove = QRect(rect.topLeft()     + QPoint(2,2)  , QSize(16, 16));
        rectEdit = QRect(rect.topLeft()     + QPoint(20,2) , QSize(16, 16));
        rectDel  = QRect(rect.topRight()    - QPoint(18,-2), QSize(16, 16));
        rectSize = QRect(rect.bottomRight() - QPoint(16,16), QSize(16, 16));

        rectMouse = rect;
        rectMouse.setTopLeft(rectMouse.topLeft() - QPoint(8,8));
        rectMouse.setBottomRight(rectMouse.bottomRight() + QPoint(8,8));

        rectDoc     = QRect(rect.topLeft()     + QPoint(5,5)  , rect.size() - QSize(10, 10));
        doc->setPageSize(rectDoc.size());

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

        rectMouse = rect;
        rectMouse.setTopLeft(rectMouse.topLeft() - QPoint(8,8));
        rectMouse.setBottomRight(rectMouse.bottomRight() + QPoint(8,8));

        rectDoc     = QRect(rect.topLeft()     + QPoint(5,5)  , rect.size() - QSize(10, 10));
        doc->setPageSize(rectDoc.size());

        theMainWindow->getCanvas()->update();
    }
    else if(rectMove.contains(pos) || rectSize.contains(pos) || rectEdit.contains(pos) || rectDel.contains(pos))
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


void COverlayText::mousePressEvent(QMouseEvent * e)
{
    if(rectMove.contains(e->pos()))
    {
        doMove = true;
    }
    else if(rectSize.contains(e->pos()))
    {
        doSize = true;
    }
    else if(rectEdit.contains(e->pos()))
    {
        CDlgEditText dlg(comment, theMainWindow->getCanvas());
        dlg.exec();
        doc->setHtml(comment);
        theMainWindow->getCanvas()->update();
        emit sigChanged();
    }
    else if(rectDel.contains(e->pos()))
    {
        QStringList keys(getKey());
        COverlayDB::self().delOverlays(keys);
        QApplication::restoreOverrideCursor();
        doSpecialCursor = false;
        theMainWindow->getCanvas()->setMouseMode(CCanvas::eMouseMoveArea);
    }
}


void COverlayText::mouseReleaseEvent(QMouseEvent * e)
{
    if(doSize || doMove)
    {
        emit sigChanged();
    }
    doSize = doMove = false;
}


void COverlayText::save(QDataStream& s)
{
    s << rect << comment << getKey();
}


void COverlayText::load(QDataStream& s)
{
    QString key;
    s >> rect >> comment >> key;
    setKey(key);
}
