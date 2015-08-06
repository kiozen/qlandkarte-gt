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

#include "CDiskCacheZip.h"
#ifndef STANDALONE
#include "CResources.h"
#endif                           //!STANDALONE

#include "../3rdparty/QZip/qzipreader.h"

#include <QtGui>

#ifdef STANDALONE
CDiskCacheZip::CDiskCacheZip(const QString &path, QObject *parent)
#else
CDiskCacheZip::CDiskCacheZip(QObject *parent)
#endif                           //STANDALONE
: IDiskCache(parent)
{
}


CDiskCacheZip::~CDiskCacheZip()
{
}


void CDiskCacheZip::store(const QString& key, QImage& img)
{
}


void CDiskCacheZip::restore(const QString& key, QImage& img)
{
    //    qDebug()  << "restore img " << key;

    int index1 = key.indexOf("file:");
    //QString url = key.right(key.size() - 6);
    QString url = QUrl(key.trimmed()).toLocalFile();

    int index2 = url.lastIndexOf('/');
    index1 = url.lastIndexOf('/',index2 - 1);

    //    qDebug()  << "restore img " << index2 << " " << index1;
    QString inZipFile = url.right(url.size() - index1 - 1);
    QString zipFile = url.left(index2) + ".zip";

    //    qDebug()  << "inZipFile" << inZipFile;
    //    qDebug()  << "zipFile" << zipFile;

    QLGT::QZipReader zipArchive(zipFile);
    if ( zipArchive.exists() )
    {

        QByteArray b = zipArchive.fileData(inZipFile);
        //        qDebug()  << "ZIP " << zipFile;

        if ( b.size() > 0 && img.loadFromData(b) )
        {
            //            qDebug()  << "Tile " << inZipFile << " in zip-cache " << zipFile;
            return;
        }
        else
        {
            //            qDebug()  << "No Tile " << inZipFile << " in zip-cache " << zipFile << " " << b.size();

        }
    }

    if ( QFileInfo(url).exists())
    {
        QFile f(url);
        if ( f.open(QIODevice::ReadOnly) && img.loadFromData(f.readAll()) )
        {
            //            qDebug()  << "Tile " << url << " in local file ";
            return;
        }
    }

    img = QImage();

}


bool CDiskCacheZip::contains(const QString& key)
{
    return true;
}
