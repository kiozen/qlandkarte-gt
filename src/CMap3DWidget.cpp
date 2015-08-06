/**********************************************************************************************
    Copyright (C) 2008 Andrew Vagin <avagin@gmail.com>

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
#include "CMap3DWidget.h"
#include "CTrackDB.h"
#include "CTrack.h"
#include "CWptDB.h"
#include "WptIcons.h"
#include "CMapQMAP.h"
#include "IUnit.h"
#include "IMap.h"
#include "CMapDB.h"
#include "CResources.h"
#include "CMainWindow.h"
#include "CDlgEditWpt.h"
#include "GeoMath.h"

#include <QtGui>
#include <QtOpenGL>
#include <QPixmap>
#include <QPainter>
#include <QGLPixelBuffer>

#include <assert.h>
#include <math.h>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

static void glError()
{
    GLenum err = glGetError();
    while (err != GL_NO_ERROR)
    {
        qDebug("glError: %s caught at %s:%u\n",
            (char *)gluErrorString(err), __FILE__, __LINE__);
        err = glGetError();
    }
}


CMap3DWidget::CMap3DWidget(QWidget * parent)
: QGLWidget(parent)
{
    QSettings cfg;
    xLight = cfg.value("map/3D/xLight", 0.0).toDouble();
    yLight = cfg.value("map/3D/yLight", 0.0).toDouble();
    zLight = cfg.value("map/3D/zLight", 5000.0).toDouble();

    xRot = 45;
    zRot = 0;
    xRotSens = 0.3;
    zRotSens = 0.3;
    step = 5;

    xShift = 0;
    yShift = 0;
    zShift = 0;
    zoomFactor = 1;

    eleZoomFactor = cfg.value("map/3D/eleZoomFactor", 1).toDouble();
    maxElevation = 0;
    minElevation = 0;

    wallCollor = QColor::fromCmykF(0.40, 0.0, 1.0, 0);
    highBorderColor = QColor::fromRgbF(0.0, 0.0, 1.0, 0);

    createActions();
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    map3DAct->setChecked(cfg.value("map/3D/3dmap", true).toBool());
    mapEleAct->setChecked(cfg.value("map/3D/trackonmap", false).toBool());
    light = cfg.value("map/3D/light", false).toBool();
    cursorFocus = false;
    cursorPress = false;
    reDraw = false;
    trackmode = false;
}


CMap3DWidget::~CMap3DWidget()
{
    int i;
    makeCurrent();
    deleteTexture(mapTexture);
    for (i = 0; i < 6; i++)
        deleteTexture(skyBox[i]);

    glDeleteLists(objectMap, 1);
    glDeleteLists(objectTrack, 1);

    QSettings cfg;
    cfg.setValue("map/3D/3dmap", map3DAct->isChecked());
    cfg.setValue("map/3D/trackonmap", mapEleAct->isChecked());
    cfg.setValue("map/3D/eleZoomFactor", eleZoomFactor);
    cfg.setValue("map/3D/light", light);
    cfg.setValue("map/3D/xLight", xLight);
    cfg.setValue("map/3D/yLight", yLight);
    cfg.setValue("map/3D/zLight", zLight);
}


void CMap3DWidget::mapResize(const QSize& size)
{
    mapSize = size;
}


void CMap3DWidget::loadMap()
{
    if (! map.isNull())
    {
        disconnect(map, SIGNAL(destroyed()), this, SLOT(deleteLater()));
        disconnect(map, SIGNAL(sigChanged()), this, SLOT(slotChanged()));
        disconnect(map, SIGNAL(sigResize(const QSize&)), this, SLOT(mapResize(const QSize&)));
    }
    map = &CMapDB::self().getMap();
    assert(!map.isNull());
    // map should be square
    mapSize = map->getSize();
    if (mapSize.width() != mapSize.height())
    {
        int side = qMax(mapSize.width(), mapSize.height());
        map->resize(QSize(side, side));
        mapSize = map->getSize();
    }
    connect(map, SIGNAL(destroyed()), this, SLOT(deleteLater()));
    connect(map, SIGNAL(sigChanged()),this,SLOT(slotChanged()));
    connect(map, SIGNAL(sigResize(const QSize&)),this,SLOT(mapResize(const QSize&)));
    emit sigChanged();
}


void CMap3DWidget::loadTrack()
{
    if (!track.isNull())
    {
        disconnect(track, SIGNAL(sigChanged()), this, SLOT(slotTrackChanged()));
    }
    track = CTrackDB::self().highlightedTrack();
    if (!track.isNull())
    {
        connect(track, SIGNAL(sigChanged()), this, SLOT(slotTrackChanged()));
    }
    emit sigTrackChanged();
}


void CMap3DWidget::createActions()
{
    map3DAct = new QAction(tr("Flat / 3D Mode"), this);
    map3DAct->setCheckable(true);
    map3DAct->setChecked(true);
    connect(map3DAct, SIGNAL(triggered()), this, SLOT(slotChanged()));

    showTrackAct = new QAction(tr("Show Track"), this);
    showTrackAct->setCheckable(true);
    showTrackAct->setChecked(true);
    connect(showTrackAct, SIGNAL(triggered()), this, SLOT(slotTrackChanged()));

    mapEleAct = new QAction(tr("Track on Map"), this);
    mapEleAct->setCheckable(true);
    mapEleAct->setChecked(false);
    connect(mapEleAct, SIGNAL(triggered()), this, SLOT(slotTrackChanged()));

    eleZoomInAct = new QAction(tr("Inc. Elevation"), this);
    eleZoomInAct->setIcon(QIcon(":/icons/iconInc16x16"));
    connect(eleZoomInAct, SIGNAL(triggered()), this, SLOT(eleZoomIn()));
    eleZoomOutAct = new QAction(tr("Dec. Elevation"), this);
    eleZoomOutAct->setIcon(QIcon(":/icons/iconDec16x16"));
    connect(eleZoomOutAct, SIGNAL(triggered()), this, SLOT(eleZoomOut()));
    eleZoomResetAct = new QAction(tr("Reset Elevation"), this);
    eleZoomResetAct->setIcon(QIcon(":/icons/iconClear16x16"));
    connect(eleZoomResetAct, SIGNAL(triggered()), this, SLOT(eleZoomReset()));
    lightResetAct = new QAction(tr("Reset light source"), this);
    lightResetAct->setIcon(QIcon(":/icons/iconClear16x16"));
    connect(lightResetAct, SIGNAL(triggered()), this, SLOT(lightReset()));

}


void CMap3DWidget::changeMode()
{
    map3DAct->setChecked(!map3DAct->isChecked());
    slotChanged();
}


void CMap3DWidget::changeTrackmode()
{
    if(track.isNull())
    {
        return;
    }

    double ele1;
    XY pt1;
    IMap& dem = CMapDB::self().getDEM();
    CTrack::pt_t trkpt = track->getTrackPoints().first();

    pt1.u = trkpt.lon * DEG_TO_RAD;
    pt1.v = trkpt.lat * DEG_TO_RAD;
    if (mapEleAct->isChecked())
    {
        ele1 = dem.getElevation(pt1.u, pt1.v) + 1;
    }
    else
    {
        ele1 = trkpt.ele;
    }

    map->convertRad2Pt(pt1.u, pt1.v);
    convertPt23D(pt1.u, pt1.v, ele1);

    qDebug() << pt1.u << pt1.v << ele1;

    xShift = - pt1.u;
    yShift = - pt1.v;
    zShift = - ele1;

    zoomFactor = 1.0;
    updateGL();
}


void CMap3DWidget::eleZoomOut()
{
    eleZoomFactor = eleZoomFactor / 1.2;
    updateGL();
}


void CMap3DWidget::eleZoomIn()
{
    eleZoomFactor = eleZoomFactor * 1.2;
    updateGL();
}


void CMap3DWidget::eleZoomReset()
{
    eleZoomFactor = 1;
    updateGL();
}


void CMap3DWidget::lightReset()
{
    xLight = 0;
    yLight = 0;
    zLight = 5000;
    updateGL();
}


void CMap3DWidget::showEvent ( QShowEvent * event )
{
    //restore size
    map->resize(mapSize);

    qDebug() << "show event";
    connect(map, SIGNAL(sigChanged()),this,SLOT(slotChanged()));
    connect(map, SIGNAL(sigResize(const QSize&)), this, SLOT(mapResize(const QSize&)));
    connect(&CTrackDB::self(), SIGNAL(sigHighlightTrack(CTrack *)), this, SLOT(loadTrack()));
    connect(this, SIGNAL(sigChanged()), this, SLOT(slotChanged()));
    connect(this, SIGNAL(sigTrackChanged()), this, SLOT(slotTrackChanged()));
    reDraw = true;
}


void CMap3DWidget::hideEvent ( QHideEvent * event )
{
    qDebug() << "hide event";
    disconnect(map, SIGNAL(sigChanged()),this,SLOT(slotChanged()));
    disconnect(map, SIGNAL(sigResize(const QSize&)), this, SLOT(mapResize(const QSize&)));
    disconnect(&CTrackDB::self(), SIGNAL(sigHighlightTrack(CTrack *)), this, SLOT(loadTrack()));
    disconnect(this, SIGNAL(sigChanged()), this, SLOT(slotChanged()));
    disconnect(this, SIGNAL(sigTrackChanged()), this, SLOT(slotTrackChanged()));
}


void CMap3DWidget::contextMenuEvent(QContextMenuEvent *event)
{
    double x, y, z;

    x = mousePos.x();
    y = mousePos.y();
    convertMouse23D(x, y, z);
    convert3D2Pt(x, y, z);

    selWpt = 0;
    // find the waypoint close to the cursor
    QMap<QString,CWpt*>::const_iterator wpt = CWptDB::self().begin();
    while(wpt != CWptDB::self().end())
    {
        double u = (*wpt)->lon * DEG_TO_RAD;
        double v = (*wpt)->lat * DEG_TO_RAD;
        map->convertRad2Pt(u,v);

        if(((x - u) * (x - u) + (y - v) * (y - v)) < 1225)
        {
            selWpt = *wpt;
            break;
        }

        ++wpt;
    }

    QMenu menu(this);
    if (selWpt.isNull())
    {
        menu.addAction(QPixmap(":/icons/iconAdd16x16.png"),tr("Add Waypoint ..."),this,SLOT(slotAddWpt()));
    }
    else
    {
        menu.addAction(QPixmap(":/icons/iconClipboard16x16.png"),tr("Copy Pos. Waypoint"),this,SLOT(slotCopyPositionWpt()));
        menu.addAction(QPixmap(":/icons/iconEdit16x16.png"),tr("Edit Waypoint..."),this,SLOT(slotEditWpt()));
        if(!selWpt->sticky)
            menu.addAction(QPixmap(":/icons/iconClear16x16.png"),tr("Delete Waypoint"),this,SLOT(slotDeleteWpt()));
    }

    menu.addSeparator();

    menu.addAction(eleZoomInAct);
    menu.addAction(eleZoomOutAct);
    menu.addAction(eleZoomResetAct);
    menu.addAction(lightResetAct);
    menu.addAction(map3DAct);
    menu.addAction(showTrackAct);
    menu.addAction(mapEleAct);

    if(track.isNull())
    {
        showTrackAct->setEnabled(false);
        mapEleAct->setEnabled(false);
    }
    else
    {
        showTrackAct->setEnabled(true);
        mapEleAct->setEnabled(true);
    }

    menu.exec(event->globalPos());
}


void CMap3DWidget::setMapTexture()
{
    QPixmap pm(mapSize.width(), mapSize.height());
    QPainter p(&pm);
    p.eraseRect(pm.rect());
    map->draw(p);
    mapTexture = bindTexture(pm, GL_TEXTURE_2D);
}


void CMap3DWidget::slotTrackChanged(bool updateGLFlag)
{
    makeTrackObject();
    if (updateGLFlag)
        updateGL();
}


void CMap3DWidget::slotChanged()
{
    reDraw = true;
    updateGL();
}


void CMap3DWidget::convertPt23D(double& u, double& v, double &ele)
{
    u = u - mapSize.width()/2;
    v = mapSize.height()/2 - v;
}


void CMap3DWidget::convert3D2Pt(double& u, double& v, double &ele)
{
    u = u + mapSize.width()/2;
    v = mapSize.height()/2 - v;
}


void CMap3DWidget::convertMouse23D(double &u, double& v, double &ele)
{
    GLdouble projection[16];
    GLdouble modelview[16];
    GLdouble gl_x0, gl_y0, gl_z0;
    GLsizei vx, vy;
    GLfloat depth;
    GLint viewport[4];
    vx = u;
    vy = height() - v;
    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);

    glReadPixels(vx, vy, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
    gluUnProject(vx, vy, depth, modelview, projection, viewport, &gl_x0, &gl_y0, &gl_z0);
    u = gl_x0;
    v = gl_y0;
    ele = gl_z0;
}


void CMap3DWidget::drawFlatMap()
{
    double w = mapSize.width();
    double h = mapSize.height();

    glEnable(GL_NORMALIZE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, mapTexture);
    glBegin(GL_QUADS);
    glTexCoord2d(0.0, 0.0);
    glVertex3d(-w/2, -h/2, minElevation);
    glNormal3d(0.0, 0.0, -1.0);
    glTexCoord2d(1.0, 0.0);
    glVertex3d( w/2, -h/2, minElevation);
    glNormal3d(0.0, 0.0, -1.0);
    glTexCoord2d(1.0, 1.0);
    glVertex3d( w/2,  h/2, minElevation);
    glNormal3d(0.0, 0.0, -1.0);
    glTexCoord2d(0.0, 1.0);
    glVertex3d(-w/2,  h/2, minElevation);
    glNormal3d(0.0, 0.0, -1.0);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glError();
}


bool CMap3DWidget::getEleRegion(QVector<qint16>& eleData, int& xcount, int& ycount)
{
    double w = mapSize.width();
    double h = mapSize.height();

    IMap& dem = CMapDB::self().getDEM();
    XY p1, p2;
    p1.u = 0;
    p1.v = 0;
    p2.u = w;
    p2.v = h;
    map->convertPt2Rad(p1.u, p1.v);
    map->convertPt2Rad(p2.u, p2.v);

    return dem.getOrigRegion(eleData.data(), p1, p2, xcount, ycount);

}


float CMap3DWidget::getRegionValue(float *buffer, int x, int y)
{
    int w = mapSize.width() / step + 1;
    return buffer[x + y * w];
}


void CMap3DWidget::drawSkybox(double x, double y, double z, double xs, double ys, double zs)
{
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

    glEnable(GL_TEXTURE_2D);

    glColor4f(1.0, 1.0, 0.0,1.0f);

    // Save Current Matrix
    glPushMatrix();

    // Second Move the render space to the correct position (Translate)
    glTranslatef(x, y, z);

    // First apply scale matrix
    glScalef(xs, ys, zs);

    float f = 1;
    float r = 1.005f;            // If you have border issues change this to 1.005f
    glBindTexture(GL_TEXTURE_2D,skyBox[0]);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 1); glVertex3f( r/f, 1.0f/f, r/f);
    glTexCoord2f(1, 1); glVertex3f(-r/f, 1.0f/f, r/f);
    glTexCoord2f(1, 0); glVertex3f(-r/f, 1.0f/f,-r/f);
    glTexCoord2f(0, 0); glVertex3f( r/f, 1.0f/f,-r/f);
    glEnd();

    glBindTexture(GL_TEXTURE_2D,skyBox[1]);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 1); glVertex3f(-1.0f/f,  r/f, r/f);
    glTexCoord2f(1, 1); glVertex3f(-1.0f/f, -r/f, r/f);
    glTexCoord2f(1, 0); glVertex3f(-1.0f/f, -r/f,-r/f);
    glTexCoord2f(0, 0); glVertex3f(-1.0f/f,  r/f,-r/f);
    glEnd();

    glBindTexture(GL_TEXTURE_2D,skyBox[2]);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 1); glVertex3f(-r/f, -1.0f/f,  r/f);
    glTexCoord2f(1, 1); glVertex3f( r/f, -1.0f/f,  r/f);
    glTexCoord2f(1, 0); glVertex3f( r/f, -1.0f/f, -r/f);
    glTexCoord2f(0, 0); glVertex3f(-r/f, -1.0f/f, -r/f);
    glEnd();

    glBindTexture(GL_TEXTURE_2D,skyBox[3]);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 1); glVertex3f(1.0f/f, -r/f, r/f);
    glTexCoord2f(1, 1); glVertex3f(1.0f/f,  r/f, r/f);
    glTexCoord2f(1, 0); glVertex3f(1.0f/f,  r/f,-r/f);
    glTexCoord2f(0, 0); glVertex3f(1.0f/f, -r/f,-r/f);
    glEnd();

    glBindTexture(GL_TEXTURE_2D,skyBox[4]);
    glBegin(GL_QUADS);
    glTexCoord2f(1, 1); glVertex3f( r/f, r/f, 1.0f/f);
    glTexCoord2f(1, 0); glVertex3f( r/f,-r/f, 1.0f/f);
    glTexCoord2f(0, 0); glVertex3f(-r/f,-r/f, 1.0f/f);
    glTexCoord2f(0, 1); glVertex3f(-r/f, r/f, 1.0f/f);
    glEnd();

    glBindTexture(GL_TEXTURE_2D,skyBox[5]);
    glBegin(GL_QUADS);
    glTexCoord2f(1, 0); glVertex3f( r/f, r/f, -1.0f/f);
    glTexCoord2f(0, 0); glVertex3f(-r/f, r/f, -1.0f/f);
    glTexCoord2f(0, 1); glVertex3f(-r/f,-r/f, -1.0f/f);
    glTexCoord2f(1, 1); glVertex3f( r/f,-r/f, -1.0f/f);
    glEnd();

    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
    glError();
}


inline void getNormal(GLdouble *a, GLdouble *b, GLdouble *c, GLdouble *r)
{
    GLdouble v1[3], v2[3];
    int i;
    for (i = 0; i < 3; i++)
    {
        v1[i] = c[i] - a[i];
        v2[i] = c[i] - b[i];
    }
    for (i =0; i < 3; i++)
    {
        r[i] = v1[(i + 1) % 3] * v2[(i + 2) % 3] - v1[(i + 2) % 3] * v2[(i + 1) % 3];
    }
}


void CMap3DWidget::getPopint(double v[], int xi, int yi, int xi0, int yi0, int xcount, int ycount, double current_step_x, double current_step_y, qint16 *eleData)
{
    if (xi < 0)
        xi = 0;
    if (yi <0)
        yi = 0;
    if (xi >= xcount)
        xi = xcount - 1;
    if (yi >= ycount)
        yi = ycount - 1;
    v[0] = xi * current_step_x;
    v[1] = yi * current_step_y;
    v[2] = eleData[xi + yi * xcount];
    convertPt23D(v[0], v[1], v[2]);
}


void CMap3DWidget::draw3DMap()
{
    double w = mapSize.width();
    double h = mapSize.height();
    int xcount, ycount;
    // increment xcount, because the number of points are on one more
    // than number of lengths |--|--|--|--|
    if (map->getFastDrawFlag())
    {
        xcount = (w / (step * 10.0) + 1);
        ycount = (h / (step * 10.0) + 1);

    }
    else
    {
        xcount = (w / step + 1);
        ycount = (h / step + 1);
    }

    QVector<qint16> eleData(xcount*ycount);

    bool ok = getEleRegion(eleData, xcount, ycount);
    if (!ok)
    {
        qDebug() << "can't get elevation data";
        qDebug() << "draw flat map";
        drawFlatMap();
        return;
    }

    double current_step_x = w / (double) (xcount - 1);
    double current_step_y = h / (double) (ycount - 1);

    int ix=0, iy, iv, it, j, k, end;
    double x, y, u, v;
    GLdouble *vertices;
    GLdouble *texCoords;
    GLdouble *normals;
    GLuint idx[4];

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnable(GL_NORMALIZE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, mapTexture);

    /*
     * next code can be more optimal if used array of coordinates or VBO
     */
    vertices = new GLdouble[xcount * 3 * 2];
    normals = new GLdouble[xcount * 3 * 2];
    texCoords = new GLdouble[xcount * 2 * 2];
    it = 0;
    iv = 0;
    idx[0] = 0 + xcount;
    idx[1] = 1 + xcount;
    idx[2] = 1;
    idx[3] = 0;
    glVertexPointer(3, GL_DOUBLE, 0, vertices);
    glTexCoordPointer(2, GL_DOUBLE, 0, texCoords);
    glNormalPointer(GL_DOUBLE, 0, normals);
    glColor3f(1.0, 0.0, 0.0);
    glPointSize(2.0);

    for (iy = 0, y = 0; iy < ycount; y += current_step_y, iy++)
    {
        /* array vertices contain two lines ( previouse and current)
         * they change position that avoid memcopy
         * one time current line is at the begin of array vertices,
         * than at the end and etc
         */
        iv = iv % (xcount * 3 * 2);
        it = it % (xcount * 2 * 2);
        end = ix + xcount;
        for (x = 0, ix = 0; ix < xcount; x += current_step_x, iv += 3, it += 2, ix++)
        {
            vertices[iv + 0] = x;
            vertices[iv + 1] = y;
            u = x;
            v = y;
            texCoords[it  + 0] = u / w;
            texCoords[it + 1] = 1 - v / h;
            vertices[iv + 2] = eleData[ix + iy * xcount];
            convertPt23D(vertices[iv + 0], vertices[iv + 1], vertices[iv + 2]);
            GLdouble a[3],b[3],c[3];
            int s = 3;
            a[0] = ix - s;
            a[1] = iy - s;
            b[0] = ix + s;
            b[1] = iy + s;
            c[0] = ix - s;
            c[1] = iy + s;
            getPopint(a, ix + s, iy + s , ix, iy, xcount, ycount, current_step_x, current_step_y, eleData.data());
            getPopint(b, ix - s, iy - s, ix, iy, xcount, ycount, current_step_x, current_step_y, eleData.data());
            getPopint(c, ix + s, iy - s, ix, iy, xcount, ycount, current_step_x, current_step_y, eleData.data());
            getNormal(a, b, c, &normals[iv]);
        }

        for (j = 0; j < 4; j++)
            idx[j] = idx[j] % (xcount * 2);

        if (iy == 0)
            continue;

        for (k = 0; k < xcount - 1; k ++)
        {
            glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, idx);
            for (j = 0; j < 4; j++)
                idx[j]++;
        }
        for (j = 0; j < 4; j++)
            idx[j]++;
    }
    delete [] vertices;
    delete [] texCoords;
    delete [] normals;
    glDisable(GL_TEXTURE_2D);
    glError();
}


