/**********************************************************************************************
    Copyright (C) 2008 Oliver Eichler oliver.eichler@gmx.de
    Copyright (C) 2010 Joerg Wunsch <j@uriah.heep.sax.de>

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
#include "config.h"

#include <stdio.h>
#include <limits>

#include "CTrackDB.h"
#include "CTrack.h"
#include "CTrackToolWidget.h"
#include "CQlb.h"
#include "CGpx.h"
#include "CResources.h"
#include "IDevice.h"
#include "CDlgCombineTracks.h"
#include "CMapDB.h"
#include "IMap.h"
#include "IUnit.h"
#include "CSettings.h"
#include "GeoMath.h"

#include <QtGui>
#include <QMessageBox>
#include "CUndoStackModel.h"
#include "CTrackUndoCommandSelect.h"

#if WIN32
#include <math.h>
#include <float.h>
#ifndef __MINGW32__
typedef __int32 int32_t;
#endif
#define isnan _isnan
#define FP_NAN NAN
#endif

CTrackDB * CTrackDB::m_self = 0;

bool CTrackDB::keyLessThanAlpha(CTrackDB::keys_t&  s1, CTrackDB::keys_t&  s2)
{
    return s1.name.toLower() < s2.name.toLower();
}


bool CTrackDB::keyLessThanTime(keys_t&  s1, keys_t&  s2)
{
    return s1.time < s2.time;
}


CTrackDB::CTrackDB(QTabWidget * tb, QObject * parent)
: IDB(IDB::eTypeTrk, tb, parent)
, cnt(0)
, showBullets(true)
, showMinMax(true)
{
    m_self      = this;

    SETTINGS;
    showBullets = cfg.value("track/showBullets", showBullets).toBool();
    showMinMax = cfg.value("track/showMinMax", showMinMax).toBool();
    toolview    = new CTrackToolWidget(tb);
    undoStack   = CUndoStackModel::getInstance();

    connect(&CMapDB::self(), SIGNAL(sigChanged()), this, SLOT(slotMapChanged()));
}


CTrackDB::~CTrackDB()
{
    SETTINGS;
    cfg.setValue("track/showBullets", showBullets);
    cfg.setValue("track/showMinMax", showMinMax);
}


void CTrackDB::clear()
{
    if(tracks.isEmpty()) return;

    cnt = 0;
    delTracks(tracks.keys());
    CTrack::resetKeyCnt();
    emit sigHighlightTrack(0);
    emitSigChanged();
}


CTrackToolWidget * CTrackDB::getToolWidget()
{
    return qobject_cast<CTrackToolWidget*>(toolview);
}


QRectF CTrackDB::getBoundingRectF(const QString key)
{
    if(!tracks.contains(key))
    {
        return QRectF();
    }
    return tracks.value(key)->getBoundingRectF();
}


QRectF CTrackDB::getBoundingRectF()
{
    QRectF r;
    foreach(CTrack *track, tracks.values())
    {
        r = r.united(track->getBoundingRectF());
    }
    return r;
}


void CTrackDB::loadQLB(CQlb& qlb, bool newKey)
{
    QDataStream stream(&qlb.tracks(),QIODevice::ReadOnly);
    stream.setVersion(QDataStream::Qt_4_5);

    while(!stream.atEnd())
    {
        CTrack * track = new CTrack(this);
        stream >> *track;
        if(newKey)
        {
            track->setKey(track->getKey() + QString("%1").arg(QDateTime::currentDateTime().toTime_t()));
        }
        addTrack(track, true);
    }

    if(qlb.tracks().size())
    {
        emitSigChanged();
    }

}


void CTrackDB::saveQLB(CQlb& qlb)
{
    QMap<QString, CTrack*>::const_iterator track = tracks.begin();
    while(track != tracks.end())
    {
        qlb << *(*track);
        ++track;
    }
}


void CTrackDB::loadGPX(CGpx& gpx)
{
    bool hasItems = false;
    QDomElement tmpelem;
    QDomElement trk = gpx.firstChildElement("gpx").firstChildElement("trk");
    while (!trk.isNull())
    {
        hasItems = true;
        CTrack* track = new CTrack(this);
                                 //preset a random color
#if defined(HAVE_ARC4RANDOM)
        track->setColor((arc4random() % 13)+1);
#else
        track->setColor((rand() % 13)+1);
#endif

        /*
         *  Global track information
         */

        QMap<QString,QDomElement> trkmap = CGpx::mapChildElements(trk);

        // GPX 1.1

        tmpelem = trkmap.value("name");
        if(!tmpelem.isNull()) track->setName(tmpelem.text());

        tmpelem = trkmap.value("desc");
        if(!tmpelem.isNull()) track->comment = tmpelem.text();

        tmpelem = trkmap.value("link");
        if(!tmpelem.isNull())
        {
            track->url = tmpelem.attribute("href");

            // For now we are ignoring the following elements:
            // * text - hyperlink text (string)
            // * type - content mime type (string)
            // When the URL is somehow supported, we may need those
        }

        tmpelem = trkmap.value("parent");
        if(!tmpelem.isNull())
        {
            track->setParentWpt(tmpelem.text());
        }

        // For now we are ignoring the following elements:
        // * cmt - GPS comment (string)
        // * src - data source (string)
        // * number - track number (integer)
        // * type - track classification (string)
        // I haven't seen any software using those, but who knows

        tmpelem = trkmap.value("extensions");
        if(!tmpelem.isNull())
        {
            QMap<QString,QDomElement> extensionsmap = CGpx::mapChildElements(tmpelem);

            // Garmin extensions v3

            tmpelem = extensionsmap.value(CGpx::gpxx_ns + ":" + "TrackExtension");
            if(!tmpelem.isNull())
            {
                QMap<QString,QDomElement> trackextensionmap = CGpx::mapChildElements(tmpelem);

                tmpelem = trackextensionmap.value(CGpx::gpxx_ns + ":" + "DisplayColor");
                if (!tmpelem.isNull())
                {
                    int colorID = gpx.getTrackColorMap().value(tmpelem.text(), -1);
                    if (colorID >= 0) track->setColor(colorID);
                }
            }

            tmpelem = extensionsmap.value(CGpx::ql_ns + ":" + "key");
            if(!tmpelem.isNull())
            {
                track->setKey(tmpelem.text());
            }

        }

        // QLandkarteGT backward compatibility

        if (gpx.version() == CGpx::qlVer_1_0)
        {
            tmpelem = trkmap.value("extension");
            if(!tmpelem.isNull())
            {
                QMap<QString,QDomElement> extensionsmap = CGpx::mapChildElements(tmpelem);

                tmpelem = extensionsmap.value("color");
                if(!tmpelem.isNull()) track->setColor(tmpelem.text().toUInt());
            }
        }

        /*
         *  Trackpoint information
         */

        bool foundTraineeData = false;

        QDomElement trkseg = trk.firstChildElement("trkseg");
        while(!trkseg.isNull())
        {
            QDomElement trkpt = trkseg.firstChildElement("trkpt");
            while (!trkpt.isNull())
            {
                CTrack::pt_t pt;

                QMap<QString,QDomElement> trkptmap = CGpx::mapChildElements(trkpt);

                // GPX 1.1

                pt.lon = trkpt.attribute("lon").toDouble();
                pt.lat = trkpt.attribute("lat").toDouble();
                pt._lon = pt.lon;
                pt._lat = pt.lat;

                tmpelem = trkptmap.value("ele");
                if(!tmpelem.isNull()) pt.ele = tmpelem.text().toDouble();
                pt._ele = pt.ele;

                tmpelem = trkptmap.value("time");
                if(!tmpelem.isNull())
                {
                    QString timetext = tmpelem.text();
                    (void)parseTimestamp(timetext, pt.timestamp, pt.timestamp_msec);

                    pt._timestamp = pt.timestamp;
                    pt._timestamp_msec = pt.timestamp_msec;
                }

                tmpelem = trkptmap.value("fix");
                if(!tmpelem.isNull()) pt.fix = tmpelem.text();

                tmpelem = trkptmap.value("sat");
                if(!tmpelem.isNull()) pt.sat = tmpelem.text().toUInt();

                tmpelem = trkptmap.value("hdop");
                if(!tmpelem.isNull()) pt.hdop = tmpelem.text().toDouble();

                tmpelem = trkptmap.value("vdop");
                if(!tmpelem.isNull()) pt.vdop = tmpelem.text().toDouble();

                tmpelem = trkptmap.value("pdop");
                if(!tmpelem.isNull()) pt.pdop = tmpelem.text().toDouble();

                // For now we are ignoring the following elements:
                // * magvar - magnetic variation (degrees)
                // * geoidheight - height of geoid above WGS84 (meters)
                // * name - waypoint name (string)
                // * cmt - GPS waypoint comment (string)
                // * desc - desctiption (string)
                // * src - data source (string)
                // * link - link to additional information (link)
                // * sym - symbol name (string)
                // * type - waypoint type (string)
                // * ageofdgpsdata - seconds since last DGPS update (decimal)
                // * dgpsid - DGPS station ID (ID)

                // GPX 1.0 backward compatibility, to be kept

                tmpelem = trkptmap.value("course");
                if(!tmpelem.isNull()) pt.heading = tmpelem.text().toDouble();

                tmpelem = trkptmap.value("speed");
                if(!tmpelem.isNull()) pt.velocity = tmpelem.text().toDouble();

                tmpelem = trkptmap.value("extensions");
                if(!tmpelem.isNull())
                {
#ifdef GPX_EXTENSIONS
                    pt.gpx_exts.setValues(tmpelem);
                    track->tr_ext.addKey2List(tmpelem);
#endif

                    QMap<QString,QDomElement> extensionsmap = CGpx::mapChildElements(tmpelem);

                    // Garmin extensions v3

                    // For now we are ignoring the following elements:
                    // * Depth - depth (meters)
                    // * Temperature - temperature (Celsius)

                    // Garmin Trackpoint Extension v1

                    // For now we are ignoring the following elements:
                    // * atemp - ambient temperature (Celsius)
                    // * wtemp - water temperature (Celsius)
                    // * depth - depth (meters)

                    tmpelem = extensionsmap.value(CGpx::gpxtpx_ns + ":" + "TrackPointExtension");
                    if(!tmpelem.isNull())
                    {
                        QMap<QString,QDomElement> trackpointextensionmap = CGpx::mapChildElements(tmpelem);

                        tmpelem = trackpointextensionmap.value(CGpx::gpxtpx_ns + ":" + "hr");
                        if(!tmpelem.isNull())
                        {
                            pt.heartReateBpm = tmpelem.text().toUInt();
                            foundTraineeData = true;
                        }

                        tmpelem = trackpointextensionmap.value(CGpx::gpxtpx_ns + ":" + "cad");
                        if(!tmpelem.isNull())
                        {
                            pt.cadenceRpm = tmpelem.text().toUInt();
                            foundTraineeData = true;
                        }
                    }

                    // TrekBuddy extensions

                    tmpelem = extensionsmap.value(CGpx::rmc_ns + ":" + "course");
                    if(!tmpelem.isNull()) pt.heading = tmpelem.text().toDouble();

                    tmpelem = extensionsmap.value(CGpx::rmc_ns + ":" + "speed");
                    if(!tmpelem.isNull()) pt.velocity = tmpelem.text().toDouble();

                    // QLandkarteGT extensions

                    if (gpx.version() >= CGpx::qlVer_1_1)
                    {
                        tmpelem = extensionsmap.value(CGpx::ql_ns + ":" + "flags");
                        if(!tmpelem.isNull())
                        {
                            pt.flags = tmpelem.text().toUInt();
                            pt.flags &= ~CTrack::pt_t::eFocus;
                            pt.flags &= ~CTrack::pt_t::eSelected;
                            pt.flags &= ~CTrack::pt_t::eCursor;
                        }
                    }
                }

                // QLandkarteGT backward compatibility

                if (gpx.version() == CGpx::qlVer_1_0)
                {
                    tmpelem = trkptmap.value("extension");
                    if(!tmpelem.isNull())
                    {
                        QMap<QString,QDomElement> extensionmap = CGpx::mapChildElements(tmpelem);

                        tmpelem = extensionmap.value("flags");
                        if(!tmpelem.isNull())
                        {
                            pt.flags = tmpelem.text().toUInt();
                            pt.flags &= ~CTrack::pt_t::eFocus;
                            pt.flags &= ~CTrack::pt_t::eSelected;
                            pt.flags &= ~CTrack::pt_t::eCursor;
                        }
                    }
                }

                *track << pt;
                trkpt = trkpt.nextSiblingElement("trkpt");
            }

            trkseg = trkseg.nextSiblingElement("trkseg");
        }

        if (foundTraineeData) track->setTraineeData();

        if(track->getTrackPoints().count() > 0) addTrack(track, true);
        else delete track;

        trk = trk.nextSiblingElement("trk");
    }

    CTrack::resetKeyCnt();
    if(hasItems)
    {
        emitSigChanged();
    }
}


