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

#include "CMapFile.h"
#include "CMapLevel.h"
#include <gdal_priv.h>
#include <ogr_spatialref.h>

#include <QtCore>
#include <QColor>

CMapFile::CMapFile(const QString& filename, QObject * parent)
: QObject(parent)
, filename(filename)
, dataset(0)
, xsize_px(0)
, ysize_px(0)
, pj(0)
, xscale(0.0)
, yscale(0.0)
, xref1(0.0)
, yref1(0.0)
, ok(false)
, rasterBandCount(0)
{
#ifdef WIN32
    dataset = (GDALDataset*)GDALOpen(filename.toLocal8Bit(),GA_ReadOnly);
#else
    dataset = (GDALDataset*)GDALOpen(filename.toUtf8(),GA_ReadOnly);
#endif
    if(dataset == 0) return;

    char str[1024] = {0};
    if(dataset->GetProjectionRef())
    {
        strncpy(str,dataset->GetProjectionRef(),sizeof(str));
    }
    char * ptr = str;
    oSRS.importFromWkt(&ptr);
    oSRS.exportToProj4(&ptr);
    strOrigProj = strProj = ptr;

    //     if(ptr) free(ptr);

    qDebug() << strProj;

    pj = pj_init_plus(strProj.toLatin1());
    if(pj == 0) return;

    xsize_px = dataset->GetRasterXSize();
    ysize_px = dataset->GetRasterYSize();

    double adfGeoTransform[6];
    dataset->GetGeoTransform( adfGeoTransform );

    if (pj_is_latlong(pj))
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

    qDebug() << "xscale" << xscale << "yscale" << yscale;
    qDebug() << xref1 << yref1 << xref2 << yref2;

    rasterBandCount = dataset->GetRasterCount();
    if(rasterBandCount < 1) return;

    if(rasterBandCount == 1)
    {
        GDALRasterBand * pBand;
        pBand = dataset->GetRasterBand(1);
        if(pBand == 0) return;

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
            return;
        }

        int success = 0;
        double idx = pBand->GetNoDataValue(&success);

        if(success)
        {
            qDebug() << "set transparent color for index" << idx;
            QColor tmp(colortable[idx]);
            tmp.setAlpha(0);
            colortable[idx] = tmp.rgba();
        }

        pBand->GetBlockSize(&tileWidth,&tileHeight);
    }

    projPJ  pjWGS84 = pj_init_plus("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");

    lon1 = xref1;
    lat1 = yref1;
    pj_transform(pj,pjWGS84,1,0,&lon1,&lat1,0);

    lon2 = xref2;
    lat2 = yref2;
    pj_transform(pj,pjWGS84,1,0,&lon2,&lat2,0);

    pj_free(pjWGS84);

    ok = true;
}


CMapFile::~CMapFile()
{
    if(pj) pj_free(pj);
    if(dataset) delete dataset;

}


bool CMapFile::is32BitRgb()
{
    return rasterBandCount > 1;
}
