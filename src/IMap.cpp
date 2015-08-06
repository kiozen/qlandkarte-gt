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

#include "IMap.h"
#include "CWpt.h"
#include "CCanvas.h"
#include "CMapDB.h"
#include "CMapDEM.h"
#include "IMapSelection.h"
#include "GeoMath.h"
#include "CSettings.h"

#include <QtGui>
#include <QMessageBox>
#include <math.h>

double IMap::midU = 0;
double IMap::midV = 0;

IMap::IMap(maptype_e type, const QString& key, CCanvas * parent)
: QObject(parent)
, maptype(type)
, zoomidx(1)
, pjsrc(0)
, pjtar(0)
, needsRedraw(true)
, key(key)
, doFastDraw(false)
, angleNorth(0)
, fastDrawWithoutTimer(false)
{
    pjtar   = pj_init_plus("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");

    SETTINGS;
    zoomidx = cfg.value("map/zoom",zoomidx).toUInt();

    if(parent)
    {
        resize(parent->size());
        connect(parent, SIGNAL(sigResize(const QSize&)), this , SLOT(resize(const QSize&)));
        parent->update();
    }

    timerFastDraw = new QTimer(this);
    timerFastDraw->setSingleShot(true);
    connect(timerFastDraw, SIGNAL(timeout()), this, SLOT(slotResetFastDraw()));

}


IMap::~IMap()
{
    qDebug() << "IMap::~IMap()";

    if(pjtar) pj_free(pjtar);

    if(ovlMap)
    {
        delete ovlMap;
    }

    SETTINGS;
    cfg.setValue("map/zoom",zoomidx);
}


GDALDataset * IMap::getDataset()
{
    return 0;
}


void IMap::resize(const QSize& s)
{
    size = s;
    rect.setSize(s);

    if(!isThread())
    {
        pixBuffer = QPixmap(size);
    }
    imgBuffer = QImage(size,QImage::Format_ARGB32_Premultiplied);

    needsRedraw = true;
    emit sigResize(s);

    setAngleNorth();
}


void IMap::setAngleNorth()
{
    projXY p1,p2;
    double a1 = 0, a2 = 0;
    p2.u = p1.u = rect.right()  - COMPASS_OFFSET_X - COMPASS_W / 2;
    p2.v = p1.v = rect.bottom() - COMPASS_OFFSET_Y;
    p2.v -= COMPASS_H;
    convertPt2Rad(p1.u, p1.v);
    convertPt2Rad(p2.u, p2.v);
    distance(p1, p2, a1, a2);
    angleNorth = a1;
}


void IMap::draw(QPainter& p)
{
    p.fillRect(rect,QColor("#ffffcc"));
    p.drawText(rect,Qt::AlignCenter,"no map");
    needsRedraw = false;
}


void IMap::draw()
{
    QPainter p(isThread() ? (QPaintDevice*)&imgBuffer : (QPaintDevice*)&pixBuffer);

    p.fillRect(rect,QColor("#ffffcc"));
    p.drawText(rect,Qt::AlignCenter,"no map");
    needsRedraw = false;
}


void IMap::convertPt2Rad(double& u, double& v)
{
    if(pjsrc == 0)
    {
        //         u = v = 0;
        return;
    }
    convertPt2M(u,v);

    projXY pt;
    pt.u = u;
    pt.v = v;

    pj_transform(pjsrc,pjtar,1,0,&pt.u,&pt.v,0);

    u = pt.u;
    v = pt.v;
}


void IMap::convertRad2Pt(double& u, double& v)
{
    if(pjsrc == 0)
    {
        return;
    }

    projXY pt;
    pt.u = u;
    pt.v = v;

    pj_transform(pjtar,pjsrc,1,0,&pt.u,&pt.v,0);

    u = pt.u;
    v = pt.v;

    convertM2Pt(u,v);
}


void IMap::convertRad2M(double& u, double& v)
{
    if(pjsrc == 0)
    {
        return;
    }

    pj_transform(pjtar,pjsrc,1,0,&u,&v,0);
}


void IMap::convertM2Rad(double& u, double& v)
{
    if(pjsrc == 0)
    {
        return;
    }

    pj_transform(pjsrc,pjtar,1,0,&u,&v,0);
}


float IMap::getElevation(double lon, double lat)
{
    return WPT_NOFLOAT;
}


void IMap::getArea_n_Scaling_fromBase(projXY& p1, projXY& p2, float& my_xscale, float& my_yscale)
{
    CMapDB::self().getMap().getArea_n_Scaling(p1,p2,my_xscale,my_yscale);
}


static char nullstr[1] = "";
char * IMap::getProjection()
{
    if(pjsrc == 0)
    {
        return nullstr;
    }
    return pj_get_def(pjsrc,0);
}


