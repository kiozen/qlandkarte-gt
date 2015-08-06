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

#ifndef CMOUSEREFPOINT_H
#define CMOUSEREFPOINT_H

#include "IMouse.h"
#include "CCreateMapGeoTiff.h"

class CCreateMapGeoTiff;

class CMouseRefPoint :  public IMouse
{
    Q_OBJECT;
    public:
        CMouseRefPoint(CCanvas * canvas);
        virtual ~CMouseRefPoint();

        void draw(QPainter& p);
        void mouseMoveEvent(QMouseEvent * e);
        void mousePressEvent(QMouseEvent * e);
        void mouseReleaseEvent(QMouseEvent * e);

        void contextMenu(QMenu& menu);

    private slots:
        void slotCopyPosPixel();
        void slotCopyPosPixelSize();

    private:
        bool (CMouseRefPoint::*state)(QPoint& pos, CCreateMapGeoTiff& dlg);

        bool stateMove(QPoint& pos, CCreateMapGeoTiff& dlg);
        bool stateMoveMap(QPoint& pos, CCreateMapGeoTiff& dlg);
        bool stateMoveRefPoint(QPoint& pos, CCreateMapGeoTiff& dlg);
        bool stateMoveSelArea(QPoint& pos, CCreateMapGeoTiff& dlg);
        bool stateHighlightRefPoint(QPoint& pos, CCreateMapGeoTiff& dlg);
        bool stateHighlightSelArea(QPoint& pos, CCreateMapGeoTiff& dlg);

        /// the initial starting point of the transformation
        QPoint oldPoint;

        CCreateMapGeoTiff::refpt_t * selRefPt;

        enum selAreaMode_e
        {
            eSelAreaNone
            ,eSelAreaTop
            ,eSelAreaLeft
            ,eSelAreaBottom
            ,eSelAreaRight

        };

        selAreaMode_e selAreaMode;
};
#endif                           //CMOUSEREFPOINT_H
