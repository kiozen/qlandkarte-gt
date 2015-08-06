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

#include "CMouseEditWpt.h"
#include "CDlgEditWpt.h"
#include "CMainWindow.h"
#include "CCanvas.h"

#include <QtGui>

CMouseEditWpt::CMouseEditWpt(CCanvas * canvas)
: IMouse(canvas)
{
    cursor = QCursor(QPixmap(":/cursors/cursorEdit.png"),0,0);
}


CMouseEditWpt::~CMouseEditWpt()
{

}


void CMouseEditWpt::mouseMoveEvent(QMouseEvent * e)
{
    mouseMoveEventWpt(e);
}


void CMouseEditWpt::mousePressEvent(QMouseEvent * e)
{
    if(e->button() == Qt::LeftButton)
    {
        if(selWpts.size() == 1)
        {
            CWpt * selWpt = selWpts.first().wpt;
            CDlgEditWpt dlg(*selWpt,canvas);
            dlg.exec();
        }
    }
}


void CMouseEditWpt::mouseReleaseEvent(QMouseEvent * e)
{
}


void CMouseEditWpt::draw(QPainter& p)
{
    drawSelWpt(p);
}