void IMap::registerDEM(CMapDEM& dem)
{
    if(pjsrc == 0)
    {
        dem.deleteLater();
        throw tr("No basemap projection. That shouldn't happen.");
    }

    char * ptr1 = 0, *ptr2 = 0;
    QString proj1 = ptr1 = pj_get_def(pjsrc,0);
    QString proj2 = ptr2 = dem.getProjection();
    //     if(ptr1) free(ptr1);
    //     if(ptr2) free(ptr2);

    SETTINGS;
    bool ignoreWarning = cfg.value(QString("map/dem/%1/ignoreWarning").arg(getKey()), false).toBool();

    if(proj1 != proj2)
    {
        if(!OSRIsSame(&oSRS, &dem.getOSrs()) && !ignoreWarning)
        {
            QString msg = tr("DEM projection does not match the projection of the basemap.\n\nMap: %1\n\nDEM: %2\n\nIn my point of view this is bad. But if you think I am wrong just go on with 'Apply'. Else abort operation.").arg(proj1).arg(proj2);
            QMessageBox::StandardButton res = QMessageBox:: critical(0,tr("Error..."), msg, QMessageBox::Abort|QMessageBox::Apply, QMessageBox::Abort);

            if(res == QMessageBox::Abort)
            {
                dem.deleteLater();
                throw tr("DEM projection does not match the projection of the basemap.\n\nMap: %1\n\nDEM: %2").arg(proj1).arg(proj2);
            }
            cfg.setValue(QString("map/dem/%1/ignoreWarning").arg(getKey()), true);

        }
    }
}


void IMap::addOverlayMap(const QString& k)
{
    // prevent registering twice
    if(key == k)
    {
        return;
    }

    needsRedraw = true;

    // pass request to next overlay map
    if(!ovlMap.isNull())
    {
        ovlMap->addOverlayMap(k);
        emit sigChanged();
        return;
    }

    // add overlay to last overlay map
    ovlMap = CMapDB::self().createMap(k);
    connect(ovlMap, SIGNAL(sigChanged()), this, SLOT(slotOvlChanged()));
    emit sigChanged();
}


void IMap::slotOvlChanged()
{
    needsRedraw = true;
    emit sigChanged();
}


void IMap::delOverlayMap(const QString& k)
{
    if(ovlMap.isNull()) return;

    if(ovlMap->getKey() != k)
    {
        ovlMap->delOverlayMap(k);
        emit sigChanged();
        return;
    }

    ovlMap->deleteLater();
    ovlMap = ovlMap->ovlMap;
    emit sigChanged();
}


bool IMap::hasOverlayMap(const QString& k)
{

    if(ovlMap.isNull()) return (k == key);

    if(key != k)
    {
        return ovlMap->hasOverlayMap(k);
    }

    return true;
}


void IMap::setFastDrawTimer()
{
    if(fastDrawWithoutTimer)
    {
        return;
    }
    timerFastDraw->start(300);
    doFastDraw = true;
}


void IMap::slotResetFastDraw()
{
    needsRedraw = true;
    doFastDraw  = false;
    emit sigChanged();
}


void IMap::fastDrawOn()
{
    doFastDraw = true;
    fastDrawWithoutTimer = true;
}


void IMap::fastDrawOff()
{
    needsRedraw = true;
    doFastDraw  = false;
    fastDrawWithoutTimer = false;
    emit sigChanged();
}


bool IMap::isLonLat()
{
    if(pjsrc)
    {
        return pj_is_latlong(pjsrc);
    }
    return true;
}


void IMap::select(IMapSelection& ms, const QRect& rect)
{
    throw tr("This map does not support this feature.");
}


void IMap::incXOffset(int i)
{
    QMessageBox::warning(0, tr("Error..."),tr("Changing the offset is not supported by this map."),QMessageBox::Abort,QMessageBox::Abort);
}


void IMap::decXOffset(int i)
{
    QMessageBox::warning(0, tr("Error..."),tr("Changing the offset is not supported by this map."),QMessageBox::Abort,QMessageBox::Abort);
}


void IMap::incYOffset(int i)
{
    QMessageBox::warning(0, tr("Error..."),tr("Changing the offset is not supported by this map."),QMessageBox::Abort,QMessageBox::Abort);
}


void IMap::decYOffset(int i)
{
    QMessageBox::warning(0, tr("Error..."),tr("Changing the offset is not supported by this map."),QMessageBox::Abort,QMessageBox::Abort);
}


void IMap::getClosePolyline(QPoint& pt1, QPoint& pt2, qint32 threshold, QPolygon& line)
{
    line.clear();
    if(!ovlMap.isNull())
    {
        ovlMap->getClosePolyline(pt1, pt2, threshold, line);
    }
}


const QImage& IMap::getBuffer()
{
    if(!isThread())
    {
        imgBuffer = pixBuffer.toImage();
    }

    return imgBuffer;
}
