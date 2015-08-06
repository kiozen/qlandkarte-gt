/**********************************************************************************************
    Copyright (C) 2014 Oliver Eichler oliver.eichler@gmx.de

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

#ifndef COVERLAYAREA_H
#define COVERLAYAREA_H

#include "IOverlay.h"
#include <proj_api.h>

class IMap;
class COverlayAreaEditWidget;

extern QPointer<COverlayAreaEditWidget> overlayAreaEditWidget;

class COverlayArea : public IOverlay
{
    Q_OBJECT;
    public:
        struct pt_t : public projXY
        {
            int idx;
        };

        COverlayArea(const QString& name, const QString& comment, const QColor &color, const Qt::BrushStyle style, const QList<pt_t>& pts, QObject * parent);

        /// draw what ever you want
        void draw(QPainter& p, const QRect& viewport);
        /// returns name, comment and length
        QString getInfo();
        /// return true if coursor is close to the overlay to redirect mouse events into the overlay
        bool isCloseEnough(const QPoint& pt);

        /// returns true while moving a waypoint
        bool mouseActionInProgress(){return doMove;}

        void keyPressEvent(QKeyEvent * e);
        void mouseMoveEvent(QMouseEvent * e);
        void mousePressEvent(QMouseEvent * e);
        void mouseReleaseEvent(QMouseEvent * e);

        /// add "Make Track" and "Edit..." to custom menu
        void customMenu(QMenu& menu);

        /// iterate over all waypoints to get zoom area
        void makeVisible();
        void looseFocus();
        QRectF getBoundingRectF();
        void delPointsByIdx(const QList<int>& idx);

        void save(QDataStream& s);
        void load(QDataStream& s);

        void setWidth(quint32 w){width = w;}
        void setOpacity(quint8 o){opacity = o;}

        signals:
        void sigSelectionChanged();

    private slots:
        void slotShow();
        void slotEdit();

    private:
        friend class COverlayDB;
        friend class COverlayAreaEditWidget;
        void drawDistanceInfo(projXY p1, projXY p2, QPainter& p, IMap& map);
        void calc();

        /// the polyline as list of points [rad]
        QList<pt_t> points;
        /// indices of selected points
        QList<int> selectedPoints;

        QColor color;
        Qt::BrushStyle style;
        quint32 width;
        quint8 opacity;

        /// pointer to point of polyline if cursor is closer than 30px
        pt_t * thePoint;
        pt_t * thePointBefor;
        pt_t * thePointAfter;
        /// need to restore point if move command is aborted
        pt_t savePoint;

        /// rectangle in function wheel for del icon
        QRect rectDel;
        /// rectangle in function wheel for move icon
        QRect rectMove;
        /// rectangle in function wheel for add1 icon
        QRect rectAdd1;
        /// rectangle in function wheel for add2 icon
        QRect rectAdd2;

        /// to show special cursor over function wheel icon
        bool doSpecialCursor;
        /// set true while moving a point
        bool doMove;
        /// set true while showing the function wheel
        bool doFuncWheel;

        enum addType_e{eNone, eBefore, eAfter, eAtEnd};
        ///
        addType_e addType;

        double anglePrev;
        double angleNext;

        /// the complete polyline close to the cursor from a vector map
        QPolygon leadline;
        /// the subline of the leadline between the last point and the cursor
        QPolygon subline;

        bool isEdit;
};
#endif                           // COVERLAYAREA_H