void CTrackDB::saveGPX(CGpx& gpx, const QStringList& keys)
{
    QString str;
    QDomElement root = gpx.documentElement();

    quint32 dummyTimestamp = 0;

    QList<keys_t> _keys = this->keys();
    QList<keys_t>::const_iterator _key = _keys.begin();

    while(_key != _keys.end())
    {
        CTrack * track = tracks[_key->key];

        if(!keys.isEmpty() && !keys.contains(track->getKey()))
        {
            ++_key;
            continue;
        }

        QDomElement trk = gpx.createElement("trk");
        root.appendChild(trk);

        QDomElement name = gpx.createElement("name");
        trk.appendChild(name);
        QDomText _name_ = gpx.createTextNode(track->getName());
        name.appendChild(_name_);

        if(!track->getParentWpt().isEmpty())
        {
            QDomElement parent = gpx.createElement("parent");
            parent.setAttribute("xmlns", "http://opencachemanage.sourceforge.net/schema1");
            trk.appendChild(parent);
            QDomText _parent_ = gpx.createTextNode(track->getParentWpt());
            parent.appendChild(_parent_);
        }

        QDomElement extensions = gpx.createElement("extensions");
        trk.appendChild(extensions);

        QDomElement gpxx_ext = gpx.createElement("gpxx:TrackExtension");
        extensions.appendChild(gpxx_ext);

        QDomElement color = gpx.createElement("gpxx:DisplayColor");
        gpxx_ext.appendChild(color);

        QString colname = gpx.getTrackColorMap().key(track->getColorIdx());
        QDomText _color_ = gpx.createTextNode(colname);
        color.appendChild(_color_);

        QDomElement qlkey = gpx.createElement("ql:key");
        extensions.appendChild(qlkey);
        QDomText _qlkey_ = gpx.createTextNode(track->getKey());
        qlkey.appendChild(_qlkey_);

        QDomElement trkseg = gpx.createElement("trkseg");
        trk.appendChild(trkseg);

        QList<CTrack::pt_t>& pts = track->getTrackPoints();
        QList<CTrack::pt_t>::const_iterator pt = pts.begin();
        while(pt != pts.end())
        {
            QDomElement trkpt = gpx.createElement("trkpt");
            if ((gpx.getExportMode() != CGpx::eQlgtExport) && (pt->flags.flag() & CTrack::pt_t::eDeleted))
            {
                // skip deleted points when exporting
                ++pt;
                continue;
            }
            trkseg.appendChild(trkpt);
            str.sprintf("%1.8f", pt->lat);
            trkpt.setAttribute("lat",str);
            str.sprintf("%1.8f", pt->lon);
            trkpt.setAttribute("lon",str);

            if(pt->ele != WPT_NOFLOAT)
            {
                QDomElement ele = gpx.createElement("ele");
                trkpt.appendChild(ele);
                QDomText _ele_ = gpx.createTextNode(QString::number(pt->ele,'f'));
                ele.appendChild(_ele_);
            }

            if(pt->timestamp != 0x000000000 && pt->timestamp != 0xFFFFFFFF)
            {
                dummyTimestamp = pt->timestamp;
            }
            else
            {
                dummyTimestamp++;
            }

            QDateTime t = QDateTime::fromTime_t(dummyTimestamp).toUTC();
            t = t.addMSecs(pt->timestamp_msec);
            QDomElement time = gpx.createElement("time");
            trkpt.appendChild(time);
            QDomText _time_ = gpx.createTextNode(t.toString("yyyy-MM-dd'T'hh:mm:ss.zzz'Z'"));
            time.appendChild(_time_);

            if(pt->hdop != WPT_NOFLOAT)
            {
                QDomElement hdop = gpx.createElement("hdop");
                trkpt.appendChild(hdop);
                QDomText _hdop_ = gpx.createTextNode(QString::number(pt->hdop));
                hdop.appendChild(_hdop_);
            }
            if(pt->vdop != WPT_NOFLOAT)
            {
                QDomElement vdop = gpx.createElement("vdop");
                trkpt.appendChild(vdop);
                QDomText _vdop_ = gpx.createTextNode(QString::number(pt->vdop));
                vdop.appendChild(_vdop_);
            }
            if(pt->pdop != WPT_NOFLOAT)
            {
                QDomElement pdop = gpx.createElement("pdop");
                trkpt.appendChild(pdop);
                QDomText _pdop_ = gpx.createTextNode(QString::number(pt->pdop));
                pdop.appendChild(_pdop_);
            }

            if(pt->fix != "")
            {
                QDomElement fix = gpx.createElement("fix");
                trkpt.appendChild(fix);
                QDomText _fix_ = gpx.createTextNode(pt->fix);
                fix.appendChild(_fix_);
            }

            if(pt->sat != 0)
            {
                QDomElement sat = gpx.createElement("sat");
                trkpt.appendChild(sat);
                QDomText _sat_ = gpx.createTextNode(QString::number(pt->sat));
                sat.appendChild(_sat_);
            }

            // gpx extensions
            if(gpx.getExportMode() == CGpx::eQlgtExport)
            {
                bool hasExtensions = false;
                QDomElement extensions = gpx.createElement("extensions");

                if(pt->flags.flag() != 0)
                {
                    QDomElement flags = gpx.createElement("ql:flags");
                    extensions.appendChild(flags);
                    QDomText _flags_ = gpx.createTextNode(QString::number(pt->flags.flag()));
                    flags.appendChild(_flags_);
                    hasExtensions = true;
                }

                if(pt->heading != WPT_NOFLOAT)
                {
                    QDomElement heading = gpx.createElement("rmc:course");
                    extensions.appendChild(heading);
                    QDomText _heading_ = gpx.createTextNode(QString::number(pt->heading));
                    heading.appendChild(_heading_);
                    hasExtensions = true;
                }

                if(pt->velocity != WPT_NOFLOAT)
                {
                    QDomElement velocity = gpx.createElement("rmc:speed");
                    extensions.appendChild(velocity);
                    QDomText _velocity_ = gpx.createTextNode(QString::number(pt->velocity));
                    velocity.appendChild(_velocity_);
                    hasExtensions = true;
                }

                if(hasExtensions)
                {
                    trkpt.appendChild(extensions);
                }
            }

            ++pt;
        }

        ++_key;
    }

}


