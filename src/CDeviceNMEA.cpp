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
#include "CDeviceNMEA.h"

#include <QtGui>
#include <QMessageBox>
#include <proj_api.h>

CDeviceNMEA::CDeviceNMEA(const QString& serialport,
const QString& baudrate,
QObject * parent,
bool watchdogEnabled )
: IDevice("NMEA",parent)
, serialport(serialport)
, haveSeenData(false)
, haveSeenGPRMC(false)
, haveSeenGPGGA(false)
, haveSeenGPGSA(false)
, haveSeenGPVTG(false)
{
#ifdef QK_QT5_SERIAL_PORT
    QSerialPort::BaudRate eBaudrate = QSerialPort::Baud4800;
    if (baudrate.compare("9600")   == 0) { eBaudrate = QSerialPort::Baud9600;   }
    if (baudrate.compare("19200")  == 0) { eBaudrate = QSerialPort::Baud19200;  }
    if (baudrate.compare("38400")  == 0) { eBaudrate = QSerialPort::Baud38400;  }
    if (baudrate.compare("57600")  == 0) { eBaudrate = QSerialPort::Baud57600;  }
    if (baudrate.compare("115200") == 0) { eBaudrate = QSerialPort::Baud115200; }

    tty.setBaudRate(eBaudrate);
    tty.setDataBits(QSerialPort::Data8);
    tty.setParity(QSerialPort::NoParity);
    tty.setStopBits(QSerialPort::OneStop);
    tty.setFlowControl(QSerialPort::NoFlowControl);

    tty.setPortName(serialport);
#else
    enum BaudRateType eBaudrate = BAUD4800;
    if (baudrate.compare("9600")   == 0) { eBaudrate = BAUD9600;   }
    if (baudrate.compare("19200")  == 0) { eBaudrate = BAUD19200;  }
    if (baudrate.compare("38400")  == 0) { eBaudrate = BAUD38400;  }
    if (baudrate.compare("57600")  == 0) { eBaudrate = BAUD57600;  }
    if (baudrate.compare("115200") == 0) { eBaudrate = BAUD115200; }

    tty.setBaudRate(eBaudrate);  //BaudRate
    tty.setDataBits(DATA_8);     //DataBits
    tty.setParity(PAR_NONE);     //Parity
    tty.setStopBits(STOP_1);     //StopBits
    tty.setFlowControl(FLOW_OFF);//FlowControl

    tty.setTimeout(0, 10);
    tty.enableReceiving();
    tty.setPort(serialport);
#endif

    if (watchdogEnabled)
    {
        watchdog = new QTimer(this);
        connect(watchdog, SIGNAL(timeout()), this, SLOT(slotWatchdog()));
    }
    else
    {
        watchdog = NULL;
    }

#ifdef QK_QT5_SERIAL_PORT
    connect(&tty, SIGNAL(readyRead()), this, SLOT(slotNewDataReceived()));
#else
    connect(&tty, SIGNAL(newDataReceived(const QByteArray &)), this, SLOT(slotNewDataReceived(const QByteArray &)));
#endif
}


CDeviceNMEA::~CDeviceNMEA()
{
    tty.close();
}


