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

#include "CLiveLogDB.h"
#include "CLiveLogToolWidget.h"
#include "CLiveLog.h"
#include "GeoMath.h"
#include "CMapDB.h"
#include "IMap.h"
#include "CWptDB.h"
#include "CResources.h"
#include "IDevice.h"
#include "CTrackDB.h"
#include "CTrack.h"
#include "IUnit.h"
#include "CSettings.h"

#include <QtGui>

#include <limits>
#ifdef WIN32
#define isnan(x) _isnan(x)
#endif

CLiveLogDB * CLiveLogDB::m_self = 0;

CLiveLogDB::CLiveLogDB(QTabWidget * tb, QObject * parent)
: IDB(IDB::eTypeLog,  tb, parent)
, m_lockToCenter(false)
, m_useSmallArrow(false)
{
    m_self      = this;

    SETTINGS;
    m_lockToCenter = cfg.value("livelog/lockToCenter", m_lockToCenter).toBool();
    m_useSmallArrow = cfg.value("livelog/useSmallArrow", m_useSmallArrow).toBool();

    toolview    = new CLiveLogToolWidget(tb);

    connect(&CMapDB::self(), SIGNAL(sigChanged()), this, SLOT(slotMapDBChanged()));
    slotMapDBChanged();

    saveBackupLog();
    backup = new QFile(QDir::temp().filePath("qlBackupLog"),this);

}


CLiveLogDB::~CLiveLogDB()
{
    SETTINGS;
    cfg.setValue("livelog/lockToCenter", m_lockToCenter);
    cfg.setValue("livelog/useSmallArrow", m_useSmallArrow);
}


void CLiveLogDB::saveBackupLog()
{
    if(QFile::exists(QDir::temp().filePath("qlBackupLog")))
    {
        QFile tmp(QDir::temp().filePath("qlBackupLog"));
        tmp.open(QIODevice::ReadOnly);
        QDataStream in(&tmp);
        in.setVersion(QDataStream::Qt_4_5);

        CTrack * t = new CTrack(&CTrackDB::self());
        t->setName(tr("LiveLog"));

        CLiveLog log;

        in >> log;

        while(!tmp.atEnd())
        {
            CTrack::pt_t pt;
            pt.timestamp = log.timestamp;
            pt.lon       = log.lon;
            pt.lat       = log.lat;
            pt.ele       = log.ele;
            *t << pt;

            in >> log;
        }
        if(t->getTrackPoints().size())
        {
            CTrackDB::self().addTrack(t, false);
        }
        else
        {
            delete t;
        }

        tmp.close();
        tmp.remove();
    }
}


void CLiveLogDB::start(bool yes)
{
    IDevice * dev = CResources::self().device();
    if(dev == 0) return;

    dev->setLiveLog(yes);
    if(!yes)
    {
        clear();
    }

}


bool CLiveLogDB::logging()
{
    IDevice * dev = CResources::self().device();
    if(dev == 0) return false;

    return dev->liveLog();
}


void CLiveLogDB::clear()
{
    saveBackupLog();
    track.clear();
    polyline.clear();
    m_log.fix = CLiveLog::eOff;
    emitSigChanged();
}


void CLiveLogDB::setLockToCenter(bool on)
{
    m_lockToCenter = on;

    CLiveLogToolWidget * w = qobject_cast<CLiveLogToolWidget*>(toolview);
    if(w == 0) return;

    w->labelCenter->setVisible(m_lockToCenter);
}

