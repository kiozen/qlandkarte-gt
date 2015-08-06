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
#include <assert.h>

#include "CMapDEM.h"
#include "CMapDB.h"
#include "CWpt.h"
#include "CStatusDEM.h"
#include "CMainWindow.h"
#include "GeoMath.h"
#include "CMapDEMSlopeSetup.h"
#include "CSettings.h"

#include <gdal_priv.h>
#include <ogr_spatialref.h>

#ifdef WIN32
#include <float.h>
#define isnan(x) _isnan(x)
#endif

#include <QtGui>
#include <QMessageBox>
#include <QStatusBar>

const QRgb CMapDEM::slopeColorTable[] =
{
    qRgba(0,0,0,0)
    ,qRgba(0,128,0,100)
    ,qRgba(0,255,0,100)
    ,qRgba(255,255,0,100)
    ,qRgba(255,128,0,100)
    ,qRgba(255,0,0,100)
};

const double CMapDEM::grade1[6] =
{
    0.0
    ,27.0
    ,31.0
    ,34.0
    ,39.0
    ,50.0
};

const double CMapDEM::grade2[6] =
{
    0.0
    ,27.0
    ,30.0
    ,32.0
    ,35.0
    ,39.0
};

const double CMapDEM::grade3[6] =
{
    0.0
    ,27.0
    ,29.0
    ,30.0
    ,31.0
    ,34.0
};

const double CMapDEM::grade4[6] =
{
    0.0
    ,23.0
    ,25.0
    ,27.0
    ,28.0
    ,30.0
};

const double * CMapDEM::grade[5] =
{
    CMapDEM::grade1              // dummy
    ,CMapDEM::grade1
    ,CMapDEM::grade2
    ,CMapDEM::grade3
    ,CMapDEM::grade4
};

CMapDEM::CMapDEM(const QString& filename, CCanvas * parent)
: IMap(eDEM, "",parent)
, canvas(parent)
, old_my_xscale(0)
, old_my_yscale(0)
, old_overlay(IMap::eNone)
, idxGrade(1)
{
#ifdef WIN32
    dataset = (GDALDataset*)GDALOpen(filename.toLocal8Bit(),GA_ReadOnly);
#else
    dataset = (GDALDataset*)GDALOpen(filename.toUtf8(),GA_ReadOnly);
#endif
    if(dataset == 0)
    {
        QMessageBox::warning(0, tr("Error..."),
            tr("Failed to load file: %1\n\n").arg(filename).append(tr(CPLGetLastErrorMsg())));
        status = 0;
        return;
    }

    GDALRasterBand * pBand;
    pBand = dataset->GetRasterBand(1);
    if(pBand == 0)
    {
        delete dataset; dataset = 0;
        QMessageBox::warning(0, tr("Error..."),
            tr("Failed to load file: %1").arg(filename).append(tr(CPLGetLastErrorMsg())));
        status = 0;
        return;
    }
    pBand->GetBlockSize(&tileWidth,&tileHeight);

    char str[1024]= {0};
    if(dataset->GetProjectionRef())
    {
        strncpy(str,dataset->GetProjectionRef(),sizeof(str));
    }
    char * ptr = str;
    oSRS.importFromWkt(&ptr);
    oSRS.exportToProj4(&ptr);
    QString strProj = ptr;

    qDebug() << "DEM:" << strProj;

    pjsrc = pj_init_plus(strProj.toLatin1());

    xsize_px = dataset->GetRasterXSize();
    ysize_px = dataset->GetRasterYSize();

    double adfGeoTransform[6];
    dataset->GetGeoTransform( adfGeoTransform );

    if (strProj.contains("longlat"))
    {
        xref1   = adfGeoTransform[0] * DEG_TO_RAD;
        yref1   = adfGeoTransform[3] * DEG_TO_RAD;

        xscale  = adfGeoTransform[1] * DEG_TO_RAD;
        yscale  = adfGeoTransform[5] * DEG_TO_RAD;

    }
    else
    {
        xref1   = adfGeoTransform[0];
        yref1   = adfGeoTransform[3];

        xscale  = adfGeoTransform[1];
        yscale  = adfGeoTransform[5];
    }
    xref2   = xref1 + xsize_px * xscale;
    yref2   = yref1 + ysize_px * yscale;

    //     qDebug() << xref1 << yref1 << xref2 << yref2 << xscale << yscale;

    int i;
    for(i = 0; i < 256; ++i)
    {
        graytable2 << qRgba(0,0,0,i);
    }

    for(i = 0; i < 128; ++i)
    {
        graytable1 << qRgba(0,0,0,(128 - i));
    }

    for(i = 128; i < 255; ++i)
    {
        graytable1 << qRgba(255,255,255, 1);
    }

    slopetable << slopeColorTable[0];
    slopetable << slopeColorTable[1];
    slopetable << slopeColorTable[2];
    slopetable << slopeColorTable[3];
    slopetable << slopeColorTable[4];
    slopetable << slopeColorTable[5];

    status = new CStatusDEM(theMainWindow->getCanvas());
    theMainWindow->statusBar()->insertPermanentWidget(0,status);

    SETTINGS;
    idxGrade = cfg.value("map/overlay/grade",idxGrade).toInt();

}


