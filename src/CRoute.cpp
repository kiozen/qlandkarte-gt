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

#include "CRoute.h"
#include "WptIcons.h"
#include "GeoMath.h"
#include "CRouteDB.h"
#include "IUnit.h"

#include <QtGui>
#include <QtXml>

struct rte_head_entry_t
{
    rte_head_entry_t() : type(CRoute::eEnd), offset(0) {}
    qint32      type;
    quint32     offset;
    QByteArray  data;
};

QDataStream& operator >>(QDataStream& s, CRoute& route)
{
    quint32 nRtePts = 0;
    QIODevice * dev = s.device();
    qint64      pos = dev->pos();

    char magic[9];
    s.readRawData(magic,9);

    if(strncmp(magic,"QLRte   ",9))
    {
        dev->seek(pos);
        return s;
    }

    QList<rte_head_entry_t> entries;

    while(1)
    {
        rte_head_entry_t entry;
        s >> entry.type >> entry.offset;
        entries << entry;
        if(entry.type == CRoute::eEnd) break;
    }

    QList<rte_head_entry_t>::iterator entry = entries.begin();
    while(entry != entries.end())
    {
        qint64 o = pos + entry->offset;
        dev->seek(o);
        s >> entry->data;

        switch(entry->type)
        {
            case CRoute::eBase:
            {
                QString key;
                QString icon;

                QDataStream s1(&entry->data, QIODevice::ReadOnly);
                s1.setVersion(QDataStream::Qt_4_5);

                s1 >> key;
                s1 >> route.timestamp;
                s1 >> route.name;
                s1 >> icon;
                s1 >> route.ttime;
                s1 >> route.parentWpt;

                route.setIcon(icon);
                route.setKey(key);

                break;
            }

            case CRoute::eRtePts:
            {
                QDataStream s1(&entry->data, QIODevice::ReadOnly);
                s1.setVersion(QDataStream::Qt_4_5);
                quint32 n;

                route.priRoute.clear();
                s1 >> nRtePts;

                for(n = 0; n < nRtePts; ++n)
                {
                    CRoute::pt_t rtept;
                    float u, v;
                    QString action;

                    s1 >> u;
                    s1 >> v;
                    s1 >> action;

                    rtept.lon = u;
                    rtept.lat = v;
                    rtept.action = action;
                    route.priRoute << rtept;
                }
                break;
            }
            case CRoute::eRteSec:
            {
                QDataStream s1(&entry->data, QIODevice::ReadOnly);
                s1.setVersion(QDataStream::Qt_4_5);
                quint32 n;

                route.secRoute.clear();
                s1 >> nRtePts;

                for(n = 0; n < nRtePts; ++n)
                {
                    CRoute::pt_t rtept;
                    float u, v;
                    QString action;

                    s1 >> u;
                    s1 >> v;
                    s1 >> action;

                    rtept.lon = u;
                    rtept.lat = v;
                    rtept.action = action;
                    route.secRoute << rtept;
                }
                break;
            }
            default:;
        }

        ++entry;
    }

    return s;
}