void CMap3DWidget::updateElevationLimits()
{
    double ele;
    int i, j;
    double w = mapSize.width();
    double h = mapSize.height();

    // increment xcount, because the number of points are on one more
    // than number of lengths |--|--|--|--|
    int xcount = (w / step + 1);
    int ycount = (h / step + 1);

    QVector<qint16> eleData(xcount*ycount);

    bool ok = getEleRegion(eleData, xcount, ycount);
    minElevation = maxElevation = 0;
    if (ok)
    {
        minElevation = maxElevation = eleData[0];

        for (i = 0; i < xcount; i++)
        {
            for (j = 0; j < ycount; j++)
            {
                ele = eleData[i + j * xcount];
                if (ele > maxElevation)
                    maxElevation = ele;

                if (ele < minElevation)
                    minElevation = ele;
            }
        }
    }

    if (! track.isNull() && (maxElevation - minElevation < 1))
    {
        /*selected track exist and dem isn't present for this map*/
        QList<CTrack::pt_t>& trkpts = track->getTrackPoints();
        QList<CTrack::pt_t>::const_iterator trkpt = trkpts.begin();
        maxElevation = trkpt->ele;
        minElevation = trkpt->ele;
        while(trkpt != trkpts.end())
        {
            if(trkpt->flags & CTrack::pt_t::eDeleted)
            {
                ++trkpt; continue;
            }
            if (trkpt->ele > maxElevation)
                maxElevation = trkpt->ele;
            if (trkpt->ele < minElevation)
                minElevation = trkpt->ele;
            ++trkpt;
        }
    }

    if (maxElevation - minElevation < 1)
    {
        /*selected track and deb are absent*/
        maxElevation = 1;
        minElevation = 0;
    }

}