bool CMapDEM::loaded()
{
    return (dataset != 0) && (status != 0);
}


CMapDEM::~CMapDEM()
{
    qDebug() << "CMapDEM::~CMapDEM()";

    if(CMapDEMSlopeSetup::self())
    {
        CMapDEMSlopeSetup::self()->registerDEMMap(0);
    }

    SETTINGS;
    cfg.setValue("map/overlay/grade",idxGrade);

    if(pjsrc) pj_free(pjsrc);
    if(dataset) delete dataset;
    if(status) delete status;
}


void CMapDEM::convertPt2M(double& u, double& v)
{
}


void CMapDEM::convertM2Pt(double& u, double& v)
{
}


void CMapDEM::move(const QPoint& old, const QPoint& next)
{
    setAngleNorth();
}


void CMapDEM::zoom(bool zoomIn, const QPoint& p)
{
}


void CMapDEM::zoom(double lon1, double lat1, double lon2, double lat2)
{
}


void CMapDEM::zoom(qint32& level)
{
}


void CMapDEM::select(const QRect& rect)
{
}


void CMapDEM::dimensions(double& lon1, double& lat1, double& lon2, double& lat2)
{
}


bool CMapDEM::getOrigRegion(QVector<qint16>& data,projXY &topLeft, projXY &bottomRight, int& w, int& h)
{
    //memset(data, 0, sizeof(qint16) * h * w);
    data.fill(0);
    if(pjsrc == 0) return false;

    // 1. convert top left and bottom right point into the projection system used by the DEM data
    pj_transform(pjtar, pjsrc, 1, 0, &topLeft.u, &topLeft.v, 0);
    pj_transform(pjtar, pjsrc, 1, 0, &bottomRight.u, &bottomRight.v, 0);

    // 2. get floating point offset of topleft corner
    double xoff1_f = (topLeft.u - xref1) / xscale;
    double yoff1_f = (topLeft.v - yref1) / yscale;

    // 3. truncate floating point offset into integer offset
    int xoff1 = floor(xoff1_f);  //qDebug() << "xoff1:" << xoff1 << xoff1_f;
    int yoff1 = floor(yoff1_f);  //qDebug() << "yoff1:" << yoff1 << yoff1_f;

    // 4. get floating point offset of bottom right corner
    double xoff2_f = (bottomRight.u - xref1) / xscale;
    double yoff2_f = (bottomRight.v - yref1) / yscale;

    // 5. truncate floating point offset into integer offset.
    int xoff2 = ceil(xoff2_f);   //qDebug() << "xoff2:" << xoff2 << xoff2_f;
    int yoff2 = ceil(yoff2_f);   //qDebug() << "yoff2:" << yoff2 << yoff2_f;

    // 6. get width and height to read from file
    qint32 w1 = xoff2 - xoff1 + 1;
                                 //qDebug() << "w1:" << w1 << "h1:" << h1;
    qint32 h1 = yoff2 - yoff1 + 1;

    topLeft.u = xoff1 * xscale + xref1;
    topLeft.v = yoff1 * yscale + yref1;
    bottomRight.u = xoff2 * xscale + xref1;
    bottomRight.v = yoff2 * yscale + yref1;
    pj_transform(pjsrc, pjtar, 1, 0, &topLeft.u, &topLeft.v, 0);
    pj_transform(pjsrc, pjtar, 1, 0, &bottomRight.u, &bottomRight.v, 0);

    // memory sanity check
    if(double(w1) * double(h1) > pow(2.0f,31)) return false;

    if (w1 < w)
    {
        w = w1;
    }
    if (h1 < h)
    {
        h = h1;
    }

    Q_ASSERT(w1 <= w);
    Q_ASSERT(h1 <= h);

    // 7. read DEM data from file
    CPLErr err = dataset->RasterIO(GF_Read, xoff1, yoff1, w1, h1, data.data(), w, h, GDT_Int16, 1, 0, 0, 0, 0);
    return (err != CE_Failure);
}


