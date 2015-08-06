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

#ifndef COVERLAYDISTANCE_H
#define COVERLAYDISTANCE_H

#include "IOverlay.h"
#include "GeoMath.h"

#include <proj_api.h>
#ifdef __MINGW32__
#undef LP
#endif

#include <QPointer>

class COverlayDistanceEditWidget;
class IMap;

/// the one and only edit widget fo rdistance lines
extern QPointer<COverlayDistanceEditWidget> overlayDistanceEditWidget;

class COverlayDistance : public IOverlay
{
    Q_OBJECT;
    public:

        struct pt_t : public projXY
        {
            int idx;
        };

        COverlayDistance(const QString& name, const QString& comment, double speed, const QList<pt_t>& pts, QObject * parent);
        virtual ~COverlayDistance();

        /// returns true while moving a waypoint
        bool mouseActionInProgress(){return doMove;}
        /// returns name, comment and length
        QString getInfo();
        /// returns true if pt is close as 30px to a waypoint
        bool isCloseEnough(const QPoint& pt);

        /// draw the ployline, waypoints and action icons
        void draw(QPainter& p, const QRect& viewport);

        void keyPressEvent(QKeyEvent * e);
        void mouseMoveEvent(QMouseEvent * e);
        void mousePressEvent(QMouseEvent * e);
        void mouseReleaseEvent(QMouseEvent * e);

        /// get last point of polyline
        projXY getLast(){return points.last();}

        /// add "Make Track" and "Edit..." to custom menu
        void customMenu(QMenu& menu);

        void save(QDataStream& s);
        void load(QDataStream& s);

        /// iterate over all waypoints to get zoom area
        void makeVisible();

        void looseFocus();

        QRectF getBoundingRectF();

        void delPointsByIdx(const QList<int>& idx);

        const QList<pt_t>& getPoints(){return points;}

        static void setShowBullets(bool show){showBullets = show;}
        static bool getShowBullets(){return showBullets;}
        signals:
        void sigSelectionChanged();

    private slots:
        void slotToTrack();
        void slotToRoute();
        void slotEdit();
        void slotRevert();
        void slotShow();
        void slotShowBullets();

    private:
        friend class COverlayDB;
        friend class COverlayDistanceEditWidget;

        void calcDistance();
        void drawArrows(const QPolygon& line, const QRect& viewport, QPainter& p);
        void drawDistanceInfo(projXY p1, projXY p2, QPainter& p, IMap& map);

        /// the polyline as list of points [rad]
        QList<pt_t> points;
        /// indices of selected points
        QList<int> selectedPoints;
        /// pointer to point of polyline if cursor is closer than 30px
        pt_t * thePoint;
        pt_t * thePointBefor;
        pt_t * thePointAfter;
        /// need to restore point if move command is aborted
        pt_t savePoint;

        /// the distance in [m]
        double distance;
        /// optional speed in km/s, <=0 is no speed
        float speed;

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

        static bool showBullets;
};

#define OVL_NOFLOAT 1e25f
#endif                           //COVERLAYDISTANCE_H
