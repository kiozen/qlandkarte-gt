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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <wctype.h>

#include <gdal_priv.h>
#include <proj_api.h>
#include <ogr_spatialref.h>

#include <QtCore>
#include <QtXml>

#include "CDiskCache.h"

#ifndef _MKSTR_1
#define _MKSTR_1(x)         #x
#define _MKSTR(x)           _MKSTR_1(x)
#endif

#define VER_STR             _MKSTR(VER_MAJOR)"."_MKSTR(VER_MINOR)"."_MKSTR(VER_STEP)
#define WHAT_STR            "cache2gtiff, Version " VER_STR


struct map_t
{
    map_t() : pjsrc(0)
    {
        pjtar = pj_init_plus("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");
    }

    ~map_t()
    {
        if(pjsrc) pj_free(pjsrc);
        if(pjtar) pj_free(pjtar);
    }

    projPJ pjsrc;
    projPJ pjtar;

    int level;
    double xref1;
    double yref1;
    double xref2;
    double yref2;

    quint32 blockSizeX;
    quint32 blockSizeY;
    quint32 xsize_px;
    quint32 ysize_px;

    double xscale;
    double yscale;

    QString url;
};

static void convertPt2M(map_t& map, double& u, double& v)
{
    u = map.xref1 + u * map.xscale/* * map.level*/;
    v = map.yref1 + v * map.yscale/* * map.level*/;
}

static void convertM2Pt(map_t& map, double& u, double& v)
{
    u = (u - map.xref1) / (map.xscale/* * map.level*/);
    v = (v - map.yref1) / (map.yscale/* * map.level*/);
}


static void convertPt2Rad(map_t& map, double& u, double& v)
{
    if(map.pjsrc == 0)
    {
        //         u = v = 0;
        return;
    }
    convertPt2M(map,u,v);

    projXY pt;
    pt.u = u;
    pt.v = v;

    pj_transform(map.pjsrc,map.pjtar,1,0,&pt.u,&pt.v,0);

    u = pt.u;
    v = pt.v;
}


static void convertRad2Pt(map_t& map, double& u, double& v)
{
    if(map.pjsrc == 0)
    {
        return;
    }

    projXY pt;
    pt.u = u;
    pt.v = v;

    pj_transform(map.pjtar,map.pjsrc,1,0,&pt.u,&pt.v,0);

    u = pt.u;
    v = pt.v;

    convertM2Pt(map,u,v);
}


static void convertRad2M(map_t& map, double& u, double& v)
{
    if(map.pjsrc == 0)
    {
        return;
    }

    pj_transform(map.pjtar,map.pjsrc,1,0,&u,&v,0);
}


static void convertM2Rad(map_t& map, double& u, double& v)
{
    if(map.pjsrc == 0)
    {
        return;
    }

    pj_transform(map.pjsrc,map.pjtar,1,0,&u,&v,0);
}


static int lon2tile(double lon, int z)
{
    return (int)(qRound(256*(lon + 180.0) / 360.0 * pow(2.0, z)));
}


static int lat2tile(double lat, int z)
{
    return (int)(qRound(256*(1.0 - log( tan(lat * M_PI/180.0) + 1.0 / cos(lat * M_PI/180.0)) / M_PI) / 2.0 * pow(2.0, z)));
}

static double tile2lon(int x, int z)
{
    return x / pow(2.0, z) * 360.0 - 180;
}

static double tile2lat(int y, int z)
{
    double n = M_PI - 2.0 * M_PI * y / pow(2.0, z);
    return 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
}


/// this code is from the GDAL project
static void printProgress(int current, int total)
{
    double dfComplete = double(current)/double(total);

    static int nLastTick = -1;
    int nThisTick = (int) (dfComplete * 40.0);

    nThisTick = MIN(40,MAX(0,nThisTick));

    // Have we started a new progress run?
    if( nThisTick < nLastTick && nLastTick >= 39 )
    {
        nLastTick = -1;
    }

    if( nThisTick <= nLastTick )
    {
        return;
    }

    while( nThisTick > nLastTick )
    {
        nLastTick++;
        if( nLastTick % 4 == 0 )
        {
            fprintf( stdout, "%d", (nLastTick / 4) * 10 );
        }
        else
        {
            fprintf( stdout, "." );
        }
    }

    if( nThisTick == 40 )
    {
        fprintf( stdout, " - done.\n" );
    }
    fflush( stdout );

}

