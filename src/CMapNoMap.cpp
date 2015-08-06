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

#include "CMapNoMap.h"
#include "CMainWindow.h"
#include "CDlgNoMapConfig.h"
#include "CSettings.h"

#include <QtGui>
#include <QCheckBox>

CMapNoMap::CMapNoMap(CCanvas * parent)
: IMap(eNoMap, "NoMap", parent)
, xscale( 1.0)
, yscale(-1.0)
, x(0)
, y(0)
, zoomFactor(1.0)
, quadraticZoom(0)
{
    quadraticZoom = theMainWindow->getCheckBoxQuadraticZoom();

    SETTINGS;
    QString proj = cfg.value("map/nomap/proj", "+proj=merc +a=6378137.0000 +b=6356752.3142 +towgs84=0,0,0,0,0,0,0,0 +units=m  +no_defs").toString();
    setup(proj, cfg.value("map/nomap/xscale", xscale).toDouble(), cfg.value("map/nomap/yscale", yscale).toDouble());
}


CMapNoMap::~CMapNoMap()
{
    SETTINGS;
    cfg.setValue("map/nomap/proj", getProjection());
    cfg.setValue("map/nomap/xscale", xscale);
    cfg.setValue("map/nomap/yscale", yscale);

    midU = rect.center().x();
    midV = rect.center().y();
    convertPt2Rad(midU, midV);

    if(pjsrc) pj_free(pjsrc);
}


void CMapNoMap::setup(const QString& proj, double xscale, double yscale)
{
    if(pjsrc) pj_free(pjsrc);

    pjsrc   = pj_init_plus(proj.toLatin1().data());
    oSRS.importFromProj4(getProjection());
    this->xscale = xscale;
    this->yscale = yscale;

    setAngleNorth();

    emit sigChanged();
}


void CMapNoMap::convertPt2M(double& u, double& v)
{
    u = x + u * xscale * zoomFactor;
    v = y + v * yscale * zoomFactor;
}


void CMapNoMap::convertM2Pt(double& u, double& v)
{
    u = floor((u - x) / (xscale * zoomFactor) + 0.5);
    v = floor((v - y) / (yscale * zoomFactor) + 0.5);
}


void CMapNoMap::move(const QPoint& old, const QPoint& next)
{
    double xx = x, yy = y;
    convertM2Pt(xx, yy);

    // move top left point by difference
    xx += old.x() - next.x();
    yy += old.y() - next.y();

    convertPt2M(xx,yy);
    x = xx;
    y = yy;
    needsRedraw = true;

    setAngleNorth();

    emit sigChanged();
}


void CMapNoMap::zoom(bool zoomIn, const QPoint& p0)
{
    projXY p1;

    // convert point to geo. coordinates
    p1.u = p0.x();
    p1.v = p0.y();
    convertPt2Rad(p1.u, p1.v);

    if(quadraticZoom->isChecked())
    {

        if(zoomidx > 1)
        {
            zoomidx = pow(2.0, ceil(log(zoomidx*1.0)/log(2.0)));
            zoomidx = zoomIn ? (zoomidx>>1) : (zoomidx<<1);
        }
        else
        {
            zoomidx += zoomIn ? -1 : 1;
        }
    }
    else
    {
        zoomidx += zoomIn ? -1 : 1;
    }

    // sigChanged will be sent at the end of this function
    blockSignals(true);
    zoom(zoomidx);

    // convert geo. coordinates back to point
    convertRad2Pt(p1.u, p1.v);

    double xx = x, yy = y;
    convertM2Pt(xx, yy);

    // move top left point by difference point befor and after zoom
    xx += p1.u - p0.x();
    yy += p1.v - p0.y();

    // convert back to new top left geo coordinate
    convertPt2M(xx, yy);
    x = xx;
    y = yy;

    needsRedraw = true;
    blockSignals(false);
    emit sigChanged();
}


void CMapNoMap::zoom(qint32& level)
{
    // no level less than 1
    if(level < 1)
    {
        zoomFactor  = 1.0 / - (level - 2);
        qDebug() << "zoom:" << zoomFactor;
        return;
    }
    zoomFactor = level;

    needsRedraw = true;
    emit sigChanged();

}


void CMapNoMap::zoom(double lon1, double lat1, double lon2, double lat2)
{
    double u[3];
    double v[3];
    double dU, dV;

    u[0] = lon1;
    v[0] = lat1;
    u[1] = lon2;
    v[1] = lat1;
    u[2] = lon1;
    v[2] = lat2;

    pj_transform(pjtar, pjsrc,3,0,u,v,0);
    dU = u[1] - u[0];
    dV = v[0] - v[2];

    int z1 = dU / size.width();
    int z2 = dV / size.height();

    zoomFactor = (z1 > z2 ? z1 : z2)  + 1;
    if(quadraticZoom->isChecked())
    {
        zoomFactor = zoomidx = pow(2.0, ceil(log(zoomFactor)/log(2.0)));
    }
    else
    {
        zoomidx = zoomFactor;
    }

    double u_ = lon1 + (lon2 - lon1)/2;
    double v_ = lat1 + (lat2 - lat1)/2;
    convertRad2Pt(u_,v_);
    move(QPoint(u_,v_), rect.center());

    needsRedraw = true;
    emit sigChanged();

    qDebug() << "zoom:" << zoomFactor;
}


void CMapNoMap::dimensions(double& lon1, double& lat1, double& lon2, double& lat2)
{
    lon1 = -180 * DEG_TO_RAD;
    lon2 =  180 * DEG_TO_RAD;
    lat1 =   90 * DEG_TO_RAD;
    lat2 =  -90 * DEG_TO_RAD;
}


void CMapNoMap::config()
{
    CDlgNoMapConfig dlg(*this);
    dlg.exec();
}
