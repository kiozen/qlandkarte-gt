/**********************************************************************************************
    Copyright (C) 2006, 2007 Oliver Eichler oliver.eichler@gmx.de

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

#ifndef CPLOT_H
#define CPLOT_H

#include <QWidget>
#include <QPointer>

#include "CPlotData.h"
#include "CTrack.h"

class CPlot : public QWidget
{
    Q_OBJECT
        public:

        enum mode_e {eNormal, eIcon};

        CPlot(CPlotData::axis_type_e type, mode_e mode, QWidget * parent);
        virtual ~CPlot();

        void setShowScale(bool show){showScale = show;}
        void setThinLine(bool thin){thinLine = thin;}
        void setYLabel(const QString& str);
        void setXLabel(const QString& str);

        void setSelTrackPoint(CTrack::pt_t * pt){selTrkPt = pt;}

        void newLine(const QPolygonF& line, const QList<QPointF>& focus, const QString& label);
        void addLine(const QPolygonF& line, const QString& label);
        void newFocus(const QList<QPointF>& focus);
        void newMarks(const QPolygonF& line);
        void addTag(CPlotData::point_t& tag);
        void setLimits();

        void clear();

        double getXValByPixel(int px);
        double getYValByPixel(int px);

        void draw(QPainter& p);
        void draw(QPainter& p, const QSize& s);

        signals:
        void sigActivePoint(double dist);
        void sigSetWaypoint(double dist);
        void sigClicked();

    public slots:
        void slotPointOfFocus(const int idx);
        void slotHighlightSection(double x1, double x2);
        void resetZoom();

    protected slots:
        void slotTrkPt(CTrack::pt_t * pt);
        void slotSave();
        void slotAddWpt();

    protected:
        void draw();
        void contextMenuEvent(QContextMenuEvent *event);
        void mousePressEvent(QMouseEvent * e);
        void mouseMoveEvent(QMouseEvent * e);
        void mouseReleaseEvent(QMouseEvent * e);
        void wheelEvent ( QWheelEvent * e );
        void paintEvent(QPaintEvent * e);
        void resizeEvent(QResizeEvent * e);
        void leaveEvent(QEvent * event);
        void enterEvent(QEvent * event);

        /// draw the actual plot
        void drawLabels(QPainter& p);
        void drawXScale(QPainter& p);
        void drawYScale(QPainter& p);
        void drawXTic(QPainter& p);
        void drawYTic(QPainter& p);
        void drawGridX(QPainter& p);
        void drawGridY(QPainter& p);
        void drawData(QPainter& p);
        void drawLegend(QPainter& p);
        void drawTags(QPainter& p);

        void setSizes();
        void setLRTB();
        void setSizeIconArea();
        void setSizeXLabel();
        void setSizeYLabel();
        void setSizeTrackInfo();
        void setSizeDrawArea();
        void zoom(CPlotAxis &axis, bool in, int cur = 0);

        void createActions();
        QAction *hZoomAct;
        QAction *vZoomAct;
        QAction *resetZoomAct;
        QAction *save;
        QAction *addWpt;

        CPlotData * m_pData;

        ///width of the used font
        int fontWidth;
        ///height of the used font
        int fontHeight;
        ///width of the scale at the bottom of the plot
        int scaleWidthX1;
        ///width of the scale at the left of the plot
        int scaleWidthY1;

        int deadAreaX;
        int deadAreaY;

        int left;
        int right;
        int top;
        int bottom;

        QRect rectX1Label;
        QRect rectY1Label;
        QRect rectGraphArea;
        QRect rectIconArea;
        QRect rectTrackInfo;

        QFontMetrics fm;

        QPoint startMovePos;

        double initialYMax;
        double initialYMin;

        mode_e mode;
        bool showScale;
        bool thinLine;
        bool cursorFocus;
        bool needsRedraw;
        bool mouseMoveMode;
        bool checkClick;

        QPoint posMouse;
        QPoint posWpt;
        QImage buffer;
        CTrack::pt_t * selTrkPt;

        int idxHighlight1;
        int idxHighlight2;
};
#endif                           //CPLOT_H
