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
#include "CDeviceQLandkarteM.h"
#include "CWpt.h"
#include "CWptDB.h"
#include "CTrack.h"
#include "CTrackDB.h"

#include <QtGui>
#include <QNetworkInterface>
#include <QMessageBox>
#include <QProgressDialog>
#include <QApplication>

#define REMOTE_PORT 45454

CDeviceQLandkarteM::CDeviceQLandkarteM(const QString& ipaddr, quint16 port, QObject * parent)
: IDevice("QLandkarteM",parent)
, ipaddr(ipaddr)
, port(port)
, timeout(120000)
{
    udpSocket.bind(45453, QUdpSocket::ShareAddress);
    connect(&udpSocket, SIGNAL(readyRead()),this, SLOT(detectedDevice()));

}


CDeviceQLandkarteM::~CDeviceQLandkarteM()
{

}


bool CDeviceQLandkarteM::acquire(const QString& operation, int max)
{
    QUdpSocket udpSocketSend;
    createProgress(operation, tr("Connect to device."), max);
    qApp->processEvents();

    QByteArray datagram;
    datagram = "START";
    udpSocketSend.writeDatagram(datagram.data(), datagram.size(), QHostAddress(ipaddr), port);

    if ((tcpSocket.state() != QAbstractSocket::ConnectedState))
        if(!waitTcpServerStatus()) return false;

    qApp->processEvents();
    return true;
}


void CDeviceQLandkarteM::send(const packet_e type, const QByteArray& data)
{
    QByteArray packet;
    QDataStream out(&packet,QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_5);
    out << (qint32)type;
    out << (qint32)0;
    out << data;
    out.device()->seek(sizeof(type));
    out << (qint32)(packet.size() - 2 * sizeof(qint32));

    tcpSocket.write(packet);
}


bool CDeviceQLandkarteM::recv(packet_e& type, QByteArray& data)
{
    qint32 size;
    QDataStream in(&tcpSocket);
    in.setVersion(QDataStream::Qt_4_5);

    while(tcpSocket.bytesAvailable() < (int)(2 * sizeof(qint32)))
    {
        if (!tcpSocket.waitForReadyRead(timeout))
        {
            return false;
        }
    }

    in >> (qint32&)type;
    in >> size;

    while(tcpSocket.bytesAvailable() < size)
    {
        if (!tcpSocket.waitForReadyRead(timeout))
        {
            return false;
        }
    }

    in >> data;
    return true;
}


bool CDeviceQLandkarteM::exchange(packet_e& type,QByteArray& data)
{
    send(type,data);
    qApp->processEvents();
    data.clear();
    return recv(type,data);
}


void CDeviceQLandkarteM::release()
{
    QByteArray datagram;
    datagram = "STOP";
    QUdpSocket udpSocketSend;
    udpSocketSend.writeDatagram(datagram.data(), datagram.size(), QHostAddress(ipaddr), port);
    if ((tcpSocket.state() == QAbstractSocket::ConnectedState))
        waitTcpServerStatus();
    if(progress) progress->close();
    tcpSocket.disconnectFromHost();
}


void CDeviceQLandkarteM::uploadWpts(const QList<CWpt*>& wpts)
{
    if(!startDeviceDetection()) return;
    if(!acquire(tr("Upload waypoints ..."), wpts.count())) return;

    packet_e type;
    int cnt = 0;
    QList<CWpt*>::const_iterator wpt = wpts.begin();
    while(wpt != wpts.end() && !progress->wasCanceled())
    {
        QByteArray data;
        QDataStream s(&data,QIODevice::WriteOnly);
        s.setVersion(QDataStream::Qt_4_5);

        progress->setLabelText(tr("%1\n%2 of %3").arg((*wpt)->getName()).arg(++cnt).arg(wpts.count()));
        progress->setValue(cnt);
        qApp->processEvents();

        s << *(*wpt);

        if(!exchange(type = eC2HWpt,data))
        {
            QMessageBox::critical(0,tr("Error..."), tr("QLandkarteM: Failed to transfer waypoints."),QMessageBox::Abort,QMessageBox::Abort);
            return release();
        }

        if(type == eError)
        {
            QMessageBox::critical(0,tr("Error..."), QString(data),QMessageBox::Abort,QMessageBox::Abort);
            return release();
        }

        ++wpt;
    }
    release();
    return;
}