void CTrackDB::addTrack(CTrack* track, bool silent)
{
    if(track->getName().isEmpty())
    {
        track->setName(tr("Track%1").arg(cnt++));
    }
    track->rebuild(false);
    delTrack(track->getKey(), silent);
    tracks[track->getKey()] = track;

    connect(track,SIGNAL(sigChanged()),this, SLOT(slotModified()));
    connect(track,SIGNAL(sigNeedUpdate()),this, SLOT(slotNeedUpdate()));
    if(!silent)
    {
        emitSigChanged();
    }
}


void CTrackDB::delTrack(const QString& key, bool silent)
{
    if(!tracks.contains(key)) return;

    CTrack * track = take(key, silent);
    track->deleteLater();

}


void CTrackDB::delTracks(const QStringList& keys)
{
    undoStack->beginMacro("delTracks");
    foreach(QString key,keys)
    {
        delTrack(key,true);
    }
    undoStack->endMacro();
    if(!keys.isEmpty())
    {
        emitSigChanged();
    }
}


void CTrackDB::highlightTrack(const QString& key)
{

    if(highlightedTrack() && (key == highlightedTrack()->getKey()))
    {
        return;
    }

    QMap<QString,CTrack*>::iterator track = tracks.begin();
    while(track != tracks.end())
    {
        (*track)->setHighlight(false);
        ++track;
    }

    if(tracks.contains(key))
    {
        CTrack * track = tracks[key];
        track->setHighlight(true);
        emit sigHighlightTrack(track);
    }
    else
    {
        emit sigHighlightTrack(0);
    }
}


