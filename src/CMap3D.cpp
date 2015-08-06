/**********************************************************************************************
    Copyright (C) 2010 Oliver Eichler oliver.eichler@gmx.de

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

#include "CMapDB.h"
#include "CMap3D.h"
#include "CResources.h"
#include "GeoMath.h"
#include "CCanvas.h"
#include "CMainWindow.h"
#include "CDlg3DHelp.h"
#include "CDlgConfig3D.h"
#include "CTrack.h"
#include "CTrackDB.h"
#include "CWpt.h"
#include "CWptDB.h"
#include "WptIcons.h"
#include "CSettings.h"

#include <QtGui>
#include <QtOpenGL>
#include <math.h>
#if defined(Q_OS_MAC)
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#ifdef WIN32
#include <float.h>
#define isnan(x) _isnan(x)
#define isinf(x) (!_finite(x))
#endif

#define APPERTURE_ANGLE 60.0

static void glError()
{
    GLenum err = glGetError();
    while (err != GL_NO_ERROR)
    {
        qDebug("glError: %s caught at %s:%u\n", (char *)gluErrorString(err), __FILE__, __LINE__);
        err = glGetError();
    }
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


inline double calcZRotationDelta(double delta)
{
    if(delta >= 20.0)
    {
        return 5;
    }
    else if(delta < 20.0)
    {
        return 2.5;
    }
    else if(delta < 5.0)
    {
        return 1.0;
    }

    return 1.0;
}


static QColor wallCollor = QColor::fromCmykF(0.40, 0.0, 1.0, 0);
static QColor highBorderColor = QColor::fromRgbF(0.0, 0.0, 1.0, 0);

CMap3D::CMap3D(IMap * map, QWidget * parent)
: QGLWidget(parent)
, xRotation(0)
, yRotation(0)
, zRotation(0)
, xpos(0)
, ypos(-400)
, zpos(200)
, zoomFactorEle(1.0)
, zoomFactor(0.5)
, zoomFactorZ(1.0)
, needsRedraw(true)
, mapTextureId(0)
, mapObjectId(0)
, trkObjectId(0)
, keyShiftPressed(false)
, keyLPressed(false)
, light(true)
, xLight(200.0)
, yLight(400.0)
, zLight(5000.0)
, angleNorth(0)
, quality3D(eCoarse)
, coupleElePOV(true)
, pen0(Qt::white, 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)
, pen1(Qt::black, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)
, pen2(Qt::yellow, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)
, trkPointIndex(-1)
, targetZRotation(0)
{
    setObjectName("CMap3D");
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setAutoFillBackground(false);

    theMap = map;
    connect(map, SIGNAL(destroyed()), this, SLOT(deleteLater()));
    connect(map, SIGNAL(sigChanged()), this, SLOT(slotMapChanged()));

    connect(&CTrackDB::self(), SIGNAL(sigChanged()), this, SLOT(slotTrackChanged()));

    act3DMap = new QAction(tr("3D / 2D"), this);
    act3DMap->setCheckable(true);
    act3DMap->setChecked(true);
    connect(act3DMap, SIGNAL(triggered()), this, SLOT(slotMapChanged()));

    actFPVMode = new QAction(tr("FPV / Rot."), this);
    actFPVMode->setCheckable(true);
    actFPVMode->setChecked(true);
    connect(actFPVMode, SIGNAL(triggered()), this, SLOT(slotFPVModeChanged()));

    actResetLight = new QAction(QIcon(":icons/iconLight16x16"),tr("Reset Light"), this);
    connect(actResetLight, SIGNAL(triggered()), this, SLOT(slotResetLight()));

    actTrackOnMap =  new QAction(tr("Track on map"), this);
    actTrackOnMap->setCheckable(true);
    actTrackOnMap->setChecked(true);
    connect(actTrackOnMap, SIGNAL(triggered()), this, SLOT(slotMapChanged()));

    actTrackMode = new QAction(tr("POV on track"), this);
    actTrackMode->setCheckable(true);
    actTrackMode->setChecked(false);
    connect(actTrackMode, SIGNAL(triggered()), this, SLOT(slotTrackModeChanged()));

    SETTINGS;
    act3DMap->setChecked(cfg.value("map/3D/3dmap", true).toBool());
    actFPVMode->setChecked(cfg.value("map/3D/fpv", true).toBool());
    actTrackOnMap->setChecked(cfg.value("map/3D/trackonmap", false).toBool());
    zoomFactorEle = cfg.value("map/3D/zoomFactorEle", zoomFactorEle).toDouble();
    light = cfg.value("map/3D/light", light).toBool();
    xLight = cfg.value("map/3D/xLight", xLight).toDouble();
    yLight = cfg.value("map/3D/yLight", yLight).toDouble();
    zLight = cfg.value("map/3D/zLight", zLight).toDouble();
    quality3D = (EQuality3D)cfg.value("map/3D/quality3D", quality3D).toInt();
    coupleElePOV = cfg.value("map/3D/coupleElePOV", coupleElePOV).toBool();

    helpButton = new QPushButton(tr("Help 3d"));
    connect(helpButton, SIGNAL(clicked()), this, SLOT(slotHelp3D()));
    theMainWindow->statusBar()->insertPermanentWidget(0,helpButton);

    timerAnimateRotation = new QTimer(this);
    timerAnimateRotation->setSingleShot(false);
    timerAnimateRotation->setInterval(30);
    connect(timerAnimateRotation, SIGNAL(timeout()), this, SLOT(slotAnimateRotation()));

    slotTrackChanged();

    qApp->installEventFilter(this);
}


CMap3D::~CMap3D()
{
    qDebug() << "CMap3D::~CMap3D()";

    for (int i = 0; i < 6; i++)
    {
        deleteTexture(skyBox[i]);
    }
    deleteTexture(mapTextureId);

    glDeleteLists(mapObjectId, 1);
    glDeleteLists(trkObjectId, 1);

    SETTINGS;
    cfg.setValue("map/3D/3dmap", act3DMap->isChecked());
    cfg.setValue("map/3D/fpv", actFPVMode->isChecked());
    cfg.setValue("map/3D/trackonmap", actTrackOnMap->isChecked());
    cfg.setValue("map/3D/zoomFactorEle", zoomFactorEle);
    cfg.setValue("map/3D/light", light);
    cfg.setValue("map/3D/xLight", xLight);
    cfg.setValue("map/3D/yLight", yLight);
    cfg.setValue("map/3D/zLight", zLight);
    cfg.setValue("map/3D/quality3D", quality3D);
    cfg.setValue("map/3D/coupleElePOV", coupleElePOV);

    delete helpButton;

}


void CMap3D::slotMapChanged()
{
    needsRedraw = true;

    angleNorth = theMap->getAngleNorth();

    if(isHidden())
    {
        return;
    }

    update();
}


void CMap3D::slotFPVModeChanged()
{
    if(!actFPVMode->isChecked())
    {
        actTrackMode->setChecked(false);
    }
    else
    {

    }

    update();
}


void CMap3D::slotResetLight()
{
    xLight = 200;
    yLight = 400;
    zLight = 5000;
    update();
}


void CMap3D::slotSaveImage(const QString& filename)
{
    QImage image = grabFrameBuffer();
    image.save(filename);
}


void CMap3D::slotHelp3D()
{
    CDlg3DHelp dlg;
    dlg.exec();
}


void CMap3D::slotConfig3D()
{
    CDlgConfig3D dlg(*this);
    dlg.exec();

    needsRedraw = true;
    update();
}


void CMap3D::lightTurn()
{

    light = !light;
    update();
}


void CMap3D::slotTrackChanged()
{
    CTrack * trk = theTrack;
    actTrackMode->setChecked(false);
    theTrack = CTrackDB::self().highlightedTrack();

    if(trk != theTrack)
    {
        needsRedraw = true;
    }
    update();
}


void CMap3D::slotChange3DMode()
{
    act3DMap->setChecked(!act3DMap->isChecked());
    needsRedraw = true;
    update();
}


void CMap3D::slotChange3DFPVMode()
{
    actFPVMode->setChecked(!actFPVMode->isChecked());
    update();
}


void CMap3D::slotChange3DTrackMode()
{
    actTrackMode->setChecked(!actTrackMode->isChecked());
    slotTrackModeChanged();
}


void CMap3D::slotTrackModeChanged()
{
    if(!theTrack.isNull() && actTrackMode->isChecked())
    {
        //find track point closest and snap to it.
        double x0 = xpos;
        double y0 = ypos;
        double z0 = zpos;
        projXY p;
        CTrack::pt_t selTrkPt;

        convert3D2Pt(x0, y0, z0);

        int d1 =0x7FFFFFFF;

        trkPointIndex = -1;
        int cnt = -1;
        QList<CTrack::pt_t>& pts          = theTrack->getTrackPoints();
        QList<CTrack::pt_t>::iterator pt  = pts.begin();
        while(pt != pts.end())
        {
            cnt++;

            if(pt->flags & CTrack::pt_t::eDeleted)
            {
                ++pt; continue;
            }
            p.u = pt->lon * DEG_TO_RAD;
            p.v = pt->lat * DEG_TO_RAD;
            theMap->convertRad2Pt(p.u, p.v);

            int d2 = abs(x0 - p.u) + abs(y0 - p.v);

            if(d2 < d1)
            {
                trkPointIndex = cnt;
                selTrkPt = (*pt);
                d1 = d2;
            }

            ++pt;
        }

        zoomFactor = 0.5;

        x0 = selTrkPt.lon * DEG_TO_RAD;
        y0 = selTrkPt.lat * DEG_TO_RAD;

        double ele;
        if (actTrackOnMap->isChecked())
        {
            IMap& dem = CMapDB::self().getDEM();
            ele = dem.getElevation(x0, y0) + 1;
        }
        else
        {
            ele = selTrkPt.ele;
        }

        if(ele == WPT_NOFLOAT)
        {
            ele = selTrkPt.ele;
        }

        theMap->convertRad2Pt(x0, y0);
        convertPt23D(x0, y0, z0);

        xpos = x0;
        ypos = y0;
        zpos = (ele + 10 - minEle) * zoomFactorZ * zoomFactorEle * zoomFactor;

        actFPVMode->setChecked(true);
    }
    else
    {
        actTrackMode->setChecked(false);
        trkPointIndex = -1;
    }
    update();
}


void CMap3D::slotAnimateRotation()
{
    zRotation += stepRot;
    deltaRot  -= abs(stepRot);

    if(deltaRot < 0)
    {
        zRotation = targetZRotation;
        timerAnimateRotation->stop();
    }
    else
    {
        stepRot = (stepRot > 0) ? calcZRotationDelta(deltaRot) : -calcZRotationDelta(deltaRot);
    }

    update();
}


void CMap3D::initializeGL()
{
    qDebug() << "void CMap3D::initializeGL()";

    for (int i = 0; i < 6; i++)
    {
        QImage img(tr(":/skybox/%1.bmp").arg(i));
        skyBox[i] = bindTexture(img, GL_TEXTURE_2D);
    }

    mapObjectId = glGenLists(1);
    trkObjectId = glGenLists(1);

    needsRedraw = true;
    glError();
}


void CMap3D::setupViewport(int width, int height)
{
                                 //set the viewport to the current window specifications
    glViewport (0, 0, (GLsizei)width, (GLsizei)height);
    glMatrixMode (GL_PROJECTION);//set the matrix to projection
    glLoadIdentity ();
                                 //set the perspective (angle of sight, width, height, , depth)
    gluPerspective (APPERTURE_ANGLE, (GLfloat)width / (GLfloat)height, 1.0, 1000*(width > height ? width : height));
    glMatrixMode (GL_MODELVIEW); //set the matrix back to model
    glLoadIdentity ();
}


void CMap3D::resizeGL(int width, int height)
{
    qDebug() << "void CMap3D::resizeGL(int width, int height)" << width << height;

    int side = width > height ? width : height;
    xsize = width;
    ysize = height;
    zsize = side;

    if(theMap.isNull())
    {
        return;
    }
    theMap->resize(QSize(width,height));

    setupViewport(width, height);

    needsRedraw = true;
    glError();
}


void CMap3D::setElevationLimits()
{
    double ele;
    int i, j;
    QSize mapSize = theMap->getSize();
    double w = mapSize.width();
    double h = mapSize.height();

    double step = quality3D;

    // increment xcount, because the number of points are on one more
    // than number of lengths |--|--|--|--|
    int xcount = (w / step + 1);
    int ycount = (h / step + 1);

    minEle = maxEle = 0.0;

    QVector<float> eleData(xcount*ycount);
    bool ok = getEleRegion(eleData, xcount, ycount);
    if (ok)
    {
        minEle = maxEle = eleData[0];

        for (i = 0; i < xcount; i++)
        {
            for (j = 0; j < ycount; j++)
            {
                ele = eleData[i + j * xcount];
                if (ele > maxEle)
                {
                    maxEle = ele;
                }

                if (ele < minEle)
                {
                    minEle = ele;
                }
            }
        }
    }

    if(!theTrack.isNull())
    {
        /*selected track exist and dem isn't present for this map*/
        QList<CTrack::pt_t>& trkpts = theTrack->getTrackPoints();
        QList<CTrack::pt_t>::const_iterator trkpt = trkpts.begin();
        while(trkpt != trkpts.end())
        {
            if(trkpt->flags & CTrack::pt_t::eDeleted)
            {
                ++trkpt; continue;
            }

            if (trkpt->ele > maxEle)
            {
                maxEle = trkpt->ele;
            }

            if (trkpt->ele < minEle)
            {
                minEle = trkpt->ele;
            }
            ++trkpt;
        }
    }

    if (maxEle - minEle < 1)
    {
        /*selected track and dem are absent*/
        maxEle = 9000.0;
        minEle = 0.0;
    }

    zoomFactorZ =  zsize * 0.1 / (maxEle - minEle);

}


