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

#ifndef CMAPGEOTIFF_H
#define CMAPGEOTIFF_H

#include "IMap.h"

#include <QRgb>
#include <QVector>

class GDALDataset;
class QCheckBox;

class CMapGeoTiff : public IMap
{
    Q_OBJECT;
    public:
        CMapGeoTiff(const QString& filename, CCanvas * parent);
        virtual ~CMapGeoTiff();

        void draw(QPainter& p);
        void draw();
        void convertPt2M(double& u, double& v);
        void convertM2Pt(double& u, double& v);
        void convertPt2Pixel(double& u, double& v);
        void move(const QPoint& old, const QPoint& next);
        void zoom(bool zoomIn, const QPoint& p);
        void zoom(double lon1, double lat1, double lon2, double lat2);
        void select(const QRect& rect);
        void dimensions(double& lon1, double& lat1, double& lon2, double& lat2);
        bool is32BitRgb();

        void incXOffset(int i);
        void decXOffset(int i);
        void incYOffset(int i);
        void decYOffset(int i);

        GDALDataset * getDataset();

    protected:
        void getArea_n_Scaling(projXY& p1, projXY& p2, float& my_xscale, float& my_yscale);

    private:
        void zoom(qint32& level);

        /// instance of GDAL dataset
        GDALDataset * dataset;

        /// width in number of px
        quint32 xsize_px;
        /// height in number of px
        quint32 ysize_px;

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

        //        /// the longitude of the top left reference point [rad]
        //        double lon1;
        //        /// the latitude of the top left reference point [rad]
        //        double lat1;
        //        /// the longitude of the bottom right reference point [rad]
        //        double lon2;
        //        /// the latitude of the bottom right reference point [rad]
        //        double lat2;

        /// QT representation of the GeoTiff's color table
        QVector<QRgb> colortable;

        double x;
        double y;

        double zoomFactor;

        int rasterBandCount;

        QCheckBox * quadraticZoom;

};
#endif                           //CMAPGEOTIFF_H
