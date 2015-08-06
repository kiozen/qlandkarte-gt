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
#ifndef CINPUTFILE_H
#define CINPUTFILE_H

#include <QString>
#include <QVector>
#include <gdal_priv.h>
#include <proj_api.h>


class CInputFile
{
    public:
        CInputFile(const QString& filename, quint32 tileSize, int epsg);
        virtual ~CInputFile();

        void summarize();

        double getXScale(){return xscale;}
        double getYScale(){return yscale;}

        void getRefP0(double& lon, double& lat);
        void getRefP1(double& lon, double& lat);
        void getRefP2(double& lon, double& lat);
        void getRefP3(double& lon, double& lat);

        qint32 getWidth(){return width;}
        qint32 getHeight(){return height;}
        QString getProjection(){return compeProj;}
        QString getDatum(){return compeDatum;}

        quint32 calcLevels(double scaleLimit, double& globXScale, double& globYScale);

        void writeLevels(QDataStream& stream, int quality, int subsampling);
        void writeLevelOffsets(QDataStream& stream);

        static quint32 getTilesTotal(){return nTilesTotal;}

    private:
        void writeLevel(QDataStream& stream, int level, int quality, int subsampling);

        bool readTile(qint32 xoff, qint32 yoff, qint32 w1, qint32 h1, qint32 w2, qint32 h2, quint32 *output);
        quint32 writeTile(quint32 xsize, quint32 ysize, quint32 * raw_image, int quality, int subsampling);

        QString filename;
        quint32 tileSize;

        struct level_t
        {
            level_t(): offsetLevel(0), width(0), height(0), xTiles(0), yTiles(0), xscale(0), yscale(0), xCorrectionScale(0), yCorrectionScale(0){}
            quint64 offsetLevel;
            qint32 width;
            qint32 height;
            qint32 xTiles;
            qint32 yTiles;
            QVector<quint64> offsetJpegs;

            double xscale;
            double yscale;

            double xCorrectionScale;
            double yCorrectionScale;
        };

        projPJ  pj;
        GDALDataset * dataset;
        QString compeProj;
        QString compeDatum;

        qint32 width;
        qint32 height;

        double xscale;
        double yscale;
        double xref1;
        double yref1;
        double xref2;
        double yref2;

        quint32 nTiles;
        static quint32 nTilesTotal;
        static quint32 nTilesProcessed;
        QList<level_t> levels;
        QByteArray tileBuf08Bit;
        QByteArray tileBuf24Bit;
        //QByteArray tileBuf32Bit;
        quint32 tileBuf32Bit[256*256];
        quint32 colortable[256];
};

#endif //CINPUTFILE_H