void CDeviceQLandkarteM::downloadWpts(QList<CWpt*>& wpts)
{
    if(!startDeviceDetection()) return;
    if(!acquire(tr("Download waypoints ..."), wpts.count())) return;

    progress->setLabelText(tr("Query list of waypoints from the device"));
    qApp->processEvents();

    packet_e type;
    QByteArray  data1;

    if(!exchange(type = eH2CWptQuery,data1))
    {
        QMessageBox::critical(0,tr("Error..."), tr("QLandkarteM: Failed to query waypoints from device."),QMessageBox::Abort,QMessageBox::Abort);
        return release();
    }

    if(type == eError)
    {
        QMessageBox::critical(0,tr("Error..."), QString(data1),QMessageBox::Abort,QMessageBox::Abort);
        return release();
    }

    quint32     nWpt = 0;
    quint32     n;
    QString     key, name;

    QDataStream wptlist(&data1, QIODevice::ReadOnly);
    wptlist.setVersion(QDataStream::Qt_4_5);

    wptlist >> nWpt;

    progress->setMaximum(nWpt);
    for(n = 0; n < nWpt; ++n)
    {
        QByteArray data;
        QDataStream stream(&data,QIODevice::ReadWrite);
        stream.setVersion(QDataStream::Qt_4_5);
        wptlist >> key >> name;

        progress->setLabelText(tr("Download waypoint: %1").arg(name));
        qApp->processEvents();

        stream << key;

        if(!exchange(type = eH2CWpt,data))
        {
            QMessageBox::critical(0,tr("Error..."), tr("QLandkarteM: Failed to transfer waypoints."),QMessageBox::Abort,QMessageBox::Abort);
            return release();
        }

        if(type == eError)
        {
            QMessageBox::critical(0,tr("Error..."), QString(data),QMessageBox::Abort,QMessageBox::Abort);
            return release();
        }

        stream.device()->seek(0);

        CWpt * wpt = new CWpt(&CWptDB::self());
        stream >> *wpt;

        wpts.push_back(wpt);

        progress->setValue(n + 1);
    }

    release();
    return;
}


void CDeviceQLandkarteM::downloadScreenshot(QImage& image)
{

    if(!startDeviceDetection()) return;

    if(!acquire(tr("Download screenshot ..."), 1)) return;

    progress->setLabelText(tr("Download screenshot ..."));
    qApp->processEvents();

    packet_e type;
    QByteArray  data;

    if(!exchange(type = eH2CScreen,data))
    {
        QMessageBox::critical(0,tr("Error..."), tr("QLandkarteM: Failed to download screenshot from device."),QMessageBox::Abort,QMessageBox::Abort);
        return release();
    }

    if(type == eError)
    {
        QMessageBox::critical(0,tr("Error..."), QString(data),QMessageBox::Abort,QMessageBox::Abort);
        return release();
    }

    QDataStream stream(&data,QIODevice::ReadWrite);
    stream.setVersion(QDataStream::Qt_4_5);

    stream >> image;

    release();
    return;
}


void CDeviceQLandkarteM::uploadTracks(const QList<CTrack*>& trks)
{
    if(!startDeviceDetection()) return;
    if(!acquire(tr("Uplaod tracks ..."), trks.count())) return;

    packet_e type;
    int cnt = 0;
    QList<CTrack*>::const_iterator trk = trks.begin();
    while(trk != trks.end() && !progress->wasCanceled())
    {
        QByteArray data;
        QDataStream s(&data,QIODevice::WriteOnly);
        s.setVersion(QDataStream::Qt_4_5);

        progress->setLabelText(tr("%1\n%2 of %3").arg((*trk)->getName()).arg(++cnt).arg(trks.count()));
        progress->setValue(cnt - 1);
        qApp->processEvents();

        s << *(*trk);

        if(!exchange(type = eC2HTrk,data))
        {
            QMessageBox::critical(0,tr("Error..."), tr("QLandkarteM: Failed to transfer tracks."),QMessageBox::Abort,QMessageBox::Abort);
            return release();
        }

        if(type == eError)
        {
            QMessageBox::critical(0,tr("Error..."), QString(data),QMessageBox::Abort,QMessageBox::Abort);
            return release();
        }

        ++trk;
    }

    release();
    return;
}


void CDeviceQLandkarteM::downloadTracks(QList<CTrack*>& trks)
{
    if(!startDeviceDetection()) return;
    if(!acquire(tr("Download tracks ..."), trks.count())) return;

    progress->setLabelText(tr("Query list of tracks from the device"));
    qApp->processEvents();

    packet_e type;
    QByteArray  data1;

    if(!exchange(type = eH2CTrkQuery,data1))
    {
        QMessageBox::critical(0,tr("Error..."), tr("QLandkarteM: Failed to query tracks from device."),QMessageBox::Abort,QMessageBox::Abort);
        return release();
    }

    if(type == eError)
    {
        QMessageBox::critical(0,tr("Error..."), QString(data1),QMessageBox::Abort,QMessageBox::Abort);
        return release();
    }

    quint32     nWpt = 0;
    quint32     n;
    QString     key, name;

    QDataStream wptlist(&data1, QIODevice::ReadOnly);
    wptlist.setVersion(QDataStream::Qt_4_5);

    wptlist >> nWpt;

    progress->setMaximum(nWpt);
    for(n = 0; n < nWpt; ++n)
    {
        QByteArray data;
        QDataStream stream(&data,QIODevice::ReadWrite);
        stream.setVersion(QDataStream::Qt_4_5);
        wptlist >> key >> name;

        progress->setLabelText(tr("Download track: %1").arg(name));
        qApp->processEvents();

        stream << key;

        if(!exchange(type = eH2CTrk,data))
        {
            QMessageBox::critical(0,tr("Error..."), tr("QLandkarteM: Failed to transfer tracks."),QMessageBox::Abort,QMessageBox::Abort);
            return release();
        }

        if(type == eError)
        {
            QMessageBox::critical(0,tr("Error..."), QString(data),QMessageBox::Abort,QMessageBox::Abort);
            return release();
        }

        stream.device()->seek(0);

        CTrack * trk = new CTrack(&CTrackDB::self());
        stream >> *trk;
        trks.push_back(trk);

        progress->setValue(n + 1);
    }

    release();
    return;
}


