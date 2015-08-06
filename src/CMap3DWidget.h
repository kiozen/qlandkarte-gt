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
#ifndef CTRACK3DWIDGET_H
#define CTRACK3DWIDGET_H

#include <QGLWidget>
#include <QPointer>
#include <QWidget>
#include <QSet>

#include "CTrack.h"
#include "CMapQMAP.h"
#include "IMap.h"

class CMap3DWidget: public QGLWidget
{
    Q_OBJECT;
    public:
        CMap3DWidget(QWidget *parent);
        virtual ~CMap3DWidget();
        void convertPt23D(double& u, double& v, double &ele);
        void convert3D2Pt(double& u, double& v, double &ele);
        /// conver coord of point a on the window to the flat z = 0
        void convertDsp2Z0(QPoint &a);

    protected:
        QPointer<CTrack> track;
        void initializeGL();
        void paintGL();
        void resizeGL(int width, int height);
        void showEvent ( QShowEvent * event );
        void hideEvent ( QHideEvent * event );
        void mousePressEvent(QMouseEvent *event);
        void mouseMoveEvent(QMouseEvent *event);
        void mouseDoubleClickEvent ( QMouseEvent * event );
        void wheelEvent ( QWheelEvent * e );
        void contextMenuEvent(QContextMenuEvent *event);
        void keyPressEvent ( QKeyEvent * event );
        void keyReleaseEvent ( QKeyEvent * event );
        void focusOutEvent ( QFocusEvent * event );
        void createActions();
        void updateElevationLimits();
        bool getEleRegion(QVector<qint16>& eleData, int& xcount, int& ycount);
        float getRegionValue(float *buffer, int x, int y);
        void enterEvent(QEvent * );
        void leaveEvent(QEvent * );
        void mouseReleaseEvent(QMouseEvent *event);

        QAction *eleZoomInAct;
        QAction *eleZoomOutAct;
        QAction *eleZoomResetAct;
        QAction *lightResetAct;
        QAction *map3DAct;
        QAction *showTrackAct;
        QAction *mapEleAct;
        QSet<int> pressedKeys;

    private:
        unsigned int skyBox[6];
        double step;
        QPointer<IMap> map;
        QSize mapSize;
        /// expand map relative to the center
        void expandMap(bool zoomIn);
        void makeMapObject();
        void makeTrackObject();
        void setMapTexture();
        void quad(GLdouble x1, GLdouble y1, GLdouble z1, GLdouble x2, GLdouble y2, GLdouble z2);
        void normalizeAngle(double *angle);
        void drawFlatMap();
        /// using DEM data file to display terrain in 3D
        void draw3DMap();
        void drawTrack();
        void drawSkybox(double x, double y, double z, double xs, double ys, double zs);
        void drawWpt(CWpt *wpt);
        void getPopint(double v[], int xi, int yi, int xi0, int yi0, int xcount, int ycount, double current_step_x, double current_step_y, qint16 *eleData);
        void convertMouse23D(double &u, double& v, double &ele);

        GLuint objectMap;
        GLuint objectTrack;
        double xRot;
        double zRot;
        double xRotSens;
        double zRotSens;
        float xLight;
        float yLight;
        float zLight;
        bool light;
        bool cursorFocus;
        bool cursorPress;
        bool reDraw;
        bool trackmode;

        QPoint mousePos;

        GLuint mapTexture;
        double xShift;
        double yShift;
        double zShift;
        double zoomFactor;
        double eleZoomFactor;

        double maxElevation, minElevation;

        QPoint lastPos;
        QColor wallCollor;
        QColor highBorderColor;

        QPointer<CWpt> selWpt;

    private slots:
        void slotChanged();
        void slotTrackChanged(bool updateGLFlag = true);
        void loadTrack();
        void loadMap();
        void mapResize(const QSize& size);
        void slotDeleteWpt();
        void slotEditWpt();
        void slotAddWpt();
        void slotCopyPositionWpt();

    public slots:
        void setXRotation(double angle);
        void setZRotation(double angle);
        void eleZoomOut();
        void eleZoomIn();
        void eleZoomReset();
        void lightReset();
        void changeMode();
        void changeTrackmode();
        void lightTurn();
        void slotSaveImage(const QString& filename);

        signals:
        void xRotationChanged(double angle);
        void zRotationChanged(double angle);
        void sigChanged();
        void sigTrackChanged();
};
#endif                           //CTRACK3DWIDGET_H