QDataStream& operator <<(QDataStream& s, CRoute& route)
{
    QList<rte_head_entry_t> entries;

    //---------------------------------------
    // prepare base data
    //---------------------------------------
    rte_head_entry_t entryBase;
    entryBase.type = CRoute::eBase;
    QDataStream s1(&entryBase.data, QIODevice::WriteOnly);
    s1.setVersion(QDataStream::Qt_4_5);

    s1 << route.getKey();
    s1 << route.getTimestamp();
    s1 << route.getName();
    s1 << route.getIconString();
    s1 << route.getTime();
    s1 << route.getParentWpt();

    entries << entryBase;

    //---------------------------------------
    // prepare primary routepoint data
    //---------------------------------------
    rte_head_entry_t entryPriRtePts;
    entryPriRtePts.type = CRoute::eRtePts;
    QDataStream s2(&entryPriRtePts.data, QIODevice::WriteOnly);
    s2.setVersion(QDataStream::Qt_4_5);

    {
        QVector<CRoute::pt_t>& rtepts           = route.getPriRtePoints();
        QVector<CRoute::pt_t>::iterator rtept   = rtepts.begin();

        s2 << (quint32)rtepts.size();
        while(rtept != rtepts.end())
        {
            s2 << (float)rtept->lon;
            s2 << (float)rtept->lat;
            s2 << rtept->action;
            ++rtept;
        }
    }
    entries << entryPriRtePts;

    //---------------------------------------
    // prepare secondary routepoint data
    //---------------------------------------
    rte_head_entry_t entrySecRtePts;
    entrySecRtePts.type = CRoute::eRteSec;
    QDataStream s3(&entrySecRtePts.data, QIODevice::WriteOnly);
    s3.setVersion(QDataStream::Qt_4_5);

    {
        QVector<CRoute::pt_t>& rtepts           = route.getSecRtePoints();
        QVector<CRoute::pt_t>::iterator rtept   = rtepts.begin();

        if(!rtepts.isEmpty())
        {
            s3 << (quint32)rtepts.size();
            while(rtept != rtepts.end())
            {
                s3 << (float)rtept->lon;
                s3 << (float)rtept->lat;
                s3 << rtept->action;
                ++rtept;
            }
            entries << entrySecRtePts;
        }

    }

    //---------------------------------------
    // prepare terminator
    //---------------------------------------
    rte_head_entry_t entryEnd;
    entryEnd.type = CRoute::eEnd;
    entries << entryEnd;

    //---------------------------------------
    //---------------------------------------
    // now start to actually write data;
    //---------------------------------------
    //---------------------------------------
    // write magic key
    s.writeRawData("QLRte   ",9);

    // calculate offset table
    quint32 offset = entries.count() * 8 + 9;

    QList<rte_head_entry_t>::iterator entry = entries.begin();
    while(entry != entries.end())
    {
        entry->offset = offset;
        offset += entry->data.size() + sizeof(quint32);
        ++entry;
    }

    // write offset table
    entry = entries.begin();
    while(entry != entries.end())
    {
        s << entry->type << entry->offset;
        ++entry;
    }

    // write entry data
    entry = entries.begin();
    while(entry != entries.end())
    {
        s << entry->data;
        ++entry;
    }

    return s;
}


void operator >>(QFile& f, CRoute& route)
{
    f.open(QIODevice::ReadOnly);
    QDataStream s(&f);
    s.setVersion(QDataStream::Qt_4_5);
    s >> route;
    f.close();
}


void operator <<(QFile& f, CRoute& route)
{
    f.open(QIODevice::WriteOnly);
    QDataStream s(&f);
    s.setVersion(QDataStream::Qt_4_5);
    s << route;
    f.close();
}


CRoute::CRoute(QObject * parent)
: IItem(parent)
, dist(0.0)
, ttime(0)
, highlight(false)
, firstTime(true)
, calcRoutePending(false)
, routeIdx(0)
{
    setIcon("Small City");
}


CRoute::~CRoute()
{

}


void CRoute::addPosition(const double lon, const double lat, const QString& action)
{
    pt_t pt;
    pt.lon = lon;
    pt.lat = lat;
    pt.action = action;
    priRoute << pt;

    secRoute.clear();

    calcDistance();

    emit sigChanged();
}


void CRoute::calcDistance()
{
    dist = 0.0;
    if(priRoute.size() < 2) return;

    projXY pt1,pt2;
    double a1,a2;

    QVector<pt_t>::const_iterator p1;
    QVector<pt_t>::const_iterator p2;
    QVector<pt_t>::const_iterator end;
    if(secRoute.isEmpty())
    {
        p1 = priRoute.begin();
        p2 = priRoute.begin() + 1;
        end = priRoute.end();
    }
    else
    {
        p1 = secRoute.begin();
        p2 = secRoute.begin() + 1;
        end = secRoute.end();
    }

    while(p2 != end)
    {
        pt1.u = p1->lon * DEG_TO_RAD; pt1.v = p1->lat * DEG_TO_RAD;
        pt2.u = p2->lon * DEG_TO_RAD; pt2.v = p2->lat * DEG_TO_RAD;
        dist += distance(pt1,pt2,a1,a2);
        ++p2; ++p1;
    }

}