void CMap3D::setPOV (void)
{
    glRotatef(270,1.0,0.0,0.0);
    glRotatef(xRotation,1.0,0.0,0.0);
    glRotatef(yRotation,0.0,1.0,0.0);
    glRotatef(zRotation,0.0,0.0,1.0);

    glTranslated(-xpos,-ypos,-zpos);
    glScalef(zoomFactor, zoomFactor, zoomFactor);

    glError();
}


void CMap3D::setMapObject()
{
    glNewList(mapObjectId, GL_COMPILE);

    GLfloat ambient[] ={0.0, 0.0, 0.0, 0.0};
    GLfloat diffuse[] ={1.0, 1.0, 1.0, 1.0};
    GLfloat specular[] ={0.0, 0.0, 0.0, 1.0};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);

    // Save Current Matrix
    glPushMatrix();
    glScalef(1.0, 1.0, zoomFactorZ);
    if(act3DMap->isChecked())
    {
        glTranslated(0.0, 0.0, -minEle);
        draw3DMap();
    }
    else
    {
        drawFlatMap();
    }
    glPopMatrix();

    glEndList();
}


void CMap3D::setTrackObject()
{
    glNewList(trkObjectId, GL_COMPILE);
    glPushMatrix();
    glScalef(1.0, 1.0, zoomFactorZ);
    glTranslated(0.0, 0.0, -minEle);

    glLineWidth(5.0);
    glPointSize(10.0);
    double ele1, ele2;
    IMap& dem = CMapDB::self().getDEM();

    CTrack * track = CTrackDB::self().highlightedTrack();

    if (track != 0)
    {
        projXY pt1, pt2;

        glLineWidth(5.0);
        highBorderColor = track->getColor();

        QList<CTrack::pt_t>& trkpts = track->getTrackPoints();
        QList<CTrack::pt_t>::const_iterator trkpt = trkpts.begin();

        pt1.u = trkpt->lon * DEG_TO_RAD;
        pt1.v = trkpt->lat * DEG_TO_RAD;
        if (actTrackOnMap->isChecked())
        {
            ele1 = dem.getElevation(pt1.u, pt1.v) + 1;
        }
        else
        {
            ele1 = trkpt->ele;
        }

        if(ele1 == WPT_NOFLOAT)
        {
            ele1 = trkpt->ele;
        }

        theMap->convertRad2Pt(pt1.u, pt1.v);
        convertPt23D(pt1.u, pt1.v, ele1);

        while(trkpt != trkpts.end())
        {
            if(trkpt->flags & CTrack::pt_t::eDeleted)
            {
                ++trkpt; continue;
            }
            pt2.u = trkpt->lon * DEG_TO_RAD;
            pt2.v = trkpt->lat * DEG_TO_RAD;
            if (actTrackOnMap->isChecked())
            {
                ele2 = dem.getElevation(pt2.u, pt2.v) + 2;
            }
            else
            {
                ele2 = trkpt->ele + 2;
            }

            if(ele2 == WPT_NOFLOAT)
            {
                ele2 = trkpt->ele;
            }

            theMap->convertRad2Pt(pt2.u, pt2.v);
            convertPt23D(pt2.u, pt2.v, ele2);

            quad(pt1.u, pt1.v, ele1, pt2.u, pt2.v, ele2);

            //draw selected points
            if (trkpt->flags & CTrack::pt_t::eSelected)
            {
                glBegin(GL_LINES);
                glColor3f(0.8, 0, 0);
                glVertex3d(pt1.u, pt1.v, ele1);
                glVertex3d(pt1.u, pt1.v, minEle);
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

    glPopMatrix();
    glEndList();
}


void CMap3D::paintEvent( QPaintEvent * e)
{
    // start 3D painting

    // restore all settings
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_LINE_SMOOTH);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

    setupViewport(width(), height());

    // render objects on change
    if(needsRedraw)
    {
        needsRedraw = false;

        QApplication::setOverrideCursor(Qt::WaitCursor);

        setElevationLimits();

        deleteTexture(mapTextureId);
        QPixmap pm(theMap->getSize());
        QPainter p(&pm);
        p.eraseRect(pm.rect());
        theMap->draw(p);
        mapTextureId = bindTexture(pm, GL_TEXTURE_2D);

        setMapObject();
        setTrackObject();

        QApplication::restoreOverrideCursor();

    }

    // start rendering the sceene
                                 //clear the screen to black
    glClearColor (0.0,0.0,0.0,0.0);
                                 //clear the color buffer and the depth buffer
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    setPOV();

    drawSkybox();

    glPushMatrix();
    glScalef(2.0, 2.0, zoomFactorEle);

    if (light)
    {
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);

        GLfloat light0_pos[] = {xLight, yLight, - GLfloat(zLight + minEle), 0.0};

        glLightfv(GL_LIGHT0, GL_POSITION, light0_pos);
        glMaterialf (GL_FRONT,GL_SHININESS, 10);
    }
    glCallList(mapObjectId);

    if (light)
    {
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
        glDisable(GL_LIGHT0);
        glDisable(GL_LIGHTING);
    }

    glCallList(trkObjectId);

    drawWaypoints();

    glPopMatrix();

    drawBaseGrid();
    drawCenterStar();

    // restore 2D context
    glShadeModel(GL_FLAT);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LINE_SMOOTH);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glFinish();

    // start 2D painting
    QPainter p;
    p.begin(this);
    USE_ANTI_ALIASING(p, true);

    drawCompass(p);
    drawElevation(p);
    drawHorizont(p);

    p.end();
}