void CMap3DWidget::drawTrack()
{
    glLineWidth(2.0);
    glPointSize(5.0);
    double ele1, ele2;
    IMap& dem = CMapDB::self().getDEM();

    if (! track.isNull())
    {
        XY pt1, pt2;

        QList<CTrack::pt_t>& trkpts = track->getTrackPoints();
        QList<CTrack::pt_t>::const_iterator trkpt = trkpts.begin();

        pt1.u = trkpt->lon * DEG_TO_RAD;
        pt1.v = trkpt->lat * DEG_TO_RAD;
        if (mapEleAct->isChecked())
            ele1 = dem.getElevation(pt1.u, pt1.v) + 1;
        else
            ele1 = trkpt->ele;
        map->convertRad2Pt(pt1.u, pt1.v);
        convertPt23D(pt1.u, pt1.v, ele1);

        while(trkpt != trkpts.end())
        {
            if(trkpt->flags & CTrack::pt_t::eDeleted)
            {
                ++trkpt; continue;
            }
            pt2.u = trkpt->lon * DEG_TO_RAD;
            pt2.v = trkpt->lat * DEG_TO_RAD;
            if (mapEleAct->isChecked())
                ele2 = dem.getElevation(pt2.u, pt2.v) + 1;
            else
                ele2 = trkpt->ele +1;
            map->convertRad2Pt(pt2.u, pt2.v);
            convertPt23D(pt2.u, pt2.v, ele2);

            quad(pt1.u, pt1.v, ele1, pt2.u, pt2.v, ele2);

            //draw selected points
            if (trkpt->flags & CTrack::pt_t::eSelected)
            {
                glBegin(GL_LINES);
                glColor3f(1.0, 0.0, 0.0);
                glVertex3d(pt1.u, pt1.v, ele1);
                glVertex3d(pt1.u, pt1.v, minElevation);
                glEnd();
                glBegin(GL_POINTS);
                glVertex3d(pt1.u, pt1.v, ele1);
                glEnd();
            }

            ele1 = ele2;
            pt1 = pt2;
            ++trkpt;
        }
    }

    // restore line width by default
    glLineWidth(1);

}


