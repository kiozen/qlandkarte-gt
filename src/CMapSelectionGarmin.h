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
#ifndef CMAPSELECTIONGARMIN_H
#define CMAPSELECTIONGARMIN_H

#include "IMapSelection.h"

#include <QVector>
#include <QMap>
#include <QRectF>

class CMapSelectionGarmin : public IMapSelection
{
    public:
        CMapSelectionGarmin(QObject * parent);
        virtual ~CMapSelectionGarmin();

        QDataStream& operator>>(QDataStream&);

        void draw(QPainter& p, const QRect& rect);

        bool isEmpty(){return tilecnt == 0;}

        quint32 getMemSize();
        void calcArea();

        QString getInfo(){return getDescription();}
        QString getDescription() const {return IItem::getDescription();}
        void setIcon(const QString&){}

        struct tile_t
        {
            quint32 id;
            QString name;
            QString filename;
            QVector<double> u;
            QVector<double> v;
            quint32 memSize;
            QRectF area;

            quint16 fid;
            quint16 pid;
        };

        struct map_t
        {
            QString unlockKey;
            QString name;
            QString typfile;
            QString mdrfile;
            quint16 fid;
            quint16 pid;
            QMap<QString, tile_t> tiles;

        };

        QMap<QString, map_t> maps;

        quint32 tilecnt;
};
#endif                           //CMAPSELECTIONGARMIN_H