void CTrackDB::hideTrack(const QStringList& keys, bool hide)
{
    QString key;
    foreach(key, keys)
    {
        if(tracks.contains(key))
        {
            CTrack * track = tracks[key];
            track->hide(hide);
            if(track->isHighlighted() && hide)
            {
                tracks[key]->setHighlight(false);
                emit sigHighlightTrack(tracks[key]);
            }
        }
    }
    emitSigChanged();
}


CTrack* CTrackDB::highlightedTrack()
{

    QMap<QString,CTrack*>::iterator track = tracks.begin();
    while(track != tracks.end())
    {
        if((*track)->isHighlighted()) return *track;
        ++track;
    }
    return 0;

}


void CTrackDB::upload(const QStringList& keys)
{
    if(tracks.isEmpty()) return;

    IDevice * dev = CResources::self().device();
    if(dev)
    {
        QList<CTrack*> tmptrks;

        if(keys.isEmpty())
        {
            tmptrks = tracks.values();
        }
        else
        {
            QString key;
            foreach(key, keys)
            {
                tmptrks << tracks[key];
            }
        }
        dev->uploadTracks(tmptrks);
    }
}


void CTrackDB::download()
{
    IDevice * dev = CResources::self().device();
    if(dev)
    {
        QList<CTrack*> tmptrk;
        dev->downloadTracks(tmptrk);

        if(tmptrk.isEmpty()) return;

        CTrack * trk;
        foreach(trk,tmptrk)
        {
            addTrack(trk, true);
        }
    }

    emitSigChanged();
}


void CTrackDB::CombineTracks()
{
    CDlgCombineTracks dlg(0);
    dlg.exec();
}