static int exportTMS(int level, double lon1, double lat1, double lon2, double lat2, const QString infile, const QString& outfile, CDiskCache& diskCache);
static int exportWMS(int level, double lon1, double lat1, double lon2, double lat2, const QString infile, const QString& outfile, CDiskCache& diskCache);

int main(int argc, char ** argv)
{
    int level;
    double lon1, lat1, lon2, lat2;
    QString infile;
    QString outfile;
    QString cachePath;

    printf("\n****** %s ******\n", WHAT_STR);

    if(argc < 2)
    {
        fprintf(stderr,"\nusage: cache2gtiff -a level lon1 lat1 lon2 lat2 -c <path> -i <file|url> -o <file> \n");
        fprintf(stderr,"\n");
        fprintf(stderr,"  -a The level and the area to export.\n");
        fprintf(stderr,"     The level is an integer from 1..19\n");
        fprintf(stderr,"     All lon/lat values are in degree\n");
        fprintf(stderr,"     lon1, lat1 is the top left corner of the area\n");
        fprintf(stderr,"     lon2, lat2 is the bottom right corner of the area\n");
        fprintf(stderr,"  -c The path to the tile cache\n");
        fprintf(stderr,"  -i The xml definition or an url of the server\n");
        fprintf(stderr,"  -o the target geotiff filename\n");
        fprintf(stderr,"\n");
        fprintf(stderr,"\n");
        exit(-1);
    }

    GDALAllRegister();

    int skip_next_arg = 0;
    for(int i = 1; i < (argc - 1); i++)
    {
        while(skip_next_arg)
        {
            skip_next_arg--;
            continue;
        }

        if (argv[i][0] == '-')
        {
            if (towupper(argv[i][1]) == 'A')
            {
                level = atol(argv[i+1]);
                lon1    = atof(argv[i+2]) * DEG_TO_RAD;
                lat1    = atof(argv[i+3]) * DEG_TO_RAD;
                lon2    = atof(argv[i+4]) * DEG_TO_RAD;
                lat2    = atof(argv[i+5]) * DEG_TO_RAD;
                skip_next_arg = 5;
                continue;
            }
            else if (towupper(argv[i][1]) == 'C')
            {
                cachePath = QString(argv[i+1]);
                skip_next_arg = 1;
                continue;
            }
            else if (towupper(argv[i][1]) == 'I')
            {
                infile = QString(argv[i+1]);
                skip_next_arg = 1;
                continue;
            }
            else if (towupper(argv[i][1]) == 'O')
            {
                outfile = QString(argv[i+1]);
                skip_next_arg = 1;
                continue;
            }

        }
    }

    CDiskCache diskCache(cachePath,0);

    bool isTMS = infile.startsWith("http");

    if(isTMS)
    {
        level = log((double)level)/log(2.0) + 1;
        return exportTMS(level, lon1, lat1, lon2, lat2, infile, outfile, diskCache);
    }
    else
    {
        return exportWMS(level, lon1, lat1, lon2, lat2, infile, outfile, diskCache);
    }
}