QRectF CRoute::getBoundingRectF()
{

    double north =  -90.0;
    double south =  +90.0;
    double west  = +180.0;
    double east  = -180.0;

    //CRoute * route = routes[key];
    QVector<pt_t>& rtepts = secRoute.isEmpty() ? getPriRtePoints() : getSecRtePoints();
    QVector<pt_t>::const_iterator rtept = rtepts.begin();
    while(rtept != rtepts.end())
    {

        if(rtept->lon < west)  west  = rtept->lon;
        if(rtept->lon > east)  east  = rtept->lon;
        if(rtept->lat < south) south = rtept->lat;
        if(rtept->lat > north) north = rtept->lat;

        ++rtept;
    }

    return QRectF(QPointF(west,north),QPointF(east,south));
}


void CRoute::setIcon(const QString& symname)
{
    iconString = symname;
    iconPixmap = getWptIconByName(symname);

    emit sigChanged();
}


QPixmap CRoute::getIcon()
{
    if(calcRoutePending)
    {
        return QPixmap(":/icons/iconReload16x16.png");
        //        return QPixmap(":/icons/iconInProgress.mng");
    }
    else
    {
        return iconPixmap;
    }
}


QString CRoute::getInfo()
{
    QTime time;
    QString val1, unit1, val2, unit2, str;
    quint32 days;

    str = name;
    IUnit::self().meter2distance(dist, val1, unit1);
    str += tr("\nlength: %1 %2").arg(val1).arg(unit1);

    if(ttime)
    {
        days  = ttime / 86400;
        time = time.addSecs(ttime);
        if(days)
        {
            str += tr("\ntime: %1:").arg(days) + time.toString("HH:mm:ss");
        }
        else
        {
            str += tr("\ntime: ") + time.toString("HH:mm:ss");
        }
    }
    return str;
}


void CRoute::loadSecondaryRoute(QDomDocument& xml, service_e service)
{
    //    qDebug() << xml.toString();

    calcRoutePending = false;

    secRoute.clear();
    firstTime = true;

    switch(service)
    {
        case eOpenRouteService:
            loadSecondaryRouteORS(xml);
            break;
        case eMapQuest:
            loadSecondaryRouteMQ(xml);
            break;
        case eBRouter:
            loadSecondaryRouteBR(xml);
            break;
    }

    calcDistance();
    emit sigChanged();

}


void CRoute::loadSecondaryRouteMQ(QDomDocument& xml)
{
    QStringList instructions;

    QDomElement response    = xml.firstChildElement("response");
    QDomElement route       = response.firstChildElement("route");

    // get time of travel
    QDomElement time        = route.firstChildElement("time");
    ttime = time.text().toUInt();

    // build list of maneuvers
    QDomNodeList legs       = route.firstChildElement("legs").elementsByTagName("leg");
    const qint32 L = legs.size();
    for(int j = 0; j < L; j++)
    {
	QDomNode leg            = legs.item(j);
	QDomNodeList maneuvers  = leg.firstChildElement("maneuvers").elementsByTagName("maneuver");
	const qint32 M = maneuvers.size();
	for(int i = 0; i < M; i++)
	{
	    QDomNode maneuver    = maneuvers.item(i);
	    instructions << maneuver.firstChildElement("narrative").text();
	}
    }

    // read the shape
    QDomElement shape       = route.firstChildElement("shape");
    QDomElement shapePoints = shape.firstChildElement("shapePoints");
    QDomNodeList latLng     = shapePoints.elementsByTagName("latLng");
    const qint32 N = latLng.size();
    for(int i = 0; i < N; i++)
    {
        QDomNode elem   = latLng.item(i);
        QDomElement lat = elem.firstChildElement("lat");
        QDomElement lng = elem.firstChildElement("lng");

        pt_t rtept;
        rtept.lon = lng.text().toFloat();
        rtept.lat = lat.text().toFloat();

        secRoute << rtept;
    }

    // maneuvre index
    QDomElement maneuverIndexes = shape.firstChildElement("maneuverIndexes");
    QDomNodeList index = maneuverIndexes.elementsByTagName("index");
    const qint32 I = index.size();

    if(I == instructions.size())
    {
        for(int i = 0; i < I; i++)
        {
            int idx = index.item(i).toElement().text().toInt();
            if(idx < secRoute.size())
            {
                secRoute[idx].action = instructions[i];
            }
        }
    }
}


