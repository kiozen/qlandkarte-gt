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
#include "CFileGenerator.h"

#include <stdio.h>
#include <math.h>
#include <gdal_priv.h>
#include <ogr_spatialref.h>

#ifndef _MKSTR_1
#define _MKSTR_1(x)         #x
#define _MKSTR(x)           _MKSTR_1(x)
#endif

#define VER_STR             _MKSTR(VER_MAJOR)"."_MKSTR(VER_MINOR)"."_MKSTR(VER_STEP)


extern "C"
{
    #include <jpeglib.h>
}

#include <QtCore>
#include <QImage>
#include <QColor>
#include <QPainter>

#define TILE_SIZE       256

#define N_TILES_X       9
#define N_TILES_Y       9
#define N_BIG_TILES_X   9
#define N_BIG_TILES_Y   9

#define INDEX_BMP2BIT   0
#define INDEX_BMP4BIT   1
#define INDEX_CVGMAP    2
#define INDEX_RMPINI    3

#define INDEX_OFFSET_A00  4
#define INDEX_OFFSET_TLM  5

static std::vector<JOCTET> jpgbuf;

static const char bmp2bit[] =
{
    73, 99, 111, 110, 32, 102, 105, 108, 101, 32, 118, 101,
    114, 115, 105, 111, 110, 32, 49, 46, 48, 46, 48, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0,
    0, 1, 0, 0, 60, 0, 0, 0, -16, -1, -1, 127, -40, 0, 0, 0, -15, -1, -1, 127, 116, 1, 0,
    0, 0, 1, 0, 0, -1, -1, -1, -1, 88, 0, 0, 0, -104, 0, 0, 0, 0, 0, 0, 0, 16, 0, 16, 0, 4,
    2, 0, 0, 0, 0, 0, 0, 63, -1, -1, -4, 48, 0, 0, 12, 48, 0, 0, 12, 48, 0, 0, 12, 48, 0,
    0, 12, 48, 0, 0, 12, 48, 0, 0, 12, 48, 0, 0, 12, 48, 0, 0, 12, 48, 0, 0, 12, 48, 0, 0,
    12, 48, 0, 0, 12, 48, 0, 0, 12, 63, -1, -1, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 3, -1, -1, -64, 3, -1, -1, -64, 3, -1, -1, -64, 3, -1, -1, -64, 3, -1, -1,
    -64, 3, -1, -1, -64, 3, -1, -1, -64, 3, -1, -1, -64, 3, -1, -1, -64, 3, -1, -1, -64, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -16, -1, -1, 127, -1, -1, -1, -1, -12, 0, 0, 0, 52, 1,
    0, 0, 0, 0, 0, 0, 1, 0, 64, 0, 4, 8, 0, 0, 81, -3, -40, -70, -124, 24, -99, -19, 46,
    -36, -3, -21, -128, 98, 96, -75, 82, -99, 78, 108, -79, 18, -9, -36, -20, -88, -41, 84,
    -99, 88, 98, 40, 74, 93, 118, -31, -73, -62, -98, -43, -41, 12, -75, 60, 69, 54, -54,
    -40, 51, -73, -18, 36, 83, 86, -8, 80, -2, 105, -61, -122, 37, 114, 7, -7, 0, -1, -1,
    -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1,
    -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1,
    -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -15, -1, -1, 127, -1,
    -1, -1, -1, -112, 1, 0, 0, -88, 1, 0, 0, 0, 0, 0, 0, 1, 0, 24, 0, 4, 8, 0, 0, 49, 50,
    51, 52, 53, 54, 55, 56, 57, 48, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107,
    108, 109, 110, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1,
    -1, -1, 0, -1, -1
};

static const char bmp4bit[] =
{
    73, 99, 111, 110, 32, 102, 105, 108, 101, 32, 118, 101,
    114, 115, 105, 111, 110, 32, 49, 46, 48, 46, 48, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0,
    0, 1, 0, 0, 60, 0, 0, 0, -16, -1, -1, 127, 88, 1, 0, 0, -15, -1, -1, 127, -12, 1, 0, 0,
    0, 1, 0, 0, -1, -1, -1, -1, 88, 0, 0, 0, -40, 0, 0, 0, 0, 0, 0, 0, 16, 0, 16, 0, 4, 4,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 51, 51, 51, 51, 51, 51, 48, 3, 0, 0, 0, 0, 0, 0, 48,
    3, 0, 0, 0, 0, 0, 0, 48, 3, 0, 0, 0, 0, 0, 0, 48, 3, 0, 0, 0, 0, 0, 0, 48, 3, 0, 0, 0,
    0, 0, 0, 48, 3, 0, 0, 0, 0, 0, 0, 48, 3, 0, 0, 0, 0, 0, 0, 48, 3, 0, 0, 0, 0, 0, 0, 48,
    3, 0, 0, 0, 0, 0, 0, 48, 3, 0, 0, 0, 0, 0, 0, 48, 3, 0, 0, 0, 0, 0, 0, 48, 3, 0, 0, 0,
    0, 0, 0, 48, 3, 51, 51, 51, 51, 51, 51, 48, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15, -1, -1, -1, -1, -16, 0, 0,
    15, -1, -1, -1, -1, -16, 0, 0, 15, -1, -1, -1, -1, -16, 0, 0, 15, -1, -1, -1, -1, -16,
    0, 0, 15, -1, -1, -1, -1, -16, 0, 0, 15, -1, -1, -1, -1, -16, 0, 0, 15, -1, -1, -1, -1,
    -16, 0, 0, 15, -1, -1, -1, -1, -16, 0, 0, 15, -1, -1, -1, -1, -16, 0, 0, 15, -1, -1,
    -1, -1, -16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    -16, -1, -1, 127, -1, -1, -1, -1, 116, 1, 0, 0, -76, 1, 0, 0, 0, 0, 0, 0, 1, 0, 64, 0,
    4, 8, 0, 0, 81, -3, -40, -70, -124, 24, -99, -19, 46, -36, -3, -21, -128, 98, 96, -75,
    82, -99, 78, 108, -79, 18, -9, -36, -20, -88, -41, 84, -99, 88, 98, 40, 74, 93, 118,
    -31, -73, -62, -98, -43, -41, 12, -75, 60, 69, 54, -54, -40, 51, -73, -18, 36, 83, 86,
    -8, 80, -2, 105, -61, -122, 37, 114, 7, -7, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1,
    -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1,
    -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1,
    -1, 0, -1, -1, -1, -1, -1, -1, 0, -15, -1, -1, 127, -1, -1, -1, -1, 16, 2, 0, 0, 40, 2,
    0, 0, 0, 0, 0, 0, 1, 0, 24, 0, 4, 8, 0, 0, 49, 50, 51, 52, 53, 54, 55, 56, 57, 48, 97,
    98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 0, -1, -1, -1, -1, -1,
    -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, 0, -1, -1
};

