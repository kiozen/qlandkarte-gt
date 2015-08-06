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

#ifndef CIMAGE_H
#define CIMAGE_H

#include <QObject>
#include <QImage>
#include <QPixmap>
#include <QList>

class CImage : public QObject
{
    Q_OBJECT;
    public:
        CImage(QObject * parent = 0);
        CImage(const QImage& pix, QObject * parent = 0);
        virtual ~CImage();

        void setPixmap(const QImage& pix);

        /// get treshold found by the Otsu algorithm
        int getThreshold(){return threshold;}
        /// binarize the image by the given threshold
        const QImage& binarize(int threshold);
        /// create a mask for correlation
        /**
            The resuling image will be 8 bit pallet with only 3 colors
            0 = white, 1 = black, 2 = transparent
        */
        QImage mask();
        const QImage& rgb(){return imgRgb;}

        /// find symbols by crrelation with the mask
        void findSymbol(QList<QPoint>& finds, CImage& mask);

    private:
        int calcThreshold(const QVector<double>& hist);

        QVector<QRgb> graytable;
        QVector<QRgb> bintable;
        QImage  imgRgb;
        QImage  imgGray;
        QImage  imgBinary;

        QVector<double> grayHistogram;

        int threshold;
};
#endif                           //CIMAGE_H
