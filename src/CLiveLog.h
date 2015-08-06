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
#ifndef CLIVELOG_H
#define CLIVELOG_H

#include <QtGlobal>
#include "CWpt.h"

class CLiveLog
{
    public:
        CLiveLog() : fix(eOff), lon(WPT_NOFLOAT), lat(WPT_NOFLOAT), ele(WPT_NOFLOAT)
            , timestamp(0xFFFFFFFF), error_horz(WPT_NOFLOAT), error_vert(WPT_NOFLOAT)
            , error_unit(""), sat_used(-1), heading(WPT_NOFLOAT), velocity(WPT_NOFLOAT)
            , count_bytes(0), count_nmea(0), count_fix(0)
            {};
        virtual ~CLiveLog();

        enum fix_e
        {
            //fix states determined from protocol data
            eNoFix,
            e2DFix,
            e3DFix,
            eEstimated,
            //off means: we do not even try to receive live log data
            eOff,
            //additional states for improved conection establisment diagnostics
            eConnectionFailed,
            eConnectionEstablished,
            eConnectionReceiving
        };

        fix_e fix;
        float lon;
        float lat;
        float ele;
        quint32 timestamp;
        float error_horz;
        float error_vert;
        QString error_unit;
        int sat_used;
        float heading;
        float velocity;
        //the following fields are for NMEA statistics
        //They may not be useful for other types of devices
        int count_bytes;         //number of bytes received
        int count_nmea;          //number of valid nmea sentences received
        int count_fix;           //number fixes/updates received
};

extern void operator <<(QDataStream& s, const CLiveLog& log);
extern void operator <<(QFile& f, const CLiveLog& log);
extern void operator >>(QDataStream& s, CLiveLog& log);
#endif                           //CLIVELOG_H
