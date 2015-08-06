/**********************************************************************************************
    Copyright (C) 2011 Oliver Eichler oliver.eichler@gmx.de

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
#include <math.h>
#include <wctype.h>

#include <list>

#include <gdal_priv.h>
#include <proj_api.h>
#include <ogr_spatialref.h>

extern "C"
{
    #include <jpeglib.h>
}

#include <qzipwriter.h>

#ifndef _MKSTR_1
#define _MKSTR_1(x)         #x
#define _MKSTR(x)           _MKSTR_1(x)
#endif

#define VER_STR             _MKSTR(VER_MAJOR)"."_MKSTR(VER_MINOR)"."_MKSTR(VER_STEP)
#define WHAT_STR            "map2gcm, Version " VER_STR

#define MAX_TILE_SIZE   1024
#define JPG_BLOCK_SIZE      (MAX_TILE_SIZE * MAX_TILE_SIZE)

#define KMLFILE "doc.kml"

const char * kmzHead =
    "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>"
    "<kml xmlns=\"http://www.opengis.net/kml/2.2\">"
    "<Folder>";

const char * kmzOverlay =
     "\n<GroundOverlay>"
      "<name>%1</name>"
      "<Icon>"
       "<href>%2</href>"
       "<drawOrder>%3</drawOrder>"
      "</Icon>"
      "<LatLonBox>"
       "<north>%4</north>"
       "<south>%5</south>"
       "<east>%6</east>"
       "<west>%7</west>"
       "<rotation>0.0</rotation>"
      "</LatLonBox>"
     "</GroundOverlay>";

const char * kmzFoot =
    "</Folder>"
    "\n</kml>";


struct file_t
{
    file_t(): dataset(0), pj(0){memset(colortable,0, sizeof(colortable));}
    ~file_t()
    {
//        if(dataset) delete dataset;
        if(pj) pj_free(pj);
    }

    bool operator<(const file_t& other) const
    {
        return (xscale < other.xscale);
    }

    std::string     filename;
    std::string     projection;
    GDALDataset *   dataset;
    projPJ             pj;
    uint32_t        width;
    uint32_t        height;
    double          xscale;
    double          yscale;
    double          scale;
    double          xref1;
    double          yref1;
    double          xref2;
    double          yref2;

    double          lon1;
    double          lat1;
    double          lon2;
    double          lat2;

    uint32_t        colortable[256];

};

struct level_t
{
    std::list<file_t *> files;
};

struct tile_t
{
    tile_t() : top(0), right(0), bottom(0), left(0), width(0), height(0), size(0){}
    double  top;
    double  right;
    double  bottom;
    double  left;
    uint16_t width;
    uint16_t height;
    uint32_t size;
};


/// the target lon/lat WGS84 projection
static projPJ  wgs84 = 0;
/// information about all files
static std::list<file_t> files;
/// number of used levels
static int32_t nLevels;
/// up to five levels. nLevels gives the actual count
static level_t levels[5];
/// tile buffer for 8 bit palette tiles, private to readTile
static uint8_t  tileBuf8Bit[MAX_TILE_SIZE * MAX_TILE_SIZE] = {0};
/// tile buffer for 24 bit raw RGB tiles, private to writeTile
static uint8_t tileBuf24Bit[MAX_TILE_SIZE * MAX_TILE_SIZE * 3] = {0};
/// tile buffer for 32 bit raw RGBA tiles
static uint32_t tileBuf32Bit[MAX_TILE_SIZE * MAX_TILE_SIZE] = {0};
///
static std::vector<JOCTET> jpgbuf;

static double distance(const double u1, const double v1, const double u2, const double v2)
{
    double dU = u2 - u1; // lambda
    double dV = v2 - v1; // roh

    double d = 2*asin(sqrt(sin(dV/2) * sin(dV/2) + cos(v1) * cos(v2) * sin(dU/2) * sin(dU/2)));

    return 6371010 * d;
}

static void prinfFileinfo(const file_t& file)
{
    printf("\n\n----------------------");
    printf("\n%s:", file.filename.c_str());
    printf("\nprojection: %s", file.projection.c_str());
    printf("\nwidth: %i pixel height: %i pixel", file.width, file.height);

    if(pj_is_latlong(file.pj))
    {
        printf("\narea (top/left, bottom/right): %f %f, %f %f", file.lat1, file.lon1, file.lat2, file.lon2);
        printf("\nxscale: %f deg/px, yscale: %f deg/px", file.xscale, file.yscale);
    }
    else
    {
        printf("\narea (top/left, bottom/right): %f %f, %f %f", file.lat1, file.lon1, file.lat2, file.lon2);
        printf("\nxscale: %f m/px, yscale: %f m/px", file.xscale, file.yscale);
    }
    printf("\nreal scale: %f m/px", file.scale);
}

bool readTile(uint32_t xoff, uint32_t yoff, uint32_t xsize, uint32_t ysize, file_t& file, uint32_t * output)
{
    GDALDataset * dataset = file.dataset;
    int32_t rasterBandCount = dataset->GetRasterCount();

    memset(output,-1, sizeof(uint32_t) * xsize * ysize);

    if(rasterBandCount == 1)
    {
        GDALRasterBand * pBand;
        pBand = dataset->GetRasterBand(1);
        if(pBand->RasterIO(GF_Read,(int)xoff,(int)yoff, xsize, ysize, tileBuf8Bit,xsize,ysize,GDT_Byte,0,0) == CE_Failure)
        {
            return false;
        }

        for(unsigned int i = 0; i < (xsize * ysize); i++)
        {
            output[i] = file.colortable[tileBuf8Bit[i]];
        }
    }
    else
    {
        for(int b = 1; b <= rasterBandCount; ++b)
        {
            GDALRasterBand * pBand;
            pBand = dataset->GetRasterBand(b);

            uint32_t mask = ~(0x000000FF << (8*(b-1)));

            if(pBand->RasterIO(GF_Read,(int)xoff,(int)yoff, xsize, ysize, tileBuf8Bit,xsize,ysize,GDT_Byte,0,0) == CE_Failure)
            {
                return false;
            }

            for(unsigned int i = 0; i < (xsize * ysize); i++)
            {
                uint32_t pixel = output[i];

                pixel &= mask;
                pixel |= tileBuf8Bit[i] << (8*(b-1));
                output[i] = pixel;
            }
        }
    }

    return true;
}

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


static uint32_t writeTile(uint32_t xsize, uint32_t ysize, uint32_t * raw_image, int quality, int subsampling)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_pointer[1];

    jpeg_destination_mgr destmgr    = {0};
    destmgr.init_destination        = init_destination;
    destmgr.empty_output_buffer     = empty_output_buffer;
    destmgr.term_destination        = term_destination;

    // convert from RGBA to RGB
    for(uint32_t r = 0; r < ysize; r++)
    {
        for(uint32_t c = 0; c < xsize; c++)
        {
            uint32_t pixel = raw_image[r * xsize + c];
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
        row_pointer[0] = (JSAMPLE*)&tileBuf24Bit[ cinfo.next_scanline * cinfo.image_width *  cinfo.input_components];
        jpeg_write_scanlines( &cinfo, row_pointer, 1 );
    }
    /* similar to read file, clean up after we're done compressing */
    jpeg_finish_compress( &cinfo );
    jpeg_destroy_compress( &cinfo );

    return jpgbuf.size();
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
    else
    {
        fflush( stdout );
    }

}