void CMap3D::drawSkybox()
{
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

    glEnable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);

    glColor4f(1.0, 1.0, 0.0,1.0f);

    // Save Current Matrix
    glPushMatrix();

    // First apply scale matrix
    glScalef(xsize, ysize, zsize);

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
    glDisable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);

    glError();
}


void CMap3D::drawCenterStar()
{
    glBegin(GL_LINES);

    glColor3f(1.0, 0.0, 0.0);
    glVertex3f(-100.0, 0.0, 0.0);
    glVertex3f( 100.0, 0.0, 0.0);

    glColor3f(0.0, 1.0, 0.0);
    glVertex3f(0.0, -100.0, 0.0);
    glVertex3f(0.0,  100.0, 0.0);

    glColor3f(0.0, 0.0, 1.0);
    glVertex3f(0.0, 0.0, -100.0);
    glVertex3f(0.0, 0.0,  100.0);

    glEnd();

    glError();
}


void CMap3D::drawBaseGrid()
{
    /*draw the grid*/
    int i, d = 100, n;

    glBegin(GL_LINES);
    glColor3f(0.5, 0.5, 0.5);

    n = (xsize + 50) / 100;
    for(i = -n; i <= n; i ++)
    {
        glVertex3f(i * d, -ysize, 0);
        glVertex3f(i * d, +ysize, 0);
    }

    n = (ysize + 50) / 100;
    for(i = -n; i <= n; i ++)
    {
        glVertex3f(-xsize, i * d, 0);
        glVertex3f(+xsize, i * d, 0);
    }
    glEnd();

    glError();
}