static const char * cvgmap =
";Map Support File : Contains Meta Data Information about the Image\015\012"
"IMG_NAME = %name%\015\012"
"PRODUCT = %product%\015\012"
"PROVIDER = %provider%\015\012"
"IMG_DATE = %date%\015\012"
"IMG_VERSION = \015\012"
"Version = \015\012"
"BUILD =\015\012"
"VENDOR_ID = -1\015\012"
"REGION_ID = -1\015\012"
"MAP_TYPE = TNDB_RASTER_MAP\015\012"
"ADDITIONAL_COMMENTS = Created with map2rmp %my_version%\015\012"
"%copyright%"
;

/// this code is from the GDAL project
void printProgress(int current, int total)
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
    else
    {
        fflush( stdout );
    }

}

bool qSortInFiles(CFileGenerator::file_t& f1, CFileGenerator::file_t& f2)
{
    return f1.xscale < f2.xscale;
}

void CFileGenerator::file_t::convertPx2Deg(double& u, double& v)
{
    u = u*xscale  + lon1;
    v = v*yscale  + lat1;
}

void CFileGenerator::file_t::convertDeg2Px(double& u, double& v)
{
    u = (u - lon1) / xscale;
    v = (v - lat1) / yscale;
}

void CFileGenerator::file_t::roundDeg2Tile(double& u, double& v)
{
    u = qRound(u / (TILE_SIZE * level.xscale)) * TILE_SIZE * level.xscale;
    v = qRound(v / (TILE_SIZE * level.yscale)) * TILE_SIZE * level.yscale;
}

void CFileGenerator::file_t::roundPx2Tile(double& u, double& v)
{
    convertPx2Deg(u,v);
    roundDeg2Tile(u,v);
    convertDeg2Px(u,v);
}


CFileGenerator::CFileGenerator(const QStringList& input, const QString& output, const QString &provider, const QString &product, const QString& copyright, int quality, int subsampling, bool intermediateLevels)
    : input(input)
    , output(output)
    , provider(provider)
    , product(product)
    , copyright(copyright)
    , quality(quality)
    , subsampling(subsampling)
    , intermediateLevels(intermediateLevels)
    , tileBuf08Bit(32*TILE_SIZE * TILE_SIZE,0)
    , tileBuf24Bit(32*TILE_SIZE * TILE_SIZE * 3,0)
    , nTilesTotal(0)
    , nTilesProcessed(0)
{
    epsg4326 = pj_init_plus("+init=epsg:4326");

    for(int i = 0; i < MAX_ZOOM_LEVEL; i++)
    {
        scales[i].xscale =  360.0 / (1<<i) / TILE_SIZE;
        scales[i].yscale = -180.0 / (1<<i) / TILE_SIZE;

//        qDebug() << (scales[i].xscale* TILE_SIZE) << (scales[i].yscale * TILE_SIZE);
    }
}

CFileGenerator::~CFileGenerator()
{
    if(epsg4326) pj_free(epsg4326);
}

void CFileGenerator::findBestLevelScale(file_level_t &scale)
{
    int idx = 0;
    double delta = 100000.0;

    for(int i = 0; i < MAX_ZOOM_LEVEL; i++)
    {
        double tmpDelta = fabs(scales[i].xscale - scale.xscale);
        if(tmpDelta < delta)
        {
            delta   = tmpDelta;
            idx     = i;
        }
    }

//    qDebug() << "z" << idx;
    scale.z      = idx;
    scale.xscale = scales[idx].xscale;
    scale.yscale = scales[idx].yscale;
}

