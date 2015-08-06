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

#include "CImage.h"

#include <QtGui>

CImage::CImage(QObject * parent)
: QObject(parent)
, bintable(256, qRgba(0,0,0,0))
, grayHistogram(256, 0.0)
, threshold(128)
{
    bintable[0] = qRgba(255,255,255,255);
    bintable[1] = qRgba(0,0,0,255);

    int i;
    for(i = 0; i < 256; ++i)
    {
        graytable << qRgb(i,i,i);
    }
}


CImage::CImage(const QImage& pix, QObject * parent)
: QObject(parent)
, bintable(256, qRgba(0,0,0,0))
, grayHistogram(256, 0.0)
, threshold(128)
{
    bintable[0] = qRgba(255,255,255,255);
    bintable[1] = qRgba(0,0,0,255);

    int i;
    for(i = 0; i < 256; ++i)
    {
        graytable << qRgb(i,i,i);
    }

    setPixmap(pix);
}


CImage::~CImage()
{

}


void CImage::setPixmap(const QImage& pix)
{

    QRect rect  = pix.rect();
    rect.setWidth((rect.width() >> 2) << 2);
    imgRgb      = pix.copy(rect);

    int i;
    imgGray = QImage(rect.size(), QImage::Format_Indexed8);
    imgGray.setColorTable(graytable);

    QRgb *   p1 = (QRgb*)imgRgb.bits();
    quint8 * p2 = (quint8*)imgGray.bits();

    grayHistogram = QVector<double>(256, 0.0);

    const int nPixel = imgGray.width() * imgGray.height();
    for(i = 0; i < nPixel; ++i, ++p1, ++p2)
    {
        *p2 = qGray(*p1);
        grayHistogram[*p2] += 1.0;
    }

    double Sum = 0;
    for(int j = 0 ; j < 256 ; j++ )
    {
        Sum += grayHistogram[j] ;
    }

    for(int j = 0 ; j < 256 ; j++ )
    {
        grayHistogram[j] /= Sum ;
    }

    threshold = calcThreshold(grayHistogram);
}


int CImage::calcThreshold(const QVector<double>& hist)
{
    // histogram histo;
    int nThreshold;
    double criterion;
    double expr_1;

    // double p[256];
    double omega_k;
    double sigma_b_k;
    double sigma_T;
    double fMu_T;
    double fMu_k;
    int k_low, k_high;

    fMu_T   = 0.0;
    for (int i = 0 ; i < 256 ; i++)
    {
        fMu_T += i * hist[i];
    }

    //Standard deviation
    sigma_T = 0.0;
    for (int i = 0; i < 256 ; i++)
    {
        sigma_T += (i - fMu_T ) * (i - fMu_T ) * hist[i];
    }

    //Means for classes c1 and c2
    //c'est un peu lger!!!!
    for (k_low = 0;   (hist[k_low]  == 0) && (k_low  < 255); k_low++);
    for (k_high =255; (hist[k_high] == 0) && (k_high > 0);   k_high--);

    criterion   = 0.0;
    nThreshold  = 127;

    omega_k     = 0.0;
    fMu_k       = 0.0;
    //minimize S_W/S_B to find the treshold
    for (int k = k_low ; k <= k_high ; k++)
    {
        omega_k += hist[k];

        if( omega_k == 0 || omega_k == 1 ) continue;

        fMu_k      += k * hist[k];
        expr_1      = ( fMu_T * omega_k - fMu_k );
        sigma_b_k   = expr_1 * expr_1 / ( omega_k * ( 1 - omega_k ));

        if (  criterion < sigma_b_k / sigma_T )
        {
            criterion   = sigma_b_k / sigma_T;
            nThreshold  = k;
        }
    }

    return nThreshold;
}


const QImage& CImage::binarize(int t)
{
    t = t < 0 ? threshold : t;

    imgBinary = QImage(imgGray.size(), QImage::Format_Indexed8);
    imgBinary.setColorTable(bintable);

    quint8 * p1 = imgGray.bits();
    quint8 * p2 = imgBinary.bits();

    int i, n = imgGray.width() * imgGray.height();
    for(i = 0; i < n; ++i, ++p1, ++p2)
    {
        *p2 = *p1 > t ? 0 : 1;
    }

    return imgBinary;
}