void CDeviceNMEA::setLiveLog(bool on)
{
    //    qDebug() << "void CDeviceNMEA::setLiveLog()" << on;
    if(on)
    {
        //reset log
        CLiveLog fresh_log;
        log = fresh_log;
        log.error_unit = "DOP";
        //HS: If I don't deactivate the following line,
        //then the first track point is most/all times contains trash.
        //I don't yet know why since only valid points
        //should go into the log ...
        //emit sigLiveLog(log);

#ifdef QK_QT5_SERIAL_PORT
        if(tty.open(QIODevice::ReadOnly))
        {
            bool recv_ok = tty.isOpen() && tty.isReadable();
#else
            if(tty.open())
            {
                uchar recv_ok = tty.receiveData();
#endif
                if (recv_ok == 1)
                {
                    log.fix = CLiveLog::eConnectionEstablished;
                }
                else
                {
                    log.fix = CLiveLog::eConnectionFailed;
                }

            }
            else
            {
                log.fix = CLiveLog::eConnectionFailed;
            }
            if (watchdog != NULL) watchdog->start(10000);
            haveSeenData    = false;
            haveSeenGPRMC   = false;
            haveSeenGPGGA   = false;
            haveSeenGPGSA   = false;
            haveSeenGPVTG   = false;
        }
        else
        {
            if (watchdog != NULL) watchdog->stop();
            tty.close();
            log.fix = CLiveLog::eOff;
        }
        //always update the status display;
        emit sigLiveLog(log);
    }

    bool CDeviceNMEA::liveLog()
    {
        return tty.isOpen();
    }

#ifdef QK_QT5_SERIAL_PORT
    void CDeviceNMEA::slotNewDataReceived()
    {
        QByteArray dataReceived = tty.readAll();
#else
        void CDeviceNMEA::slotNewDataReceived(const QByteArray &dataReceived)
        {
#endif
            int i;

            if (log.fix == CLiveLog::eConnectionEstablished)
            {
                log.fix = CLiveLog::eConnectionReceiving;
            }

            log.count_bytes += dataReceived.size();

            for(i = 0; i < dataReceived.size(); ++i)
            {

                if(dataReceived[i] == '\n')
                {
                    line = line.trimmed();
                    if (isChecksumValid())
                    {
                        log.count_nmea++;
                        decode();
                    }
                    else
                    {
                        line.clear();
                    }
                }
                else
                {
                    line += dataReceived[i];
                }
            }

            //update bytes received statistics while we are in state eConnectionReceiving
            if (log.fix == CLiveLog::eConnectionReceiving)
            {
                emit sigLiveLog(log);
            }
        }

        bool CDeviceNMEA::isChecksumValid()
        {
            //the checksum is the exclusive or
            //of all characters between $ and *
            bool ret = false;
            unsigned char calc_checksum = 0;
            unsigned char nmea_checksum = 0;
            int i = 1;
            int len = line.length();

            if ( (len > 1) && (line.at(0).toLatin1() == '$') )
            {
                //calculate checksum
                while ( (i < len-1) && (line.at(i).toLatin1() != '*') )
                {
                    calc_checksum = calc_checksum ^ line.at(i).toLatin1();
                    i++;
                }
                if ( (len >= i+3) && (line.at(i).toLatin1() == '*') )
                {
                    bool ok;
                    nmea_checksum = line.mid(i+1,2).toUShort(&ok, 16);
                    if (ok && (nmea_checksum == calc_checksum))
                    {
                        ret = true;
                    }
                }
                else             //no checksum considered as valid
                {
                    ret = true;
                }
            }
            return ret;
        }

        void CDeviceNMEA::decode()
        {
            //Unfortunately , the original NMEA-0183 protocol specification is not freely available:
            //  http://www.nmea.org charges several hundred US$ for the document
            //Fortunately, we are not completely lost since there are many freely available descriptions in the web
            //  Good ones can be downloaded e.g. from the u-blox web sites (search for "protocol").
            //And we are able to test with a wide variety of receivers floating around in the community
            //
            //There are several bad thing in the NMEA protocol
            //  a) It contains a lot of redundancy, i.e. some fields may be contained in different sentences
            //  b) Different GPS receivers may provide different sets of NMEA sentences
            //  c) Not all NMEA sentences need to be provided in the same frequency
            //  d) Different GPS receivers may emit NMEA sentences in different order for the same GPS fix
            //  e) There are several protocol versions around.
            //     So some fields may be empty or (typically at the end of a sentence) not present
            //
            //Therefore a generic NMEA parser must be based on some heuristics.
            //For this one, the following have been chosen
            //  - support $GPRMC, $GPGGA, $GPGSA, $GPVTG sentences
            //  - in general, each sentence is provided cyclically
            //  - trigger emitting the collected data on a sentence which contains the most important data: lat/lon
            //    -- those are $GPRMC and $GPGGA
            //    -- prefer $GPRMC over $GPGGA since it contains a more complete set of data for land navigation
            //      --- $GPRMC contains heading, speed and date which are not present in $GPGGA
            //      --- $GPGGA contains fix status, uset sat, hdop and altitude which are not present in $GPRMC
            //    -- but trigger on $GPGGA if no $GPRMC has been observed in the collection cycle
            //      --- accepted drawback when both $GPGGA and $GPRMC are provided:
            //        if $GGGPA is collected first (50/50 chance) the date of the first trackpoint will be wrong
            //  - take as much as possible data from $GPRMC
            //    -- collect the rest from the other sentences
            //    -- cache (i.e. do not reset) "slowly" changing data
            //  - handle position fix flags in a tolerant way, i.e. do not require a $GPGSA sentence

            QString tok;
            QStringList tokens = line.split(QRegExp("[,*]"));
            //     qDebug() << line;
            //     qDebug() << tokens.count() << tokens;
            if((tokens[0] == "$GNGGA") || (tokens[0] == "$GPGGA"))
            {
                //             0      1                  2       3         4        5    6     7     8       9     10    11     12   13    14
                //     15 ("$GPGGA", "130108.000", "4901.7451", "N", "01205.8656", "E", "1", "06", "1.8", "331.6", "M", "47.3", "M", "", "0000*5F")
                //         qDebug() << tokens.count() << tokens;
                haveSeenGPGGA = true;

                //field 1: time
                //time can contain an arbitrary number of fractional digits
                //therefore, the  QDateTime/QTime::fromString() functions do not work here
                //there is no date in $GPGGA - take any date far away from now
                if (tokens[1] == "") { tokens[1] = "000000"; }
                tok = tokens[1];
                int hours=0, minutes = 0, seconds = 0, milliseconds=0;
                if (tok.length() >= 6)
                {
                    hours   = tok.mid(0,2).toInt();
                    minutes = tok.mid(2,2).toInt();
                    double s, frac;
                    frac = modf(tok.mid(4).toFloat(), &s);
                    seconds = s;
                    milliseconds = floor(frac*1000.0+0.5);
                }
                QTime time(hours, minutes, seconds, milliseconds);
                QDate date = QDate::fromString("311299","ddMMyy");
                QDateTime datetime(date, time, Qt::UTC);
                datetime = datetime.addYears(100);
                log.timestamp = datetime.toTime_t();

                //fields 2,3: latitude
                tok = tokens[2];
                log.lat = tok.left(2).toInt() + tok.mid(2).toDouble() / 60.0;
                log.lat = (tokens[3] == "N" ? log.lat : -log.lat);

                //fields 4,5: longitude
                tok = tokens[4];
                log.lon = tok.left(3).toInt() + tok.mid(3).toDouble() / 60.0;
                log.lon = (tokens[5] == "E" ? log.lon : -log.lon);

                //field 6: fix status: 0: no Fix, 1: GPS, 2: DGPS, 6: Estimated
                int fix = tokens[6].toInt();
                if (fix == 0)
                {
                    log.fix = CLiveLog::eNoFix;
                }
                else if (fix == 6)
                {
                    log.fix = CLiveLog::eEstimated;
                }
                else if ( (!haveSeenGPGSA)
                    || (log.fix == CLiveLog::eNoFix)
                    || (log.fix == CLiveLog::eOff) )
                {
                    //only $GPGSA can distinguish between 2D and 3D fix
                    //assume 3D fix here, if there is GPS or DGPS fix
                    log.fix = (fix == 1) ? CLiveLog::e3DFix :
                    (fix == 2) ? CLiveLog::e3DFix :
                    CLiveLog::eNoFix;
                }

                //field 7: sat used
                log.sat_used =  tokens[7].toInt();

                //field 8: hdop
                double hdop =  tokens[8].toDouble();
                //estimate horizontal error from dop values
                log.error_horz = hdop;

                //field 9: MSL altitude
                log.ele = tokens[9].toDouble();

                if(!haveSeenGPRMC)
                {
                    log.count_fix++;
                    emit sigLiveLog(log);
                }
            }
            else if((tokens[0] == "$GNGSA") || (tokens[0] == "$GPGSA"))
            {
                //             0      1    2     3    4      5     6     7    8   9  10  11  12  13  14    15    16     17
                //     18 ("$GPGSA", "A", "3", "11", "23", "13", "04", "17", "", "", "", "", "", "", "", "3.5", "2.2", "2.6*31")
                //         qDebug() << tokens.count() << tokens;
                haveSeenGPGSA = true;

                //field 2: fix status: 1: no Fix, 2: 2D, 3: 3D
                int fix = tokens[2].toInt();
                if (fix == 2)
                {                //let $GPGGA:estimated take precedence
                    if (log.fix != CLiveLog::eEstimated)
                    {
                        log.fix = CLiveLog::e2DFix;
                    }
                }
                else if (fix == 3)
                {                //let $GPGGA:estimated take precedence
                    if (log.fix != CLiveLog::eEstimated)
                    {
                        log.fix = CLiveLog::e3DFix;
                    }
                }
                else
                {
                    log.fix = CLiveLog::eNoFix;
                }

                //field 15: pdop
                //double pdop = tokens[15].toDouble(); // currently unused

                //field 16: hdop
                double hdop = tokens[16].toDouble();

                //field 17: vdop
                double vdop = tokens[17].toDouble();

                //estimate horizontal and vertical errors from dop values
                log.error_horz = hdop;
                log.error_vert = vdop;
            }
            else if((tokens[0] == "$GNRMC") || (tokens[0] == "$GPRMC"))
            {
                //         13 ("$GPRMC", "122450.539", "V", "4901.6288", "N", "01205.5946", "E", "", "", "030408", "", "", "N*76")
                //            ("$GPRMC", "183956.648", "A", "4341.0506", "N", "00407.7897", "E", "3.81", "186.84", "060408", "", "", "A", "64")
                //         qDebug() << tokens.count() << tokens[1] << tokens[9];
                haveSeenGPRMC = true;

                //field 1: time, field 9: date
                //time can contain an arbitrary number of fractional digits
                //therefore, the  QDateTime/QTime::fromString() functions do not work here
                if (tokens[1] == "") { tokens[1] = "000000"; }
                tok = tokens[1];
                int hours=0, minutes = 0, seconds = 0, milliseconds=0;
                if (tok.length() >= 6)
                {
                    hours   = tok.mid(0,2).toInt();
                    minutes = tok.mid(2,2).toInt();
                    double s, frac;
                    frac = modf(tok.mid(4).toFloat(), &s);
                    seconds = s;
                    milliseconds = floor(frac*1000.0+0.5);
                }
                QTime time(hours, minutes, seconds, milliseconds);
                if (tokens[9] == "") { tokens[9] = "311299"; }
                QDate date = QDate::fromString(tokens[9],"ddMMyy");
                QDateTime datetime(date, time, Qt::UTC);
                datetime = datetime.addYears(100);
                log.timestamp = datetime.toTime_t();

                //field 2: status
                tok = tokens[2];
                if (tok != "A")
                {                //conservative approach
                    log.fix = CLiveLog::eNoFix;
                }
                else if ( ((!haveSeenGPGSA) && (!haveSeenGPGGA))
                    || (log.fix == CLiveLog::eNoFix)
                    || (log.fix == CLiveLog::eOff) )
                {                //$GPGSA and $GPGGA have more detailed fix information
                    log.fix = CLiveLog::e3DFix;
                }

                //fields 3,4: latitude
                tok = tokens[3];
                log.lat = tok.left(2).toInt() + tok.mid(2).toDouble() / 60.0;
                log.lat = (tokens[4] == "N" ? log.lat : -log.lat);

                //fields 5,6: longitude
                tok = tokens[5];
                log.lon = tok.left(3).toInt() + tok.mid(3).toDouble() / 60.0;
                log.lon = (tokens[6] == "E" ? log.lon : -log.lon);

                //field 7: velocity in knots
                if (tokens[7] != "")
                {                //when the field is empty: hope that velocity is in $GPVTG
                    log.velocity    = tokens[7].toFloat() * 0.514444;
                }

                //field 8: heading in degrees
                if (tokens[8] != "")
                {                //when the field is empty: hope that heading is in $GPVTG
                    log.heading     = tokens[8].toFloat();
                }

                //always!
                log.count_fix++;
                emit sigLiveLog(log);
            }
            else if((tokens[0] == "$GPVTG"))
            {
                //                  0        1       2    3   4      5     6      7     8    9
                //          11 ("$GPVTG", "357.22", "T", "", "M", "0.09", "N", "0.16", "K", "A", "32")
                haveSeenGPVTG = true;

                //field 5: velocity in knots
                if (tokens[5] != "")
                {                //when the field is empty: hope that velocity is in $GPRMC
                    log.velocity    = tokens[5].toFloat() * 0.514444;
                }

                //field 1: heading in degrees
                if (tokens[1] != "")
                {                //when the field is empty: hope that heading is in $GPRMC
                    log.heading     = tokens[1].toFloat();
                }
            }

            haveSeenData = true;
            line.clear();
        }

        void CDeviceNMEA::slotWatchdog()
        {
            if(tty.isOpen() && haveSeenData)
            {
                haveSeenData = false;
                return;
            }

            setLiveLog(false);
        }

        void CDeviceNMEA::uploadWpts(const QList<CWpt*>& /*wpts*/)
        {
            QMessageBox::information(0,tr("Error..."), tr("NMEA: Upload waypoints is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
        }

        void CDeviceNMEA::downloadWpts(QList<CWpt*>& /*wpts*/)
        {
            QMessageBox::information(0,tr("Error..."), tr("NMEA: Download waypoints is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
        }

        void CDeviceNMEA::uploadTracks(const QList<CTrack*>& /*trks*/)
        {
            QMessageBox::information(0,tr("Error..."), tr("NMEA: Upload tracks is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
        }

        void CDeviceNMEA::downloadTracks(QList<CTrack*>& /*trks*/)
        {
            QMessageBox::information(0,tr("Error..."), tr("NMEA: Download tracks is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
        }

        void CDeviceNMEA::uploadRoutes(const QList<CRoute*>& /*rtes*/)
        {
            QMessageBox::information(0,tr("Error..."), tr("NMEA: Upload routes is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
        }

        void CDeviceNMEA::downloadRoutes(QList<CRoute*>& /*rtes*/)
        {
            QMessageBox::information(0,tr("Error..."), tr("NMEA: Download routes is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
        }

        void CDeviceNMEA::uploadMap(const QList<IMapSelection*>& /*mss*/)
        {
            QMessageBox::information(0,tr("Error..."), tr("NMEA: Upload maps is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
        }

        void CDeviceNMEA::downloadScreenshot(QImage& /*image*/)
        {
            QMessageBox::information(0,tr("Error..."), tr("NMEA: Download screenshots is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
        }