void CLiveLogDB::slotLiveLog(const CLiveLog& log)
{
    CLiveLogToolWidget * w = qobject_cast<CLiveLogToolWidget*>(toolview);
    if(w == 0) return;

    //1.) always update the date/time and used sat text fields
    w->lblTime->setText(QDateTime::fromTime_t(log.timestamp).toString());
    if (log.sat_used != -1)
    {
        w->lblSatUsed->setText(tr("%1").arg(log.sat_used));
    }
    else
    {
        w->lblSatUsed->setText("-");
    }

    //mark standstill
    bool standstill = m_log.lat == log.lat && m_log.lon == log.lon && m_log.fix == log.fix;

    //2.) always update m_log which is used to draw the position symbolin the map
    m_log = log;
    float speed_km_h = 0;
    if (log.velocity != WPT_NOFLOAT)
    {
        speed_km_h = log.velocity * 3.6;
    }
    float heading = log.heading;
    //HS: depending on what log.error_horz for the different devices,
    //we could use it here as well.
    //If it's HDOP as for NMEA, simply multiply with the speed threshold.
    //We might consider moving this filter to the devices ...
    if( speed_km_h < 0.3 )
    {
        // some pretty arbitrary threshold ...
        // with a horizontal error of +/-5m it never goes above
        // 0.06 km/h while standing still, but you don't always
        // have +/-5m...
        speed_km_h = 0.0;
        heading = WPT_NOFLOAT;
    }
    m_log.heading = heading;

    QString pos;
    GPS_Math_Deg_To_Str(log.lon, log.lat, pos);

    //3.) only if the position is considered valid
    if(log.fix == CLiveLog::e2DFix || log.fix == CLiveLog::e3DFix ||  log.fix == CLiveLog::eEstimated)
    {
        //3.1) update the other text fields
        QString val, unit;
        if (log.fix == CLiveLog::e2DFix)
        {
            w->lblStatus->setText((log.count_fix==0)?"2D":tr("2D (%1)").arg(log.count_fix));
        }
        else if (log.fix == CLiveLog::e3DFix)
        {
            w->lblStatus->setText((log.count_fix==0)?"3D":tr("3D (%1)").arg(log.count_fix));
        }
        else if (log.fix == CLiveLog::eEstimated)
        {
            w->lblStatus->setText((log.count_fix==0)?"DR":tr("DR (%1)").arg(log.count_fix));
        }
        w->lblPosition->setText(pos);

        if (log.ele != WPT_NOFLOAT)
        {
            IUnit::self().meter2elevation(log.ele, val,unit);
            w->lblAltitude->setText(tr("%1 %2").arg(val).arg(unit));
        }
        else
        {
            w->lblAltitude->setText("-");
        }

        if (log.error_horz != WPT_NOFLOAT)
        {
            //HS: removed division by 2: error estimates should be done by device
            w->lblErrorHoriz->setText(tr("%1 %2").arg(log.error_horz,0,'f',1).arg(log.error_unit));
        }
        else
        {
            w->lblErrorHoriz->setText("-");
        }

        if (log.error_vert != WPT_NOFLOAT)
        {
            //HS: removed division by 2: error estimates should be done by device
            w->lblErrorVert->setText(tr("%1 %2").arg(log.error_vert,0,'f',1).arg(log.error_unit));
        }
        else
        {
            w->lblErrorVert->setText("-");
        }

        if (log.velocity != WPT_NOFLOAT)
        {
            IUnit::self().meter2speed(log.velocity, val,unit);
            w->lblSpeed->setText(tr("%1 %2").arg(val).arg(unit));
        }
        else
        {
            w->lblSpeed->setText("-");
        }

        if(log.heading != WPT_NOFLOAT)
        {
            //HS: removed stand-still filtering for displayed values
            w->lblHeading->setText(tr("%1%2 T").arg((int)(log.heading + 0.5),3,'f',0,'0').arg(QChar(0260)));
        }
        else
        {
            w->lblHeading->setText("-");
        }

        //3.2) no saving and mapcentering at standstill
        if (!standstill)
        {
            //3.3) att point to "backup" which is used to generate a track afterwards
            *backup << log;

            //3.4) add point to "track" which is needed to re-draw polyline when map is changed
            simplelog_t slog;
            slog.timestamp  = log.timestamp;
            slog.lon        = log.lon;
            slog.lat        = log.lat;
            slog.ele        = log.ele;
            track << slog;

            //3.5) add point to current polyline
            IMap& map = CMapDB::self().getMap();
            double u = slog.lon * DEG_TO_RAD;
            double v = slog.lat * DEG_TO_RAD;
            map.convertRad2Pt(u,v);
            polyline << QPoint(u,v);

            //3.6) move map center if necessary
            if(m_lockToCenter)
            {
                QSize size  = map.getSize();
                int   dx    = size.width()  / 6;
                int   dy    = size.height() / 6;
                QRect area  = QRect(2*dx, 2*dy, 2*dx, 2*dy);
                if(!area.contains(u,v))
                {
                    map.move(QPoint(u,v), area.center());
                }
            }
        }
    }
    //4.) when the position is considered invalid
    else if(log.fix == CLiveLog::eNoFix)
    {
        w->lblStatus->setText((log.count_fix==0)?"GPS signal low":tr("GPS signal low (%1)").arg(log.count_fix));
        w->lblPosition->setText("-");
        w->lblAltitude->setText("-");
        w->lblErrorHoriz->setText("-");
        w->lblErrorVert->setText("-");
        w->lblSpeed->setText("-");
        w->lblHeading->setText("-");
    }
    //5.) when logging is off
    else
    {
        if(log.fix == CLiveLog::eConnectionFailed)
        {
            w->lblStatus->setText(tr("GPS connection failed"));
        }
        else if(log.fix == CLiveLog::eConnectionEstablished)
        {
            w->lblStatus->setText(tr("GPS connection established"));
        }
        else if(log.fix == CLiveLog::eConnectionReceiving)
        {
            w->lblStatus->setText(tr("GPS connection receiving %1 bytes").arg(log.count_bytes));
        }
        else
        {
            w->lblStatus->setText(tr("GPS off"));
        }
        w->lblPosition->setText("-");
        w->lblAltitude->setText("-");
        w->lblErrorHoriz->setText("-");
        w->lblErrorVert->setText("-");
        w->lblSatUsed->setText("-");
        w->lblSpeed->setText("-");
        w->lblHeading->setText("-");
        w->lblTime->setText("-");
    }

    emitSigChanged();
}