QImage CImage::mask()
{
    binarize(-1);

    quint8 *p1, *p2, *p3, *p;
    int i, j;

    const int w = imgBinary.width();
    const int h = imgBinary.height();

    QImage result(w,h,QImage::Format_Indexed8);
    result.setColorTable(bintable);

    p1  = imgBinary.bits();
    p2  = imgBinary.bits() + w;
    p3  = imgBinary.bits() + w + w;

    p   = result.bits();

    // first row
    *p++    = *p1 + *(p1 + 1)
        + *p2 + *(p2 + 1)
        ? *p1 : 2;

    ++p1; ++p2;

    for(i = 1; i < (w - 1); ++i, ++p1, ++p2)
    {
        *p++    = *(p1 - 1) + *p1 + *(p1 + 1)
            + *(p2 - 1) + *p2 + *(p2 + 1)
            ? *p1 : 2;
    }

    *p++    = *(p1 - 1) + *p1
        + *(p2 - 1) + *p2
        ? *p1 : 2;

    // 2nd to h - 1 row
    p1  = imgBinary.bits();
    p2  = imgBinary.bits() + w;
    p3  = imgBinary.bits() + w + w;

    for(j = 1; j < (h - 1); ++j)
    {

        *p++    = *p1 + *(p1 + 1)
            + *p2 + *(p2 + 1)
            + *p3 + *(p3 + 1)
            ? *p2 : 2;

        ++p1; ++p2; ++p3;

        for(i = 1; i < (w - 1); ++i, ++p1, ++p2, ++p3)
        {
            *p++    = *(p1 -1) + *p1 + *(p1 + 1)
                + *(p2 -1) + *p2 + *(p2 + 1)
                + *(p3 -1) + *p3 + *(p3 + 1)
                ? *p2 : 2;
        }

        *p++    = *(p1 - 1) + *p1
            + *(p2 - 1) + *p2
            + *(p3 - 1) + *p3
            ? *p2 : 2;

        ++p1; ++p2; ++p3;
    }

    // last row
    *p++    = *p1 + *(p1 + 1)
        + *p2 + *(p2 + 1)
        ? *p2 : 2;

    ++p1; ++p2;

    for(i = 1; i < (w - 1); ++i, ++p1, ++p2)
    {
        *p++    = *(p1 - 1) + *p1 + *(p1 + 1)
            + *(p2 - 1) + *p2 + *(p2 + 1)
            ? *p2 : 2;
    }

    *p++    = *(p1 - 1) + *p1
        + *(p2 - 1) + *p2
        ? *p2 : 2;

    // optimize size of mask
    int x1  = 0;
    int x2  = w;
    int y1  = 0;
    int y2  = h;
    p1      = result.bits();

    // top rows of void
    for(j = 0; j < h; ++j)
    {
        for(i = 0; i < w; ++i)
        {
            if( *(p1 + i + (j * w)) != 0x2)
            {
                y1 = j;
                j  = h;
                break;
            }
        }
    }

    // bottom rows of void
    for(j = h - 1; j >= 0; --j)
    {
        for(i = 0; i < w; ++i)
        {
            if( *(p1 + i + (j * w)) != 0x2)
            {
                y2 = j + 1;
                j  = -1;
                break;
            }
        }
    }

    // left columns of void
    for(i = 0; i < w; ++i)
    {
        for(j = 0; j < h; ++j)
        {
            if( *(p1 + i + (j * w)) != 0x2)
            {
                x1 = i;
                i  = w;
                break;
            }
        }
    }

    // right columns of void
    for(i = w - 1; i >= 0; --i)
    {
        for(j = 0; j < h; ++j)
        {
            if( *(p1 + i + (j * w)) != 0x2)
            {
                x2 = i + 1;
                i  = -1;
                break;
            }
        }
    }

    // 32bit align the width (important!)
    int w1 = x2 - x1;
    if(w1 & 0x03)
    {
        int cnt = 4 - (w1 & 0x03);
        for(; cnt && x2 <  w; --cnt, ++x2);
        for(; cnt && x1 >= 0; --cnt, --x1);
    }
    return result.copy(x1, y1, x2 - x1, y2 - y1);

}


void CImage::findSymbol(QList<QPoint>& finds, CImage& mask)
{
    //     qDebug() << "CImage::findSymbol(QList<QPoint>& finds, CImage& mask)";
    finds.clear();

    QPoint last;
    int i, j, n, m, blacks;
    QImage imgMask  = mask.mask();
    const int w1    = imgMask.width();
    const int h1    = imgMask.height();
    const int w2    = imgBinary.width();
    const int h2    = imgBinary.height();

    //     quint8 * pd     = imgGray.bits();

    quint8 * p      = imgMask.bits();
    int denom       = 0;
    for(i = 0; i  < (w1*h1); ++i, ++p)
    {
        if(*p & 0x02) continue;
        ++denom;
    }

    quint8 * p1     = imgMask.bits();
    quint8 * p2     = imgBinary.bits();

    // for each line in image
    for(n = 0; n < (h2 - h1); ++n)
    {
        p2 = imgBinary.bits() + n * w2;

        // for each pixel in line
        for(m = 0; m < (w2 - w1); ++m, ++p2)
        {
            int nom = 0;

            blacks  = 0;
            p1      = imgMask.bits();
            // for each line in mask
            for(i = 0; i < h1; ++i)
            {
                quint8 * p3 = p2 + i * w2;
                // for each 32 bit value in line
                for(j = 0; j < w1; ++j, ++p1, ++p3)
                {

                    blacks += *p3;

                    if(*p1 & 0x02) continue;
                    if(*p1 == *p3) ++nom;

                }
            }
            quint8 correlation = ((double)nom / denom) * 255;
            //             *(pd + n * w2 + m) = correlation;

            if(correlation > 190)
            {
                QPoint pt(m + (w1 >> 1), n + (h1 >> 1));
                if(abs(pt.x() - last.x()) > w1 || abs(pt.y() - last.y()) > h1)
                {
                    finds << pt;
                    last = pt;
                }
            }

            if(blacks == 0)
            {
                m  += w1 - 2;
                p2 += w1 - 2;
            }
        }
    }

    //     imgGray.save("dbg.png");
}