void CMap3DWidget::makeMapObject()
{
    glNewList(objectMap, GL_COMPILE);

    GLfloat ambient[] ={0.0, 0.0, 0.0, 0.0};
    GLfloat diffuse[] ={1.0, 1.0, 1.0, 1.0};
    GLfloat specular[] ={0.0, 0.0, 0.0, 1.0};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);

    //draw map
    if (!map3DAct->isChecked())
        /*draw flat map*/
        drawFlatMap();
    else
        /*using DEM data file to display terrain in 3D*/
        draw3DMap();

    glEndList();
}


void CMap3DWidget::makeTrackObject()
{
    glNewList(objectTrack, GL_COMPILE);

    if (showTrackAct->isChecked())
        drawTrack();

    glEndList();
}


void CMap3DWidget::setXRotation(double angle)
{
    normalizeAngle(&angle);
    if (angle > 0 && angle < 90)
    {
        xRot = angle;
        emit xRotationChanged(angle);
        updateGL();
    }
}


void CMap3DWidget::setZRotation(double angle)
{
    normalizeAngle(&angle);
    if (angle != zRot)
    {
        zRot = angle;
        emit zRotationChanged(angle);
        updateGL();
    }
}


void CMap3DWidget::initializeGL()
{
    objectMap = glGenLists(1);
    objectTrack = glGenLists(1);

    loadMap();
    loadTrack();
    updateElevationLimits();
    connect(&CTrackDB::self(), SIGNAL(sigHighlightTrack(CTrack *)), this, SLOT(loadTrack()));
    connect(this, SIGNAL(sigChanged()), this, SLOT(slotChanged()));
    connect(this, SIGNAL(sigTrackChanged()), this, SLOT(slotTrackChanged()));
    emit sigChanged();

    glClearColor(1.0, 1.0, 1.0, 0.0);
    int i;
    for (i = 0; i < 6; i++)
    {
        QImage img(tr(":/skybox/%1.bmp").arg(i));
        skyBox[i] = bindTexture(img, GL_TEXTURE_2D);
    }

    glShadeModel(GL_SMOOTH);
    glEnable(GL_LINE_SMOOTH);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glEnable(GL_DEPTH_TEST);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    glEnable(GL_CULL_FACE);
}


