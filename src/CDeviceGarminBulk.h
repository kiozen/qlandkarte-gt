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

#ifndef CDEVICEGARMINBULK_H
#define CDEVICEGARMINBULK_H

#include "IDevice.h"

class QDeviceWatcher;

class CDeviceGarminBulk : public IDevice
{
    Q_OBJECT;
    public:
        CDeviceGarminBulk(QObject * parent);
        virtual ~CDeviceGarminBulk();

        void uploadWpts(const QList<CWpt*>& wpts);
        void downloadWpts(QList<CWpt*>& wpts);

        void uploadTracks(const QList<CTrack*>& trks);
        void downloadTracks(QList<CTrack*>& trks);

        void uploadRoutes(const QList<CRoute*>& rtes);
        void downloadRoutes(QList<CRoute*>& rtes);

        void uploadMap(const QList<IMapSelection*>& mss);

        void setLiveLog(bool on);
        bool liveLog(){return false;}

        void downloadScreenshot(QImage& image);

    private slots:
        void slotDevice(const QString& dev);

    private:
        void readDeviceXml(const QString& filename);
        bool aquire(QDir& dir);
        void createDayPath(const QDir &root, const QString &what);

        QString pathRoot;
        QString pathPictures;
        QString pathGpx;
        QString pathSpoilers;

        QString pathDay;

        QDeviceWatcher * deviceWatcher;

};
#endif                           //CDEVICEGARMINBULK_H
