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

#include "CMapSearchCanvas.h"

#include <QtGui>

CMapSearchCanvas::CMapSearchCanvas(QWidget * parent)
: QWidget(parent)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
}


CMapSearchCanvas::~CMapSearchCanvas()
{

}


void CMapSearchCanvas::setBuffer(const QPixmap& pic)
{
    buffer = pic;
    update();
}


void CMapSearchCanvas::paintEvent(QPaintEvent * e)
{
    QPainter p;
    p.begin(this);
    p.fillRect(rect(),Qt::white);
    p.drawPixmap(0,0,buffer);

    p.setPen(QPen(Qt::yellow, 2));
    p.drawRect(rectSelect);

    p.end();
}


void CMapSearchCanvas::mouseMoveEvent(QMouseEvent * e)
{
    rectSelect.setBottomRight(e->pos());
    rectSelect.setWidth((rectSelect.width()>>2)<<2);
    update();
}


void CMapSearchCanvas::mousePressEvent(QMouseEvent * e)
{
    if(e->button() == Qt::LeftButton)
    {
        rectSelect = QRect(e->pos(), QSize(1, 1));
        update();
    }
}


void CMapSearchCanvas::mouseReleaseEvent(QMouseEvent * e)
{
    if(e->button() == Qt::LeftButton)
    {
        rectSelect.setBottomRight(e->pos());
        rectSelect.setWidth((rectSelect.width()>>2)<<2);
        emit sigSelection(buffer.copy(rectSelect));

        update();

        rectSelect = QRect();
    }
}