void CMap3DWidget::lightTurn()
{
    light = ! light;
    updateGL();
}


void CMap3DWidget::paintGL()
{
    if (reDraw)
    {
        reDraw = false;
        QApplication::setOverrideCursor(Qt::WaitCursor);
        deleteTexture(mapTexture);
        setMapTexture();
        makeMapObject();
        slotTrackChanged(false);
        QApplication::restoreOverrideCursor();
    }
    int side = qMax(mapSize.width(), mapSize.height()) / 2;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    glTranslated(0.0, -0.25 * side, 0.0);
    glTranslated(0.0, 0.0, -side);
    glRotated(-xRot, 1.0, 0.0, 0.0);
    glScalef(zoomFactor, zoomFactor, zoomFactor);

    glTranslated(xShift, yShift, zShift);

    glRotated(zRot, 0.0, 0.0, 1.0);

    drawSkybox(0,0,0, side, side, side);

    /* subtract the offset and set the Z axis scale */
    glScalef(1.0, 1.0, eleZoomFactor * (mapSize.width() / 10.0) / (maxElevation - minElevation));
    glTranslated(0.0, 0.0, -minElevation);

    if (light)
    {
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);

        GLfloat light0_pos[] = {xLight, yLight, - (zLight + minElevation), 0.0};

        //         GLfloat diffuse0[] = {0.5, 1.0, 1.0, 1.0};
        //         GLfloat ambient0[] = {1.0, 0.5, 1.0, 1.0};
        //         GLfloat specular0[] = {1.0, 1.0, 0.5, 1.0};
        //         glLightfv(GL_LIGHT0, GL_AMBIENT, ambient0);
        //         glLightfv(GL_LIGHT0, GL_DIFFUSE, specular0);
        //         glLightfv(GL_LIGHT0, GL_SPECULAR, diffuse0);

        glLightfv(GL_LIGHT0, GL_POSITION, light0_pos);
        glShadeModel(GL_SMOOTH);

        glMaterialf (GL_FRONT,GL_SHININESS, 10);
    }

    glCallList(objectMap);
    if (light)
    {
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
        glDisable(GL_LIGHT0);
        glDisable(GL_LIGHTING);
    }
    glCallList(objectTrack);

    /*draw axis*/
    /*    glBegin(GL_LINES);

        glColor3f(1.0, 0.0, 0.0);
        glVertex3f(0.0, 0.0, 0.0);
        glVertex3f(100.0, 0.0, 0.0);

        glColor3f(0.0, 1.0, 0.0);
        glVertex3f(0.0, 0.0, 0.0);
        glVertex3f(0.0, 100.0, 0.0);

        glColor3f(0.0, 0.0, 1.0);
        glVertex3f(0.0, 0.0, 0.0);
        glVertex3f(0.0, 0.0, 100.0);

        glEnd();*/

    /*draw the grid*/
    int i, d = 100, n = 10;

    glBegin(GL_LINES);
    glColor3f(0.5, 0.5, 0.5);
    for(i = -n; i <= n; i ++)
    {
        glVertex3f(-d * n, i * d, minElevation);
        glVertex3f(d * n, i * d, minElevation);

        glVertex3f(i * d, -d * n, minElevation);
        glVertex3f(i * d, d * n, minElevation);
    }
    glEnd();
    glError();

    const QMap<QString,CWpt*>& wpts = CWptDB::self().getWpts();

    QMap<QString,CWpt*>::const_iterator wpt  = wpts.begin();
    while(wpt != wpts.end())
    {
        drawWpt(wpt.value());
        ++wpt;
    }

}