bool CMapDEM::getRegion(QVector<float>& data, projXY topLeft, projXY bottomRight, int w, int h)
{
    //     qDebug() << topLeft.u << topLeft.v << bottomRight.u << bottomRight.v << w << h;
    data.fill(0.0);

    if(pjsrc == 0) return false;

    // 1. convert top left and bottom right point into the projection system used by the DEM data
    pj_transform(pjtar, pjsrc, 1, 0, &topLeft.u, &topLeft.v, 0);
    pj_transform(pjtar, pjsrc, 1, 0, &bottomRight.u, &bottomRight.v, 0);

    // 2. get floating point offset of topleft corner
    double xoff1_f = (topLeft.u - xref1) / xscale;
    double yoff1_f = (topLeft.v - yref1) / yscale;

    // 3. truncate floating point offset into integer offset
    int xoff1 = floor(xoff1_f);  //qDebug() << "xoff1:" << xoff1 << xoff1_f;
    int yoff1 = floor(yoff1_f);  //qDebug() << "yoff1:" << yoff1 << yoff1_f;

    // 4. get floating point offset of bottom right corner
    double xoff2_f = (bottomRight.u - xref1) / xscale;
    double yoff2_f = (bottomRight.v - yref1) / yscale;

    // 5. truncate floating point offset into integer offset.
    int xoff2 = ceil(xoff2_f);   //qDebug() << "xoff2:" << xoff2 << xoff2_f;
    int yoff2 = ceil(yoff2_f);   //qDebug() << "yoff2:" << yoff2 << yoff2_f;

    // 6. get width and height to read from file
    quint32 w1 = xoff2 - xoff1 + 1;
                                 //qDebug() << "w1:" << w1 << "h1:" << h1;
    quint32 h1 = yoff2 - yoff1 + 1;

    // memory sanity check
    if(double(w1) * double(h1) > pow(2.0f,31)) return false;

    // 7. read DEM data from file
    QVector<qint16> _tmp_(w1*h1);
    qint16 * pData = _tmp_.data();

    CPLErr err = dataset->RasterIO(GF_Read, xoff1, yoff1, w1, h1, pData, w1, h1, GDT_Int16, 1, 0, 0, 0, 0);
    if(err == CE_Failure)
    {
        qDebug() << "failure" << endl;

        return false;
    }

    double xstep = (xoff2_f  - xoff1_f) / (w - 1);
    double ystep = (yoff2_f  - yoff1_f) / (h - 1);

    double xf, x;
    double yf, y;

    qint32 xi;
    qint32 yi;

    yf = yoff1_f - yoff1;
    for(int j = 0; j < h; ++j, yf += ystep)
    {
        yi = floor(yf);
        y  = yf - yi;

        xf = xoff1_f - xoff1;
        for(int i = 0; i < w; ++i, xf += xstep)
        {
            xi = floor(xf);
            x  = xf - xi;

#define GET_VALUE(X, Y)    pData[(X) + (Y) * w1]

            double f1 = GET_VALUE(xi,     yi);
            double f2 = GET_VALUE(xi + 1, yi);
            double f3 = GET_VALUE(xi,     yi + 1);
            double f4 = GET_VALUE(xi + 1, yi + 1);

            double b1 = f1;
            double b2 = f2 - f1;
            double b3 = f3 - f1;
            double b4 = f1 - f2 - f3 + f4;

            data[j * w + i] = b1 + b2 * x + b3 * y + b4 * x * y;

        }
    }

    return true;
}


