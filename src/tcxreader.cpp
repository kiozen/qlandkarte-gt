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

#include "tcxreader.h"

#include <QObject>
#include <QDebug>
#include "CTrack.h"
#include "CTrackDB.h"

TcxReader::TcxReader(QObject *parent) :
parent(parent)
{

}


TcxReader::~TcxReader()
{

}


bool TcxReader::read(QString path)
{
    QFile file(path);
    qDebug() << path;
    if (file.open(QIODevice::ReadOnly))
    {
        return read(&file);
    }
    else
    {
        raiseError(QObject::tr("Error open file '%1': %2").arg(path).arg(
            file.errorString()));
    }

    return !error();
}


bool TcxReader::read(QIODevice *device)
{

    setDevice(device);

    while (!atEnd())
    {
        readNext();
        if (isStartElement())
        {

                                 // && attributes().value("xmlns") == "http://www.garmin.com/xmlschemas/TrainingCenterDatabase/v2")
            if (name() == "TrainingCenterDatabase")
                readTcx();
            else
                raiseError(
                    QObject::tr(
                    "The file is not an http://www.garmin.com/xmlschemas/TrainingCenterDatabase/v2 file."));
        }
    }

    return !error();
}


void TcxReader::readUnknownElement()
{
    Q_ASSERT( isStartElement());
    qDebug() << "unknown element: " << name().string();
    while (!atEnd())
    {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement())
            readUnknownElement();
    }
}


void TcxReader::readTcx()
{
    Q_ASSERT(isStartElement() && name() == "TrainingCenterDatabase");

    while (!atEnd())
    {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement())
        {
            if (name() == "Activities")
            {
                readActivities();
            }
            else if (name() == "Courses")
            {
                readCourses();
            }
            /*			else if (name() == "Author")
             {
             readAuthor();
             }
                         */
            else
                readUnknownElement();
        }
    }
}


void TcxReader::readCourses()
{
    Q_ASSERT(isStartElement() && name() == "Courses");

    while (!atEnd())
    {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement())
        {
            if (name() == "Course")
            {
                readCourse();
            }
            /*      else if (name() == "Author")
             {
             readAuthor();
             }
                         */
            else
                readUnknownElement();
        }
    }
}


void TcxReader::readCourse()
{
    Q_ASSERT(isStartElement() && name() == "Course");

    CTrack *track = new CTrack(parent);
    //track->setTraineeData();
    while (!atEnd())
    {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement())
        {
            if (name() == "Name")
            {
                track->setName(readElementText());
            }
            else if (name() == "Track")
            {
                readTrack(track,1);
            }
            else
                readUnknownElement();
        }
    }
    if (track->getTrackPoints().count() > 0)
    {
        track->rebuild(true);
        track->sortByTimestamp();
        CTrackDB::self().addTrack(track, false);
    }
    else
    {
        delete track;
    }
}


void TcxReader::readActivities()
{
    Q_ASSERT(isStartElement() && name() == "Activities");

    while (!atEnd())
    {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement())
        {
            if (name() == "Activity")
            {
                readActivity();
            }
            /*			else if (name() == "Author")
             {
             readAuthor();
             }
                         */
            else
                readUnknownElement();
        }
    }
}


void TcxReader::readActivity()
{
    Q_ASSERT(isStartElement() && name() == "Activity");

    CTrack *track = new CTrack(parent);
    track->setTraineeData();
    while (!atEnd())
    {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement())
        {
            if (name() == "Id")
            {
                track->setName(readElementText());
            }
            else if (name() == "Lap")
            {
                readLap(track);
            }
            else
                readUnknownElement();
        }
    }
    if (track->getTrackPoints().count() > 0)
    {
        track->rebuild(true);
        track->sortByTimestamp();
        CTrackDB::self().addTrack(track, false);
    }
    else
    {
        delete track;
    }
}


void TcxReader::readLap(CTrack *track)
{
    Q_ASSERT(isStartElement() && name() == "Lap");

    int lap = 1;
    while (!atEnd())
    {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement())
        {
            if (name() == "Track")
            {
                readTrack(track, lap);
            }
            else
                readUnknownElement();
        }
    }

}


void TcxReader::readTrack(CTrack *track, int lap)
{
    Q_ASSERT(isStartElement() && name() == "Track");

    firstPositionFound = false;

    while (!atEnd())
    {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement())
        {
            if (name() == "Trackpoint")
            {
                readTrackpoint(track, lap);
            }
            else
                readUnknownElement();
        }
    }

}


void TcxReader::readTrackpoint(CTrack *track, int lap)
{
    Q_ASSERT(isStartElement() && name() == "Trackpoint");

    CTrack::pt_t *pt = new CTrack::pt_t();
    pt->lat = pold.lat;
    pt->lon = pold.lon;
    pt->ele = pold.ele;
    pt->distance = pold.distance;
    pt->heartReateBpm = pold.heartReateBpm;

    while (!atEnd())
    {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement())
        {
            if (name() == "Time")
            {
                QDateTime time(QDateTime::fromString(readElementText(),
                    Qt::ISODate));
                time.setTimeSpec(Qt::UTC);
                pt->timestamp = time.toTime_t();
            }
            else if (name() == "AltitudeMeters")
            {
                pt->ele = readElementText().toDouble();
                pold.ele = pt->ele;
            }
            else if (name() == "DistanceMeters")
            {
                pt->distance = readElementText().toDouble();
                pold.distance = pt->distance;
            }
            else if (name() == "Position")
            {
                readPosition(pt);
                pold.lat = pt->lat;
                pold.lon = pt->lon;
                firstPositionFound = true;
            }
            else if (name() == "HeartRateBpm")
            {
                readHeartRateBpm(pt);
                pold.heartReateBpm  = pt->heartReateBpm;
            }
            else if (name() == "Cadence")
            {
                //readCadenceRpm(pt);
                pt->cadenceRpm = readElementText().toInt();
                pold.cadenceRpm  = pt->cadenceRpm;
            }
            else
                readUnknownElement();
        }
    }
    pt->_lat=pt->lat;
    pt->_lon=pt->lon;
    pt->_ele=pt->ele;
    if (firstPositionFound)
    {
        *track << *pt;
    }
}


void TcxReader::readHeartRateBpm(CTrack::pt_t *pt)
{
    Q_ASSERT(isStartElement() && name() == "HeartRateBpm");

    while (!atEnd())
    {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement())
        {
            if (name() == "Value")
            {
                pt->heartReateBpm = readElementText().toInt();
            }
            else
                readUnknownElement();
        }
    }
}


void TcxReader::readPosition(CTrack::pt_t *pt)
{
    Q_ASSERT(isStartElement() && name() == "Position");

    while (!atEnd())
    {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement())
        {
            if (name() == "LatitudeDegrees")
                pt->lat = readElementText().toDouble();
            else if (name() == "LongitudeDegrees")
                pt->lon = readElementText().toDouble();
            else
                readUnknownElement();
        }
    }
    return;
}
