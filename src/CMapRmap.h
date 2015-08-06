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
#ifndef CMAPRMAP_H
#define CMAPRMAP_H

#include "IMap.h"

class QCheckBox;

class CMapRmap : public IMap
{
    Q_OBJECT;
    public:
        CMapRmap(const QString& key, const QString& filename, CCanvas * parent);
        virtual ~CMapRmap();

        void convertPt2M(double& u, double& v);
        void convertM2Pt(double& u, double& v);

        void move(const QPoint& old, const QPoint& next);
        void zoom(bool zoomIn, const QPoint& p0);
        void zoom(double lon1, double lat1, double lon2, double lat2);
        void zoom(qint32& level);
        void dimensions(double& lon1, double& lat1, double& lon2, double& lat2);
        void getArea_n_Scaling(projXY& p1, projXY& p2, float& my_xscale, float& my_yscale);

        QString getName(){return name;}

        void draw(QPainter& p);

    private:
        struct level_t
        {
            level_t(): offsetLevel(0), width(0), height(0), xTiles(0), yTiles(0), xscale(0), yscale(0){}
            quint64 offsetLevel;
            qint32 width;
            qint32 height;
            qint32 xTiles;
            qint32 yTiles;
            QVector<quint64> offsetJpegs;

            quint64 getOffsetJpeg(quint32 x, quint32 y)
            {
                qint32 idx = y * xTiles + x;
                return idx < offsetJpegs.size() ? offsetJpegs[idx] : 0;
            }

            double xscale;
            double yscale;
        };

        bool setProjection(const QString& projection, const QString& datum);
        void draw();
        level_t& findBestLevel(double sx, double sy);

        QString name;

        quint32 blockSizeX;
        quint32 blockSizeY;

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

        /// left of viewport
        double x;
        /// top of viewport
        double y;

        double zoomFactor;

        QList<level_t> levels;

        bool needsRedrawOvl;
        QCheckBox * quadraticZoom;

};
#endif                           //CMAPRMAP_H