void CMap3D::drawFlatMap()
{
    QSize   s = theMap->getSize();
    double  w = s.width();
    double  h = s.height();

    glEnable(GL_NORMALIZE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, mapTextureId);
    glBegin(GL_QUADS);
    glTexCoord2d(0.0, 0.0);
    glVertex3d(-w/2, -h/2, 0);
    glNormal3d(0.0, 0.0, -1.0);
    glTexCoord2d(1.0, 0.0);
    glVertex3d( w/2, -h/2, 0);
    glNormal3d(0.0, 0.0, -1.0);
    glTexCoord2d(1.0, 1.0);
    glVertex3d( w/2,  h/2, 0);
    glNormal3d(0.0, 0.0, -1.0);
    glTexCoord2d(0.0, 1.0);
    glVertex3d(-w/2,  h/2, 0);
    glNormal3d(0.0, 0.0, -1.0);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_NORMALIZE);
    glError();
}


void CMap3D::draw3DMap()
{
    QSize   s = theMap->getSize();
    double  w = s.width();
    double  h = s.height();

    double step = quality3D;

    int xcount, ycount;
    // increment xcount, because the number of points are on one more
    // than number of lengths |--|--|--|--|
    if (theMap->getFastDrawFlag())
    {
        xcount = (w / (step * 10.0) + 1);
        ycount = (h / (step * 10.0) + 1);

    }
    else
    {
        xcount = (w / step + 1);
        ycount = (h / step + 1);
    }

    QVector<float> eleData(xcount * ycount);
    bool ok = getEleRegion(eleData, xcount, ycount);

    if (!ok)
    {
        qDebug() << "can't get elevation data";
        qDebug() << "draw flat map";
        act3DMap->setChecked(false);
        drawFlatMap();
        return;
    }

    // getEleRegion() might have changed xcount and ycount
    double current_step_x = w / (double) (xcount - 1);
    double current_step_y = h / (double) (ycount - 1);

    int ix=0, iy, iv, it, j, k;
    double x, y, u, v;
    GLuint idx[4];

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnable(GL_NORMALIZE);
    glEnable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, mapTextureId);
    /*
     * next code can be more optimal if used array of coordinates or VBO
     */
    QVector<GLdouble> vertices(xcount * 3 * 2);
    QVector<GLdouble> normals(xcount * 3 * 2);
    QVector<GLdouble> texCoords(xcount * 2 * 2);
    it = 0;
    iv = 0;
    idx[0] = 0 + xcount;
    idx[1] = 1 + xcount;
    idx[2] = 1;
    idx[3] = 0;
    glVertexPointer(3, GL_DOUBLE, 0, vertices.data());
    glTexCoordPointer(2, GL_DOUBLE, 0, texCoords.data());
    glNormalPointer(GL_DOUBLE, 0, normals.data());
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
            getPoint(a, ix + s, iy + s , ix, iy, xcount, ycount, current_step_x, current_step_y, eleData.data());
            getPoint(b, ix - s, iy - s, ix, iy, xcount, ycount, current_step_x, current_step_y, eleData.data());
            getPoint(c, ix + s, iy - s, ix, iy, xcount, ycount, current_step_x, current_step_y, eleData.data());
            getNormal(a, b, c, &normals[iv]);
        }

        for (j = 0; j < 4; j++)
        {
            idx[j] = idx[j] % (xcount * 2);
        }

        if (iy == 0)
        {
            continue;
        }

        for (k = 0; k < xcount - 1; k ++)
        {
            glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, idx);
            for (j = 0; j < 4; j++)
            {
                idx[j]++;
            }
        }
        for (j = 0; j < 4; j++)
        {
            idx[j]++;
        }
    }

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_NORMALIZE);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glError();
}


