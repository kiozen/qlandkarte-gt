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
#include "CDeviceGPSD.h"

#include <QtGui>
#include <QtCore/QMutex>
#include <QMessageBox>
#include <sys/select.h>
#include <unistd.h>

CDeviceGPSD::CDeviceGPSD(QObject * parent)
: IDevice("GPSD",parent)
, timer( new QTimer(this) )
{
    connect(timer, SIGNAL(timeout()), this, SLOT(slotTimeout()));

    int pipefd[ 2 ];
    // TODO: check for error
    pipe( pipefd );
    thread_fd = pipefd[ 1 ];
    thread = new CGPSDThread( pipefd[ 0 ] );
}


CDeviceGPSD::~CDeviceGPSD()
{
    if( thread->isRunning() )
    {
        // writing to thread_fd wakes up the select in the thread.
        char s = 's';
        write( thread_fd, &s, 1 );
        thread->wait();
    }
    delete thread;
}


void CDeviceGPSD::setLiveLog(bool on)
{
    //    qDebug() << "void CDeviceGPSD::setLiveLog() " << on;
    if(on)
    {
        log.fix = CLiveLog::eNoFix;
        emit sigLiveLog(log);

        // connect to gpsd
        //        qDebug() << "starting thread";
        thread->start();

        if(!timer->isActive())
        {
            timer->start(500);
        }
    }
    else
    {
        timer->stop();
        if( thread->isRunning() )
        {
            char s = 's';
            write( thread_fd, &s, 1 );
            thread->wait();
        }
        log.fix = CLiveLog::eOff;
        emit sigLiveLog(log);
    }
}


bool CDeviceGPSD::liveLog()
{
    return thread->isRunning();
}


void CDeviceGPSD::slotTimeout()
{
    if( !thread->isRunning() )
    {
        timer->stop();
        log.fix = CLiveLog::eOff;
        emit sigLiveLog(log);
        //        qDebug() << "stopped thread detected.";
    }

    if( thread->log(log) )
        emit sigLiveLog(log);
}


void CDeviceGPSD::uploadWpts(const QList<CWpt*>& /*wpts*/)
{
    QMessageBox::information(0,tr("Error..."), tr("GPSD: Upload waypoints is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
}


void CDeviceGPSD::downloadWpts(QList<CWpt*>& /*wpts*/)
{
    QMessageBox::information(0,tr("Error..."), tr("GPSD: Download waypoints is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
}


void CDeviceGPSD::uploadTracks(const QList<CTrack*>& /*trks*/)
{
    QMessageBox::information(0,tr("Error..."), tr("GPSD: Upload tracks is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
}


void CDeviceGPSD::downloadTracks(QList<CTrack*>& /*trks*/)
{
    QMessageBox::information(0,tr("Error..."), tr("GPSD: Download tracks is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
}


void CDeviceGPSD::uploadRoutes(const QList<CRoute*>& /*rtes*/)
{
    QMessageBox::information(0,tr("Error..."), tr("GPSD: Upload routes is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
}


void CDeviceGPSD::downloadRoutes(QList<CRoute*>& /*rtes*/)
{
    QMessageBox::information(0,tr("Error..."), tr("GPSD: Download routes is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
}


void CDeviceGPSD::uploadMap(const QList<IMapSelection*>& /*mss*/)
{
    QMessageBox::information(0,tr("Error..."), tr("GPSD: Upload maps is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
}


void CDeviceGPSD::downloadScreenshot(QImage& /*image*/)
{
    QMessageBox::information(0,tr("Error..."), tr("GPSD: Download screenshots is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
}


CGPSDThread::CGPSDThread( int _pipe_fd )
: QThread(),
log_mutex( new QMutex() ),
pipe_fd( _pipe_fd )
{
    gpsdata = NULL;
}


CGPSDThread::~CGPSDThread()
{
    delete log_mutex;
}


void CGPSDThread::run()
{
#if GPSD_API_MAJOR_VERSION >= 5
    gpsdata = new gps_data_t();
    if(gpsdata)
    {
        gps_open( "localhost", DEFAULT_GPSD_PORT, gpsdata );
    }
#else
    gpsdata = gps_open( "localhost", DEFAULT_GPSD_PORT );
#endif
    if( !gpsdata )
    {
        //        qDebug() << "gps_open failed.";
        return;
    }                            // if
    //    qDebug() << "connected to gpsd.";

    gps_stream( gpsdata, WATCH_NEWSTYLE, NULL );

    fd_set fds;

    while( true )
    {
        // sleep until either GPSD or our controlling thread has data for us.
        FD_ZERO(&fds);
        FD_SET(gpsdata->gps_fd, &fds);
        FD_SET(pipe_fd, &fds);
        int nfds = (pipe_fd > gpsdata->gps_fd ? pipe_fd : gpsdata->gps_fd) + 1;
        int data = select(nfds, &fds, NULL, NULL, NULL);

        if( data == -1 )
        {
            break;
        }                        // if

        else if( data )
        {
            if( FD_ISSET( pipe_fd, &fds ) )
            {
                //                qDebug() << "stop command received";
                char s;
                read( pipe_fd, &s, 1 );
                break;
            }                    // if
            else if( FD_ISSET( gpsdata->gps_fd, &fds ) )
            {
#if GPSD_API_MAJOR_VERSION >= 5
                gps_read( gpsdata );
#else
                gps_poll( gpsdata );
#endif
                if( !decodeData() ) break;
            }                    // else if
        }                        // else if
    }                            // while

    gps_close( gpsdata );
#if GPSD_API_MAJOR_VERSION >= 5
    delete gpsdata;
#endif
    //    qDebug() << "thread done";
}


bool CGPSDThread::decodeData()
{
    // see, if it's interesting
    if( gpsdata->fix.time == 0 )
        return true;
    static const gps_mask_t interesting_mask = TIME_SET | LATLON_SET
        | ALTITUDE_SET | SPEED_SET | TRACK_SET | STATUS_SET | MODE_SET
        | HERR_SET | VERR_SET | SATELLITE_SET | ONLINE_SET;

    if( (gpsdata->set & interesting_mask) == 0 )
        return true;

    //if( !gpsdata->online ) return false;

    QMutexLocker locker( log_mutex );

    switch( gpsdata->fix.mode )
    {
        case MODE_NOT_SEEN:
        case MODE_NO_FIX:
            current_log.fix = CLiveLog::eNoFix;
            break;
        case MODE_2D:
            current_log.fix = CLiveLog::e2DFix;
            break;
        case MODE_3D:
            current_log.fix = CLiveLog::e3DFix;
            break;
    }

    current_log.lon = gpsdata->fix.longitude;
    current_log.lat = gpsdata->fix.latitude;
    current_log.ele = gpsdata->fix.altitude;
    current_log.timestamp = gpsdata->fix.time;
    current_log.error_horz = gpsdata->fix.epx;
    current_log.error_vert = gpsdata->fix.epv;
    current_log.heading = gpsdata->fix.track;
    current_log.velocity = gpsdata->fix.speed;

    current_log.sat_used = gpsdata->satellites_used;

    changed = true;

    return true;
}


bool CGPSDThread::log( CLiveLog& out )
{
    QMutexLocker locker( log_mutex );
    out = current_log;
    bool ret = changed;
    changed = false;
    return ret;
}
