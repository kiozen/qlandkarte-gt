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

#ifndef CMAPFILE_H
#define CMAPFILE_H

#include <QObject>
#include <QRgb>
#include <QVector>
#include <proj_api.h>
#include <ogr_spatialref.h>
#ifdef __MINGW32__
#undef LP
#endif

class CMapLevel;
class GDALDataset;

/// data object for easy access to GeoTiff information
/**
    This will create a dataset and read basic information
    from the GeoTiff header.
*/
class CMapFile : public QObject
{
    Q_OBJECT;
    public:
        CMapFile(const QString& filename, QObject * parent);
        virtual ~CMapFile();

        /// source file name
        QString filename;

        /// instance of GDAL dataset
        GDALDataset * dataset;

        /// width in number of px
        quint32 xsize_px;
        /// height in number of px
        quint32 ysize_px;

        /// configuration string for projection
        QString strProj;
        QString strOrigProj;
        /// projection context
        projPJ  pj;

        /// scale [px/m]
        double xscale;
        /// scale [px/m]
        double yscale;

        /// reference point [m] (left hand side of map)
        double xref1;
        /// reference point [m] (top of map)
        double yref1;
        /// reference point [m] (right hand side of map)
        double xref2;
        /// reference point [m] (bottom of map)
        double yref2;

        /// the longitude of the top left reference point [rad]
        double lon1;
        /// the latitude of the top left reference point [rad]
        double lat1;
        /// the longitude of the bottom right reference point [rad]
        double lon2;
        /// the latitude of the bottom right reference point [rad]
        double lat2;

        /// QT representation of the GeoTiff's color table
        QVector<QRgb> colortable;

        /// width of GeoTiff tiles / blocks [px]
        qint32 tileWidth;
        /// height of GeoTiff tiles / blocks [px]
        qint32 tileHeight;

        bool ok;

        int rasterBandCount;

        bool is32BitRgb();

        OGRSpatialReference oSRS;

};
#endif                           //CMAPFILE_H