void CTrackDB::splitTrack(int idx)
{
    CTrack * theTrack = highlightedTrack();
    if(theTrack == 0) return;

    QList<CTrack::pt_t>& track = theTrack->getTrackPoints();
    if(track.size() < idx - 1) return;

    QList<CTrack::pt_t>::iterator trkpt, splitpt = track.begin() + idx;

    CTrack * track1 = new CTrack(this);
    track1->setName(theTrack->getName() + "_1");
    track1->setColor(theTrack->getColorIdx());

    for (trkpt = track.begin(); trkpt != splitpt + 1; ++trkpt)
    {
        *track1 << *trkpt;
    }

    CTrack * track2 = new CTrack(this);
    track2->setName(theTrack->getName() + "_2");
    track2->setColor(theTrack->getColorIdx());
    for (trkpt = splitpt; trkpt != track.end(); ++trkpt)
    {
        *track2 << *trkpt;
    }

    addTrack(track1, true);
    addTrack(track2, true);
    delTrack(theTrack->getKey(), true);

    emitSigChanged();
}


void CTrackDB::splitLineToViewport(const QPolygon& line, const QRect& extViewport, QList<QPolygon>& lines)
{
    QPolygon subline;

    int i;
    QPoint pt, ptt, pt1;
    const int size = line.size();

    pt = line[0];
    subline << pt;

    for(i = 1; i < size; i++)
    {
        pt1 = line[i];

        if(!GPS_Math_LineCrossesRect(pt, pt1, extViewport))
        {
            pt = pt1;
            if(subline.size() > 1)
            {
                lines << subline;
            }
            subline.clear();
            subline << pt;
            continue;
        }

        ptt = pt1 - pt;
        if(ptt.manhattanLength() < 15)
        {
            continue;
        }

        subline << pt1;
        pt = pt1;
    }

    if(subline.size() > 1)
    {
        lines << subline;
    }

}


void CTrackDB::drawLine(const QPolygon& line, const QRect& extViewport, QPainter& p)
{
    int i;
    QList<QPolygon> lines;

    splitLineToViewport(line, extViewport, lines);

    for(i = 0; i < lines.count(); i++)
    {
        p.drawPolyline(lines[i]);
    }
}


void CTrackDB::drawLine(const QPolygon& line, const QVector<QColor> colors, const QRect& extViewport, QPainter& p)
{
    QPen pen = p.pen();
    for(int i = 1; i < line.size(); i++)
    {
        QLinearGradient g(line[i-1], line[i]);
        g.setColorAt(0, colors[i-1]);
        g.setColorAt(1, colors[i]);
        pen.setBrush(QBrush(g));
        p.setPen(pen);
        p.drawLine(line[i-1], line[i]);
    }
}


static void drawMarker(QPainter& p, const QString& text, CTrack::pt_t& pt)
{
    IMap& map = CMapDB::self().getMap();

    double u = pt.lon * DEG_TO_RAD;
    double v = pt.lat * DEG_TO_RAD;

    map.convertRad2Pt(u,v);

    QPen pen1(Qt::white,5);
    pen1.setCapStyle(Qt::RoundCap);
    pen1.setJoinStyle(Qt::RoundJoin);

    QPen pen2(Qt::black,2);
    pen2.setCapStyle(Qt::RoundCap);
    pen2.setJoinStyle(Qt::RoundJoin);

    p.setPen(pen1);
    p.drawLine(u,v, u + 10, v - 10);
    p.drawLine(u + 10, v - 10, u + 80, v - 10);
    p.setPen(pen2);
    p.drawLine(u,v, u + 10, v - 10);
    p.drawLine(u + 10, v - 10, u + 80, v - 10);
    CCanvas::drawText(text, p, QPoint(u + 45, v - 10),Qt::black );

}


void CTrackDB::drawArrows(const QPolygon& line, const QRect& viewport, QPainter& p)
{
    QPointF arrow[4] =
    {
        QPointF( 20.0, 7.0),     //front
        QPointF( 0.0, 0.0),      //upper tail
        QPointF( 5.0, 7.0),      //mid tail
        QPointF( 0.0, 15.0)      //lower tail
    };

    QPoint  pt, pt1, ptt;

    // draw direction arrows
    bool    start = true;
    double  heading;

    //generate arrow pic
    QImage arrow_pic(21,16, QImage::Format_ARGB32);
    arrow_pic.fill( qRgba(0,0,0,0));
    QPainter t_paint(&arrow_pic);
    USE_ANTI_ALIASING(t_paint, true);
    t_paint.setPen(QPen(Qt::white, 2));
    t_paint.setBrush(p.brush());
    t_paint.drawPolygon(arrow, 4);
    t_paint.end();

    foreach(pt,line)
    {
        if(start)                // no arrow on  the first loop
        {
            start = false;
        }
        else
        {
            if(!viewport.contains(pt))
            {
                pt1 = pt;
                continue;
            }
            if((abs(pt.x() - pt1.x()) + abs(pt.y() - pt1.y())) < 7)
            {
                pt1 = pt;
                continue;
            }
            // keep distance
            if((abs(pt.x() - ptt.x()) + abs(pt.y() - ptt.y())) > 100)
            {
                if(0 != pt.x() - pt1.x() && (pt.y() - pt1.y()))
                {
                    heading = ( atan2((double)(pt.y() - pt1.y()), (double)(pt.x() - pt1.x())) * 180.) / M_PI;

                    p.save();
                    // draw arrow between bullets
                    p.translate((pt.x() + pt1.x())/2,(pt.y() + pt1.y())/2);
                    p.rotate(heading);
                    p.drawImage(-11, -7, arrow_pic);
                    p.restore();
                    //remember last point
                    ptt = pt;
                }
            }
        }
        pt1 = pt;
    }

}