static int exportTMS(int level, double lon1, double lat1, double lon2, double lat2, const QString infile, const QString& outfile, CDiskCache& diskCache)
{
    map_t map;
    map.level       = 1 << (level - 1);//pow(2,level - 1); ==> there is no compatible function prototype for pow() in VC2008
    map.url         = infile;
    map.pjsrc       = pj_init_plus("+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext +no_defs");
    map.blockSizeX  = 256;
    map.blockSizeY  = 256;

    map.xref1       = -40075016/2;
    map.yref1       =  40075016/2;
    map.xref2       =  40075016/2;
    map.yref2       = -40075016/2;

    map.xscale      =  1.19432854652;
    map.yscale      = -1.19432854652;

    map.xsize_px    = -1; // n/a
    map.ysize_px    = -1; // n/a

    printf("map source:         %s\n", map.url.toLocal8Bit().data());

    int z       = 18 - level;

    // convert to abs pixel in map
    double x1   = lon1;
    double y1   = lat1;
    convertRad2Pt(map, x1, y1);
    x1 = x1/map.level;
    y1 = y1/map.level;

    double x2   = lon2;
    double y2   = lat2;
    convertRad2Pt(map, x2, y2);
    x2 = x2/map.level;
    y2 = y2/map.level;


    int w       = int(x2 - x1);
    int h       = int(y2 - y1);

    int idxx    = lon2tile(lon1 * RAD_TO_DEG, z) / 256;
    int idxy    = lat2tile(lat1 * RAD_TO_DEG, z) / 256;
    x1  = tile2lon(idxx, z) * DEG_TO_RAD;
    y1  = tile2lat(idxy, z) * DEG_TO_RAD;
    convertRad2Pt(map, x1, y1);
    x1 = x1/map.level;
    y1 = y1/map.level;


    int total       = ceil((x2 - x1)/(map.blockSizeX)) * ceil((y2 - y1)/(map.blockSizeY));
    int prog        = 1;
    int badTiles    = 0;

    printf("\n");
    printf("Export area of %i x %i pixel\n", w, h);
    printf("Need to summon %i tiles from cache.\n\n", total);


    // create output dataset
    char * cargs[] = {"tiled=yes", "compress=DEFLATE", 0};
    GDALDriverManager * drvman  = GetGDALDriverManager();
    GDALDriver * driver         = drvman->GetDriverByName("GTiff");
    GDALDataset * dataset       = driver->Create(outfile.toLocal8Bit().data(), w, h, 3, GDT_Byte, cargs);

    if(dataset == 0)
    {
        fprintf(stderr, "Failed top open target file %s", outfile.toLocal8Bit().data());
        exit(-1);
    }

    // set projection of output, same as input
    char * ptr = 0;
    OGRSpatialReference oSRS;
    oSRS.importFromProj4(pj_get_def(map.pjsrc,0));
    oSRS.exportToWkt(&ptr);
    dataset->SetProjection(ptr);
    CPLFree(ptr);

    // set referencing data
    double adfGeoTransform[6] = {0};
    double u = lon1;
    double v = lat1;
    convertRad2M(map, u, v);

    adfGeoTransform[0] = u;             /* top left x */
    adfGeoTransform[1] = map.xscale * map.level;    /* w-e pixel resolution */
    adfGeoTransform[2] = 0;             /* rotation, 0 if image is "north up" */
    adfGeoTransform[3] = v;             /* top left y */
    adfGeoTransform[4] = 0;             /* rotation, 0 if image is "north up" */
    adfGeoTransform[5] = map.yscale * map.level;    /* n-s pixel resolution */

    dataset->SetGeoTransform(adfGeoTransform);

    // create color bands
    GDALRasterBand * bandR  = dataset->GetRasterBand(1);
    GDALRasterBand * bandG  = dataset->GetRasterBand(2);
    GDALRasterBand * bandB  = dataset->GetRasterBand(3);

    bandR->SetColorInterpretation(GCI_RedBand);
    bandG->SetColorInterpretation(GCI_GreenBand);
    bandB->SetColorInterpretation(GCI_BlueBand);

    QByteArray bufferR(map.blockSizeX * map.blockSizeY, 0);
    QByteArray bufferG(map.blockSizeX * map.blockSizeY, 0);
    QByteArray bufferB(map.blockSizeX * map.blockSizeY, 0);


    int n = 0;
    int m = 0;

    double xx1 = lon1;
    double yy1 = lat1;
    convertRad2Pt(map, xx1, yy1);
    xx1 = xx1/map.level;
    yy1 = yy1/map.level;


    double tx1; // tile pixel left
    double ty1; // tile pixel top
    double tx2; // tile pixel right
    double ty2; // tile pixel bottom
    do
    {
        do
        {
            printProgress(prog++, total);
            tx1 = x1 + n * map.blockSizeX;
            ty1 = y1 + m * map.blockSizeY;
            tx2 = x1 + (n + 1) * map.blockSizeX;
            ty2 = y1 + (m + 1) * map.blockSizeY;
            QUrl url(map.url.arg(z).arg(idxx + n).arg(idxy + m));

            QImage img;
            diskCache.restore(url.toString(),img);

            if(img.isNull())
            {
                badTiles++;
                n++;
                continue;
            }

            int xoff    = tx1 - xx1;
            int yoff    = ty1 - yy1;

            if(xoff < 0)
            {
                img     = img.copy(-xoff, 0, img.width() + xoff, img.height());
                xoff    = 0;
            }
            if(yoff < 0)
            {
                img     = img.copy(0, -yoff, img.width(), img.height() + yoff);
                yoff    = 0;
            }

            if((xoff + img.width()) > w)
            {
                img     = img.copy(0, 0, w - xoff, img.height());
            }

            if((yoff + img.height()) > h)
            {
                img     = img.copy(0, 0, img.width(), h - yoff);
            }

            int width   = img.width();
            int height  = img.height();

            if(img.format() != QImage::Format_ARGB32)
            {
                img = img.convertToFormat(QImage::Format_ARGB32);
            }

            quint8 * pSrc = img.bits();
            for(int i=0; i < (width * height); i++)
            {
                bufferB[i] = *pSrc++;
                bufferG[i] = *pSrc++;
                bufferR[i] = *pSrc++;
                pSrc++;
            }



            bandR->RasterIO(GF_Write, xoff, yoff, width, height, bufferR.data(), width, height, GDT_Byte, 0, 0);
            bandG->RasterIO(GF_Write, xoff, yoff, width, height, bufferG.data(), width, height, GDT_Byte, 0, 0);
            bandB->RasterIO(GF_Write, xoff, yoff, width, height, bufferB.data(), width, height, GDT_Byte, 0, 0);

            n++;
        }
        while(tx2 < x2);

        n = 0;
        m++;
    }
    while(ty2 < y2);

    if(badTiles)
    {
        fprintf(stderr, "\n\nWarning: I could not summon all tiles from the cache. %i tiles are missing.\n", badTiles);
    }


    dataset->FlushCache();
    delete dataset;
    GDALDestroyDriverManager();
    printf("\n");
    return 0;
}