int main(int argc, char ** argv)
{
    char str[1024];
    char projstr[1024];
    OGRSpatialReference oSRS;
    int quality         = -1;
    int subsampling     = -1;
    uint32_t tileCnt    = 0;
    uint32_t zorder     = 50;
    QList<int> selTiles;

    printf("\n****** %s ******\n", WHAT_STR);

    if(argc < 2)
    {
        fprintf(stderr,"\nusage: map2gcm -q <1..100> -s <411|422|444> -t <file> <file1> <file2> ... <fileN> <outputfile>\n");
        fprintf(stderr,"\n");
        fprintf(stderr,"  -q The JPEG quality from 1 to 100. Default is 75 \n");
        fprintf(stderr,"  -s The chroma subsampling. Default is 411  \n");
        fprintf(stderr,"  -t File with list of selected tile index \n");
        fprintf(stderr,"\n");
        fprintf(stderr,"\nThe list of selected tiles is a string in a file. The index count starts");
        fprintf(stderr,"\nwith 0 at the top left corner and is incremented for each element in a row.");
        fprintf(stderr,"\nA 2x2 tile selection will have the index string '0 1 2 3'.");
        fprintf(stderr,"\n");
        fprintf(stderr,"\nThe projection of the input files must have the same latitude along");
        fprintf(stderr,"\na pixel row. Mecator and Longitude/Latitude projections match ");
        fprintf(stderr,"\nthis property. Transversal Merkator or Lambert projections do not.");
        fprintf(stderr,"\n");
        fprintf(stderr,"\nTo rectify a geotiff map, you can use the gdalwarp command, e.g.");
        fprintf(stderr,"\ngdalwarp -t_srs \"EPSG:4326\" <inputfile> <outputfile>");
        fprintf(stderr,"\n");
        fprintf(stderr,"\n");
        exit(-1);
    }

    GDALAllRegister();
    wgs84 = pj_init_plus("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");

    int skip_next_arg = 0;
    int files_count = 0;

    for(int i = 1; i < (argc - 1); i++)
    {
        if (skip_next_arg)
        {
            skip_next_arg = 0;
            continue;
        }

        if (argv[i][0] == '-')
        {
            if (towupper(argv[i][1]) == 'Q')
            {
                quality = atol(argv[i+1]);
                skip_next_arg = 1;
                continue;
            }
            else if (towupper(argv[i][1]) == 'S')
            {
                subsampling = atol(argv[i+1]);
                skip_next_arg = 1;
                continue;
            }
            else if (towupper(argv[i][1]) == 'Z')
            {
                zorder = atol(argv[i+1]);
                skip_next_arg = 1;
                continue;
            }
            else if (towupper(argv[i][1]) == 'T')
            {
                int res;
                int idx;
                FILE * fid = fopen(argv[i+1], "r");

                res = fscanf(fid,"%i ", &idx);
                while(res == 1)
                {
                    selTiles << idx;
                    res = fscanf(fid,"%i ", &idx);
                }

                fclose(fid);
                skip_next_arg = 1;
                continue;
            }
        }

        files_count++;
        files.resize(files_count);

        double dist;

        GDALDataset * dataset = (GDALDataset*)GDALOpen(argv[i],GA_ReadOnly);
        if(dataset == 0)
        {
            fprintf(stderr,"\nFailed to open %s\n", argv[i]);
            exit(-1);
        }

        projPJ   pj;
        char * ptr = projstr;

        if(dataset->GetProjectionRef())
        {
            strncpy(projstr,dataset->GetProjectionRef(),sizeof(projstr));
        }
        oSRS.importFromWkt(&ptr);
        oSRS.exportToProj4(&ptr);

        pj = pj_init_plus(ptr);
        if(pj == 0)
        {
            fprintf(stderr,"\nUnknown projection in file %s\n", argv[i]);
            exit(-1);
        }

        double adfGeoTransform[6];
        dataset->GetGeoTransform( adfGeoTransform );

        std::list<file_t>::iterator f = files.begin();
        std::advance(f, files_count - 1);

        file_t& file    = *f;
        file.filename   = argv[i];
        file.projection = ptr;
        file.dataset    = dataset;
        file.pj         = pj;
        file.width      = dataset->GetRasterXSize();
        file.height     = dataset->GetRasterYSize();
        file.xscale     = adfGeoTransform[1];
        file.yscale     = adfGeoTransform[5];
        file.xref1      = adfGeoTransform[0];
        file.yref1      = adfGeoTransform[3];
        file.xref2      = file.xref1 + file.width  * file.xscale;
        file.yref2      = file.yref1 + file.height * file.yscale;

        if(pj_is_latlong(file.pj))
        {
            file.lon1 = file.xref1;
            file.lat1 = file.yref1;
            file.lon2 = file.xref2;
            file.lat2 = file.yref2;
        }
        else
        {
            file.lon1 = file.xref1;
            file.lat1 = file.yref1;
            file.lon2 = file.xref2;
            file.lat2 = file.yref2;

            pj_transform(pj,wgs84,1,0,&file.lon1,&file.lat1,0);
            pj_transform(pj,wgs84,1,0,&file.lon2,&file.lat2,0);

            file.lon1 *= RAD_TO_DEG;
            file.lat1 *= RAD_TO_DEG;
            file.lon2 *= RAD_TO_DEG;
            file.lat2 *= RAD_TO_DEG;
        }

        dist = distance(file.lon1 * DEG_TO_RAD, file.lat1 * DEG_TO_RAD, file.lon2 * DEG_TO_RAD, file.lat1 * DEG_TO_RAD);
        file.scale = dist/file.width;

        // fill color table if necessary
        GDALRasterBand * pBand;
        pBand = dataset->GetRasterBand(1);

        if(pBand->GetColorInterpretation() == GCI_PaletteIndex)
        {
            GDALColorTable * pct = pBand->GetColorTable();
            for(int c=0; c < pct->GetColorEntryCount(); ++c)
            {
                const GDALColorEntry& e = *pct->GetColorEntry(c);
                file.colortable[c] = e.c1 | (e.c2 << 8) | (e.c3 << 16) | (e.c4 << 24);
            }
        }
        else if(pBand->GetColorInterpretation() ==  GCI_GrayIndex )
        {
            for(int c=0; c < 256; ++c)
            {
                file.colortable[c] = c | (c << 8) | (c << 16) | 0xFF000000;
            }
        }

        int success = 0;
        int idx = pBand->GetNoDataValue(&success);

        if(success)
        {
            file.colortable[idx] &= 0x00FFFFFF;
        }
    }

    // apply sorted files to levels and extract file header data
    double right    = -180.0;
    double top      =  -90.0;
    double left     =  180.0;
    double bottom   =   90.0;

    int totalTiles  = 0;

    double scale = 0.0;
    files.sort();
    std::list<file_t>::iterator f;
    for(f = files.begin(); f != files.end(); f++)
    {
        file_t& file = *f;
        prinfFileinfo(file);

        if(file.lon1 < left)    left   = file.lon1;
        if(file.lat1 > top)     top    = file.lat1;
        if(file.lat2 < bottom)  bottom = file.lat2;
        if(file.lon2 > right)   right  = file.lon2;

        if(scale != 0.0 && ((fabs(scale - file.xscale)) / scale) > 0.02)
        {
            printf("\n");
            fprintf(stderr, "\nWARNING! There is more than one detail level. I will just convert files on level 1 (best m/px ratio).");
            printf("\n");
            break;
        }
        scale = file.xscale;

        double xTiles = file.width  / double(1024);
        double yTiles = file.height / double(1024);
        totalTiles += int(ceil(xTiles)) * int(ceil(yTiles));

        levels[nLevels].files.push_back(&file);
    }
    nLevels = 1;

    QFile zipfile(argv[argc-1]);
    zipfile.remove();
    zipfile.open(QIODevice::WriteOnly);
    QLGT::QZipWriter zip(&zipfile);

    QByteArray doc;

    doc.append(kmzHead);
    if(zip.status() != QLGT::QZipWriter::NoError)
    {

        fprintf(stderr, "\nERROR! Failed to create '%s'. Is the traget path writeable?\n\n", argv[argc-1]);
        exit(-1);
    }

    printf("\nStart to extract tiles and to build KMZ.\n");


    for(int l = 0; l < nLevels; l++)
    {
        level_t& level = levels[l];
        std::list<file_t *>::iterator f;
        for(f = level.files.begin(); f != level.files.end(); f++)
        {
            file_t& file  = *(*f);

            uint32_t xoff = 0;
            uint32_t yoff = 0;

            uint32_t xsize = 1024;
            uint32_t ysize = 1024;

            while(yoff < file.height)
            {
                if(ysize > (file.height - yoff))
                {
                    ysize = file.height - yoff;
                }

                xsize = 1024;
                xoff  = 0;

                while(xoff < file.width)
                {

                    if(xsize > (file.width - xoff))
                    {
                        xsize = (file.width - xoff);
                    }

                    if(!selTiles.isEmpty() && !selTiles.contains(tileCnt))
                    {
                        tileCnt++;
                        xoff += xsize;
                        continue;
                    }

                    if(!readTile(xoff, yoff, xsize, ysize, file, tileBuf32Bit))
                    {
                        fprintf(stderr,"\nError reading tiles from map file\n");
                        exit(-1);
                    }
                    tileCnt++;

                    tile_t tile;

                    if(pj_is_latlong(file.pj))
                    {

                        double u1 = file.lon1 + xoff * file.xscale;
                        double v1 = file.lat1 + yoff * file.yscale;
                        double u2 = file.lon1 + (xoff + xsize) * file.xscale;
                        double v2 = file.lat1 + (yoff + ysize) * file.yscale;


                        tile.left   = u1;
                        tile.top    = v1;
                        tile.right  = u2;
                        tile.bottom = v2;

                    }
                    else
                    {
                        double u1 = file.xref1 + xoff * file.xscale;
                        double v1 = file.yref1 + yoff * file.yscale;
                        double u2 = file.xref1 + (xoff + xsize) * file.xscale;
                        double v2 = file.yref1 + (yoff + ysize) * file.yscale;

                        pj_transform(file.pj,wgs84,1,0,&u1,&v1,0);
                        pj_transform(file.pj,wgs84,1,0,&u2,&v2,0);

                        tile.left    = (u1 * RAD_TO_DEG);
                        tile.top     = (v1 * RAD_TO_DEG);
                        tile.right   = (u2 * RAD_TO_DEG);
                        tile.bottom  = (v2 * RAD_TO_DEG);
                    }

                    tile.width  = xsize;
                    tile.height = ysize;
                    tile.size   = writeTile(xsize, ysize, tileBuf32Bit, quality, subsampling);
                    sprintf(str, "files/map%i.jpg", tileCnt);

                    QByteArray buffer((const char*)&jpgbuf[0], tile.size);
                    zip.addFile(str, buffer);
                    if(zip.status() != QLGT::QZipWriter::NoError)
                    {
                        fprintf(stderr,"\nFailed to add file '%s' to KMZ. Is the traget path writeable?\n", str);
                        exit(-1);
                    }

                    doc.append(QString(kmzOverlay).arg(str).arg(str).arg(zorder).arg(tile.top).arg(tile.bottom).arg(tile.right).arg(tile.left));

                    printProgress(tileCnt, totalTiles);

                    xoff += xsize;
                }

                yoff += ysize;
            }
        }
    }


    doc.append(kmzFoot);
    zip.addFile(KMLFILE, doc);
    if(zip.status() != QLGT::QZipWriter::NoError)
    {
        fprintf(stderr,"\nFailed to add file '%s' to KMZ. Is the traget path writeable?\n", KMLFILE);
        exit(-1);
    }

    printf("\n");

    zip.close();
    zipfile.close();
    pj_free(wgs84);
    GDALDestroyDriverManager();
    printf("\n");

    return 0;
}