void CTrackDB::draw(QPainter& p, const QRect& rect, bool& needsRedraw)
{

    QPoint focus(-1,-1);
    QVector<QPoint> selected;
    IMap& map = CMapDB::self().getMap();

    //     QMap<QString,CTrack*> tracks                = CTrackDB::self().getTracks();
    QMap<QString,CTrack*>::iterator track       = tracks.begin();
    QMap<QString,CTrack*>::iterator highlighted = tracks.end();

    QPixmap bullet_red(":/icons/bullet_red.png");

    // extended vieport rectangle to cut line segments properly
    QRect extRect = rect.adjusted(-10, -10, 10, 10);

    while(track != tracks.end())
    {

        if((*track)->m_hide)
        {

            ++track;
            continue;
        }

        QPolygon& line          = (*track)->getPolyline();
        QVector<QColor>& color  = (*track)->getPolylineColor();
        line.clear();
        color.clear();

        bool firstTime = (*track)->firstTime;

        QList<CTrack::pt_t>& trkpts = (*track)->getTrackPoints();
        QList<CTrack::pt_t>::iterator trkpt = trkpts.begin();
        while(trkpt != trkpts.end())
        {

            if ( needsRedraw || firstTime)
            {
                double u = trkpt->lon * DEG_TO_RAD;
                double v = trkpt->lat * DEG_TO_RAD;

                map.convertRad2Pt(u,v);
                trkpt->px = QPoint(u,v);

            }

            if((*track)->isHighlighted() && trkpt->flags & CTrack::pt_t::eSelected)
            {
                selected << trkpt->px;
            }

            if((*track)->isHighlighted() && trkpt->flags & CTrack::pt_t::eFocus)
            {
                focus = trkpt->px;
            }

            // skip deleted points, however if they are selected the
            // selection mark is shown
            if(trkpt->flags & CTrack::pt_t::eDeleted)
            {
                ++trkpt; continue;
            }

            line    << trkpt->px;
            color   << trkpt->color;
            ++trkpt;
        }

        if(!rect.intersects(line.boundingRect()))
        {
            ++track; continue;
        }

        if((*track)->isHighlighted())
        {
            // store highlighted track to draw it later
            // it must be drawn above all other tracks
            highlighted = track;
        }
        else
        {
            // draw normal track
            QPen pen1(Qt::white,5);
            pen1.setCapStyle(Qt::RoundCap);
            pen1.setJoinStyle(Qt::RoundJoin);

            QPen pen2((*track)->getColor(),3);
            pen2.setCapStyle(Qt::RoundCap);
            pen2.setJoinStyle(Qt::RoundJoin);

            p.setPen(pen1);
            drawLine(line, extRect, p);
            p.setPen(pen2);
            drawLine(line, extRect, p);

            p.setBrush((*track)->getColor());
            drawArrows(line, rect, p);
        }

        (*track)->firstTime = false;
        ++track;
    }

    // if there is a highlighted track, draw it
    if(highlighted != tracks.end())
    {
        QPoint pt1, pt;

        track = highlighted;

        QPixmap bullet = (*track)->getBullet();

        QPolygon& line = (*track)->getPolyline();

        // draw skunk line
        QPen pen1(QColor(255,255,255,128),15);
        pen1.setCapStyle(Qt::RoundCap);
        pen1.setJoinStyle(Qt::RoundJoin);

        QColor color = (*track)->getColor();
        color.setAlpha(128);
        QPen pen2(color,11);
        pen2.setCapStyle(Qt::RoundCap);
        pen2.setJoinStyle(Qt::RoundJoin);

        p.setPen(pen1);
        drawLine(line, extRect, p);
        p.setPen(pen2);
        if((*track)->isMultiColor())
        {
            drawLine(line, (*track)->getPolylineColor(), extRect, p);
        }
        else
        {
            drawLine(line, extRect, p);
        }

        if(showBullets)
        {
            foreach(pt,line)
            {
                if(!rect.contains(pt)) continue;
                if((abs(pt.x() - pt1.x()) + abs(pt.y() - pt1.y())) < 7) continue;
                p.drawPixmap(pt.x() - 3 ,pt.y() - 3, bullet);
                pt1 = pt;
            }
        }

        p.setBrush((*track)->getColor());
        drawArrows(line, rect, p);

        pt1 = QPoint();
        foreach(pt,selected)
        {
            if((abs(pt.x() - pt1.x()) + abs(pt.y() - pt1.y())) < 7) continue;
            p.drawPixmap(pt.x() - 5 ,pt.y() - 5, bullet_red);
            pt1 = pt;
        }

        if(focus != QPoint(-1,-1))
        {
            p.setPen(QPen(Qt::white,3));
            p.drawLine(focus + QPoint(0,-20),focus + QPoint(0,20));
            p.drawLine(focus + QPoint(-20,0),focus + QPoint(20,0));
            p.setPen(Qt::red);
            p.drawLine(focus + QPoint(0,-20),focus + QPoint(0,20));
            p.drawLine(focus + QPoint(-20,0),focus + QPoint(20,0));

            QString str = (*track)->getFocusInfo();
            //-----------------------------------------------------------------------------------------------------------
            if (str != "")
            {
                QFont           f = CResources::self().getMapFont();
                QFontMetrics    fm(f);
                QRect           r1 = fm.boundingRect(QRect(0,0,300,0), Qt::AlignLeft|Qt::AlignTop, str);

                r1.moveTopLeft(focus + QPoint(15,15));

                QRect           r2 = r1;
                r2.setWidth(r1.width() + 20);
                r2.moveLeft(r1.left() - 10);
                r2.setHeight(r1.height() + 10);
                r2.moveTop(r1.top() - 5);

                p.setPen(QPen(CCanvas::penBorderBlue));
                p.setBrush(CCanvas::brushBackWhite);
                PAINT_ROUNDED_RECT(p,r2);

                p.setFont(CResources::self().getMapFont());
                p.setPen(Qt::darkBlue);
                p.drawText(r1, Qt::AlignLeft|Qt::AlignTop,str);
            }

        }

        QString val, unit;

        if(showMinMax)
        {
            if((*track)->ptMaxEle.ele != WPT_NOFLOAT)
            {
                IUnit::self().meter2elevation((*track)->ptMaxEle.ele, val, unit);
                drawMarker(p, tr("Hmax=%1%2").arg(val).arg(unit), (*track)->ptMaxEle);
            }
            if((*track)->ptMinEle.ele != WPT_NOFLOAT)
            {
                IUnit::self().meter2elevation((*track)->ptMinEle.ele, val, unit);
                drawMarker(p, tr("Hmin=%1%2").arg(val).arg(unit), (*track)->ptMinEle);
            }
            if((*track)->ptMaxSpeed.speed != WPT_NOFLOAT)
            {
                IUnit::self().meter2speed((*track)->ptMaxSpeed.speed, val, unit);
                drawMarker(p, tr("Vmax=%1%2").arg(val).arg(unit), (*track)->ptMaxSpeed);
            }
        }

    }
}