int CFileGenerator::start()
{
    fprintf(stdout,"analyze input files:\n");

    QList<file_t> infiles;
    QList<file_t> levels;
    QVector<rmp_file_t> outfiles;

    OGRSpatialReference oSRS_EPSG4326;
    oSRS_EPSG4326.importFromProj4("+init=epsg:4326");

    foreach(const QString& filename, input)
    {

        OGRSpatialReference oSRS;
        char projstr[1024]          = {0};
        double adfGeoTransform[6]   = {0};
        file_t file;

        file.name = filename;

        file.dataset = (GDALDataset*)GDALOpen(filename.toLocal8Bit(),GA_ReadOnly);
        if(file.dataset == 0)
        {
            fprintf(stderr,"\nFailed to open %s\n", filename.toLocal8Bit().data());
            exit(-1);
        }

        char * ptr = projstr;
        if(file.dataset->GetProjectionRef())
        {
            strncpy(projstr,file.dataset->GetProjectionRef(),sizeof(projstr));
        }
        oSRS.importFromWkt(&ptr);

        if(!oSRS.IsSame(&oSRS_EPSG4326))
        {
            fprintf(stderr,"\nBad projection of file %s. The file must have EPSG:4326.\n", filename.toLocal8Bit().data());
            exit(-1);
        }

        qint32 rasterBandCount = file.dataset->GetRasterCount();
        if(rasterBandCount == 1)
        {
            GDALRasterBand * pBand;
            pBand = file.dataset->GetRasterBand(1);


            if(pBand->GetColorInterpretation() ==  GCI_PaletteIndex )
            {
                GDALColorTable * pct = pBand->GetColorTable();
                for(int i=0; i < pct->GetColorEntryCount(); ++i)
                {
                    const GDALColorEntry& e = *pct->GetColorEntry(i);
                    file.colortable[i] = ((e.c4 & 0x0ff) << 24) | ((e.c3 & 0x0ff) << 16) | ((e.c2 & 0x0ff) << 8) | (e.c1 & 0x0ff);
                }
            }
            else if(pBand->GetColorInterpretation() ==  GCI_GrayIndex )
            {
                for(int i=0; i < 256; ++i)
                {
                    file.colortable[i] << i | (i << 8) | (i << 16) | 0xFF000000;
                }
            }
            else
            {
                fprintf(stderr,"\nFile must be 8 bit palette or gray indexed. %s\n", filename.toLocal8Bit().data());
                exit(-1);
            }

            int success = 0;
            int idx = pBand->GetNoDataValue(&success);

            if(success)
            {
                file.colortable[idx] &= 0x00FFFFFF;
            }
        }

        file.dataset->GetGeoTransform( adfGeoTransform );
        file.xsize = file.dataset->GetRasterXSize();
        file.ysize = file.dataset->GetRasterYSize();
        file.xscale = adfGeoTransform[1];
        file.yscale = adfGeoTransform[5];
        file.lon1   = adfGeoTransform[0];
        file.lat1   = adfGeoTransform[3];
        file.lon2   = file.lon1 + file.xscale * file.xsize;
        file.lat2   = file.lat1 + file.yscale * file.ysize;
        file.level.xscale = file.xscale;
        file.level.yscale = file.yscale;

        findBestLevelScale(file.level);

        file.level.xsize = qRound(file.xsize * file.xscale / file.level.xscale);
        file.level.ysize = qRound(file.ysize * file.yscale / file.level.yscale);

        if(file.level.xsize < 2*TILE_SIZE || file.level.ysize < 2*TILE_SIZE)
        {
            fprintf(stderr,"\nfile %s too small. Minimum size is %ix%i pixel.\n", filename.toLocal8Bit().data(),2*TILE_SIZE,2*TILE_SIZE);
            fprintf(stderr,"File is skipped\n");
            continue;
        }

        infiles << file;
    }
    qSort(infiles.begin(), infiles.end(), qSortInFiles);

    if(intermediateLevels)
    {
        for(int i = 0; i < (infiles.size() - 1); i++)
        {
            int idx1 = infiles[i].level.z;
            int idx2 = infiles[i + 1].level.z;

            file_t f = infiles[i];
            for(int z = 0; z < (idx1 - idx2); z++)
            {
                f.level.xscale = scales[f.level.z].xscale;
                f.level.yscale = scales[f.level.z].yscale;

                f.level.xsize = qRound(f.xsize * f.xscale / f.level.xscale);
                f.level.ysize = qRound(f.ysize * f.yscale / f.level.yscale);

                if(f.level.xsize < 512 || f.level.ysize < 512)
                {
                    break;
                }

                levels << f;
                f.level.z--;
            }
        }


        file_t f = infiles.last();
        for(int z = 0; z < 3; z++)
        {
            f.level.xscale = scales[f.level.z].xscale;
            f.level.yscale = scales[f.level.z].yscale;

            f.level.xsize = qRound(f.xsize * f.xscale / f.level.xscale);
            f.level.ysize = qRound(f.ysize * f.yscale / f.level.yscale);

            if(f.level.xsize < 512 || f.level.ysize < 512)
            {
                break;
            }

            levels << f;
            f.level.z--;
        }
    }
    else
    {
        levels = infiles;
    }

    foreach(const file_t& file, levels)
    {
        fprintf(stdout, "\n%s\n", file.name.toLocal8Bit().data());
        fprintf(stdout, "p1 lon/lat:        %1.8f %1.8f\n", file.lon1, file.lat1);
        fprintf(stdout, "p2 lon/lat:        %1.8f %1.8f\n", file.lon2, file.lat2);
        fprintf(stdout, "x/y scale (file):  %1.8f %1.8f\n", file.xscale, file.yscale);
        fprintf(stdout, "x/y pixel (file):  %i %i\n", file.xsize, file.ysize);

        fprintf(stdout, "x/y scale (level): %1.6f %1.6f\n", file.level.xscale, file.level.yscale);
        fprintf(stdout, "x/y pixel (level): %i %i\n", file.level.xsize, file.level.ysize);
    }

    fprintf(stdout, "\n");
    file_t& file = levels.first();

    // calculate the basic map area
    double lon1 = 0;
    double lat1 = 0;
    double lon2 = file.xsize;
    double lat2 = file.ysize;

    file.convertPx2Deg(lon1, lat1);
    file.convertPx2Deg(lon2, lat2);

    file.roundDeg2Tile(lon1, lat1);
    file.roundDeg2Tile(lon2, lat2);

    double u1 = lon1;
    double v1 = lat1;
    double u2 = lon2;
    double v2 = lat2;

    file.convertDeg2Px(u1, v1);
    file.convertDeg2Px(u2, v2);

//    qDebug() << lon1 << lat1 << lon2 << lat2;
//    qDebug() << qRound(u1) << qRound(v1) << qRound(u2) << qRound(v2);
//    qDebug() << (lon2 - lon1)/(TILE_SIZE * file.level.xscale) << (lat2 - lat1)/(TILE_SIZE * file.level.yscale);

    int nTilesX     = (lon2 - lon1)/(TILE_SIZE * file.level.xscale);
    int nTilesY     = (lat2 - lat1)/(TILE_SIZE * file.level.yscale);
    int nBigTilesX  = ceil(double(nTilesX) / N_TILES_X);
    int nBigTilesY  = ceil(double(nTilesY) / N_TILES_Y);
    int nPartX      = ceil(double(nBigTilesX) / N_BIG_TILES_X);
    int nPartY      = ceil(double(nBigTilesY) / N_BIG_TILES_Y);

    fprintf(stdout, "x/y sect: %i %i\n", nPartX, nPartY);
    fprintf(stdout, "files: %i \n", nPartX * nPartY);

    outfiles.resize(nPartX * nPartY);

    double lon0 = lon1;
    double lat0 = lat1;
    for(int y = 0; y < nPartY; y++)
    {
        for(int x = 0; x < nPartX; x++)
        {
            const int index = x + y * nPartX;
            rmp_file_t& rmp = outfiles[index];
            rmp.index = index;

            double lon1 = lon0 + x * file.level.xscale * TILE_SIZE * N_TILES_X * N_BIG_TILES_X;
            double lat1 = lat0 + y * file.level.yscale * TILE_SIZE * N_TILES_Y * N_BIG_TILES_Y;
            double lon2 = lon1 + file.level.xscale * TILE_SIZE * N_TILES_X * N_BIG_TILES_X;
            double lat2 = lat1 + file.level.yscale * TILE_SIZE * N_TILES_Y * N_BIG_TILES_Y;

            setupOutFile(lon1, lat1, lon2, lat2, levels, rmp);

            rmp.directory.resize(4 + rmp.levels.size() * 2);
            sprintf(rmp.directory[INDEX_BMP2BIT].name, "bmp2bit");
            sprintf(rmp.directory[INDEX_BMP2BIT].ext, "ics");
            sprintf(rmp.directory[INDEX_BMP4BIT].name, "bmp4bit");
            sprintf(rmp.directory[INDEX_BMP4BIT].ext, "ics");
            sprintf(rmp.directory[INDEX_RMPINI].name, "rmp");
            sprintf(rmp.directory[INDEX_RMPINI].ext, "ini");
            sprintf(rmp.directory[INDEX_CVGMAP].name, "cvg_map");
            sprintf(rmp.directory[INDEX_CVGMAP].ext, "msf");

            QFileInfo fi(rmp.name);
            QString name = fi.baseName().left(7);
            for(int i = 0; i < rmp.levels.size(); i++)
            {
                sprintf(rmp.directory[INDEX_OFFSET_A00 + i * 2].name, "%s%i", name.toLocal8Bit().data(), i);
                sprintf(rmp.directory[INDEX_OFFSET_A00 + i * 2].ext, "a00");
                sprintf(rmp.directory[INDEX_OFFSET_TLM + i * 2].name, "%s%i", name.toLocal8Bit().data(), i);
                sprintf(rmp.directory[INDEX_OFFSET_TLM + i * 2].ext, "tlm");
            }

            if(!copyright.isEmpty())
            {
                rmp_dir_entry_t e;
                sprintf(e.name, "cprt_txt");
                sprintf(e.ext,"txt");
                rmp.directory << e;
            }
        }
    }

    printf("\n\n");
    for(int i = 0; i < outfiles.size(); i++)
    {
        rmp_file_t& rmp = outfiles[i];
        writeRmp(rmp);
    }

    return 0;

}