void drawQuad(GLdouble x, GLdouble y, GLdouble xsize, GLdouble ysize, GLdouble z, GLint texture)
{
    glBindTexture(GL_TEXTURE_2D, texture);

    glBegin(GL_QUADS);
    glTexCoord2f(0, 1);
    glVertex3d(x, ysize + y, z);
    glTexCoord2f(0, 0);
    glVertex3d(x, y, z);
    glTexCoord2f(1, 0);
    glVertex3d(xsize + x, y, z);
    glTexCoord2f(1, 1);
    glVertex3d(xsize + x, ysize + y, z);
    glEnd();

}


void CMap3DWidget::drawWpt(CWpt *wpt)
{
    double x, y, z, wsize;
    double u = 0,v = 0,ele = 0;
    int w, h, side;
    GLint texture, mask_texture, text_texture, mask_text_texture;

    w = size().width();
    h = size().height();
    side = qMax(w, h);

    QPixmap icon = getWptIconByName(wpt->icon);
    u = wpt->lon * DEG_TO_RAD;
    v = wpt->lat * DEG_TO_RAD;
    IMap& dem = CMapDB::self().getDEM();
    if (map3DAct->isChecked())
        ele = dem.getElevation(u, v);

    map->convertRad2Pt(u, v);
    if (u < 0 || u > mapSize.width())
        return;
    if (v < 0 || v > mapSize.height())
        return;

    convertPt23D(u,v,ele);

    GLdouble modelview[16];
    int i, j;
    GLdouble a[4], b[4] = {u,v,ele,1};
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    for (i = 0; i < 4; i++)
    {
        a[i] = 0;
        for (j = 0; j < 4; j ++)
            a[i] += modelview[i+j*4] * b[j];
    }

    x = a[0];
    y = a[1];
    z = a[2];

    wsize = 15;

    //draw text
    QFont           f = CResources::self().getMapFont();
    //increase quality of text texture
    if (f.pixelSize() > 0)
        f.setPixelSize(f.pixelSize()*3);
    else
        f.setPointSize(f.pointSize()*3);
    QFontMetrics    fm(f);
    QRect           r = fm.boundingRect(wpt->name);
    QSize text_size = r.size() / 3;
    QPixmap text_pic(r.size());
    QString str = wpt->name;
    text_pic.fill(Qt::white);
    QPainter p(&text_pic);
    //draw mask
    p.setFont(f);
    p.setPen(Qt::black);
    p.drawText(text_pic.rect(), Qt::AlignCenter, wpt->name);
    p.end();
    mask_text_texture = bindTexture(text_pic);
    //draw text
    text_pic.fill(Qt::transparent);
    p.begin(&text_pic);
    p.setPen(Qt::darkBlue);
    p.setFont(f);
    p.drawText(text_pic.rect(), Qt::AlignCenter, wpt->name);
    p.end();
    text_texture = bindTexture(text_pic);

    mask_texture = bindTexture(icon.alphaChannel().createMaskFromColor(Qt::black));
    texture = bindTexture(icon);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_ALPHA_TEST);
    glEnable(GL_BLEND);
    //glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendFunc(GL_DST_COLOR,GL_ZERO);
    glEnable(GL_TEXTURE_2D);
    glColor4f(0,0,0,0);

    //draw mask
    drawQuad(x, y, wsize, wsize, z, mask_texture);
    drawQuad(x - text_size.width() / 2, y + wsize, text_size.width(), text_size.height(), z, mask_text_texture);

    glBlendFunc(GL_ONE, GL_ONE);

    //draw icon
    //icon should be before mask
    z += 0.1;
    drawQuad(x, y, wsize, wsize, z, texture);
    drawQuad(x - text_size.width() / 2, y + wsize, text_size.width(), text_size.height(), z, text_texture);

    deleteTexture(texture);
    deleteTexture(mask_texture);
    deleteTexture(text_texture);
    deleteTexture(mask_text_texture);

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
}


