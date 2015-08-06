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

#include "CMapRaster.h"
#include <gdal_priv.h>
#include <ogr_spatialref.h>

#include <QtGui>
#include <QMessageBox>

CMapRaster::CMapRaster(const QString& fn, CCanvas * parent)
: IMap(eRaster, "",parent)
, x(0)
, y(0)
, zoomlevel(1)
, zoomfactor(1.0)
, rasterBandCount(0)
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

        if(pBand->GetColorInterpretation() !=  GCI_PaletteIndex && pBand->GetColorInterpretation() !=  GCI_GrayIndex)
        {
            delete dataset; dataset = 0;
            QMessageBox::warning(0, tr("Error..."), tr("File must be 8 bit palette or gray indexed."));
            return;
        }

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
    maparea.setWidth(dataset->GetRasterXSize());
    maparea.setHeight(dataset->GetRasterYSize());

}


CMapRaster::~CMapRaster()
{
    if(dataset) delete dataset;
}


bool CMapRaster::is32BitRgb()
{
    return rasterBandCount > 1;
}


void CMapRaster::convertPt2M(double& u, double& v)
{
    u = x + u * zoomfactor;
    v = y + v * zoomfactor;
}


void CMapRaster::convertM2Pt(double& u, double& v)
{
    u = (u - x) / zoomfactor;
    v = (v - y) / zoomfactor;
}


void CMapRaster::convertPt2Pixel(double& u, double& v)
{
    convertPt2M(u,v);
    if(u < 0 || u > maparea.width())
    {
        u = -1;
        v = -1;
        return;
    }
    if(v < 0 || v > maparea.height())
    {
        u = -1;
        v = -1;
        return;
    }

}


void CMapRaster::move(const QPoint& old, const QPoint& next)
{
    // move top left point by difference
    x += (old.x() - next.x()) * zoomfactor;
    y += (old.y() - next.y()) * zoomfactor;

}


void CMapRaster::zoom(bool zoomIn, const QPoint& p)
{
    double x1 = p.x();
    double y1 = p.y();

    convertPt2M(x1,y1);

    zoomlevel += zoomIn ? -1 : +1;
    if(zoomlevel < 1)
    {
        zoomfactor  = 1.0 / - (zoomlevel - 2);
    }
    else
    {
        zoomfactor = zoomlevel;
    }

    convertM2Pt(x1,y1);
    move(QPoint(x1,y1),p);
}


void CMapRaster::zoom(qint32& level)
{

}


void CMapRaster::zoom(double lon1, double lat1, double lon2, double lat2)
{
}


void CMapRaster::select(const QRect& rect)
{
}


void CMapRaster::dimensions(double& lon1, double& lat1, double& lon2, double& lat2)
{
    lon1 = 0;
    lat1 = 0;
    lon2 = maparea.width();
    lat2 = maparea.height();
}


void CMapRaster::draw(QPainter& p)
{
    if(!dataset) return;

    draw();

    p.drawPixmap(0,0,pixBuffer);

    QString str;
    if(zoomfactor < 1.0)
    {
        str = tr("Overzoom x%1").arg(1/zoomfactor,0,'f',0);
    }
    else
    {
        str = tr("Zoom level x%1").arg(zoomlevel);
    }

    p.setPen(Qt::white);
    p.setFont(QFont("Sans Serif",14,QFont::Black));

    p.drawText(9 ,23, str);
    p.drawText(11,23, str);
    p.drawText(9 ,25, str);
    p.drawText(11,25, str);

    p.setPen(Qt::darkBlue);
    p.drawText(10,24,str);

}


void CMapRaster::draw()
{
    if(!dataset) return;

    pixBuffer.fill(Qt::white);
    QPainter _p_(&pixBuffer);

    QRectF viewport(x, y, size.width() * zoomfactor,  size.height() *  zoomfactor);
    QRectF intersect = viewport.intersected(maparea);

    // x/y offset [pixel] into file matrix
    qint32 xoff = intersect.left();
    qint32 yoff = intersect.top();

    // number of x/y pixel to read
    qint32 pxx  = intersect.width();
    qint32 pxy  = intersect.height();

    // the final image width and height in pixel
    qint32 w    = (qint32)(pxx / zoomfactor) & 0xFFFFFFFC;
    qint32 h    = (qint32)(pxy / zoomfactor);

    if((w*h) == 0)
    {
        return;
    }

    CPLErr err = CE_Failure;
    if(rasterBandCount == 1)
    {
        GDALRasterBand * pBand;
        pBand = dataset->GetRasterBand(1);

        QImage img(QSize(w,h),QImage::Format_Indexed8);
        img.setColorTable(colortable);

        err = pBand->RasterIO(GF_Read,(int)xoff,(int)yoff,pxx,pxy,img.bits(),w,h,GDT_Byte,0,0);

        if(!err)
        {
            double xx = (intersect.left() - x) / zoomfactor, yy = (intersect.top() - y)  / zoomfactor;
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
            double xx = (intersect.left() - x) / zoomfactor, yy = (intersect.top() - y)  / zoomfactor;
            _p_.drawPixmap(xx,yy,QPixmap::fromImage(img));
        }

    }
}