void CTrackDB::select(const QRect& rect, bool select /*= true*/)
{
    CTrack * track = highlightedTrack();
    if(track == 0) return;

    undoStack->push(new CTrackUndoCommandSelect(track, rect, select));
}


void CTrackDB::copyToClipboard(bool deleteSelection /* = false */)
{
    CTrack * track = highlightedTrack();
    if(track == 0)
    {
        QMessageBox::warning(0,tr("Failed..."), tr("Failed to copy track. You must select a track or track points of a track."), QMessageBox::Abort, QMessageBox::Abort);
        return;
    }
    QClipboard *clipboard = QApplication::clipboard();

    CTrack *tmpTrack = new CTrack(0);

    QList<CTrack::pt_t>& trkpts = track->getTrackPoints();
    QList<CTrack::pt_t>::iterator trkpt = trkpts.begin();

    while(trkpt != trkpts.end())
    {
        if (trkpt->flags & CTrack::pt_t::eSelected)
        {
            *tmpTrack << *trkpt;
        }

        ++trkpt;
    }

    CQlb qlb(this);
    if(tmpTrack->getTrackPoints().count())
    {
        qlb << *tmpTrack;
    }
    else
    {
        qlb << *track;
    }
    QBuffer buffer;
    qlb.save(&buffer);
    QMimeData *md = new QMimeData;
    buffer.open(QIODevice::ReadOnly);
    md->setData("qlandkartegt/qlb",buffer.readAll());
    buffer.close();
    clipboard->clear();
    clipboard->setMimeData(md);

    delete tmpTrack;
}


void CTrackDB::pasteFromClipboard()
{
    QClipboard *clipboard = QApplication::clipboard();

    if (clipboard->mimeData()->hasFormat("qlandkartegt/qlb"))
    {
        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);
        buffer.write(clipboard->mimeData()->data("qlandkartegt/qlb"));
        buffer.close();
        CQlb qlb(this);
        qlb.load(&buffer);
        loadQLB(qlb, true);

        emitSigChanged();
    }
}


CTrack *CTrackDB::take(const QString& key, bool silent)
{
    CTrack *track =  tracks.take(key);

    if (!silent)
    {
        emitSigChanged();
    }
    return track;
}


void CTrackDB::insert(const QString& key, CTrack *track, bool silent)
{
    tracks.insert(key,track);
    if (!silent)
    {
        emitSigChanged();
    }
}


void CTrackDB::revertTrack(const QString& key)
{
    if(!tracks.contains(key))
    {
        return;
    }
    CTrack *torg =  tracks[key];
    CTrack *tnew = new CTrack(this);

    tnew->name = torg->name + tr("_rev");
    tnew->setColor(torg->getColorIdx());

    QList<CTrack::pt_t> track = torg->track;
    while(track.size())
    {
        CTrack::pt_t pt = track.takeLast();

        pt.timestamp        = 0;
        pt.timestamp_msec   = 0;

        *tnew << pt;
    }

    addTrack(tnew, false);

}


QList<CTrackDB::keys_t> CTrackDB::keys()
{
    QList<keys_t> k;

    QString k1;
    QStringList ks = tracks.keys();

    foreach(k1, ks)
    {
        QPixmap icon(16,16);
        keys_t k2;
        CTrack * track = tracks[k1];

        k2.key      = k1;
        k2.name     = track->name;
        k2.comment  = track->comment.left(32);
        if(track->track.isEmpty())
        {
            k2.time     = track->timestamp;
        }
        else
        {
            k2.time     = track->track.first().timestamp;
        }

        icon.fill(track->getColor());
        k2.icon     = icon;

        k << k2;
    }

    CTrackToolWidget::sortmode_e sortmode = CTrackToolWidget::getSortMode();

    switch(sortmode)
    {
        case CTrackToolWidget::eSortByName:
            qSort(k.begin(), k.end(), CTrackDB::keyLessThanAlpha);
            break;
        case CTrackToolWidget::eSortByTime:
            qSort(k.begin(), k.end(), CTrackDB::keyLessThanTime);
            break;
    }

    return k;
}


CTrack * CTrackDB::getTrackByKey(const QString& key)
{
    if(!tracks.contains(key)) return 0;

    return tracks[key];

}