void CDeviceQLandkarteM::uploadMap(const QList<IMapSelection*>& mss)
{
    if(!startDeviceDetection()) return;
    QMessageBox::information(0,tr("Error..."), tr("QLandkarteM: Upload map is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
}


void CDeviceQLandkarteM::uploadRoutes(const QList<CRoute*>& rtes)
{
    if(!startDeviceDetection()) return;
    QMessageBox::information(0,tr("Error..."), tr("QLandkarteM: Download routes is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
}


void CDeviceQLandkarteM::downloadRoutes(QList<CRoute*>& rtes)
{
    if(!startDeviceDetection()) return;
    QMessageBox::information(0,tr("Error..."), tr("QLandkarteM: Download routes is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
}


bool CDeviceQLandkarteM::startDeviceDetection()
{
    QUdpSocket udpSocketSend;
    if(ipaddr.isEmpty() || port == 0)
    {
        QApplication::setOverrideCursor(Qt::WaitCursor);

        QByteArray datagram = "GETADRESS";
        // Send query on all network broadcast for each network interface
        QList<QNetworkInterface> netdevices = QNetworkInterface::allInterfaces();
        QNetworkInterface netdevice;
        foreach(netdevice, netdevices)
        {
            QList<QNetworkAddressEntry> networks = netdevice.addressEntries();
            QNetworkAddressEntry network;
            foreach(network, networks)
            {
                udpSocketSend.writeDatagram(datagram.data(), datagram.size(), network.broadcast(), REMOTE_PORT);
            }
        }
        QTime time;
        time.start();
        while(time.elapsed() < 3000)
        {
            QApplication::processEvents();
        }
        QApplication::restoreOverrideCursor();
    }

    if(ipaddr.isEmpty() || port == 0)
    {
        QMessageBox::critical(0,tr("Error..."), tr("QLandkarteM: No device found. Is it connected to the network?"),QMessageBox::Abort,QMessageBox::Abort);
        return false;
    }

    return true;
}


void CDeviceQLandkarteM::detectedDevice()
{
    // Detect device only if none already
    if (ipaddr !="") return;

    while (udpSocket.hasPendingDatagrams())
    {
        QByteArray datagram;
        QHostAddress qlmAddress;
        quint16 qlmPort;

        datagram.resize(udpSocket.pendingDatagramSize());
        udpSocket.readDatagram(datagram.data(), datagram.size(), &qlmAddress, &qlmPort);

        ipaddr  = qlmAddress.toString();
        port    = qlmPort;
        qDebug() << "Device detected is " << datagram << " with address " << ipaddr << " and port " << port << "\r\n";

    }
}


bool CDeviceQLandkarteM::waitTcpServerStatus()
{
    if (udpSocket.waitForReadyRead( 30000 ))
        while (udpSocket.hasPendingDatagrams())
    {
        QByteArray datagram;
        QHostAddress qlmAddress;
        quint16 qlmPort;

        datagram.resize(udpSocket.pendingDatagramSize());
        udpSocket.readDatagram(datagram.data(), datagram.size(), &qlmAddress, &qlmPort);
        // Answer from the good M device we are speaking to
        if (ipaddr == qlmAddress.toString() )
        {
            qDebug() << "Device detected send " << datagram << " with address " << ipaddr << " and port " << port << "\r\n";
            if (datagram== "ACK_START")
            {
                qDebug() << "ACK_START\r\n";
                tcpSocket.connectToHost(ipaddr,port);
                if(!tcpSocket.waitForConnected(timeout))
                {
                    QMessageBox::critical(0,tr("Error..."), tr("QLandkarteM: Failed to connect to device."),QMessageBox::Abort,QMessageBox::Abort);
                    tcpSocket.disconnectFromHost();
                    return false;
                }
                qDebug() << "Connected to M\r\n";
                break;
            }
            else
            {
                qDebug() << "ACK_STOP\r\n";
                break;
            }
        }
    }
    return true;
}