void CFileGenerator::setupOutFile(double lon1, double lat1, double lon2, double lat2, QList<file_t> &infiles, rmp_file_t &rmp)
{
    QFileInfo fi(output);
    QDir dir(fi.absolutePath());

    if(rmp.index != 0)
    {
        rmp.name = dir.absoluteFilePath(fi.baseName() + QString("_%1").arg(rmp.index) + ".rmp");
    }
    else
    {
        rmp.name = dir.absoluteFilePath(fi.baseName() + ".rmp");
    }
    rmp.levels.resize(infiles.size());

    printf("\nsetup file: %s\n", rmp.name.toLocal8Bit().data());
    for(int i = 0; i < infiles.size(); i++)
    {
        rmp_level_t& level  = rmp.levels[i];
        level.src           = &infiles[i];
        level.nTiles        = 0;

        file_t& file        = *level.src;

        file.roundDeg2Tile(lon1, lat1);

        if(lon2 > file.lon2)
        {
            lon2 = file.lon2;
        }
        if(lat2 < file.lat2)
        {
            lat2 = file.lat2;
        }
        file.roundDeg2Tile(lon2, lat2);


        double u1 = level.lon1 = lon1;
        double v1 = level.lat1 = lat1;
        double u2 = level.lon2 = lon2;
        double v2 = level.lat2 = lat2;

        file.convertDeg2Px(u1, v1);
        file.convertDeg2Px(u2, v2);

        level.x1 = qRound(u1);
        level.y1 = qRound(v1);
        level.x2 = qRound(u2);
        level.y2 = qRound(v2);

        qint32 nTilesX = ceil((lon2 - lon1)/(file.level.xscale * TILE_SIZE));
        qint32 nTilesY = ceil((lat2 - lat1)/(file.level.yscale * TILE_SIZE));

        if(nTilesX * nTilesY < 81)
        {
            // single tile container case

            rmp_tile_container_t& container = level.tileContainers[0];
            level.nContainer = 1;

            for(int x = 0; x < nTilesX; x++)
            {
                for(int y = 0; y < nTilesY; y++)
                {
                    rmp_tile_t& tile = container.tiles[container.nTiles];

                    double u1 = tile.lon1 = lon1 +  x * file.level.xscale * TILE_SIZE;
                    double v1 = tile.lat1 = lat1 +  y * file.level.yscale * TILE_SIZE;
                    double u2 = tile.lon2 = tile.lon1 + file.level.xscale * TILE_SIZE;
                    double v2 = tile.lat2 = tile.lat1 + file.level.yscale * TILE_SIZE;

                    file.convertDeg2Px(u1, v1);
                    file.convertDeg2Px(u2, v2);

                    tile.x1 = qRound(u1);
                    tile.y1 = qRound(v1);
                    tile.x2 = qRound(u2);
                    tile.y2 = qRound(v2);

//                    printf("      tile area (px):  %i,%i (%ix%i)\n", tile.x1, int(tile.y1), int(tile.x2 - tile.x1), int(tile.y2 - tile.y1));
//                    printf("      tile area (deg): %f,%f to %f,%f)\n", tile.lon1, tile.lat1, tile.lon2, tile.lat2);

                    container.nTiles++;
                }
            }
            level.nTiles = container.nTiles;
        }
        else
        {
            // multiple tile container case

            rmp_tile_container_t& idxContainer = level.tileContainers[0];
            level.nContainer = 1;
            level.tileContainers[level.nContainer].nTiles = 0;

            for(int x = 0; x < nTilesX; x++)
            {
                for(int y = 0; y < nTilesY; y++)
                {
                    rmp_tile_container_t& container = level.tileContainers[level.nContainer];
                    rmp_tile_t& tile = container.nTiles == -1 ? idxContainer.tiles[idxContainer.nTiles++] : container.tiles[container.nTiles];


                    double u1 = tile.lon1 = lon1 +  x * file.level.xscale * TILE_SIZE;
                    double v1 = tile.lat1 = lat1 +  y * file.level.yscale * TILE_SIZE;
                    double u2 = tile.lon2 = tile.lon1 + file.level.xscale * TILE_SIZE;
                    double v2 = tile.lat2 = tile.lat1 + file.level.yscale * TILE_SIZE;

                    file.convertDeg2Px(u1, v1);
                    file.convertDeg2Px(u2, v2);

                    tile.x1 = qRound(u1);
                    tile.y1 = qRound(v1);
                    tile.x2 = qRound(u2);
                    tile.y2 = qRound(v2);

//                    printf("      tile area (px):  %i,%i (%ix%i)\n", tile.x1, int(tile.y1), int(tile.x2 - tile.x1), int(tile.y2 - tile.y1));
//                    printf("      tile area (deg): %f,%f to %f,%f)\n", tile.lon1, tile.lat1, tile.lon2, tile.lat2);

                    container.nTiles++;
                    if(container.nTiles == 80)
                    {
                        level.nTiles += container.nTiles;
                        level.nContainer++;
                    }
                }
            }

            if(level.tileContainers[level.nContainer].nTiles != -1)
            {
                level.nTiles += level.tileContainers[level.nContainer].nTiles;
                level.nContainer++;
            }

            level.nTiles += idxContainer.nTiles;
        }
        nTilesTotal += level.nTiles;

        printf("level[%i] area (px):  %i,%i (%ix%i)\tnTiles:  %i\n", i, level.x1, int(level.y1), int(level.x2 - level.x1), int(level.y2 - level.y1), level.nTiles);
        printf("level[%i] area (deg): %f,%f to %f,%f)\n", i, level.lon1, level.lat1, level.lon2, level.lat2);

    }

}

