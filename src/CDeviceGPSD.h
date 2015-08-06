/** ********************************************************************************************
    Copyright (c) ???????

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
#ifndef CDEVICEGPSD_H
#define CDEVICEGPSD_H

#include "IDevice.h"

#include <gps.h>
#include <QtCore/QThread>

class QMutex;
class QTimer;
class CGPSDThread;

class CDeviceGPSD : public IDevice
{
    Q_OBJECT;
    public:
        CDeviceGPSD(QObject * parent);
        virtual ~CDeviceGPSD();

        void uploadWpts(const QList<CWpt*>& wpts);
        void downloadWpts(QList<CWpt*>& wpts);

        void uploadTracks(const QList<CTrack*>& trks);
        void downloadTracks(QList<CTrack*>& trks);

        void uploadRoutes(const QList<CRoute*>& rtes);
        void downloadRoutes(QList<CRoute*>& rtes);

        void uploadMap(const QList<IMapSelection*>& mss);

        void downloadScreenshot(QImage& image);

        void setLiveLog(bool on);
        bool liveLog();

    private slots:
        void slotTimeout();

    private:
        QTimer * timer;
        CGPSDThread* thread;
        int thread_fd;
};

class CGPSDThread : public QThread
{
    public:
        CGPSDThread( int _pipe_fd );
        virtual ~CGPSDThread();

        QMutex* log_mutex;
        CLiveLog log();

        bool log( CLiveLog& out );

    protected:
        virtual void run();

        gps_data_t* gpsdata;

        bool decodeData();

        CLiveLog current_log;
        bool changed;

        int pipe_fd;
};                               // class CGPSDThread
#endif                           //CDEVICEGPSD_H