void CTrackDB::makeVisible(const QStringList& keys)
{
    QRectF r;
    QString key;
    foreach(key, keys)
    {

        CTrack * trk =  tracks[key];

        if(r.isNull())
        {
            r = trk->getBoundingRectF();
        }
        else
        {
            r |= trk->getBoundingRectF();
        }

    }

    if (!r.isNull ())
    {
        CMapDB::self().getMap().zoom(r.left() * DEG_TO_RAD, r.top() * DEG_TO_RAD, r.right() * DEG_TO_RAD, r.bottom() * DEG_TO_RAD);
    }

    if(keys.size() == 1)
    {
        highlightTrack(keys[0]);
    }
}


void CTrackDB::setPointOfFocusByDist(double distance)
{
    CTrack * track = highlightedTrack();
    if(track == 0)
    {
        return;
    }

    QList<CTrack::pt_t>& trkpts = track->getTrackPoints();
    QList<CTrack::pt_t>::iterator trkpt = trkpts.begin();

    int idx  = -1;
    double d = WPT_NOFLOAT;
    while(trkpt != trkpts.end())
    {
        if(trkpt->flags & CTrack::pt_t::eDeleted)
        {
            ++trkpt; continue;
        }

        if(fabs(distance - trkpt->distance) < d)
        {
            d   = fabs(distance - trkpt->distance);
            idx = trkpt->idx;
        }
        ++trkpt;
    }
    track->setPointOfFocus(idx, CTrack::eHoover, false);
    emit sigPointOfFocus(idx);
}


void CTrackDB::setPointOfFocusByTime(quint32 timestamp)
{
    CTrack * track = highlightedTrack();
    if(track == 0)
    {
        return;
    }

    QList<CTrack::pt_t>& trkpts = track->getTrackPoints();
    QList<CTrack::pt_t>::iterator trkpt = trkpts.begin();

    int idx = -1;
    int d   = 0x7FFFFFFF;
    while(trkpt != trkpts.end())
    {
        if(trkpt->flags & CTrack::pt_t::eDeleted)
        {
            ++trkpt; continue;
        }

        qint32 timestamp_diff = (timestamp > trkpt->timestamp)?(timestamp - trkpt->timestamp):(trkpt->timestamp - timestamp);
        if(timestamp_diff < d)
        {
            d   = timestamp_diff;
            idx = trkpt->idx;
        }
        ++trkpt;
    }
    track->setPointOfFocus(idx, CTrack::eHoover, false);
    emit sigPointOfFocus(idx);
}


void CTrackDB::setPointOfFocusByIdx(qint32 idx)
{
    CTrack * track = highlightedTrack();
    if(track == 0)
    {
        return;
    }

    QList<CTrack::pt_t>& trkpts = track->getTrackPoints();
    if(idx < trkpts.size())
    {
        track->setPointOfFocus(idx, CTrack::eHoover, false);
        emit sigPointOfFocus(idx);
    }
    else
    {
        track->setPointOfFocus(-1, CTrack::eHoover, false);
        emit sigPointOfFocus(-1);
    }
}


bool CTrackDB::getClosestPoint2Position(double &lon, double &lat, quint32& timestamp, double maxDelta)
{
    double delta = WPT_NOFLOAT;
    const CTrack::pt_t * selTrkPt = 0;
    double d, a1, a2;
    projUV p1, p2;

    p1.u = lon*DEG_TO_RAD;
    p1.v = lat*DEG_TO_RAD;

    foreach(CTrack * track, tracks)
    {
        QList<CTrack::pt_t>& trkpts = track->getTrackPoints();

        foreach(const CTrack::pt_t& trkpt, trkpts)
        {
            p2.u = trkpt.lon*DEG_TO_RAD;
            p2.v = trkpt.lat*DEG_TO_RAD;
            d = distance(p1, p2, a1, a2);

            if(d < delta)
            {
                delta = d;
                if(delta < maxDelta)
                {
                    selTrkPt = &trkpt;
                }
            }
        }
    }

    if(selTrkPt)
    {
        lon         = selTrkPt->lon;
        lat         = selTrkPt->lat;
        timestamp   = selTrkPt->timestamp;
        return true;
    }

    return false;
}


bool CTrackDB::getClosestPoint2Timestamp(quint32 timestamp, quint32 maxDelta, double& lon, double& lat)
{
    quint32 delta = std::numeric_limits<quint32>::max();
    const CTrack::pt_t * selTrkPt = 0;

    foreach(CTrack * track, tracks)
    {
        QList<CTrack::pt_t>& trkpts = track->getTrackPoints();

        foreach(const CTrack::pt_t& trkpt, trkpts)
        {
            quint32 delta_trkpt = (timestamp > trkpt.timestamp)?(timestamp - trkpt.timestamp):(trkpt.timestamp - timestamp);
            if(delta_trkpt < delta)
            {
                delta = delta_trkpt;
                if(delta < maxDelta)
                {
                    selTrkPt = &trkpt;
                }
            }
        }
    }

    if(selTrkPt)
    {
        lon = selTrkPt->lon;
        lat = selTrkPt->lat;
        return true;
    }

    return false;
}


void CTrackDB::slotMapChanged()
{
    foreach(CTrack * track, tracks)
    {
        track->blockSignals(true);
        track->rebuild(false);
        track->blockSignals(false);
    }
}


void CTrackDB::slotModified()
{
    CTrack * trk = qobject_cast<CTrack*>(sender());
    if(trk)
    {
        emitSigModified(trk->getKey());
    }
}


void CTrackDB::slotNeedUpdate()
{
    CTrack * trk = qobject_cast<CTrack*>(sender());
    if(trk)
    {
        emitSigNeedUpdate(trk->getKey());
    }
}