void CMap3DWidget::resizeGL(int width, int height)
{
    int side = qMax(width, height);
    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    /* 20 is equal to value of a maximum zoom factor. */
    glFrustum(-width/100.0, width/100.0, -height/100.0, height/100.0, side/100.0, 200.0 * side);
    //     glOrtho(-width, width, -height, height, 0, 20 * side);
    glMatrixMode(GL_MODELVIEW);
}


void CMap3DWidget::convertDsp2Z0(QPoint &a)
{
    GLdouble projection[16];
    GLdouble modelview[16];
    GLdouble k1, z1, x0, xk, y0, yk, z0, zk;
    GLint viewport[4];
    GLsizei vx, vy;

    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);

    vx = a.x();
    vy = height() - a.y();
    gluUnProject(vx, vy, 0, modelview, projection, viewport, &x0, &y0, &z0);
    gluUnProject(vx, vy, 1, modelview, projection, viewport, &xk, &yk, &zk);

    xk -= x0;
    yk -= y0;
    zk -= z0;
    /* the line equation A0 + tAk, where A0 = |x0, y0, z0|, Ak = |xk, yk, zk| */
    /* point of intersection with flat z = 0 */
    z1 = 0;
    k1 = (z1 - z0) / zk;
    a.rx() = x0 + xk * k1;
    a.ry() = y0 + yk * k1;
}


void CMap3DWidget::mouseDoubleClickEvent ( QMouseEvent * event )
{
    CTrack::pt_t * selTrkPt;
    double x0, y0, z0;

    x0 = event->pos().x();
    y0 = event->pos().y();
    convertMouse23D(x0, y0, z0);

    convert3D2Pt(x0, y0, z0);

    if(track.isNull()) return;

    selTrkPt = 0;
    int d1 = 20;
    XY p;

    QList<CTrack::pt_t>& pts          = track->getTrackPoints();
    QList<CTrack::pt_t>::iterator pt  = pts.begin();
    while(pt != pts.end())
    {
        if(pt->flags & CTrack::pt_t::eDeleted)
        {
            ++pt; continue;
        }
        p.u = pt->lon * DEG_TO_RAD;
        p.v = pt->lat * DEG_TO_RAD;
        map->convertRad2Pt(p.u, p.v);

        int d2 = abs(x0 - p.u) + abs(y0 - p.v);

        if(d2 < d1)
        {
            selTrkPt = &(*pt);
            d1 = d2;
        }

        ++pt;
    }
    if (selTrkPt)
    {
        selTrkPt->flags |= CTrack::pt_t::eSelected;
        track->setPointOfFocus(selTrkPt->idx);
        updateGL();
    }
}


void CMap3DWidget::mousePressEvent(QMouseEvent *event)
{
    qDebug() << "CMap3DWidget::mousePressEvent";
    lastPos = event->pos();
    if (!cursorPress)
    {
        QApplication::setOverrideCursor(QCursor(QPixmap(":/cursors/cursorMove")));
        cursorPress = true;
    }
}


void CMap3DWidget::mouseReleaseEvent(QMouseEvent *event)
{
    qDebug() << "CMap3DWidget::mouseReleaseEvent";
    if (cursorPress)
    {
        QApplication::restoreOverrideCursor();
        cursorPress = false;
    }
}


void CMap3DWidget::expandMap(bool zoomIn)
{
    double zoomFactor = zoomIn ? 1.1 : 1/1.1;
    XY pv;
    /*save coord of the center map*/
    pv.u = mapSize.width() / 2;
    pv.v = mapSize.height() / 2;
    map->convertPt2Rad(pv.u, pv.v);

    /*slotChanged will be executed by the operation move*/
    disconnect(map, SIGNAL(sigChanged()),this,SLOT(slotChanged()));
    map->resize(QSize(mapSize.width() * zoomFactor, mapSize.height() * zoomFactor));

    /*restore coord of the center map*/
    map->convertRad2Pt(pv.u, pv.v);
    connect(map, SIGNAL(sigChanged()),this,SLOT(slotChanged()));
    map->move(QPoint(pv.u, pv.v), QPoint(mapSize.width()/2, mapSize.height()/2));
}


void CMap3DWidget::keyReleaseEvent ( QKeyEvent * event )
{
    pressedKeys.remove(event->key());
}


void CMap3DWidget::keyPressEvent ( QKeyEvent * event )
{
    pressedKeys.insert(event->key());

    qint32 dx = 0, dy = 0;
    qint32 zoomMap = 0;
    switch (event->key())
    {
        case Qt::Key_Up:
            dy -= 100;
            break;

        case Qt::Key_Down:
            dy += 100;
            break;

        case Qt::Key_Left:
            dx -= 100;
            break;

        case Qt::Key_Right:
            dx += 100;
            break;
        case Qt::Key_PageUp:
            zoomMap = 1;
            break;
        case Qt::Key_PageDown:
            zoomMap = -1;
            break;
        case Qt::Key_Home:
            expandMap(true);
            break;
        case Qt::Key_End:
            expandMap(false);
            break;
        default:
            event->ignore();
            return;
    }
    if (zoomMap)
    {
        map->zoom(zoomMap > 0 ? true : false, QPoint(mapSize.width() / 2, mapSize.height() / 2));
    }

    if (dx || dy)
    {
        map->move(QPoint(dx, dy), QPoint(0, 0));
    }
    updateGL();
}


