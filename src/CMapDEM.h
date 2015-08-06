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

#ifndef CMAPDEM_H
#define CMAPDEM_H

#include "IMap.h"

#include <QVector>
#include <QRgb>

class GDALDataset;
class QImage;
class CStatusDEM;

class CMapDEM : public IMap
{
    Q_OBJECT;
    public:
        CMapDEM(const QString& filename, CCanvas * parent);
        virtual ~CMapDEM();

        /// draw map
        void draw(QPainter& p);
        void draw();

        void convertPt2M(double& u, double& v);
        void convertM2Pt(double& u, double& v);
        void move(const QPoint& old, const QPoint& next);
        void zoom(bool zoomIn, const QPoint& p);
        void zoom(double lon1, double lat1, double lon2, double lat2);
        void zoom(qint32& level);
        void select(const QRect& rect);
        void dimensions(double& lon1, double& lat1, double& lon2, double& lat2);
        float getElevation(double lon, double lat);
        bool getRegion(QVector<float>& buffer, projXY p1, projXY p2, int w, int h);
        bool getOrigRegion(QVector<qint16>& data, projXY &topLeft, projXY &bottomRight, int& w, int& h);

        bool is32BitRgb(){return false;}
        bool loaded();

        void setGrade(int i);
        int getGrade(){return idxGrade;}

        static const QRgb slopeColorTable[6];
        static const double * grade[5];

    private:
        void shading(QImage& img, qint16 * data, float xscale, float yscale);
        void contour(QImage& img, qint16 * data, float xscale, float yscale);
        void slope(QImage& img, qint16 * data, int xoff, int yoff);

        CCanvas * canvas;

        /// instance of GDAL dataset
        GDALDataset * dataset;
        /// width of GeoTiff tiles / blocks
        qint32 tileWidth;
        /// height of GeoTiff tiles / blocks
        qint32 tileHeight;

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

        QVector<QRgb> graytable1;
        QVector<QRgb> graytable2;
        QVector<QRgb> slopetable;

        CStatusDEM * status;

        projXY old_p1;
        projXY old_p2;
        float old_my_xscale;
        float old_my_yscale;

        IMap::overlay_e old_overlay;

        int idxGrade;

        static const double grade1[];
        static const double grade2[];
        static const double grade3[];
        static const double grade4[];

};
#endif                           //CMAPDEM_H