quint16 CFileGenerator::crc16(QDataStream& stream, qint32 length)
{
    bool evenByte   = true;
    quint16 crc     = 0;
    quint8 tmp8;

    for(int i = 0; i < length; i++)
    {
        stream >> tmp8;
        if(evenByte)
        {
            crc ^= tmp8 << 8;
        }
        else
        {
            crc ^= tmp8;
        }
        evenByte = !evenByte;
    }

    return crc;
}

void CFileGenerator::writeRmp(rmp_file_t& rmp)
{
    char magic[30] = "MAGELLAN";
    quint64 pos1, pos2;
    quint16 crc;

    QFile::remove(rmp.name);

    QFile file(rmp.name);
    if(!file.open(QIODevice::ReadWrite))
    {
        fprintf(stderr,"\nFailed to open %s\n", rmp.name.toLocal8Bit().data());
        exit(-1);
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);
    // 1st run for directory as place holder
    writeDirectory(stream, rmp);
    pos1 = stream.device()->pos();
    // write all data listed in directory
    stream.writeRawData(magic, sizeof(magic));
    writeBmp2Bit(stream, rmp);
    writeBmp4Bit(stream, rmp);
    writeCvgMap(stream,rmp);
    writeRmpIni(stream,rmp);

    for(int i = 0; i < rmp.levels.size(); i++)
    {
        writeA00(stream,rmp, i);
        writeTLM(stream,rmp, i);
    }

    if(!copyright.isEmpty())
    {
        writeCopyright(stream, rmp);
    }

    stream.writeRawData(magic, 9);
    // calc crc
    pos2 = stream.device()->pos();
    stream.device()->seek(pos1);
    crc = crc16(stream, pos2 - pos1);
    stream << crc;

    // 2nd run to write real directory
    stream.device()->seek(0);
    writeDirectory(stream, rmp);
}