void CLiveLogDB::slotMapDBChanged()
{
    IMap& map = CMapDB::self().getMap();

    connect(&map, SIGNAL(sigChanged()), this, SLOT(slotMapChanged()));
}


void CLiveLogDB::slotMapChanged()
{
    IMap& map = CMapDB::self().getMap();
    const quint32 limit = track.size();

    polyline.clear();
    const simplelog_t * plog = track.data();

    for(quint32 i = 0; i < limit; ++i)
    {

        double u = plog->lon * DEG_TO_RAD;
        double v = plog->lat * DEG_TO_RAD;
        map.convertRad2Pt(u,v);
        polyline << QPoint(u,v);
        ++plog;
    }
}


void CLiveLogDB::draw(QPainter& p, const QRect& rect, bool& needsRedraw)
{
    if(!rect.intersects(polyline.boundingRect())) return;

    p.setPen(QPen(Qt::white,5));
    p.drawPolyline(polyline);
    p.setPen(QPen(Qt::black,3));
    p.drawPolyline(polyline);

    if( (m_log.fix == CLiveLog::e2DFix) ||
        (m_log.fix == CLiveLog::e3DFix) ||
        (m_log.fix == CLiveLog::eEstimated) )
    {
        IMap& map = CMapDB::self().getMap();

        double u = m_log.lon * DEG_TO_RAD;
        double v = m_log.lat * DEG_TO_RAD;
        map.convertRad2Pt(u,v);

        float heading = m_log.heading;
        if(heading != WPT_NOFLOAT)
        {
            p.save();
            p.translate(u,v);
            p.rotate(heading);
            if(m_useSmallArrow)
            {
                QPolygon arrow;
                arrow << QPoint(0,-50);
                arrow << QPoint(5,-40);
                arrow << QPoint(-5,-40);
                arrow << QPoint(0,-50);

                p.setPen(QPen(Qt::white,5));
                p.setBrush(Qt::white);
                p.drawEllipse(-4,-4,8,8);
                p.drawLine(0,0,0,-50);
                p.drawPolygon(arrow);

                p.setPen(QPen(Qt::black,3));
                p.setBrush(Qt::black);
                p.drawEllipse(-4,-4,8,8);
                p.drawLine(0,0,0,-50);
                p.drawPolygon(arrow);

            }
            else
            {
                p.drawPixmap(-23,-30,QPixmap(":/cursors/cursor1.png"));
            }
            p.restore();
        }
        else
        {
            p.drawPixmap(u-20 , v-20, QPixmap(":/cursors/cursor2.png"));
        }

    }

}


void CLiveLogDB::addWpt()
{
    if( (m_log.fix == CLiveLog::e2DFix) ||
        (m_log.fix == CLiveLog::e3DFix) ||
        (m_log.fix == CLiveLog::eEstimated) )
    {
        CWptDB::self().newWpt(m_log.lon * DEG_TO_RAD, m_log.lat * DEG_TO_RAD, m_log.ele, "");
    }
}