static int exportWMS(int level, double lon1, double lat1, double lon2, double lat2, const QString infile, const QString& outfile, CDiskCache& diskCache)
{
    map_t map;
    map.level = level;

    QFile file(infile);
    if(!file.open(QIODevice::ReadOnly))
    {
        fprintf(stderr, "Failed to open %s\n", infile.toLocal8Bit().data());
        exit(-1);
    }

    QString msg;
    int line, column;
    QDomDocument dom;
    if(!dom.setContent(&file, true, &msg, &line, &column))
    {
        file.close();
        fprintf(stderr, "Failed to read: %s\nline %i, column %i:\n %s", infile.toLocal8Bit().data(), line, column, msg.toLocal8Bit().data());
        exit(-1);
    }
    file.close();

    QDomElement gdal        = dom.firstChildElement("GDAL_WMS");
    QDomElement service     = gdal.firstChildElement("Service");
    QDomElement datawindow  = gdal.firstChildElement("DataWindow");

    QString format          = service.firstChildElement("ImageFormat").text();
    QString layers          = service.firstChildElement("Layers").text();
    QString version         = service.firstChildElement("Version").text();
    QString srs             = service.firstChildElement("SRS").text();


    if(srs.toLower().startsWith("epsg"))
    {
        QString str = QString("+init=%1").arg(srs.toLower());
        map.pjsrc = pj_init_plus(str.toLocal8Bit());
    }
    else
    {
        map.pjsrc = pj_init_plus(srs.toLower().toLocal8Bit());
    }


    if(map.pjsrc == 0)
    {
        fprintf(stderr, "Unknown projection %s", srs.toLocal8Bit().data());
        exit(-1);
    }

    map.url         = service.firstChildElement("ServerUrl").text();

    map.blockSizeX  = gdal.firstChildElement("BlockSizeX").text().toUInt();
    map.blockSizeY  = gdal.firstChildElement("BlockSizeY").text().toUInt();

    map.xsize_px    = datawindow.firstChildElement("SizeX").text().toInt();
    map.ysize_px    = datawindow.firstChildElement("SizeY").text().toInt();

    if (pj_is_latlong(map.pjsrc))
    {
        map.xref1 = datawindow.firstChildElement("UpperLeftX").text().toDouble()  * DEG_TO_RAD;
        map.yref1 = datawindow.firstChildElement("UpperLeftY").text().toDouble()  * DEG_TO_RAD;
        map.xref2 = datawindow.firstChildElement("LowerRightX").text().toDouble() * DEG_TO_RAD;
        map.yref2 = datawindow.firstChildElement("LowerRightY").text().toDouble() * DEG_TO_RAD;
    }
    else
    {
        map.xref1 = datawindow.firstChildElement("UpperLeftX").text().toDouble();
        map.yref1 = datawindow.firstChildElement("UpperLeftY").text().toDouble();
        map.xref2 = datawindow.firstChildElement("LowerRightX").text().toDouble();
        map.yref2 = datawindow.firstChildElement("LowerRightY").text().toDouble();
    }

    map.xscale   = (map.xref2 - map.xref1) / map.xsize_px;
    map.yscale   = (map.yref2 - map.yref1) / map.ysize_px;

    printf("map source:         %s\n", map.url.toLocal8Bit().data());
    printf("map projection:     %s\n", srs.toLocal8Bit().data());
    printf("SizeX [pixel]:      %i\n", map.xsize_px);
    printf("SizeY [pixel]:      %i\n", map.ysize_px);
    if (pj_is_latlong(map.pjsrc))
    {
        printf("ScaleX [rad/pixel]: %f\n", map.xscale);
        printf("ScaleY [rad/pixel]: %f\n", map.yscale);
    }
    else
    {
        printf("ScaleX [m/pixel]:   %f\n", map.xscale);
        printf("ScaleX [m/pixel]:   %f\n", map.yscale);
    }


    // convert to abs pixel in map
    double x1 = lon1;
    double y1 = lat1;
    convertRad2Pt(map, x1, y1);

    double x2 = lon2;
    double y2 = lat2;
    convertRad2Pt(map, x2, y2);

    int w = int(x2 - x1)/map.level;
    int h = int(y2 - y1)/map.level;


    // quantify to smalles multiple of blocksize
    x1 = floor(x1/(map.blockSizeX * map.level)) * map.blockSizeX * map.level;
    y1 = floor(y1/(map.blockSizeY * map.level)) * map.blockSizeY * map.level;


    int total       = ceil((x2 - x1)/(map.blockSizeX*map.level)) * ceil((y2 - y1)/(map.blockSizeY*map.level));
    int prog        = 1;
    int badTiles    = 0;

    printf("\n");
    printf("Export area of %i x %i pixel\n", w, h);
    printf("Need to summon %i tiles from cache.\n\n", total);

    // create output dataset
    char * cargs[] = {"tiled=yes", "compress=DEFLATE", 0};
    GDALDriverManager * drvman  = GetGDALDriverManager();
    GDALDriver * driver         = drvman->GetDriverByName("GTiff");
    GDALDataset * dataset       = driver->Create(outfile.toLocal8Bit().data(), w, h, 3, GDT_Byte, cargs);

    if(dataset == 0)
    {
        fprintf(stderr, "Failed top open target file %s", outfile.toLocal8Bit().data());
        exit(-1);
    }

    // set projection of output, same as input
    char * ptr = 0;
    OGRSpatialReference oSRS;
    oSRS.importFromProj4(pj_get_def(map.pjsrc,0));
    oSRS.exportToWkt(&ptr);
    dataset->SetProjection(ptr);
    CPLFree(ptr);

    // set referencing data
    double adfGeoTransform[6] = {0};
    if (pj_is_latlong(map.pjsrc))
    {
        double u = lon1;
        double v = lat1;
        convertRad2M(map, u, v);

        adfGeoTransform[0] = u * RAD_TO_DEG;           /* top left x */
        adfGeoTransform[1] = map.xscale * level * RAD_TO_DEG;   /* w-e pixel resolution */
        adfGeoTransform[2] = 0;                         /* rotation, 0 if image is "north up" */
        adfGeoTransform[3] = v  * RAD_TO_DEG;           /* top left y */
        adfGeoTransform[4] = 0;                         /* rotation, 0 if image is "north up" */
        adfGeoTransform[5] = map.yscale * level * RAD_TO_DEG;   /* n-s pixel resolution */
    }
    else
    {
        double u = lon1;
        double v = lat1;
        convertRad2M(map, u, v);

        adfGeoTransform[0] = u;             /* top left x */
        adfGeoTransform[1] = map.xscale * level ;    /* w-e pixel resolution */
        adfGeoTransform[2] = 0;             /* rotation, 0 if image is "north up" */
        adfGeoTransform[3] = v;             /* top left y */
        adfGeoTransform[4] = 0;             /* rotation, 0 if image is "north up" */
        adfGeoTransform[5] = map.yscale * level ;    /* n-s pixel resolution */

    }
    dataset->SetGeoTransform(adfGeoTransform);

    // create color bands
    GDALRasterBand * bandR  = dataset->GetRasterBand(1);
    GDALRasterBand * bandG  = dataset->GetRasterBand(2);
    GDALRasterBand * bandB  = dataset->GetRasterBand(3);

    bandR->SetColorInterpretation(GCI_RedBand);
    bandG->SetColorInterpretation(GCI_GreenBand);
    bandB->SetColorInterpretation(GCI_BlueBand);

    QByteArray bufferR(map.blockSizeX * map.blockSizeY, 0);
    QByteArray bufferG(map.blockSizeX * map.blockSizeY, 0);
    QByteArray bufferB(map.blockSizeX * map.blockSizeY, 0);

    double xx1 = lon1;
    double yy1 = lat1;
    convertRad2Pt(map, xx1, yy1);


    int n = 0;
    int m = 0;

    double tx1; // tile pixel left
    double ty1; // tile pixel top
    double tx2; // tile pixel right
    double ty2; // tile pixel bottom

    do
    {
        do
        {
            printProgress(prog++, total);

            double p1x = tx1 = x1 + n * map.blockSizeX * map.level;
            double p1y = ty1 = y1 + m * map.blockSizeY * map.level;
            double p2x = tx2 = x1 + (n + 1) * map.blockSizeX * map.level;
            double p2y = ty2 = y1 + (m + 1) * map.blockSizeY * map.level;

            convertPt2M(map, p1x, p1y);
            convertPt2M(map, p2x, p2y);


            QString bbox;
            if(pj_is_latlong(map.pjsrc))
            {
                bbox = QString("%1,%2,%3,%4").arg(p1x*RAD_TO_DEG,0,'f').arg(p2y*RAD_TO_DEG,0,'f').arg(p2x*RAD_TO_DEG,0,'f').arg(p1y*RAD_TO_DEG,0,'f');
            }
            else
            {
                bbox = QString("%1,%2,%3,%4").arg(p1x,0,'f').arg(p2y,0,'f').arg(p2x,0,'f').arg(p1y,0,'f');
            }

            QUrl url(map.url);
#ifdef QK_QT5_PORT
	    QUrlQuery urlQuery;
            urlQuery.addQueryItem("request", "GetMap");
            urlQuery.addQueryItem("version", version);
            urlQuery.addQueryItem("layers", layers);
            urlQuery.addQueryItem("styles", "");
            urlQuery.addQueryItem("srs", srs);
            urlQuery.addQueryItem("format", format);
            urlQuery.addQueryItem("width", QString::number(map.blockSizeX));
            urlQuery.addQueryItem("height", QString::number(map.blockSizeY));
            urlQuery.addQueryItem("bbox", bbox);
	    url.setQuery(urlQuery);
#else
	    url.addQueryItem("request", "GetMap");
	    url.addQueryItem("version", version);
	    url.addQueryItem("layers", layers);
	    url.addQueryItem("styles", "");
	    url.addQueryItem("srs", srs);
	    url.addQueryItem("format", format);
	    url.addQueryItem("width", QString::number(map.blockSizeX));
            url.addQueryItem("height", QString::number(map.blockSizeY));
	    url.addQueryItem("bbox", bbox);
#endif
            QImage img;
            diskCache.restore(url.toString(),img);

            if(img.isNull())
            {
                badTiles++;
                n++;
                continue;
            }

            int xoff    = (tx1 - xx1)/map.level;
            int yoff    = (ty1 - yy1)/map.level;


            if(xoff < 0)
            {
                img     = img.copy(-xoff, 0, img.width() + xoff, img.height());
                xoff    = 0;
            }
            if(yoff < 0)
            {
                img     = img.copy(0, -yoff, img.width(), img.height() + yoff);
                yoff    = 0;
            }

            if((xoff + img.width()) > w)
            {
                img     = img.copy(0, 0, w - xoff, img.height());
            }

            if((yoff + img.height()) > h)
            {
                img     = img.copy(0, 0, img.width(), h - yoff);
            }

            int width   = img.width();
            int height  = img.height();

            img.save(QString("%1_%2.png").arg(tx1).arg(ty1));

            if(img.format() != QImage::Format_ARGB32)
            {
                img = img.convertToFormat(QImage::Format_ARGB32);
            }

            quint8 * pSrc = img.bits();
            for(int i=0; i < (width * height); i++)
            {
                bufferB[i] = *pSrc++;
                bufferG[i] = *pSrc++;
                bufferR[i] = *pSrc++;
                pSrc++;
            }

            bandR->RasterIO(GF_Write, xoff, yoff, width, height, bufferR.data(), width, height, GDT_Byte, 0, 0);
            bandG->RasterIO(GF_Write, xoff, yoff, width, height, bufferG.data(), width, height, GDT_Byte, 0, 0);
            bandB->RasterIO(GF_Write, xoff, yoff, width, height, bufferB.data(), width, height, GDT_Byte, 0, 0);

            n++;
        }
        while(tx2 < x2);

        n = 0;
        m++;
    }
    while(ty2 < y2);

    if(badTiles)
    {
        fprintf(stderr, "\n\nWarning: I could not summon all tiles from the cache. %i tiles are missing.\n", badTiles);
    }


    dataset->FlushCache();
    delete dataset;
    GDALDestroyDriverManager();
    printf("\n");
    return 0;
}