void CFileGenerator::writeDirectory(QDataStream& stream, rmp_file_t& rmp)
{


    stream.device()->seek(0);
    stream << quint32(rmp.directory.size()) << quint32(rmp.directory.size());
    foreach(const rmp_dir_entry_t& entry, rmp.directory)
    {
        stream.writeRawData(entry.name,sizeof(entry.name));
        stream.writeRawData(entry.ext,sizeof(entry.ext));
        stream << entry.offset << entry.length;
    }

    qint32 length   = stream.device()->pos();
    stream.device()->seek(0);
    quint16 crc     = crc16(stream, length);

    stream << crc;

}

void CFileGenerator::writeBmp2Bit(QDataStream& stream, rmp_file_t& rmp)
{
    rmp.directory[INDEX_BMP2BIT].offset = stream.device()->pos();
    rmp.directory[INDEX_BMP2BIT].length = sizeof(bmp2bit);
    stream.writeRawData(bmp2bit, sizeof(bmp2bit));
}

void CFileGenerator::writeBmp4Bit(QDataStream& stream, rmp_file_t& rmp)
{
    rmp.directory[INDEX_BMP4BIT].offset = stream.device()->pos();
    rmp.directory[INDEX_BMP4BIT].length = sizeof(bmp4bit);
    stream.writeRawData(bmp4bit, sizeof(bmp4bit));
}