void CRoute::loadSecondaryRouteORS(QDomDocument& xml)
{
    QDomElement root = xml.documentElement();
    QDomNodeList RouteSummaries = root.elementsByTagName("xls:RouteSummary");
    if(!RouteSummaries.isEmpty())
    {
        QString timestr = RouteSummaries.item(0).firstChildElement("xls:TotalTime").toElement().text();

        if(timestr.left(2) == "PT")
        {
            ttime = 0;
            QString val;
            for(int i = 2; i < timestr.size(); i++)
            {
                if(timestr[i].isDigit())
                {
                    val += timestr[i];
                }
                else if(timestr[i] == 'H')
                {
                    ttime += val.toUInt() * 3600;
                    val.clear();
                }
                else if(timestr[i] == 'M')
                {
                    ttime += val.toUInt() * 60;
                    val.clear();
                }
                else if(timestr[i] == 'S')
                {
                    ttime += val.toUInt();
                    val.clear();
                }
                else
                {
                    val.clear();
                }
            }
        }
    }

    QDomNodeList instructions = root.elementsByTagName("xls:RouteInstruction");

    const qint32 N = instructions.size();
    for(int i = 0; i < N; i++)
    {
        QDomElement instr = instructions.item(i).toElement();
        QString action = instr.firstChildElement("xls:Instruction").text();

        QDomNodeList points = instr.elementsByTagName("gml:pos");
        if(points.size())
        {
            pt_t rtept;
            rtept.action = action;

            QString strpos = points.item(0).toElement().text();
            rtept.lon = strpos.section(" ", 0, 0).toFloat();
            rtept.lat = strpos.section(" ", 1, 1).toFloat();

            secRoute << rtept;

            const qint32 M = points.size();
            for(int j = 1; j < M; j++)
            {
                pt_t rtept;
                QString strpos = points.item(j).toElement().text();
                rtept.lon = strpos.section(" ", 0, 0).toFloat();
                rtept.lat = strpos.section(" ", 1, 1).toFloat();

                secRoute << rtept;
            }
        }
    }
}

void CRoute::loadSecondaryRouteBR(QDomDocument& xml)
{
//	<!-- track-length = 253153 filtered ascend = 888 plain-ascend = -282 cost=296476 -->
//	 <trk>
//	  <name>brouter_fastbike_0</name>
//	  <trkseg>
//	   <trkpt lon="11.626430" lat="48.298522"><ele>466.75</ele></trkpt>

    QDomElement root = xml.documentElement();
    QDomNodeList nodes = root.childNodes();
    for (int i = 0; i < nodes.size(); i++)
    {
        QDomNode node = nodes.item(i);
        if (node.isComment())
        {
            //TODO parse comment to set dist and ttime
        }
    }
    QDomNodeList tracks = root.elementsByTagName("trk");
    for(int i = 0; i < tracks.size(); i++)
    {
        QDomElement track = tracks.item(i).toElement();
        QDomNodeList segments = track.elementsByTagName("trkseg");
        for (int j = 0; j < segments.size(); j++)
        {
            QDomElement segment = segments.item(j).toElement();
            QDomNodeList points = segment.elementsByTagName("trkpt");
            const qint32 M = points.size();
            for(int k = 0; k < M; k++)
            {
                pt_t rtept;
                QDomElement point = points.item(k).toElement();
                rtept.lon = point.attribute("lon").toFloat();
                rtept.lat = point.attribute("lat").toFloat();
                //QString elevation = point.firstChildElement("ele").text();
                secRoute << rtept;
            }
        }
    }
    routeIdx++;
}

void CRoute::reset()
{
    secRoute.clear();
    calcDistance();

    ttime       = 0;
    firstTime   = true;

    emit sigChanged();
}
