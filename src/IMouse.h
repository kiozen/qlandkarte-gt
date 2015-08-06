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

#ifndef IMOUSE_H
#define IMOUSE_H

#include <QCursor>
#include <QObject>
#include <QRect>
#include <QPointer>

#include "CTrack.h"
#include "CRoute.h"
class QMouseEvent;
class QKeyEvent;
class QMenu;
class CCanvas;
class CWpt;
class IOverlay;
class CSearch;
class IMapSelection;

/// Base class to all mouse function objects
/**
    The function of the mouse changes depending on the mega menu selection.
    All mouse events will be forwared to a subclass of IMouse. The subclass
    will define the current function and it keeps track of all runtime
    variables.
*/
class IMouse : public QObject
{
    Q_OBJECT;
    public:
        IMouse(CCanvas * parent);
        virtual ~IMouse();

        /// the mouse move event as defined by QWidget::mouseMoveEvent
        virtual void mouseMoveEvent(QMouseEvent * e) = 0;
        /// the mouse press event as defined by QWidget::mousePressEvent
        virtual void mousePressEvent(QMouseEvent * e) = 0;
        /// the mouse release event as defined by QWidget::mouseReleaseEvent
        virtual void mouseReleaseEvent(QMouseEvent * e) = 0;
        /// the mouse double click event as defined by QWidget::mouseDoubleClickEvent
        virtual void mouseDoubleClickEvent(QMouseEvent * e){}

        /// the key press event as defined by QWidget::keyPressEvent
        virtual void keyPressEvent(QKeyEvent *) {}
        /// the key release event as defined by QWidget::keyPressEvent
        virtual void keyReleaseEvent(QKeyEvent *) {}

        /// the current mouse cursor
        /**
            Each mouse function is represented by a special cursor. The main
            widget uses this method to query the current cursor.
        */
        operator const QCursor&(){return cursor;}

        /// draw mouse function spezific elements
        /**
            Some actions demand additional graphical elements to represent the
            state of the acquired runtime variables. E.g. capture rectangle.
            This is the place to draw them.
        */
        virtual void draw(QPainter& ){}

        /// append a context menu by own actions
        virtual void contextMenu(QMenu& ){}

        /// called by CCanvas right befor a new mouse handler is selected.
        virtual void looseFocus(){}

        static void resetPos1(){pos1Pixel = pos1LonLat = QPointF(-1,-1);}

        void setSelTrackPt(CTrack::pt_t * pt);

#ifdef GPX_EXTENSIONS
        CGpxExtTr tr_ext;
#endif

    public slots:
        void slotMapChanged();

    protected slots:
        void slotSetPos1();

    protected:
        enum features_e
        {
            eFeatInfoBox = 0x00000001
            ,eFeatArrow   = 0x00000002
            ,eFeatWheel   = 0x00000004
            ,eFeatName    = 0x00000008
            ,eFeatAll     = 0xFFFFFFFF
        };

        struct wpt_t
        {
            wpt_t(): x(0), y(0), order(0x7FFFFFFF), dist(0x7FFFFFFF){}

            bool operator<(const wpt_t& w) const
            {
                if(order == w.order)
                {
                    return dist < w.dist;
                }
                return order < w.order;
            }

            QPointer<CWpt> wpt;
            int x;
            int y;
            int xReal;
            int yReal;
            int order;
            int dist;
        };

        /// for internal use to start a semi-transparent capture rectangle
        void startRect(const QPoint& p);
        /// for internal use to set the bottom right of the capture rectangle
        void resizeRect(const QPoint& p, bool normalize = false);
        /// actually draw the current capture rectangle
        void drawRect(QPainter& p);
        /// draw selected waypoint
        void drawSelWpt(QPainter& p);
        void drawSelWpt(QPainter& p, wpt_t& wptInfo, quint32 features);

        /// draw selected track point
        void drawSelTrkPt(QPainter& p);
        /// draw selected route point
        void drawSelRtePt(QPainter& p);
        /// draw selected search
        void drawSelSearch(QPainter& p);

        void drawPos1(QPainter& p);

        /// choose waypoint close to cursor
        void mouseMoveEventWpt(QMouseEvent * e);
        /// choose track point close to cursor
        void mouseMoveEventTrack(QMouseEvent * e);
        /// choose routek point close to cursor
        void mouseMoveEventRoute(QMouseEvent * e);
        /// choose overlay under cursor
        void mouseMoveEventOverlay(QMouseEvent * e);
        /// choose waypoint close to cursor
        void mouseMoveEventSearch(QMouseEvent * e);
        ///
        void mouseMoveEventMapSel(QMouseEvent * e);

        void sortSelWpts(QList<wpt_t>& list);

        void clearSelWpts();

        /// trigger waypoint function
        void mousePressEventWpt(QMouseEvent * e);
        /// trigger search function
        void mousePressEventSearch(QMouseEvent * e);
        /// the functions mouse icon
        QCursor cursor;
        /// pointer to the parent canvas
        CCanvas * canvas;
        /// capture rectangle
        QRect rect;

        /// current selected waypoint
        QList<wpt_t> selWpts;

        /// current selected trackpoint
        CTrack::pt_t * selTrkPt;
        /// current selected routepoint
        CRoute::pt_t * selRtePt;
        /// current selected overlay
        static QPointer<IOverlay> selOverlay;
        /// current selected map area
        QPointer<IMapSelection>selMap;

        QPointer<CSearch> selSearch;

        QRect rectIcon;
        QRect rectMarkWpt;
        QRect rectDelWpt;
        QRect rectMoveWpt;
        QRect rectEditWpt;
        QRect rectCopyWpt;
        QRect rectViewWpt;

        QRect rectDelSearch;
        QRect rectConvertSearch;
        QRect rectCopySearch;

        bool doSpecialCursorWpt;
        bool doSpecialCursorSearch;
        bool doShowWptBuddies;
        bool lockWptCircles;

        QPoint lastPoint;
        QPoint mousePos;
        QPoint mousePosWptCircle;

        static QPointF pos1Pixel;
        static QPointF pos1LonLat;
};
#endif                           //IMOUSE_H