float CMapDEM::getElevation(double lon, double lat)
{
    if(pjsrc == 0) return WPT_NOFLOAT;

    qint16 e[4];
    double u = lon;
    double v = lat;

    pj_transform(pjtar, pjsrc, 1, 0, &u, &v, 0);

    //     qDebug() << u << xref1 << xscale;

    double xoff = (u - xref1) / xscale;
    double yoff = (v - yref1) / yscale;

    double x    = xoff - floor(xoff);
    double y    = yoff - floor(yoff);

    //     qDebug() << xoff << yoff << x << y;

    CPLErr err = dataset->RasterIO(GF_Read, floor(xoff), floor(yoff), 2, 2, &e, 2, 2, GDT_Int16, 1, 0, 0, 0, 0);
    if(err == CE_Failure)
    {
        return WPT_NOFLOAT;
    }

    double b1 = e[0];
    double b2 = e[1] - e[0];
    double b3 = e[2] - e[0];
    double b4 = e[0] - e[1] - e[2] + e[3];

    float ele = b1 + b2 * x + b3 * y + b4 * x * y;

    return ele;
}


void CMapDEM::draw(QPainter& p)
{
    if(pjsrc == 0) return;

    draw();
    p.drawPixmap(0,0, pixBuffer);
    //     qDebug() << "--------------------------";
}


