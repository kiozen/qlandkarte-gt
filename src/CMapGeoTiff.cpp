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

#include "CMapGeoTiff.h"
#include "CMainWindow.h"
#include "CCanvas.h"
#include "CResources.h"

#include <gdal_priv.h>
#include <ogr_spatialref.h>
#include <math.h>

#include <QtGui>
#include <QMessageBox>
#include <QCheckBox>

CMapGeoTiff::CMapGeoTiff(const QString& fn, CCanvas * parent)
: IMap(eRaster, "",parent)
, dataset(0)
, xsize_px(0)
, ysize_px(0)
, xscale(1.0)
, yscale(1.0)
, xref1(0)
, yref1(0)
, xref2(0)
, yref2(0)
, x(0)
, y(0)
, zoomFactor(1.0)
, rasterBandCount(0)
, quadraticZoom(0)
{
    filename = fn;

#ifdef WIN32
    dataset = (GDALDataset*)GDALOpen(filename.toLocal8Bit(),GA_ReadOnly);
#else
    dataset = (GDALDataset*)GDALOpen(filename.toUtf8(),GA_ReadOnly);
#endif
    if(dataset == 0)
    {
        QMessageBox::warning(0, tr("Error..."), tr("Failed to load file: %1").arg(filename));
        return;
    }

    rasterBandCount = dataset->GetRasterCount();

    if(rasterBandCount == 1)
    {
        GDALRasterBand * pBand;
        pBand = dataset->GetRasterBand(1);

        if(pBand == 0)
        {
            delete dataset; dataset = 0;
            QMessageBox::warning(0, tr("Error..."), tr("Failed to load file: %1").arg(filename));
            return;
        }

        //        qDebug() << pBand->GetColorInterpretation();

        if(pBand->GetColorInterpretation() ==  GCI_PaletteIndex )
        {
            GDALColorTable * pct = pBand->GetColorTable();
            for(int i=0; i < pct->GetColorEntryCount(); ++i)
            {
                const GDALColorEntry& e = *pct->GetColorEntry(i);
                colortable << qRgba(e.c1, e.c2, e.c3, e.c4);
            }
        }
        else if(pBand->GetColorInterpretation() ==  GCI_GrayIndex )
        {
            for(int i=0; i < 256; ++i)
            {
                colortable << qRgba(i, i, i, 255);
            }
        }
        else
        {
            delete dataset; dataset = 0;
            QMessageBox::warning(0, tr("Error..."), tr("File must be 8 bit palette or gray indexed."));
            return;
        }

        int success = 0;
        double idx = pBand->GetNoDataValue(&success);

        if(success)
        {
            QColor tmp(colortable[idx]);
            tmp.setAlpha(0);
            colortable[idx] = tmp.rgba();
        }
    }

    char str[1024] = {0};
    if(dataset->GetProjectionRef())
    {
        strncpy(str,dataset->GetProjectionRef(),sizeof(str));
    }
    char * ptr = str;
    oSRS.importFromWkt(&ptr);
    oSRS.exportToProj4(&ptr);

    qDebug() << ptr;

    pjsrc = pj_init_plus(ptr);
    if(pjsrc == 0)
    {
        delete dataset; dataset = 0;
        QMessageBox::warning(0, tr("Error..."), tr("No georeference information found."));
        return;
    }

    xsize_px = dataset->GetRasterXSize();
    ysize_px = dataset->GetRasterYSize();

    double adfGeoTransform[6];
    dataset->GetGeoTransform( adfGeoTransform );

    if (pj_is_latlong(pjsrc))
    {
        xscale  = adfGeoTransform[1] * DEG_TO_RAD;
        yscale  = adfGeoTransform[5] * DEG_TO_RAD;

        xref1   = adfGeoTransform[0] * DEG_TO_RAD;
        yref1   = adfGeoTransform[3] * DEG_TO_RAD;
    }
    else
    {
        xscale  = adfGeoTransform[1];
        yscale  = adfGeoTransform[5];

        xref1   = adfGeoTransform[0];
        yref1   = adfGeoTransform[3];
    }

    xref2   = xref1 + xsize_px * xscale;
    yref2   = yref1 + ysize_px * yscale;

    x = xref1 + (xref2 - xref1) / 2;
    y = yref1 - (yref1 - yref2) / 2;

    qDebug() << "xscale" << xscale << "yscale" << yscale;
    qDebug() << xref1 << yref1 << xref2 << yref2;

    zoomidx = 1;

    quadraticZoom = theMainWindow->getCheckBoxQuadraticZoom();
}


