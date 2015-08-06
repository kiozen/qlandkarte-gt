/**********************************************************************************************
    Copyright (C) 2009 Oliver Eichler oliver.eichler@gmx.de

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

#ifndef CDEVICENMEA_H
#define CDEVICENMEA_H

#include "IDevice.h"
#ifdef QK_QT5_SERIAL_PORT
#include <QSerialPort>
#else
#include <ManageSerialPort.h>
#endif

class CDeviceNMEA : public IDevice
{
    Q_OBJECT;
    public:
        CDeviceNMEA(const QString& serialport, const QString& baudrate, QObject * parent, bool watchdog);
        virtual ~CDeviceNMEA();

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
#ifdef QK_QT5_SERIAL_PORT
        void slotNewDataReceived();
#else
        void slotNewDataReceived(const QByteArray &dataReceived);
#endif
        void slotWatchdog();

    private:
        bool isChecksumValid();
        void decode();

        QString serialport;
        QString line;
#ifdef QK_QT5_SERIAL_PORT
        QSerialPort tty;
#else
        ManageSerialPort tty;
#endif

        bool haveSeenData;
        bool haveSeenGPRMC;
        bool haveSeenGPGGA;
        bool haveSeenGPGSA;
        bool haveSeenGPVTG;

        QTimer * watchdog;

};
#endif                           //CDEVICENMEA_H