void CMap3D::quadTexture(GLdouble x, GLdouble y, GLdouble xsize, GLdouble ysize, GLdouble z, GLint texture, bool isMask)
{
    glPushMatrix();

    double m = isMask ? +0.1 : 0;

    glTranslated(x, y, z);
    glRotatef(-zRotation, 0,0,1);
    glScalef(1.0, 1.0, 2.0/(zoomFactorZ*zoomFactorEle));

    glBindTexture(GL_TEXTURE_2D, texture);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 1); glVertex3d(0, m, xsize );
    glTexCoord2f(0, 0); glVertex3d(0, m, 0);
    glTexCoord2f(1, 0); glVertex3d(ysize, m, 0);
    glTexCoord2f(1, 1); glVertex3d(ysize, m, xsize );
    glEnd();
    glPopMatrix();
}


void CMap3D::drawWaypoints()
{

    const double wsize = 5;

    GLint iconId, iconMaskId, textId, textMaskId;

    glPushMatrix();

    glEnable(GL_ALPHA_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glColor4f(0,0,0,0);

    glScalef(1.0, 1.0, zoomFactorZ);
    glTranslated(0.0, 0.0, -minEle);

    const QMap<QString,CWpt*>& wpts = CWptDB::self().getWpts();
    QMap<QString,CWpt*>::const_iterator wpt  = wpts.begin();
    while(wpt != wpts.end())
    {
        double u,v, u1, v1, ele = 0;
        QPixmap icon = (*wpt)->getIcon();

        u1 = u = (*wpt)->lon * DEG_TO_RAD;
        v1 = v = (*wpt)->lat * DEG_TO_RAD;
        theMap->convertRad2Pt(u, v);

        if (u < 0 || u > xsize)
        {
            ++wpt;
            continue;
        }
        if (v < 0 || v > ysize)
        {
            ++wpt;
            continue;
        }

        IMap& dem = CMapDB::self().getDEM();
        if (act3DMap->isChecked())
        {
            ele = dem.getElevation(u1, v1);
        }
        else
        {
            ele = minEle;

        }
        ele += 5 / (zoomFactorZ*zoomFactorEle);

        convertPt23D(u,v,ele);

        QFont f = CResources::self().getMapFont();
        QFontMetrics fm(f);
        QRect r = fm.boundingRect((*wpt)->getName());
        QPixmap text(r.width() + 4, r.height() + 2);
        text.fill(Qt::transparent);
        QPainter p(&text);
        CCanvas::drawText((*wpt)->getName(),p,text.rect().adjusted(2,1,-1,-1));
        p.end();

        text.save("text.png");

        double tw = wsize;
        double th = wsize * double(text.width())/double(text.height());

        glBlendFunc(GL_DST_COLOR,GL_ZERO);

#ifndef QK_QT5_PORT
        iconMaskId  = bindTexture(icon.alphaChannel().createMaskFromColor(Qt::black));
#endif
        iconId      = bindTexture(icon);
#ifndef QK_QT5_PORT
        textMaskId  = bindTexture(text.alphaChannel().createMaskFromColor(Qt::black));
#endif
        textId      = bindTexture(text);

#ifndef QK_QT5_PORT
        quadTexture(u, v, wsize, wsize, ele, iconMaskId, true);
        quadTexture(u, v, tw, th, ele + icon.height() / (zoomFactorZ*zoomFactorEle), textMaskId, true);
#endif

        glBlendFunc(GL_ONE, GL_ONE);

        quadTexture(u, v, wsize, wsize, ele, iconId, false);
        quadTexture(u, v, tw, th, ele + icon.height() / (zoomFactorZ*zoomFactorEle), textId, false);
#ifndef QK_QT5_PORT
        deleteTexture(iconMaskId);
#endif
        deleteTexture(iconId);
#ifndef QK_QT5_PORT
        deleteTexture(textMaskId);
#endif
        deleteTexture(textId);

        ++wpt;
    }

    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}


void CMap3D::drawCompass(QPainter& p)
{
    QFont f1 = CResources::self().getMapFont();
    QFont f2 = f1;
    f2.setPointSize(f1.pointSize() + 3);
    int textOff = QFontMetrics(f2).height();

    p.save();
    p.translate(rect().center().x(), height() - 100);

    QPolygon arrow1, arrow2;
    arrow1 << QPoint(-4,15) << QPoint(0,30) << QPoint(4,15);
    arrow2 << QPoint(-4,75) << QPoint(0,60) << QPoint(4,75);

    p.setPen(pen0);
    p.drawPolygon(arrow1);
    p.drawPolygon(arrow2);
    p.setPen(pen1);
    p.drawPolygon(arrow1);
    p.drawPolygon(arrow2);
    p.setPen(pen2);
    p.setBrush(Qt::yellow);
    p.drawPolygon(arrow1);
    p.drawPolygon(arrow2);

    double scale  = width() /(3 * APPERTURE_ANGLE);
    double offset = zRotation + angleNorth;

    for(int deg = floor(zRotation - APPERTURE_ANGLE/2); deg <= ceil(zRotation + APPERTURE_ANGLE/2); ++deg)
    {
        QString str;

        int x = (deg - offset) * scale;

        if((deg % 10) == 0)
        {
            p.setPen(pen0);
            p.drawLine(x,35,x,50);
            p.setPen(pen1);
            p.drawLine(x,35,x,50);
            p.setPen(pen2);
            p.drawLine(x,35,x,50);
            str = QString("%1%2").arg(deg < 0 ? 360 + deg : deg > 360 ? deg - 360 : deg).arg(QChar(0260));
            p.setFont(f1);
        }
        else if((deg % 5) == 0)
        {
            p.setPen(pen0);
            p.drawLine(x,40,x,50);
            p.setPen(pen1);
            p.drawLine(x,40,x,50);
            p.setPen(pen2);
            p.drawLine(x,40,x,50);

        }
        else
        {
            p.setPen(pen0);
            p.drawLine(x,47,x,50);
            p.setPen(pen1);
            p.drawLine(x,47,x,50);
            p.setPen(pen2);
            p.drawLine(x,47,x,50);
        }

        if(deg == 0 || deg == 360)
        {
            str = "N";
            p.setFont(f2);
        }
        if(deg == 45)
        {
            str = "NO";
            p.setFont(f2);
        }
        else if(deg == 90)
        {
            str = "O";
            p.setFont(f2);
        }
        else if(deg == 135)
        {
            str = "SO";
            p.setFont(f2);
        }
        else if(deg == 180)
        {
            str = "S";
            p.setFont(f2);
        }
        else if(deg == 225)
        {
            str = "SW";
            p.setFont(f2);
        }
        else if(deg == 270)
        {
            str = "W";
            p.setFont(f2);
        }
        else if(deg == 315)
        {
            str = "NW";
            p.setFont(f2);
        }

        if(!str.isEmpty())
        {
            CCanvas::drawText(str,p, QPoint(x,50 + 10 + textOff), Qt::darkBlue, p.font());
        }

    }

    p.restore();
}


#define ELE_RANGE 500.0
void CMap3D::drawElevation(QPainter& p)
{
    if(act3DMap->isChecked() == false) return;

    double elevation = minEle + zpos / (zoomFactorZ * zoomFactorEle * zoomFactor);
    if(isinf(elevation)) return;

    QFont f1 = CResources::self().getMapFont();
    int textOff = QFontMetrics(f1).descent() + QFontMetrics(f1).height()/2;

    p.save();
    p.translate(width() - 100, rect().center().y());

    QPolygon arrow1, arrow2;
    arrow1 << QPoint(15, -4) << QPoint(30, 0) << QPoint(15, 4);
    arrow2 << QPoint(75, -4) << QPoint(60, 0) << QPoint(75, 4);

    p.setPen(pen0);
    p.drawPolygon(arrow1);
    p.drawPolygon(arrow2);
    p.setPen(pen1);
    p.drawPolygon(arrow1);
    p.drawPolygon(arrow2);
    p.setPen(pen2);
    p.setBrush(Qt::yellow);
    p.drawPolygon(arrow1);
    p.drawPolygon(arrow2);

    double scale  = height() /(3 * ELE_RANGE);
    for(int ele = floor(elevation - ELE_RANGE/2); ele <= ceil(elevation + ELE_RANGE/2); ele++)
    {
        QString str;

        int y = (elevation - ele) * scale;

        if((ele % 100) == 0)
        {
            p.setPen(pen0);
            p.drawLine(35,y,50,y);
            p.setPen(pen1);
            p.drawLine(35,y,50,y);
            p.setPen(pen2);
            p.drawLine(35,y,50,y);
            str = QString("%1m").arg(ele);
            p.setFont(f1);
        }
        else if((ele % 50) == 0)
        {
            p.setPen(pen0);
            p.drawLine(40,y,50,y);
            p.setPen(pen1);
            p.drawLine(40,y,50,y);
            p.setPen(pen2);
            p.drawLine(40,y,50,y);
        }
        else if((ele % 10) == 0)
        {
            p.setPen(pen0);
            p.drawLine(47,y,50,y);
            p.setPen(pen1);
            p.drawLine(47,y,50,y);
            p.setPen(pen2);
            p.drawLine(47,y,50,y);
        }

        if(!str.isEmpty())
        {
            CCanvas::drawText(str,p, QPoint(0, y + textOff), Qt::darkBlue, p.font());
        }

    }

    p.restore();
}


void CMap3D::drawHorizont(QPainter& p)
{
    double horizont = xRotation;

    QFont f1 = CResources::self().getMapFont();
    int textOff = QFontMetrics(f1).descent() + QFontMetrics(f1).height()/2;

    p.save();
    p.translate(0, rect().center().y());
    double scale  = - height() /(3 * 180.0);

    for(int deg = -90; deg <= 90; ++deg)
    {
        QString str;
        int y = deg * scale;

        if((deg % 90) == 0)
        {
            p.setPen(pen0);
            p.drawLine(30,y,60,y);
            p.setPen(pen1);
            p.drawLine(30,y,60,y);
            p.setPen(pen2);
            p.drawLine(30,y,60,y);
        }
        else if((deg % 10) == 0)
        {
            p.setPen(pen0);
            p.drawLine(40,y,50,y);
            p.setPen(pen1);
            p.drawLine(40,y,50,y);
            p.setPen(pen2);
            p.drawLine(40,y,50,y);
        }
        else if((deg % 5) == 0)
        {
            p.setPen(pen0);
            p.drawLine(43,y,47,y);
            p.setPen(pen1);
            p.drawLine(43,y,47,y);
            p.setPen(pen2);
            p.drawLine(43,y,47,y);

        }

        if((deg % 45) == 0)
        {
            str = QString("%1%2").arg(abs(deg)).arg(QChar(0260));
            p.setFont(f1);
            CCanvas::drawText(str,p, QPoint(80, y + textOff), Qt::darkBlue, p.font());
        }

    }

    int y = horizont * scale;
    QPolygon arrow1, arrow2;
    arrow1 << QPoint(15, y - 4) << QPoint(30, y) << QPoint(15, y + 4);
    arrow2 << QPoint(75, y - 4) << QPoint(60, y) << QPoint(75, y + 4);

    p.setPen(pen0);
    p.drawPolygon(arrow1);
    p.drawPolygon(arrow2);
    p.setPen(pen1);
    p.drawPolygon(arrow1);
    p.drawPolygon(arrow2);
    p.setPen(pen2);
    p.setBrush(Qt::yellow);
    p.drawPolygon(arrow1);
    p.drawPolygon(arrow2);

    p.restore();
}


void CMap3D::mousePressEvent(QMouseEvent *e)
{
    mousePos = e->pos();

    if (e->buttons() & Qt::LeftButton)
    {
        if(!actFPVMode->isChecked())
        {
            //            qDebug() << ">>>>>>>>>"  << zRotation << xpos << ypos;
            double diff = 0;
            if(zRotation > 180)
            {
                diff = (zRotation - 180)* 2;
            }

            zRotation = atan(ypos/xpos)/M_PI * 180;

            //            qDebug() << "---------"  << zRotation << xpos << ypos;

            if(xpos < 0)
            {
                zRotation = -zRotation;
            }

            zRotation += 90 + diff;

            //            qDebug() << "<<<<<<<<<"  << zRotation << xpos << ypos;

            double r = sqrt(xpos*xpos + ypos*ypos);
            xpos = -r*sin(zRotation/180 * M_PI);
            ypos = -r*cos(zRotation/180 * M_PI);

            update();
        }
    }
    lastPos = mousePos;
}


void CMap3D::mouseMoveEvent(QMouseEvent *event)
{
    mousePos = event->pos();

    if (event->buttons() & Qt::LeftButton)
    {
        if(keyLPressed)
        {
            double x0, y0, z0;
            double x1, y1, z1;
            x0 = event->x();
            y0 = event->y();
            convertMouse23D(x0, y0, z0);
            x1 = lastPos.x();
            y1 = lastPos.y();
            convertMouse23D(x1, y1, z1);

            double z = zLight + minEle;
            xLight -= (x0 / (z -z0) * z - x1 / (z -z1) * z) * 10;
            yLight -= (y0 / (z -z0) * z - y1 / (z -z1) * z) * 10;

        }
        else
        {
            QPoint diff = mousePos - lastPos;
                                 //set the xrot to xrot with the addition of the difference in the y position
            xRotation = xRotation + (double) diff.y() * 0.3;
            if(xRotation >  90.0) xRotation =  90.0;
            if(xRotation < -90.0) xRotation = -90.0;

                                 //set the xrot to yrot with the addition of the difference in the x position
            zRotation = normalizeAngle(zRotation + (double) diff.x() * 0.3);

            if(!actFPVMode->isChecked())
            {
                double r = sqrt(xpos*xpos + ypos*ypos);
                xpos = -r*sin(zRotation/180 * M_PI);
                ypos = -r*cos(zRotation/180 * M_PI);
            }
        }
        update();
    }

    lastPos = mousePos;

}


void CMap3D::wheelEvent ( QWheelEvent * e )
{
    bool in = CResources::self().flipMouseWheel() ? (e->delta() > 0) : (e->delta() < 0);

    if(keyShiftPressed)
    {
        if (in)
        {
            zoomFactorEle *= 1.1;
        }
        else
        {
            zoomFactorEle /= 1.1;
        }
    }
    else
    {
        if (in)
        {
            zoomFactor *= 1.1;
        }
        else
        {
            zoomFactor /= 1.1;
        }
    }
    update();
}


void CMap3D::keyPressEvent ( QKeyEvent * e )
{
    bool changePOV2Track = false;

    qDebug() << hex << e->key();

    switch (e->key())
    {
        case Qt::Key_W:
        {
            if(e->modifiers() & Qt::ShiftModifier)
            {
                zpos += 1;
            }
            else if(!theTrack.isNull() && actTrackMode->isChecked())
            {
                trkPointIndex++;
                changePOV2Track = true;
            }
            else
            {
                double zRotRad = (zRotation / 180 * M_PI);
                xpos += sin(zRotRad) * 4;
                ypos += cos(zRotRad) * 4;

                if(coupleElePOV)
                {
                    double xRotRad = (xRotation / 180 * M_PI);
                    zpos -= sin(xRotRad) * 4;
                    adjustElevationToMap();
                }
            }
            break;
        }

        case Qt::Key_S:
        {
            if(e->modifiers() & Qt::ShiftModifier)
            {
                zpos -= 1;
            }
            else if(!theTrack.isNull() && actTrackMode->isChecked())
            {
                trkPointIndex--;
                changePOV2Track = true;
            }
            else
            {
                double zRotRad = (zRotation / 180 * M_PI);
                xpos -= sin(zRotRad) * 4;
                ypos -= cos(zRotRad) * 4;

                if(coupleElePOV)
                {
                    double xRotRad = (xRotation / 180 * M_PI);
                    zpos += sin(xRotRad) * 4;
                    adjustElevationToMap();
                }
            }
            break;
        }

        case Qt::Key_A:
        {
            double zRotRad = (zRotation / 180 * M_PI);
            xpos -= cos(zRotRad) * 4;
            ypos += sin(zRotRad) * 4;
            break;
        }

        case Qt::Key_D:
        {
            double zRotRad = (zRotation / 180 * M_PI);
            xpos += cos(zRotRad) * 4;
            ypos -= sin(zRotRad) * 4;
            break;
        }

        case Qt::Key_Asterisk:
            zoomFactorEle *= 1.1;
            break;

        case Qt::Key_Underscore:
            zoomFactorEle /= 1.1;
            break;

        default:
            e->ignore();
    }

    if(changePOV2Track)
    {

        QList<CTrack::pt_t>& trkpts = theTrack->getTrackPoints();
        if(trkPointIndex >= trkpts.count())
        {
            trkPointIndex = trkpts.count() - 1;
        }
        if(trkPointIndex < 0)
        {
            trkPointIndex = 0;
        }

        const CTrack::pt_t trkpt = trkpts[trkPointIndex];

        if(trkpt.azimuth != WPT_NOFLOAT)
        {
            targetZRotation = e->key() == Qt::Key_S ? trkpt.azimuth - 180.0 : trkpt.azimuth;
            if(targetZRotation < 0) targetZRotation += 360.0;

            deltaRot = fabs(targetZRotation - zRotation);
            while(deltaRot >= 360.0) deltaRot -= 360;

            if(deltaRot < 180)
            {
                stepRot     = targetZRotation > zRotation ? 1.0 : -1.0;
            }
            else
            {
                stepRot     = targetZRotation < zRotation ? 1.0 : -1.0;
                deltaRot    = 360.0 - deltaRot;
            }

            timerAnimateRotation->start();
        }

        xpos = trkpt.lon * DEG_TO_RAD;
        ypos = trkpt.lat * DEG_TO_RAD;

        double ele;
        if (actTrackOnMap->isChecked())
        {
            IMap& dem = CMapDB::self().getDEM();
            ele = dem.getElevation(xpos, ypos) + 1;
        }
        else
        {
            ele = trkpt.ele;
        }

        if(ele == WPT_NOFLOAT)
        {
            ele = trkpt.ele;
        }

        theMap->convertRad2Pt(xpos, ypos);
        convertPt23D(xpos, ypos, zpos);

        zpos = (ele + 10 - minEle) * zoomFactorZ * zoomFactorEle * zoomFactor;
    }

    update();
}


void CMap3D::contextMenuEvent(QContextMenuEvent *e)
{
    QMenu menu(this);
    menu.addAction(act3DMap);
    menu.addAction(actFPVMode);
    if(theTrack.isNull())
    {
        actTrackOnMap->setEnabled(false);
        actTrackMode->setEnabled(false);
        actTrackMode->setChecked(false);
    }
    else
    {
        actTrackOnMap->setEnabled(true);
        actTrackMode->setEnabled(true);
    }
    menu.addAction(actTrackOnMap);
    menu.addAction(actTrackMode);
    menu.addAction(actResetLight);

    menu.addAction(QIcon(":icons/iconConfig16x16"),tr("Config"), this, SLOT(slotConfig3D()));

    menu.exec(e->globalPos());
}


void CMap3D::showEvent ( QShowEvent * e )
{
    if(needsRedraw)
    {
        update();
    }
}


bool CMap3D::eventFilter(QObject *o, QEvent *e)
{
    if (e->type() == QEvent::KeyPress)
    {
        QKeyEvent * keyEvent = static_cast<QKeyEvent*>(e);
        if(keyEvent->key() == Qt::Key_Shift)
        {
            keyShiftPressed = true;
        }
        else if(keyEvent->key() == Qt::Key_L)
        {
            keyLPressed = true;
        }
        else if(keyEvent->key() == Qt::Key_Plus)
        {
            zoomFactor *= 1.1;
            e->accept();
            update();
            return true;
        }
        else if(keyEvent->key() == Qt::Key_Minus)
        {
            zoomFactor /= 1.1;
            e->accept();
            update();
            return true;
        }

    }
    else if (e->type() == QEvent::KeyRelease)
    {
        QKeyEvent * keyEvent = static_cast<QKeyEvent*>(e);
        if(keyEvent->key() == Qt::Key_Shift)
        {
            keyShiftPressed = false;
        }
        else if(keyEvent->key() == Qt::Key_L)
        {
            keyLPressed = false;
        }
    }

    return QGLWidget::eventFilter(o, e);
}


double CMap3D::normalizeAngle(double angle)
{
    while (angle < 0)
    {
        angle += 360;
    }

    while (angle > 360)
    {
        angle -= 360;
    }

    return angle;
}


bool CMap3D::getEleRegion(QVector<float>& eleData, int& xcount, int& ycount)
{
    QSize   s = theMap->getSize();
    double  w = s.width();
    double  h = s.height();

    IMap& dem = CMapDB::self().getDEM();
    projXY pen1, pen2;
    pen1.u = 0;
    pen1.v = 0;
    pen2.u = w;
    pen2.v = h;
    theMap->convertPt2Rad(pen1.u, pen1.v);
    theMap->convertPt2Rad(pen2.u, pen2.v);

    return dem.getRegion(eleData, pen1, pen2, xcount, ycount);

}


void CMap3D::convertPt23D(double& u, double& v, double &ele)
{
    QSize mapSize = theMap->getSize();
    u = u - mapSize.width()/2;
    v = mapSize.height()/2 - v;
}


void CMap3D::convert3D2Pt(double& u, double& v, double &ele)
{
    QSize mapSize = theMap->getSize();
    u = u + mapSize.width()/2;
    v = mapSize.height()/2 - v;
}


void CMap3D::getPoint(double v[], int xi, int yi, int xi0, int yi0, int xcount, int ycount, double current_step_x, double current_step_y, float *eleData)
{
    if (xi < 0)
    {
        xi = 0;
    }
    if (yi <0)
    {
        yi = 0;
    }
    if (xi >= xcount)
    {
        xi = xcount - 1;
    }
    if (yi >= ycount)
    {
        yi = ycount - 1;
    }
    v[0] = xi * current_step_x;
    v[1] = yi * current_step_y;
    v[2] = eleData[xi + yi * xcount];
    convertPt23D(v[0], v[1], v[2]);
}


void CMap3D::convertMouse23D(double &u, double& v, double &ele)
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


void CMap3D::quad(GLdouble x1, GLdouble y1, GLdouble z1, GLdouble x2, GLdouble y2, GLdouble z2)
{
    glBegin(GL_QUADS);
    double c1, c2;
    // compute colors
    c1 = z1 / maxEle * 255;
    c2 = z2 / maxEle * 255;

    qglColor(wallCollor);
    glVertex3d(x2, y2, minEle);
    glVertex3d(x1, y1, minEle);
    qglColor(wallCollor.dark(c1));
    glVertex3d(x1, y1, z1);
    qglColor(wallCollor.dark(c2));
    glVertex3d(x2, y2, z2);

    qglColor(wallCollor.dark(c2));
    glVertex3d(x2, y2, z2);
    qglColor(wallCollor.dark(c1));
    glVertex3d(x1, y1, z1);
    qglColor(wallCollor);
    glVertex3d(x1, y1, minEle);
    glVertex3d(x2, y2, minEle);

    glEnd();

    glBegin(GL_LINES);
    qglColor(highBorderColor);
    glVertex3d(x1, y1, z1);
    glVertex3d(x2, y2, z2);
    glEnd();
}


void CMap3D::adjustElevationToMap()
{
    float e1 = minEle + zpos / (zoomFactorZ * zoomFactorEle * zoomFactor);
    if(!isinf(e1))
    {
        double x = xpos;
        double y = ypos;
        double z = zpos;

        convert3D2Pt(x,y,z);
        theMap->convertPt2Rad(x,y);

        float e2 = CMapDB::self().getDEM().getElevation(x,y);
        if(e2 > e1 && !isnan(e2) && e2 != WPT_NOFLOAT)
        {
            zpos = (e2 + 10 - minEle) * (zoomFactorZ * zoomFactorEle * zoomFactor);
        }
    }
}