void CMapDEM::draw()
{

    IMap::overlay_e overlay = status->getOverlayType();

    if(CMapDEMSlopeSetup::self() && (overlay != old_overlay))
    {
        if(overlay == IMap::eSlope)
        {
            CMapDEMSlopeSetup::self()->registerDEMMap(this);
        }
        else
        {
            CMapDEMSlopeSetup::self()->registerDEMMap(0);
        }
    }

    if(overlay == IMap::eNone)
    {
        old_overlay = overlay;
        pixBuffer.fill(Qt::transparent);
        return;
    }

    // check if old area matches new request
    // kind of a different way to calculate the needRedraw flag

    projXY p1, p2;
    float my_xscale, my_yscale;

    getArea_n_Scaling_fromBase(p1, p2, my_xscale, my_yscale);

    if(    overlay == old_overlay
        && p1.u == old_p1.u && p1.v == old_p1.v
        && p2.u == old_p2.u && p2.v == old_p2.v
        && my_xscale == old_my_xscale && my_yscale == old_my_yscale
        )
    {
        if(!needsRedraw)
        {
            return;
        }
    }

    old_p1          = p1;
    old_p2          = p2;
    old_my_xscale   = my_xscale;
    old_my_yscale   = my_yscale;
    old_overlay     = overlay;

    pixBuffer.fill(Qt::transparent);

    if(pjsrc == 0) return;

    /*
        Calculate area of DEM data to be read.
    */

    projXY _p1 = p1;
    projXY _p2 = p2;

    // 1. convert top left and bottom right point into the projection system used by the DEM data
    pj_transform(pjtar, pjsrc, 1, 0, &_p1.u, &_p1.v, 0);
    pj_transform(pjtar, pjsrc, 1, 0, &_p2.u, &_p2.v, 0);

    // determine intersection rect
    projXY r1 = { qMax(_p1.u, xref1), qMin(_p1.v, yref1) };
    projXY r2 = { qMin(_p2.u, xref2), qMax(_p2.v, yref2) };

    if (r1.u > _p2.u || r1.v < _p2.v || r2.u < _p1.u || r2.v > _p1.v)
    {
        // no interscetion
        return;
    }

    // 2. get floating point offset of topleft corner
    double xoff1_f = (r1.u - xref1) / xscale;
    double yoff1_f = (r1.v - yref1) / yscale;

    // 3. truncate floating point offset into integer offset
    int xoff1 = xoff1_f;
    int yoff1 = yoff1_f;

    // 4. get floating point offset of bottom right corner
    double xoff2_f = (r2.u - xref1) / xscale;
    double yoff2_f = (r2.v - yref1) / yscale;

    // 5. qRound up (!) floating point offset into integer offset
    int xoff2 = ceil(xoff2_f);
    int yoff2 = ceil(yoff2_f);

    /*
        The defined area into DEM data (xoff1,yoff1,xoff2,yoff2) covers a larger or equal
        area in world coordinates [m] than the current viewport.

        Next the width and height of the area in the DEM data and the corresponding sceen
        size is calculated.
    */

    /*
        Calculate the width and the height of the area. Extend width until it's a multiple of 4.
        This will be of advantage as QImage will process 32bit aligned bitmaps much faster.
    */
    unsigned int w1 = (xoff2 - xoff1 + 3) & ~0x03;
    unsigned int h1 = (yoff2 - yoff1);

    // bail out if this is getting too big
    if(w1 > 10000 || h1 > 10000) return;

    // now calculate the actual width and height of the viewport
    unsigned int w2 = w1 * xscale / my_xscale;
    unsigned int h2 = h1 * yscale / my_yscale;

    // as the first point off the DEM data will not match exactly the given top left corner
    // the bitmap has to be drawn with an offset.
    int pxx = ((xoff1_f - xoff1) * xscale - (r1.u - _p1.u)) / my_xscale;
    int pxy = ((yoff1_f - yoff1) * yscale - (r1.v - _p1.v)) / my_yscale;

    // read 16bit elevation data from GeoTiff
    QVector<qint16> data(w1*h1);

    GDALRasterBand * pBand;
    pBand = dataset->GetRasterBand(1);
    CPLErr err = pBand->RasterIO(GF_Read, xoff1, yoff1, qMin(w1, xsize_px - xoff1), qMin(h1, ysize_px - yoff1), data.data(), w1, h1, GDT_Int16, 0, 0);
    if(err == CE_Failure)
    {
        return;
    }

    QImage img(w1,h1,QImage::Format_Indexed8);

    if(overlay == IMap::eShading)
    {
        shading(img,data.data(), my_xscale, my_yscale);
    }
    else if(overlay == IMap::eContour)
    {
        contour(img,data.data(), my_xscale, my_yscale);
    }
    else if(overlay == IMap::eSlope)
    {
        slope(img,data.data(), xoff1, yoff1);
    }
    else
    {
        qWarning() << "Unknown shading type";
        return;
    }

    // Finally scale the image to viewport size. QT will do the smoothing
    //img = img.scaled(w2,h2, Qt::IgnoreAspectRatio,Qt::SmoothTransformation);

    QPainter p(&pixBuffer);
    p.drawImage(-pxx, -pxy, img.scaled(w2,h2, Qt::IgnoreAspectRatio,Qt::SmoothTransformation));
}


void CMapDEM::shading(QImage& img, qint16 * data, float /*xscale*/, float /*yscale*/)
{
    int i;
    int w1 = img.width();
    int h1 = img.height();
    // find minimum and maximum elevation within area
    float min = 32768;
    float max = -32768;
    float ele;

    for(i = 0; i < (w1 * h1); i++)
    {
        ele = data[i];
        if(ele < 0) continue;
        if(ele < min) min = ele;
        if(ele > max) max = ele;
    }

    /* Convert 16bit elevation data into 8 bit indices into a gray scale table.
       The dynamic will be 200 gray scales between min and max.
    */
    uchar * pixel = img.bits();
    img.setColorTable(graytable2);

    int f = (max -min);
    f = f ? f : 1;

    for(i = 0; i < ((w1 * h1) - 1); i++)
    {
        *pixel = ((data[i] - min) * 150 / f);
        ++pixel;
    }
}


