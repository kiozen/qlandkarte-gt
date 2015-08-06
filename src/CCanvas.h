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

#ifndef CCANVAS_H
#define CCANVAS_H

#include "IMap.h"

#include <QWidget>
#include <QPointer>
#include <QPainter>

class IMouse;
class CMouseMoveMap;
class CMouseZoomMap;
class CMouseSelMap;
class CMouseAddWpt;
class CMouseMoveWpt;
class CMouseEditWpt;
class CMouseRefPoint;
class CMouseAddText;
class CMouseAddTextBox;
class CMouseAddDistance;
class CMouseAddArea;
class CMouseOverlay;
class CMouseColorPicker;
class CMouseSelWpt;
class CWpt;
class QLabel;
class QSize;
class QPrinter;
class QMenu;
class CMouseCutTrack;
class CMouseSelTrack;
class QFont;
class CPlot;
class CTrack;
class CMapDEMSlopeSetup;

#define PAINT_ROUNDED_RECT(p,r) p.drawRoundedRect(r,5,5)
#define COMPASS_H 60
#define COMPASS_W 30
#define COMPASS_OFFSET_X 50
#define COMPASS_OFFSET_Y 100

inline  void USE_ANTI_ALIASING(QPainter& p, bool useAntiAliasing)
{
    p.setRenderHints(QPainter::TextAntialiasing|QPainter::Antialiasing|QPainter::SmoothPixmapTransform|QPainter::HighQualityAntialiasing, useAntiAliasing);
}


/// the map canvas area
class CCanvas : public QWidget
{
    Q_OBJECT;
    public:

        enum move_direction_e {eMoveLeft, eMoveRight, eMoveUp, eMoveDown, eMoveLeftSmall, eMoveRightSmall, eMoveUpSmall, eMoveDownSmall, eMoveCenter};

        CCanvas(QWidget * parent);
        virtual ~CCanvas();

        void setupDelayed();

        enum mouse_mode_e
        {
            eMouseZoomArea       ///< use mouse to define a zoom area
            , eMouseMoveArea     ///< use mouse to move the map
            , eMouseSelectArea   ///< use mouse to select map tiles
            , eMouseAddWpt       ///< use mouse to add waypoints
            , eMouseEditWpt      ///< use mouse to select waypoints
            , eMouseMoveWpt      ///< use mouse to drag-n-drop waypoints
            , eMouseMoveRefPoint ///< use mouse to drag-n-drop reference points
            , eMouseCutTrack     ///< use mouse to cut a track into two pieces
            , eMouseSelTrack     ///< use mouse to select points of a track by a rectangle
            , eMouseAddText      ///< use mouse to define a new text field on the map
            , eMouseAddTextBox   ///< use mouse to define a new text field with anchor on the map
            , eMouseAddDistance  ///< use mouse to define a new distance polygon
            , eMouseAddArea      ///< use mouse to define a new area polygon
            , eMouseOverlay      ///< use mouse to change overlays
            , eMouseColorPicker  ///< use mouse to pick a color from map
            , eMouseSelWpt       ///< use mouse to select waypoints in a radius
        };

        /// zoom in/out with a given point as static
        void zoom(bool in, const QPoint& p);

        /// scroll map into given direction
        void move(move_direction_e dir);

        /// center to lon/lat coordinate
        /**
            @param lon the logitude in  []
            @param lat the logitude in  []
        */
        void move(double lon, double lat);

        void print(QPrinter& printer);

        void print(QImage& img);

        void print(QPainter& p, const QSize& pagesize);
        void print(QImage& img, const QSize& pagesize);

        /// change the current mouse mode
        void setMouseMode(mouse_mode_e mode);

        /// draw text with white border
        static void drawText(const QString& str, QPainter& p, const QPoint& center, const QColor& color = Qt::darkBlue);
        static void drawText(const QString& str, QPainter& p, const QPoint& center, const QColor& color, const QFont& font);
        static void drawText(const QString& str, QPainter& p, const QRect& r, const QColor& color = Qt::darkBlue);

        void raiseContextMenu(const QPoint& pos);

        /// get selected color from color picker cursor
        QColor getSelectedColor();

        static QPen penBorderBlue;
        static QPen penBorderBlack;
        static QBrush brushBackWhite;
        static QBrush brushBackYellow;

        void setFadingMessage(const QString& msg);

        signals:
        void sigResize(const QSize& size);

    public slots:
        void slotMapChanged();

    private slots:
        void slotCopyPosition();
        void slotHighlightTrack(CTrack * track);
        void slotTrackChanged();
        void slotPointOfFocus(const int idx);
        void slotProfileChanged();
        void slotFadingMessage();
        void slotTime();

    protected:
        void paintEvent(QPaintEvent * e);
        void resizeEvent(QResizeEvent * e);
        void mouseMoveEvent(QMouseEvent * e);
        void mousePressEvent(QMouseEvent * e);
        void mouseReleaseEvent(QMouseEvent * e);
        void mouseDoubleClickEvent(QMouseEvent * e);
        void wheelEvent(QWheelEvent * e);
        void keyPressEvent(QKeyEvent *);
        void keyReleaseEvent(QKeyEvent *);
        void showEvent ( QShowEvent * event);

        /// set override cursor
        void enterEvent(QEvent * event);
        /// restore cursor
        void leaveEvent(QEvent * event);

        void draw(QPainter& p);
        void drawRefPoints(QPainter& p);
        void drawScale(QPainter& p);
        void drawCompass(QPainter& p);
        void drawClock(QPainter& p);
        void drawFadingMessage(QPainter& p);
        void drawTrackLegend(QPainter& p);

    private:
        friend class CStatusCanvas;

        void mouseMoveEventCoord(QMouseEvent * e);

        QPointer<IMouse>  mouse;
        CMouseMoveMap * mouseMoveMap;
        CMouseZoomMap * mouseZoomMap;
        CMouseSelMap * mouseSelMap;
        CMouseAddWpt * mouseAddWpt;
        CMouseMoveWpt * mouseMoveWpt;
        CMouseEditWpt * mouseEditWpt;
        CMouseRefPoint * mouseRefPoint;
        CMouseCutTrack * mouseCutTrack;
        CMouseSelTrack * mouseSelTrack;
        CMouseAddText * mouseAddText;
        CMouseAddTextBox * mouseAddTextBox;
        CMouseAddDistance * mouseAddDistance;
        CMouseAddArea * mouseAddArea;
        CMouseOverlay * mouseOverlay;
        CMouseColorPicker * mouseColorPicker;
        CMouseSelWpt * mouseSelWpt;

        /// current mouse mode
        mouse_mode_e mouseMode;

        /// current mouse position
        QPoint posMouse;
        QLabel * info;
        CPlot * profile;
        CMapDEMSlopeSetup * slopeSetup;

        QTimer * timerFadingMessage;
        QString fadingMessage;

        // work around possible Qt/OS X bug
        bool contextMenuActive;

        QString timezone;
        QTimer * timerClock;
};
#endif                           //CCANVAS_H
