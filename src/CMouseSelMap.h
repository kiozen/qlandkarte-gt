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

#ifndef CMOUSESELMAP_H
#define CMOUSESELMAP_H

#include "IMouse.h"

#include <QPair>

/// select a subarea of the map to export
class CMouseSelMap : public IMouse
{
    Q_OBJECT;
    public:
        CMouseSelMap(CCanvas * canvas);
        virtual ~CMouseSelMap();

        void mouseMoveEvent(QMouseEvent * e);
        void mousePressEvent(QMouseEvent * e);
        void mouseReleaseEvent(QMouseEvent * e);

        void draw(QPainter& p);

        void contextMenu(QMenu& menu);
    private slots:
        void slotMapSelAll();
        void slotMapSelNone();

    private:
        void drawSelArea(QPainter& p);
        void drawSelMap(QPainter& p);

        void mousePressEventMapsel(QMouseEvent * e);

        bool selArea;
        bool moveMapSel;

        QPoint oldPoint;

        QMap< QPair<int,int>, bool> selTiles;

        QRect rectMoveMapSel;

};
#endif                           //CMOUSESELMAP_H
