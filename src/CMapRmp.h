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
#ifndef CMAPRMP_H
#define CMAPRMP_H

#include "IMap.h"

#include <QtCore>

class CMapRmp : public IMap
{
    Q_OBJECT;
    public:
        CMapRmp(const QString& key, const QString& fn, CCanvas * parent);
        virtual ~CMapRmp();

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

        const QString& getMapInfo(){return info;}

        void config();

    private:
        QString name;
        QString info;

        struct dir_entry_t
        {
            dir_entry_t() : offset(0), length(0), name(10,0), extension(8,0){}

            quint32 offset;
            quint32 length;
            QString name;
            QString extension;

        };

        struct tile_t
        {
            int t;
            int b;
            QRectF bbox;
            quint32 offset;
        };

        struct node_t
        {
            node_t() : nTiles(0), tiles(100) {}

            QRectF bbox;
            quint16 nTiles;
            QVector<tile_t> tiles;
        };

        struct tlm_t : public dir_entry_t
        {

            tlm_t& operator=(dir_entry_t& entry)
            {
                offset      = entry.offset;
                length      = entry.length;
                name        = entry.name;
                extension   = entry.extension;

                return *this;
            }

            quint32 tileCount;
            quint16 tileXSize;
            quint16 tileYSize;

            double tileHeight;
            double tileWidth;
            QRectF bbox;

            QList<node_t> nodes;
        };

        struct a00_t : public dir_entry_t
        {
            a00_t& operator=(dir_entry_t& entry)
            {
                offset      = entry.offset;
                length      = entry.length;
                name        = entry.name;
                extension   = entry.extension;

                return *this;
            }
        };

        struct level_t
        {
            QString name;
            tlm_t tlm;
            a00_t a00;
        };

        struct file_t
        {
            file_t() : lon1(180.0), lat1(-90.0), lon2(-180.0), lat2(90.0){}

            level_t& levelByName(const QString& name)
            {
                for(int i = 0; i < levels.size(); i++)
                {
                    if(levels[i].name == name)
                    {
                        return levels[i];
                    }
                }

                levels.append(level_t());
                levels.last().name = name;
                return levels.last();
            }

            double lon1;
            double lat1;
            double lon2;
            double lat2;
            QRectF bbox;

            QString filename;
            QString product;
            QString provider;
            QList<dir_entry_t> directory;
            QList<level_t> levels;
        };

        QList<file_t> files;

        friend bool qSortLevels(level_t& l1, level_t& l2);
        void draw();
        void drawTileBorder(QPainter& p, const file_t &mapFile);
        void readFile(const QString& filename, const QString& provider, const QString& product);
        void readDirectory(QDataStream& stream, file_t& file);
        void readCVGMap(QDataStream& stream, file_t &file, QString &tmpInfo);
        void readCopyright(QDataStream& stream, file_t &file, QString &tmpInfo);
        void readLevel(QDataStream& stream, level_t& level, double &lon1, double &lat1, double &lon2, double &lat2);
        void readTLMNode(QDataStream& stream, tlm_t& tlm);
        int zlevel2idx(quint32 zl, const file_t &file);

        /// scale entry
        struct scale_t
        {
            /// scale factor
            double qlgtScale;
            double tileYScale;
        };

        static scale_t scales[];

        /// reference point [m] (left hand side of map)
        double xref1;
        /// reference point [m] (top of map)
        double yref1;
        /// reference point [m] (right hand side of map)
        double xref2;
        /// reference point [m] (bottom of map)
        double yref2;

        double x;
        double y;

        double xscale;
        double yscale;

        double zoomFactor;

        int tileCnt;
        int blockCnt;

};
#endif                           //CMAPRMP_H
