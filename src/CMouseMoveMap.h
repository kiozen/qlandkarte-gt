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

#ifndef CMOUSEMOVEMAP_H
#define CMOUSEMOVEMAP_H

#include "IMouse.h"

#include <QPoint>

/// move the map area by point-click-n-move
class CMouseMoveMap : public IMouse
{
    Q_OBJECT;
    public:
        CMouseMoveMap(CCanvas * parent);
        virtual ~CMouseMoveMap();

        void mouseMoveEvent(QMouseEvent * e);
        void mousePressEvent(QMouseEvent * e);
        void mouseReleaseEvent(QMouseEvent * e);
        void mouseDoubleClickEvent(QMouseEvent * e);
        void keyPressEvent(QKeyEvent * e);
        void keyReleaseEvent(QKeyEvent * e);

        void draw(QPainter& p);

        void contextMenu(QMenu& menu);

    private slots:
        void slotEditWpt();
        void slotCopyPositionWpt();
        void slotDeleteWpt();
        void slotMoveWpt();
        void slotAddWpt();
        void slotCopyPositionTrack();
        void slotEditTrack();
        void slotSplitTrack();
        void slotOpenGoogleMaps();
        void slotCopyPosDegree();
        void slotCopyPosGrid();
        void slotCopyPosMeter();
        void slotCopyPosPixel();
        void slotCopyPosPixelSize();

        void slotReloadMap();

    private:
        /// true if left mouse button is pressed
        bool moveMap;

        bool leftButtonPressed;
        bool altKeyPressed;

        /// the initial starting point of the transformation
        QPoint oldPoint;

};
#endif                           //CMOUSEMOVEMAP_H
