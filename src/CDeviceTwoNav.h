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
#ifndef CDEVICETWONAV_H
#define CDEVICETWONAV_H

#include "IDevice.h"
#include <QDialog>

class CWpt;
class CTrack;
class QTextStream;

class CDeviceTwoNav : public IDevice
{
    Q_OBJECT;
    public:
        CDeviceTwoNav(QObject * parent);
        virtual ~CDeviceTwoNav();

        void uploadWpts(const QList<CWpt*>& wpts);
        void downloadWpts(QList<CWpt*>& wpts);

        void uploadTracks(const QList<CTrack*>& trks);
        void downloadTracks(QList<CTrack*>& trks);

        void uploadRoutes(const QList<CRoute*>& rtes);
        void downloadRoutes(QList<CRoute*>& rtes);

        void uploadMap(const QList<IMapSelection*>& mss);

        void downloadScreenshot(QImage& image);

    private:
        QString makeUniqueName(const QString name, QDir& dir);
        bool aquire(QDir& dir);
        void createDayPath(const QString &what);

        void readWptFile(QDir &dir, const QString &filename, QList<CWpt *> &wpts);
        void writeWaypointData(QTextStream& out, CWpt * wpt, QDir &dir);

        void readTrkFile(QDir &dir, const QString &filename, QList<CTrack *> &trks);
        void writeTrkData(QTextStream& out, CTrack &trk, QDir& dir);

        QString iconTwoNav2QlGt(const QString& sym);
        QString iconQlGt2TwoNav(const QString& sym);

        QString pathRoot;
        QString pathData;
        QString pathDay;

        CWpt * wpt;

};
#endif                           //CDEVICETWONAV_H