void CMapDEM::contour(QImage& img, qint16 * data, float xscl, float /*yscale*/)
{
    int w1 = img.width();
    int h1 = img.height();

    int r,c,i;
    float diff = 0;
    int idx  = 0;
    float min  =  32768;
    float max  = -32768;
    for(r = 0; r < (h1 - 1); ++r)
    {
        for(c = 0; c < (w1 - 1); ++c)
        {
            diff  = data[idx +  1    ] - data[idx];
            diff += data[idx + w1    ] - data[idx];
            diff += data[idx + w1 + 1] - data[idx];
            data[idx++] = diff;
            if(diff < min) min = diff;
            if(diff > max) max = diff;
        }
        data[idx++] = 0;
    }

    //     qDebug() << min << max;

    for(c = 0; c < w1; ++c)
    {
        data[idx++] = 0;
    }
    float f = 30;
    if(xscl < 4)
    {
        f  = abs(min) < abs(max) ? abs(max) : abs(min);
        f /= 2;
    }
    f = f ? f : 1;

    img.setColorTable(graytable1);
    uchar * pixel = img.bits();
    int val;
    for(i = 0; i < (w1 * h1); ++i)
    {
        val = 128 + data[i] * 100 / f;
        if(val > 254) val = 254;
        if(val < 1  ) val = 1;
        *pixel++ = val;
    }
}


void CMapDEM::slope(QImage& img, qint16 * data, int xoff, int yoff)
{
    int w1 = img.width();
    int h1 = img.height();

    int r,c,i;
    int idx  = 0;
    double h, ha, hb, a;
    double dx, dy, a1, a2;

    const int step = 1;

    projXY p1;
    projXY p2;

    p1.u =  xoff * xscale + xref1;
    p1.v =  yoff * yscale + yref1;

    p2.u =  xoff * xscale + xref1;
    p2.v = (yoff + 1) * yscale + yref1;

    pj_transform(pjsrc, pjtar, 1, 0, &p1.u, &p1.v, 0);
    pj_transform(pjsrc, pjtar, 1, 0, &p2.u, &p2.v, 0);

    dy = distance(p1, p2, a1, a2);

    for(r = 0; r < (h1 - step); ++r)
    {
        p1.u =  xoff * xscale + xref1;
        p1.v = (yoff + r) * yscale + yref1;

        p2.u = (xoff + 1) * xscale + xref1;
        p2.v = (yoff + r) * yscale + yref1;

        pj_transform(pjsrc, pjtar, 1, 0, &p1.u, &p1.v, 0);
        pj_transform(pjsrc, pjtar, 1, 0, &p2.u, &p2.v, 0);

        dx = distance(p1, p2, a1, a2);

        for(c = 0; c < (w1 - step); ++c)
        {
            h  = data[idx];
            ha = data[idx + step];
            hb = data[idx + step*w1];

            a = acos(1/sqrt(pow((h - ha)/(dx*step), 2.0) + pow((h - hb)/(dy*step),2.0) + 1));

            data[idx++] = (a * 360.0/(2 * M_PI));
        }

        for(i = 0; i < step; i++)
        {
            data[idx++] = 0;
        }
    }

    // finisch last rows
    for(i = 0; i < step; i++)
    {
        for(c = 0; c < w1; ++c)
        {
            data[idx++] = 0;
        }
    }

    img.setColorTable(slopetable);
    uchar * pixel = img.bits();
    int val;
    const double * g = grade[idxGrade];

    for(i = 0; i < (w1 * h1); ++i)
    {
#ifdef QK_QT5_PORT
        val = abs(data[i]);
#else
        val = abs(qRound(data[i]));
#endif
        if(val > g[5])
        {
            *pixel++ = 5;
        }
        else if(val > g[4])
        {
            *pixel++ = 4;
        }
        else if(val > g[3])
        {
            *pixel++ = 3;
        }
        else if(val > g[2])
        {
            *pixel++ = 2;
        }
        else if(val > g[1])
        {
            *pixel++ = 1;
        }
        else
        {
            *pixel++ = 0;
        }
    }
}


void CMapDEM::setGrade(int i)
{
    if(i < 1)
    {
        idxGrade = 1;
    }
    else if(i > 4)
    {
        idxGrade = 4;
    }
    else
    {
        idxGrade = i;
    }

    needsRedraw = true;

    canvas->update();
}
