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
#ifndef CDISKCACHEZIP_H
#define CDISKCACHEZIP_H

#include "IDiskCache.h"
#include <QDir>
#include <QHash>
#include <QImage>

class QTimer;

class CDiskCacheZip : public IDiskCache
{
    Q_OBJECT;
    public:
#ifdef STANDALONE
        CDiskCacheZip(const QString &path, QObject *parent);
#else
        CDiskCacheZip(QObject *parent);
#endif                       //STANDALONE
        virtual ~CDiskCacheZip();

        virtual void store(const QString& key, QImage& img);
        virtual void restore(const QString& key, QImage& img);
        virtual bool contains(const QString& key);

};
#endif                           //CDISKCACHEZIP_H
