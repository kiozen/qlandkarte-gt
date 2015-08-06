/** ********************************************************************************************
    Copyright (c) 2008 Marc Feld

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
********************************************************************************************* */

#ifndef TCXREADER_H_
#define TCXREADER_H_
#include <QXmlStreamReader>
#include <QString>
#include "CTrack.h"
class QObject;
class CTrack;

class TcxReader : public QXmlStreamReader
{
    public:
        TcxReader(QObject *parent);
        virtual ~TcxReader();
        bool read(QString path);
        bool read(QIODevice *device);
    private:
        QObject *parent;
        CTrack::pt_t pold;
        bool firstPositionFound;
        void readUnknownElement();
        void readTcx();
        void readActivities();
        void readActivity();
        void readCourses();
        void readCourse();
        void readLap(CTrack *track);
        void readTrack(CTrack *track, int lap);
        void readTrackpoint(CTrack *track, int lap);
        void readHeartRateBpm(CTrack::pt_t *pt);
        void readPosition(CTrack::pt_t *pt);

};
#endif                           /* TCXREADER_H_ */