void CMap3DWidget::focusOutEvent ( QFocusEvent * event )
{
    pressedKeys.clear();
}


void CMap3DWidget::mouseMoveEvent(QMouseEvent *event)
{
    mousePos = event->pos();
    int dx = event->x() - lastPos.x();
    int dy = event->y() - lastPos.y();

    if (pressedKeys.contains(Qt::Key_H))
    {
        zLight += dy;
    }
    else if (pressedKeys.contains(Qt::Key_L))
    {
        double x0, y0, z0;
        double x1, y1, z1;
        x0 = event->x();
        y0 = event->y();
        convertMouse23D(x0, y0, z0);
        x1 = lastPos.x();
        y1 = lastPos.y();
        convertMouse23D(x1, y1, z1);

        double z = zLight + minElevation;
        xLight += (x0 / (z -z0) * z - x1 / (z -z1) * z);
        yLight += (y0 / (z -z0) * z - y1 / (z -z1) * z);
        updateGL();
    }
    else if (pressedKeys.contains(Qt::Key_M))
    {
        QPoint p1 = event->pos(), p2 = lastPos;
        convertDsp2Z0(p1);
        convertDsp2Z0(p2);
        dx = -(p1.x() - p2.x());
        dy = p1.y() - p2.y();
        map->move(QPoint(dx, dy), QPoint(0, 0));
    }
    else if (event->buttons() & Qt::LeftButton)
    {
        setXRotation(xRot - xRotSens * dy);
        setZRotation(zRot + zRotSens * dx);
    }
    else if (event->buttons() & Qt::MidButton)
    {
        xShift += dx / zoomFactor;
        yShift -= dy / zoomFactor;
    }
    lastPos = event->pos();
    updateGL();
}


void CMap3DWidget::quad(GLdouble x1, GLdouble y1, GLdouble z1, GLdouble x2, GLdouble y2, GLdouble z2)
{
    glBegin(GL_QUADS);
    double c1, c2;
    // compute colors
    c1 = z1 / maxElevation * 255;
    c2 = z2 / maxElevation * 255;

    qglColor(wallCollor);
    glVertex3d(x2, y2, minElevation);
    glVertex3d(x1, y1, minElevation);
    qglColor(wallCollor.dark(c1));
    glVertex3d(x1, y1, z1);
    qglColor(wallCollor.dark(c2));
    glVertex3d(x2, y2, z2);

    qglColor(wallCollor.dark(c2));
    glVertex3d(x2, y2, z2);
    qglColor(wallCollor.dark(c1));
    glVertex3d(x1, y1, z1);
    qglColor(wallCollor);
    glVertex3d(x1, y1, minElevation);
    glVertex3d(x2, y2, minElevation);

    glEnd();

    glBegin(GL_LINES);
    qglColor(highBorderColor);
    glVertex3d(x1, y1, z1);
    glVertex3d(x2, y2, z2);
    glEnd();
}


void CMap3DWidget::normalizeAngle(double *angle)
{
    while (*angle < 0)
        *angle += 360;
    while (*angle > 360)
        *angle -= 360;
}


void CMap3DWidget::wheelEvent ( QWheelEvent * e )
{
    bool in = CResources::self().flipMouseWheel() ? (e->delta() > 0) : (e->delta() < 0);
    if (pressedKeys.contains(Qt::Key_M))
    {
        map->zoom(in, QPoint(mapSize.width() / 2, mapSize.height() / 2));
    }
    else
    {
        if (in)
        {
            qDebug() << "in" << endl;
            zoomFactor *= 1.1;
        }
        else
        {
            qDebug() << "out" << endl;
            zoomFactor /= 1.1;
        }
    }
    updateGL();
}


void CMap3DWidget::enterEvent(QEvent * )
{
    if (!cursorFocus)
    {
        QApplication::setOverrideCursor(QCursor(QPixmap(":/cursors/cursorMoveMap"), 0, 0));
        cursorFocus = true;
    }

}


void CMap3DWidget::leaveEvent(QEvent * )
{
    if (cursorPress)
    {
        QApplication::restoreOverrideCursor();
        cursorPress = false;
    }

    if (cursorFocus)
    {
        QApplication::restoreOverrideCursor();
        cursorFocus = false;
    }
}


void CMap3DWidget::slotAddWpt()
{
    IMap& map = CMapDB::self().getMap();
    IMap& dem = CMapDB::self().getDEM();
    double x, y, z;

    x = mousePos.x();
    y = mousePos.y();
    convertMouse23D(x, y, z);
    convert3D2Pt(x, y, z);
    map.convertPt2Rad(x, y);
    float ele = dem.getElevation(x, y);
    CWptDB::self().newWpt(x, y, ele);

}


void CMap3DWidget::slotDeleteWpt()
{
    if(selWpt.isNull()) return;

    QString key = selWpt->key();
    CWptDB::self().delWpt(key);
}


void CMap3DWidget::slotEditWpt()
{
    if(selWpt.isNull()) return;

    CDlgEditWpt dlg(*selWpt,this);
    dlg.exec();
}


void CMap3DWidget::slotCopyPositionWpt()
{
    if(selWpt.isNull()) return;

    QString position;
    GPS_Math_Deg_To_Str(selWpt->lon, selWpt->lat, position);

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(position);
}


void CMap3DWidget::slotSaveImage(const QString& filename)
{
    QImage image = grabFrameBuffer();
    image.save(filename);
}
