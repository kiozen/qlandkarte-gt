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

#include "CMouseOverlay.h"
#include "IOverlay.h"

#include <QtGui>

CMouseOverlay::CMouseOverlay(CCanvas * canvas)
: IMouse(canvas)
{
    cursor = QCursor(QPixmap(":/cursors/cursorArrow.png"),0,0);
}


CMouseOverlay::~CMouseOverlay()
{

}


void CMouseOverlay::keyPressEvent(QKeyEvent * e)
{
    if(selOverlay) selOverlay->keyPressEvent(e);
}


void CMouseOverlay::mouseMoveEvent(QMouseEvent * e)
{
    mouseMoveEventOverlay(e);
    if(selOverlay) selOverlay->mouseMoveEvent(e);
}


void CMouseOverlay::mousePressEvent(QMouseEvent * e)
{
    if(selOverlay) selOverlay->mousePressEvent(e);
}


void CMouseOverlay::mouseReleaseEvent(QMouseEvent * e)
{
    if(selOverlay) selOverlay->mouseReleaseEvent(e);
}
