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
#ifndef CMAPSELECTIONRASTER_H
#define CMAPSELECTIONRASTER_H

#include "IMapSelection.h"

class CMapSelectionRaster : public IMapSelection
{
    public:
        CMapSelectionRaster(subtype_e subtype, QObject *parent);
        virtual ~CMapSelectionRaster();

        QDataStream& operator>>(QDataStream&);

        void draw(QPainter& p, const QRect& rect);

        QString getDescription() const;

        QString getInfo(){return getDescription();}

        void setIcon(const QString&){};

        QMap< QPair<int, int>, bool > selTiles;
};
#endif                           //CMAPSELECTIONRASTER_H
