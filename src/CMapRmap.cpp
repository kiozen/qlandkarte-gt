/**********************************************************************************************
    Copyright (C) 2012 Oliver Eichler oliver.eichler@gmx.de

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
#include "CMapRmap.h"
#include "CMainWindow.h"
#include "CResources.h"
#include "CCanvas.h"
#include "CSettings.h"

#include <QtGui>
#include <QMessageBox>
#include <QCheckBox>

CMapRmap::CMapRmap(const QString &key, const QString &fn, CCanvas *parent)
: IMap(eRaster,key,parent)
, zoomFactor(0.0)
, needsRedrawOvl(true)
{
    filename = fn;

    quadraticZoom = theMainWindow->getCheckBoxQuadraticZoom();

    QFileInfo fi(fn);
    name = fi.baseName();

    QFile file(filename);
    file.open(QIODevice::ReadOnly);

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    QByteArray charbuf(20,0);
    stream.readRawData(charbuf.data(), 19);

    if("CompeGPSRasterImage" != QString(charbuf))
    {
        QMessageBox::warning(0, tr("Error..."), tr("This is not a TwoNav RMAP file."), QMessageBox::Abort, QMessageBox::Abort);
        return;
    }

    quint32 tag1, tag2, tmp32;
    stream >> tag1 >> tag2 >> tmp32;

    if(tag1 != 10 || tag2 != 7)
    {
        QMessageBox::warning(0, tr("Error..."), tr("Unknown sub-format."), QMessageBox::Abort, QMessageBox::Abort);
        return;
    }

    stream >> xsize_px >> ysize_px;
    stream >> tmp32 >> tmp32;
    stream >> blockSizeX >> blockSizeY;

    ysize_px = -ysize_px;

    quint64 mapDataOffset;
    stream >> mapDataOffset;
    stream >> tmp32;

    qint32 nZoomLevels;
    stream >> nZoomLevels;

    for(int i=0; i < nZoomLevels; i++)
    {
        level_t level;
        stream >> level.offsetLevel;
        levels << level;
    }

    for(int i=0; i<levels.size(); i++)
    {
        level_t& level = levels[i];
        file.seek(level.offsetLevel);

        stream >> level.width;
        stream >> level.height;
        stream >> level.xTiles;
        stream >> level.yTiles;

        for(int j=0; j<(level.xTiles * level.yTiles); j++)
        {
            quint64 offset;
            stream >> offset;
            level.offsetJpegs << offset;
        }
    }

    file.seek(mapDataOffset);
    stream >> tmp32 >> tmp32;

    charbuf.resize(tmp32 + 1);
    charbuf.fill(0);
    stream.readRawData(charbuf.data(), tmp32);

    QPoint p0;
    QPoint p1;
    QPoint p2;
    QPoint p3;
    projXY c0;
    projXY c1;
    projXY c2;
    projXY c3;

    bool pointsAreLongLat = true;
    QString projection;
    QString datum;
    QStringList lines = QString(charbuf).split("\r\n");
    foreach(const QString& line, lines)
    {
        if(line.startsWith("Version="))
        {
            if(line.split("=")[1] != "2")
            {
                QMessageBox::warning(0, tr("Error..."), tr("Unknown version."), QMessageBox::Abort, QMessageBox::Abort);
                return;
            }
        }
        else if(line.startsWith("Projection="))
        {
            projection = line.split("=")[1];
        }
        else if(line.startsWith("Datum="))
        {
            datum = line.split("=")[1];
        }
        else if(line.startsWith("P0="))
        {
            QStringList vals = line.split("=")[1].split(",");
            if(vals.size() < 5)
            {
                QMessageBox::warning(0, tr("Error..."), tr("Failed to read reference point."), QMessageBox::Abort, QMessageBox::Abort);
                return;
            }

            p0 = QPoint(vals[0].toInt(), vals[1].toInt());
            if(vals[2] == "A")
            {
                c0.u = vals[3].toDouble() * DEG_TO_RAD;
                c0.v = vals[4].toDouble() * DEG_TO_RAD;
            }
            else
            {
                c0.u = vals[3].toDouble();
                c0.v = vals[4].toDouble();
            }
        }
        else if(line.startsWith("P1="))
        {
            QStringList vals = line.split("=")[1].split(",");
            if(vals.size() < 5)
            {
                QMessageBox::warning(0, tr("Error..."), tr("Failed to read reference point."), QMessageBox::Abort, QMessageBox::Abort);
                return;
            }

            p1 = QPoint(vals[0].toInt(), vals[1].toInt());
            if(vals[2] == "A")
            {
                c1.u = vals[3].toDouble() * DEG_TO_RAD;
                c1.v = vals[4].toDouble() * DEG_TO_RAD;
            }
            else
            {
                pointsAreLongLat = false;
                c1.u = vals[3].toDouble();
                c1.v = vals[4].toDouble();
            }
        }
        else if(line.startsWith("P2="))
        {
            QStringList vals = line.split("=")[1].split(",");
            if(vals.size() < 5)
            {
                QMessageBox::warning(0, tr("Error..."), tr("Failed to read reference point."), QMessageBox::Abort, QMessageBox::Abort);
                return;
            }

            p2 = QPoint(vals[0].toInt(), vals[1].toInt());
            if(vals[2] == "A")
            {
                c2.u = vals[3].toDouble() * DEG_TO_RAD;
                c2.v = vals[4].toDouble() * DEG_TO_RAD;
            }
            else
            {
                pointsAreLongLat = false;
                c2.u = vals[3].toDouble();
                c2.v = vals[4].toDouble();
            }
        }
        else if(line.startsWith("P3="))
        {
            QStringList vals = line.split("=")[1].split(",");
            if(vals.size() < 5)
            {
                QMessageBox::warning(0, tr("Error..."), tr("Failed to read reference point."), QMessageBox::Abort, QMessageBox::Abort);
                return;
            }

            p3 = QPoint(vals[0].toInt(), vals[1].toInt());
            if(vals[2] == "A")
            {
                c3.u = vals[3].toDouble() * DEG_TO_RAD;
                c3.v = vals[4].toDouble() * DEG_TO_RAD;
            }
            else
            {
                pointsAreLongLat = false;
                c3.u = vals[3].toDouble();
                c3.v = vals[4].toDouble();
            }
        }
        else
        {
            //            qDebug() << line;
        }
        qDebug() << line;
    }

    if(!projection.isEmpty() && !datum.isEmpty())
    {
        if(!setProjection(projection, datum))
        {
            QMessageBox::warning(0, tr("Error..."), tr("Unknown projection and datum (%1%2).").arg(projection).arg(datum), QMessageBox::Abort, QMessageBox::Abort);
            return;
        }
    }

    if(!pj_is_latlong(pjsrc))
    {
        if(pointsAreLongLat)
        {
            pj_transform(pjtar, pjsrc, 1, 0, &c0.u, &c0.v, 0);
            pj_transform(pjtar, pjsrc, 1, 0, &c1.u, &c1.v, 0);
            pj_transform(pjtar, pjsrc, 1, 0, &c2.u, &c2.v, 0);
            pj_transform(pjtar, pjsrc, 1, 0, &c3.u, &c3.v, 0);
        }

        qDebug() << c0.u << c0.v;
        qDebug() << c1.u << c1.v;
        qDebug() << c2.u << c2.v;
        qDebug() << c3.u << c3.v;

        xref1  =  1e25;
        yref1  = -1e25;
        xref2  = -1e25;
        yref2  =  1e25;
    }
    else
    {

        xref1  =  180 * DEG_TO_RAD;
        yref1  =  -90 * DEG_TO_RAD;
        xref2  = -180 * DEG_TO_RAD;
        yref2  =   90 * DEG_TO_RAD;
    }

    if(c0.u < xref1) xref1 = c0.u;
    if(c0.u > xref2) xref2 = c0.u;
    if(c1.u < xref1) xref1 = c1.u;
    if(c1.u > xref2) xref2 = c1.u;
    if(c2.u < xref1) xref1 = c2.u;
    if(c2.u > xref2) xref2 = c2.u;

    if(c0.v > yref1) yref1 = c0.v;
    if(c0.v < yref2) yref2 = c0.v;
    if(c1.v > yref1) yref1 = c1.v;
    if(c1.v < yref2) yref2 = c1.v;
    if(c2.v > yref1) yref1 = c2.v;
    if(c2.v < yref2) yref2 = c2.v;

    xscale = (xref2 - xref1) / xsize_px;
    yscale = (yref2 - yref1) / ysize_px;

    double widthL0  = levels[0].width;
    double heightL0 = levels[0].height;

    for(int i=0; i<levels.size(); i++)
    {
        level_t& level = levels[i];

        level.xscale = xscale * widthL0  / level.width;
        level.yscale = yscale * heightL0 / level.height;

        qDebug() << i << level.xscale << level.yscale;
    }

    qDebug() << "xref1:" << xref1 << "yref1:" << yref1;
    qDebug() << "xref2:" << xref2 << "yref2:" << yref2;
    qDebug() << "map  width:" << xsize_px << "height:" << ysize_px;
    qDebug() << "tile width:" << blockSizeX << "height:" << blockSizeY;
    qDebug() << "scale x:  " << xscale << "y:" << yscale;

    x = xref1 + (xref2 - xref1);
    y = yref1 + (yref2 - yref1);

    SETTINGS;
    cfg.beginGroup("rmap/maps");
    cfg.beginGroup(getKey());

    x       = cfg.value("lon", x).toDouble();
    y       = cfg.value("lat", y).toDouble();
    zoomidx = cfg.value("zoomidx", zoomidx).toInt();

    cfg.endGroup();
    cfg.endGroup();

    zoom(zoomidx);
}


CMapRmap::~CMapRmap()
{

    midU = rect.center().x();
    midV = rect.center().y();
    convertPt2Rad(midU, midV);

    if(pjsrc) pj_free(pjsrc);

    SETTINGS;
    cfg.beginGroup("rmap/maps");
    cfg.beginGroup(getKey());

    cfg.setValue("lon", x);
    cfg.setValue("lat", y);
    cfg.setValue("zoomidx", zoomidx);

    cfg.endGroup();
    cfg.endGroup();

}


bool CMapRmap::setProjection(const QString& projection, const QString& datum)
{

    QString projstr;
    if(projection.startsWith("0,UTM"))
    {
        QStringList vals    = projection.split(",");
        int  zone           = vals[2].toInt();
        bool isSouth        = vals[3] != "N";

        projstr += QString("+proj=utm +zone=%1 %2").arg(zone).arg(isSouth ? "+south" : "");

    }
    if(projection.startsWith("1,"))
    {
        projstr += "+proj=longlat";
    }
    else if(projection.startsWith("2,"))
    {
        projstr += "+proj=merc";
    }
    else if(projection.startsWith("114,"))
    {
        projstr += "+proj=tmerc +lat_0=0 +lon_0=12 +k=1 +x_0=4500000 +y_0=0";
    }
    else if(projection.startsWith("117,"))
    {
        projstr += "+proj=tmerc +lat_0=0 +lon_0=9  +k=1 +x_0=3500000 +y_0=0";
    }

    if(datum == "WGS 84")
    {
        projstr += " +datum=WGS84 +units=m +no_defs";
    }
    else if(datum  == "Potsdam Rauenberg DHDN")
    {
        projstr += " +ellps=bessel +towgs84=606,23,413,0,0,0,0 +units=m +no_defs";
    }

    pjsrc = pj_init_plus(projstr.toLatin1().data());
    if(pjsrc == 0)
    {
        return false;
    }
    oSRS.importFromProj4(getProjection());
    char * ptr = pj_get_def(pjsrc,0);
    qDebug() << "rmap:" << ptr;

    return true;
}


void CMapRmap::convertPt2M(double& u, double& v)
{
    u = x + u * xscale * zoomFactor;
    v = y + v * yscale * zoomFactor;
}


void CMapRmap::convertM2Pt(double& u, double& v)
{
    u = floor((u - x) / (xscale * zoomFactor) + 0.5);
    v = floor((v - y) / (yscale * zoomFactor) + 0.5);
}


void CMapRmap::move(const QPoint& old, const QPoint& next)
{
    if(pjsrc == 0) return;
    needsRedraw     = true;
    needsRedrawOvl  = true;

    double xx = x, yy = y;
    convertM2Pt(xx, yy);

    // move top left point by difference
    xx += old.x() - next.x();
    yy += old.y() - next.y();

    convertPt2M(xx,yy);
    x = xx;
    y = yy;
    emit sigChanged();

    setAngleNorth();
}


void CMapRmap::zoom(bool zoomIn, const QPoint& p0)
{
    qDebug() << "zoom" << zoomIn;

    projXY p1;
    if(pjsrc == 0) return;

    needsRedraw     = true;
    needsRedrawOvl  = true;

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
    blockSignals(false);
    emit sigChanged();
}


void CMapRmap::zoom(double lon1, double lat1, double lon2, double lat2)
{
    if(pjsrc == 0) return;

    needsRedraw     = true;
    needsRedrawOvl  = true;

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
    dU = (u[1] - u[0]) / xscale;
    dV = (v[0] - v[2]) / yscale;

    int z1 = fabs(dU / size.width());
    int z2 = fabs(dV / size.height());

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

    emit sigChanged();

    qDebug() << "zoom:" << zoomFactor;
}


void CMapRmap::zoom(qint32& level)
{
    if(pjsrc == 0) return;
    needsRedraw     = true;
    needsRedrawOvl  = true;

    // no level less than 1
    if(level < 1)
    {
        zoomFactor  = 1.0 / - (level - 2);
        qDebug() << "zoom:" << zoomFactor;
        return;
    }
    zoomFactor = level;
    emit sigChanged();
    qDebug() << "zoom:" << zoomFactor;
}


void CMapRmap::dimensions(double& lon1, double& lat1, double& lon2, double& lat2)
{
    if(pjsrc == 0) return;

    lon1 = xref1;
    lat1 = yref1;
    pj_transform(pjsrc,pjtar,1,0,&lon1,&lat1,0);

    lon2 = xref2;
    lat2 = yref2;
    pj_transform(pjsrc,pjtar,1,0,&lon2,&lat2,0);

}


void CMapRmap::getArea_n_Scaling(projXY& p1, projXY& p2, float& my_xscale, float& my_yscale)
{
    if(pjsrc == 0) return;

    p1.u        = 0;
    p1.v        = 0;
    p2.u        = rect.width();
    p2.v        = rect.height();

    convertPt2Rad(p1.u, p1.v);
    convertPt2Rad(p2.u, p2.v);

    my_xscale   = xscale * zoomFactor;
    my_yscale   = yscale * zoomFactor;
}


CMapRmap::level_t& CMapRmap::findBestLevel(double sx, double sy)
{
    int i = levels.size() - 1;
    if(sx < levels[0].xscale) return levels[0];
    if(sx > levels[i].xscale) return levels[i];

    int j = 0;
    double dsx = 1e25;
    for(;j < levels.size(); j++)
    {
        level_t& level = levels[j];
        if(fabs(level.xscale - sx) < dsx)
        {
            i = j;
            dsx = fabs(level.xscale - sx);
        }
    }

    return levels[i];
}


void CMapRmap::draw(QPainter& p)
{
    if(pjsrc == 0) return IMap::draw(p);

    // render map if necessary
    if(needsRedraw)
    {
        draw();
    }

    p.drawPixmap(0,0,pixBuffer);

    // render overlay
    if(!ovlMap.isNull() && !doFastDraw)
    {
        ovlMap->draw(size, needsRedrawOvl, p);
        needsRedrawOvl = false;
    }

    needsRedraw = false;

    if(CResources::self().showZoomLevel())
    {

        QString str;
        if(zoomFactor < 1.0)
        {
            str = tr("Overzoom x%1").arg(1/zoomFactor,0,'f',0);
        }
        else
        {
            str = tr("Zoom level x%1").arg(zoomFactor);
        }

        p.setPen(Qt::white);
        p.setFont(QFont("Sans Serif",14,QFont::Black));

        p.drawText(9  ,23, str);
        p.drawText(10 ,23, str);
        p.drawText(11 ,23, str);
        p.drawText(9  ,24, str);
        p.drawText(11 ,24, str);
        p.drawText(9  ,25, str);
        p.drawText(10 ,25, str);
        p.drawText(11 ,25, str);

        p.setPen(Qt::darkBlue);
        p.drawText(10,24,str);
    }
}


void CMapRmap::draw()
{
    pixBuffer.fill(Qt::white);
    QPainter p(&pixBuffer);

    level_t& level = findBestLevel(xscale * zoomFactor, yscale * zoomFactor);

    QFile file(filename);
    file.open(QIODevice::ReadOnly);

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    // convert top left point of screen to coord. system
    double x1 = 0;
    double y1 = 0;
    convertPt2M(x1, y1);
    double x2 = size.width();
    double y2 = size.height();
    convertPt2M(x2, y2);

    int idxx1 = floor((x1 - xref1) / (level.xscale * blockSizeX));
    int idxy1 = floor((y1 - yref1) / (level.yscale * blockSizeY));
    int idxx2 =  ceil((x2 - xref1) / (level.xscale * blockSizeX));
    int idxy2 =  ceil((y2 - yref1) / (level.yscale * blockSizeY));

    if(idxx1 < 0)               idxx1 = 0;
    if(idxx1 >= level.xTiles)   idxx1 = level.xTiles;
    if(idxx2 < 0)               idxx2 = 0;
    if(idxx2 >= level.xTiles)   idxx2 = level.xTiles;

    if(idxy1 < 0)               idxy1 = 0;
    if(idxy1 >= level.yTiles)   idxy1 = level.yTiles;
    if(idxy2 < 0)               idxy2 = 0;
    if(idxy2 >= level.yTiles)   idxy2 = level.yTiles;

    for(int idxy = idxy1; idxy < idxy2; idxy++)
    {
        for(int idxx = idxx1; idxx < idxx2; idxx++)
        {

            quint32 tag;
            quint32 len;
            quint64 offset = level.getOffsetJpeg(idxx, idxy);
            file.seek(offset);
            stream >> tag >> len;

            QImage img;
            img.load(&file,"JPG");

            if(img.isNull()) continue;

            qint32 w = ceil(img.width()  * level.xscale / (xscale * zoomFactor));
            qint32 h = ceil(img.height() * level.yscale / (yscale * zoomFactor));

            double u = xref1 + idxx * level.xscale * blockSizeX;
            double v = yref1 + idxy * level.yscale * blockSizeY;
            convertM2Pt(u,v);

            p.drawImage(u,v,img.scaled(w,h,Qt::IgnoreAspectRatio,Qt::SmoothTransformation));
        }
    }
}