void CFileGenerator::writeCvgMap(QDataStream& stream, rmp_file_t& rmp)
{
    QString cvg(cvgmap);
    cvg.replace("%name%",QFileInfo(rmp.name).baseName());
    cvg.replace("%product%",product);
    cvg.replace("%provider%",provider);
    cvg.replace("%date%", QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss"));
    cvg.replace("%my_version%", VER_STR);

    if(copyright.isEmpty())
    {
        cvg.replace("%copyright%","");
    }
    else
    {
        cvg.replace("%copyright%","COPY_RIGHT_LOCATION=cprt_txt.txt");
    }

    rmp.directory[INDEX_CVGMAP].offset = stream.device()->pos();
    rmp.directory[INDEX_CVGMAP].length = cvg.size();
    stream.writeRawData(cvg.toLocal8Bit(), cvg.size());
}

void CFileGenerator::writeCopyright(QDataStream& stream, rmp_file_t& rmp)
{
    QFile f(copyright);
    if(!f.open(QIODevice::ReadOnly))
    {
        fprintf(stderr,"Failed to open copyright notice: %s\n", copyright.toLocal8Bit().data());
        exit(-1);
    }

    QString str = f.readAll();

    rmp.directory.last().offset = stream.device()->pos();
    rmp.directory.last().length = str.size();
    stream.writeRawData(str.toLatin1(), str.size());
}

void CFileGenerator::writeRmpIni(QDataStream& stream, rmp_file_t& rmp)
{
    QString ini("[T_Layers]\n");
    for(int i = 0; i < rmp.levels.size(); i++)
    {
        ini += QString("%1=%2\n").arg(i).arg(rmp.directory[INDEX_OFFSET_A00 + i * 2].name);
    }
    ini.replace("\n", "\015\012");

    rmp.directory[INDEX_RMPINI].offset = stream.device()->pos();
    rmp.directory[INDEX_RMPINI].length = ini.size();
    stream.writeRawData(ini.toLocal8Bit(), ini.size());
}

void CFileGenerator::writeA00(QDataStream& stream, rmp_file_t& rmp, int i)
{
    int w,h, x1, x2, y1, y2;
    quint32 size;
    rmp_level_t& level  = rmp.levels[i];
    quint64 pos1        = stream.device()->pos();

    quint32 totalWidth  = level.src->xsize;
    quint32 totalHeight = level.src->ysize;

    stream << level.nTiles;

    for(int c = 0; c < level.nContainer; c++)
    {
        rmp_tile_container_t& container = level.tileContainers[c];
        for(int t = 0; t < container.nTiles; t++)
        {
            rmp_tile_t& tile = container.tiles[t];

            w = tile.x2 - tile.x1;
            h = tile.y2 - tile.y1;
            QImage img(w,h,QImage::Format_ARGB32);
            img.fill(qRgb(156,168,128));

            if(tile.x1 >= 0 && tile.y1 >= 0 && tile.x2 < level.src->xsize && tile.y2 < level.src->ysize)
            {
                readTile(*level.src, tile.x1, tile.y1, w, h, (quint32*)img.bits());
            }
            else
            {
                // oh dear, let's fill the tile with what ever is left over.

                x1 = (tile.x1 < 0) ? 0 : tile.x1;
                y1 = (tile.y1 < 0) ? 0 : tile.y1;
                x2 = (tile.x2 > totalWidth)  ? totalWidth  : tile.x2;
                y2 = (tile.y2 > totalHeight) ? totalHeight : tile.y2;

                w = x2 - x1;
                h = y2 - y1;

                QImage img1(w,h,QImage::Format_ARGB32);
                readTile(*level.src, x1, y1, w, h, (quint32*)img1.bits());

                QPainter p(&img);
                p.drawImage(x1 - tile.x1, y1 - tile.y1, img1);
            }

            img  = img.scaled(TILE_SIZE,TILE_SIZE,Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            size = writeTile(TILE_SIZE, TILE_SIZE, (quint32*)img.bits(), quality, subsampling);

            tile.offset = stream.device()->pos() - pos1;

            stream << size;
            stream.writeRawData((const char*)&jpgbuf[0], size);

            nTilesProcessed++;
            printProgress(nTilesProcessed, nTilesTotal);
        }
    }

    quint64 pos2 = stream.device()->pos();
    rmp.directory[INDEX_OFFSET_A00 + i * 2].offset = pos1;
    rmp.directory[INDEX_OFFSET_A00 + i * 2].length = pos2 - pos1;
}


void CFileGenerator::writeTLM(QDataStream& stream, rmp_file_t& rmp, int i)
{
    char dummy[4000]    = {0};
    rmp_level_t& level  = rmp.levels[i];
    quint64 pos1        = stream.device()->pos();
    double tileWidth    =   level.src->level.xscale * TILE_SIZE;
    double tileHeight   = - level.src->level.yscale * TILE_SIZE;

    stream << quint32(1);
    stream << quint32(level.nTiles);
    stream << quint16(TILE_SIZE) << quint16(TILE_SIZE);
    stream << quint32(1);
    stream << tileHeight;
    stream << tileWidth;
    stream << level.lon1 << -level.lat1 << level.lon2 << -level.lat2;
    stream.writeRawData(dummy,88);
    stream << quint32(TILE_SIZE); //???

    quint32 size = 256 + 1940 + 2 * 1992 + level.nContainer * 1992;
    stream << size;

    stream.writeRawData(dummy,96);
    // --- end of first 256 bytes ---
    stream << quint32(1);
    stream << quint32(99);
    stream << quint32(1940); //firstBlockOffset
    stream.writeRawData(dummy, 1940 - 12);
    // --- start of 1st container table ---
    rmp_tile_container_t& container = level.tileContainers[0];

    stream << quint32(level.nTiles) << quint16(container.nTiles) << quint16(level.nContainer == 1 ? 1 : 0);
    foreach(const rmp_tile_t& tile, container.tiles)
    {
        qint32 x, y;
        //lon =   x * tlm.tileWidth - 180.0;
        x = qRound((tile.lon1 + 180.0) / tileWidth);
        //lat = -(y * tlm.tileHeight - 90.0);
        y = qRound((-tile.lat1 + 90.0) / tileHeight);

        stream << x << y << quint32(0) << tile.offset;

        Q_ASSERT((tile.lon1 / tileWidth) == int(tile.lon1 / tileWidth));
    }

    for(int c = 1; c < level.tileContainers.size(); c++)
    {
        rmp_tile_container_t& container = level.tileContainers[c];
        if(container.nTiles != -1)
        {
            container.offset = 1940 + c * 1992;
            stream << container.offset;
        }
        else
        {
            stream << quint32(0);
        }
    }

    for(int c = 1; c < level.nContainer; c++)
    {
        rmp_tile_container_t& container = level.tileContainers[c];
        stream.device()->seek(pos1 + 256 + container.offset);

        stream << quint32(container.nTiles) << quint16(container.nTiles) << quint16(1);
        foreach(const rmp_tile_t& tile, container.tiles)
        {
            qint32 x, y;
            //lon =   x * tlm.tileWidth - 180.0;
            x = qRound((tile.lon1 + 180.0) / tileWidth);
            //lat = -(y * tlm.tileHeight - 90.0);
            y = qRound((-tile.lat1 + 90.0) / tileHeight);

            stream << x << y << quint32(0) << tile.offset;

            Q_ASSERT((tile.lon1 / tileWidth) == int(tile.lon1 / tileWidth));
        }
    }

    // --- add two empty blocks ---
    stream.writeRawData(dummy, 1992 * 2);
    quint64 pos2 = stream.device()->pos();
    rmp.directory[INDEX_OFFSET_TLM + i * 2].offset = pos1;
    rmp.directory[INDEX_OFFSET_TLM + i * 2].length = pos2 - pos1;
}


bool CFileGenerator::readTile(file_t& file, const qint32 xoff, const qint32 yoff, const qint32 w1, const qint32 h1, quint32 *output)
{
    qint32 rasterBandCount = file.dataset->GetRasterCount();

    memset(output,-1, sizeof(quint32) * w1 * h1);

    if(rasterBandCount == 1)
    {
        GDALRasterBand * pBand;
        pBand = file.dataset->GetRasterBand(1);
        if(pBand->RasterIO(GF_Read, (int)xoff, (int)yoff, w1, h1, tileBuf08Bit.data(), w1, h1, GDT_Byte,0,0) == CE_Failure)
        {
            return false;
        }

        for(unsigned int i = 0; i < (w1 * h1); i++)
        {
            output[i] = file.colortable[(unsigned char)tileBuf08Bit[i]];
        }
    }
    else
    {
        for(int b = 1; b <= rasterBandCount; ++b)
        {
            GDALRasterBand * pBand;
            pBand = file.dataset->GetRasterBand(b);

            quint32 mask = ~(0x000000FF << (8*(b-1)));

            if(pBand->RasterIO(GF_Read,(int)xoff,(int)yoff, w1, h1, tileBuf08Bit.data(),w1,h1,GDT_Byte,0,0) == CE_Failure)
            {
                return false;
            }

            for(unsigned int i = 0; i < (w1 * h1); i++)
            {
                quint32 pixel = output[i];

                pixel &= mask;
                pixel |= tileBuf08Bit[i] << (8*(b-1));
                output[i] = pixel;
            }
        }
    }

    return true;
}

#define JPG_BLOCK_SIZE (TILE_SIZE*TILE_SIZE)

static void init_destination (j_compress_ptr cinfo)
{
    jpgbuf.resize(JPG_BLOCK_SIZE);
    cinfo->dest->next_output_byte   = &jpgbuf[0];
    cinfo->dest->free_in_buffer     = jpgbuf.size();
}

static boolean empty_output_buffer (j_compress_ptr cinfo)
{
    size_t oldsize = jpgbuf.size();
    jpgbuf.resize(oldsize + JPG_BLOCK_SIZE);
    cinfo->dest->next_output_byte   = &jpgbuf[oldsize];
    cinfo->dest->free_in_buffer     = jpgbuf.size() - oldsize;
    return true;
}
static void term_destination (j_compress_ptr cinfo)
{
    jpgbuf.resize(jpgbuf.size() - cinfo->dest->free_in_buffer);
}

quint32 CFileGenerator::writeTile(quint32 xsize, quint32 ysize, quint32 * raw_image, int quality, int subsampling)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_pointer[1];

    jpeg_destination_mgr destmgr    = {0};
    destmgr.init_destination        = init_destination;
    destmgr.empty_output_buffer     = empty_output_buffer;
    destmgr.term_destination        = term_destination;

    // convert from RGBA to RGB
    for(quint32 r = 0; r < ysize; r++)
    {
        for(quint32 c = 0; c < xsize; c++)
        {
            quint32 pixel = raw_image[r * xsize + c];

            tileBuf24Bit[r * xsize * 3 + c * 3]     =  pixel        & 0x0FF;
            tileBuf24Bit[r * xsize * 3 + c * 3 + 1] = (pixel >>  8) & 0x0FF;
            tileBuf24Bit[r * xsize * 3 + c * 3 + 2] = (pixel >> 16) & 0x0FF;
        }
    }

    cinfo.err = jpeg_std_error( &jerr );
    jpeg_create_compress(&cinfo);

    cinfo.dest              = &destmgr;
    cinfo.image_width       = xsize;
    cinfo.image_height      = ysize;
    cinfo.input_components  = 3;
    cinfo.in_color_space    = JCS_RGB;

    jpeg_set_defaults( &cinfo );

    if (subsampling != -1)
    {
        switch (subsampling)
        {
        case 422:  // 2x1, 1x1, 1x1 (4:2:2) : Medium
            {
                cinfo.comp_info[0].h_samp_factor = 2;
                cinfo.comp_info[0].v_samp_factor = 1;
                cinfo.comp_info[1].h_samp_factor = 1;
                cinfo.comp_info[1].v_samp_factor = 1;
                cinfo.comp_info[2].h_samp_factor = 1;
                cinfo.comp_info[2].v_samp_factor = 1;
                break;
            }
        case 411:  // 2x2, 1x1, 1x1 (4:1:1) : High
            {
                cinfo.comp_info[0].h_samp_factor = 2;
                cinfo.comp_info[0].v_samp_factor = 2;
                cinfo.comp_info[1].h_samp_factor = 1;
                cinfo.comp_info[1].v_samp_factor = 1;
                cinfo.comp_info[2].h_samp_factor = 1;
                cinfo.comp_info[2].v_samp_factor = 1;
                break;
            }
        case 444:  // 1x1 1x1 1x1 (4:4:4) : None
            {
                cinfo.comp_info[0].h_samp_factor = 1;
                cinfo.comp_info[0].v_samp_factor = 1;
                cinfo.comp_info[1].h_samp_factor = 1;
                cinfo.comp_info[1].v_samp_factor = 1;
                cinfo.comp_info[2].h_samp_factor = 1;
                cinfo.comp_info[2].v_samp_factor = 1;
                break;
            }
        }
    }

    if (quality != -1)
    {
        jpeg_set_quality( &cinfo, quality, TRUE );
    }

    jpeg_start_compress( &cinfo, TRUE );

    while( cinfo.next_scanline < cinfo.image_height )
    {
        row_pointer[0] = (JSAMPLE*)&tileBuf24Bit.data()[ cinfo.next_scanline * cinfo.image_width *  cinfo.input_components];
        jpeg_write_scanlines( &cinfo, row_pointer, 1 );
    }
    /* similar to read file, clean up after we're done compressing */
    jpeg_finish_compress( &cinfo );
    jpeg_destroy_compress( &cinfo );

    return jpgbuf.size();
}
