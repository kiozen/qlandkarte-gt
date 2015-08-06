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
#ifndef IMAPSELECTION_H
#define IMAPSELECTION_H

#include "IItem.h"

#include <QString>
#include <QObject>
#include <QMap>
#include <QPair>
class QPainter;
class QRect;

#define TILESIZE 1024

class IMapSelection : public IItem
{
    Q_OBJECT;
    public:
        enum head_type_e {eHeadEnd,eHeadBase,eHeadRaster,eHeadGarmin};
        struct sel_head_entry_t
        {
            sel_head_entry_t() : type(eHeadEnd), offset(0) {}
            qint32      type;
            quint32     offset;
            QByteArray  data;
        };

        enum type_e     {eNone, eRaster, eVector};
        enum subtype_e  {eNo, eGarmin, eGDAL, eWMS, eTMS};

        IMapSelection(type_e type, subtype_e subtype, QObject * parent) : IItem(parent), type(type), subtype(subtype), lon1(0), lat1(0), lon2(0), lat2(0){}
        virtual ~IMapSelection(){}

        void operator=(IMapSelection& ms)
        {
            setKey(ms.getKey());
            mapkey      = ms.mapkey;
            name        = ms.name;
            type        = ms.type;
            subtype     = ms.subtype;

            lon1        = ms.lon1;
            lat1        = ms.lat1;
            lon2        = ms.lon2;
            lat2        = ms.lat2;
        }

        virtual QDataStream& operator>>(QDataStream&) = 0;

        virtual void draw(QPainter& p, const QRect& rect){}

        virtual bool isEmpty(){return false;}

        void setTimestamp(quint32 t){timestamp = t;}

        virtual QString getDescription() const = 0;

        static QString focusedMap;

        type_e type;
        subtype_e subtype;
        QString mapkey;

        double lon1;             ///< top left longitude [rad]
        double lat1;             ///< top left latitude [rad]
        double lon2;             ///< bottom right longitude [rad]
        double lat2;             ///< bottom right latitude [rad]

        QRect rect();

};
#endif                           //IMAPSELECTION_H