CMapGeoTiff::~CMapGeoTiff()
{
    if(pjsrc) pj_free(pjsrc);
    if(dataset) delete dataset;
}


bool CMapGeoTiff::is32BitRgb()
{
    return rasterBandCount > 1;
}


void CMapGeoTiff::draw(QPainter& p)
{
    if(pjsrc == 0) return IMap::draw(p);

    draw();

    p.drawPixmap(0,0,pixBuffer);

    // render overlay
    if(!ovlMap.isNull() && !doFastDraw)
    {
        //        qDebug() << size << needsRedraw;
        ovlMap->draw(size, needsRedraw, p);
    }

    if(CResources::self().showZoomLevel())
    {

        QString str;
        if(zoomFactor < 1.0)
        {
            str = tr("Overzoom x%1").arg(1/zoomFactor,0,'f',0);
        }
        else
        {
            str = tr("Zoom level x%1").arg(zoomidx);
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


void CMapGeoTiff::draw()
{
    if(pjsrc == 0) return IMap::draw();

    pixBuffer.fill(Qt::white);
    QPainter _p_(&pixBuffer);

    QRectF viewport  = QRectF(x, y, size.width() * xscale * zoomFactor,  size.height() * yscale * zoomFactor);
    QRectF maparea   = QRectF(QPointF(xref1, yref1), QPointF(xref2, yref2));
    QRectF intersect = viewport.intersected(maparea);

    //     qDebug() << maparea << viewport << intersect;

    if(intersect.isValid())
    {

        // x/y offset [pixel] into file matrix
        qint32 xoff = (intersect.left()   - xref1) / xscale;
        qint32 yoff = (intersect.bottom() - yref1) / yscale;

        // number of x/y pixel to read
        qint32 pxx  =   (qint32)(intersect.width()  / xscale);
        qint32 pxy  =  -(qint32)(intersect.height() / yscale);

        // the final image width and height in pixel
        qint32 w    =   (qint32)(pxx / zoomFactor) & 0xFFFFFFFC;
        qint32 h    =   (qint32)(pxy / zoomFactor);

        // correct pxx by truncation
        pxx         =   (qint32)(w * zoomFactor);

        //         qDebug() << xoff << yoff << pxx << pxy << w << h;

        if(w > 0 && h > 0)
        {

            CPLErr err = CE_Failure;

            if(rasterBandCount == 1)
            {
                GDALRasterBand * pBand;
                pBand = dataset->GetRasterBand(1);

                QImage img(QSize(w,h),QImage::Format_Indexed8);
                img.setColorTable(colortable);

                err = pBand->RasterIO(GF_Read
                    ,(int)xoff,(int)yoff
                    ,pxx,pxy
                    ,img.bits()
                    ,w,h
                    ,GDT_Byte,0,0);

                if(!err)
                {
                    double xx = intersect.left(), yy = intersect.bottom();
                    convertM2Pt(xx,yy);
                    _p_.drawPixmap(xx,yy,QPixmap::fromImage(img));
                }
            }
            else
            {
                QImage img(w,h, QImage::Format_ARGB32);
                QVector<quint8> buffer(w*h);

                img.fill(qRgba(255,255,255,255));
                QRgb testPix = qRgba(GCI_RedBand, GCI_GreenBand, GCI_BlueBand, GCI_AlphaBand);

                for(int b = 1; b <= rasterBandCount; ++b)
                {

                    GDALRasterBand * pBand;
                    pBand = dataset->GetRasterBand(b);

                    err = pBand->RasterIO(GF_Read, (int)xoff, (int)yoff, pxx, pxy, buffer.data(), w, h, GDT_Byte, 0, 0);

                    if(!err)
                    {
                        int pbandColour = pBand->GetColorInterpretation();
                        unsigned int offset;

                        for (offset = 0; offset < sizeof(testPix) && *(((quint8 *)&testPix) + offset) != pbandColour; offset++);
                        if(offset < sizeof(testPix))
                        {
                            quint8 * pTar   = img.bits() + offset;
                            quint8 * pSrc   = buffer.data();
                            const int size  = buffer.size();

                            for(int i = 0; i < size; ++i)
                            {
                                *pTar = *pSrc;
                                pTar += sizeof(testPix);
                                pSrc += 1;
                            }
                        }
                    }
                }

                if(!err)
                {
                    double xx = intersect.left(), yy = intersect.bottom();
                    convertM2Pt(xx,yy);
                    _p_.drawImage(xx,yy,img);
                }
            }
        }
    }
}


void CMapGeoTiff::convertPt2M(double& u, double& v)
{
    u = x + u * xscale * zoomFactor;
    v = y + v * yscale * zoomFactor;
}


void CMapGeoTiff::convertM2Pt(double& u, double& v)
{
    u = (u - x) / (xscale * zoomFactor);
    v = (v - y) / (yscale * zoomFactor);
}


void CMapGeoTiff::convertPt2Pixel(double& u, double& v)
{
    convertPt2M(u,v);

    u = (u - xref1) / xscale;
    v = (v - yref1) / yscale;

    if(u < 0 || u > xsize_px)
    {
        u = -1;
        v = -1;
        return;
    }
    if(v < 0 || v > ysize_px)
    {
        u = -1;
        v = -1;
        return;
    }

}


void CMapGeoTiff::move(const QPoint& old, const QPoint& next)
{
    if(pjsrc == 0) return;

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


void CMapGeoTiff::zoom(bool zoomIn, const QPoint& p0)
{
    projXY p1;
    if(pjsrc == 0) return;

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


void CMapGeoTiff::zoom(qint32& level)
{
    if(pjsrc == 0) return;

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


void CMapGeoTiff::zoom(double lon1, double lat1, double lon2, double lat2)
{
    if(pjsrc == 0) return;

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


void CMapGeoTiff::select(const QRect& rect)
{
}


void CMapGeoTiff::dimensions(double& lon1, double& lat1, double& lon2, double& lat2)
{

    lon1 = xref1;
    lat1 = yref1;
    pj_transform(pjsrc,pjtar,1,0,&lon1,&lat1,0);

    lon2 = xref2;
    lat2 = yref2;
    pj_transform(pjsrc,pjtar,1,0,&lon2,&lat2,0);
}


void CMapGeoTiff::incXOffset(int i)
{
    xref1 += i * xscale;
    xref2 += i * xscale;

    emit sigChanged();
}


void CMapGeoTiff::decXOffset(int i)
{
    xref1 -= i * xscale;
    xref2 -= i * xscale;

    emit sigChanged();
}


void CMapGeoTiff::incYOffset(int i)
{
    yref1 -= i * yscale;
    yref2 -= i * yscale;

    emit sigChanged();
}


void CMapGeoTiff::decYOffset(int i)
{
    yref1 += i * yscale;
    yref2 += i * yscale;

    emit sigChanged();
}


GDALDataset * CMapGeoTiff::getDataset()
{
    return dataset;
}


void CMapGeoTiff::getArea_n_Scaling(projXY& p1, projXY& p2, float& my_xscale, float& my_yscale)
{
    p1.u = 0;
    p1.v = 0;
    convertPt2Rad(p1.u, p1.v);

    p2.u = size.width();
    p2.v = size.height();
    convertPt2Rad(p2.u, p2.v);

    my_xscale   = xscale*zoomFactor;
    my_yscale   = yscale*zoomFactor;

    qDebug() << p1.u << p1.v << p2.u << p2.v << my_xscale << my_yscale;
}
