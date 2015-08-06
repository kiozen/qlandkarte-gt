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

#include "CLiveLog.h"

void operator <<(QFile& f, const CLiveLog& log)
{
    f.open(QIODevice::Append);
    QDataStream s(&f);
    s.setVersion(QDataStream::Qt_4_5);
    s << log;
    f.close();
}


void operator <<(QDataStream& s, const CLiveLog& log)
{
    s << log.timestamp;
    s << log.lon;
    s << log.lat;
    s << log.ele;
    s << (quint32)0;             // terminator, non-zero defines additional data
    //     s << (quint32)0; // sizeof additional data in bytes if terminator is non-zero
}


void operator >>(QDataStream& s, CLiveLog& log)
{
    quint32 dummy;
    s >> log.timestamp;
    s >> log.lon;
    s >> log.lat;
    s >> log.ele;
    s >> dummy;

}


CLiveLog::~CLiveLog()
{

}
