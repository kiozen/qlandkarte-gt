/**********************************************************************************************
    Copyright (C) 2007 Oliver Eichler oliver.eichler@gmx.de

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

#include "CWpt.h"
#include "CWptDB.h"
#include "WptIcons.h"
#include "IUnit.h"
#include "config.h"
#include "GeoMath.h"

#include <QtGui>
#include <QtXml>

#ifndef QK_QT5_TZONE
#include <tzdata.h>
#endif

#ifndef _MKSTR_1
#define _MKSTR_1(x)    #x
#define _MKSTR(x)      _MKSTR_1(x)
#endif

QDir CWpt::path(_MKSTR(MAPPATH) "/wpt");

const QString CWpt::html =  ""
"<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.01 Transitional//EN'  'http://www.w3.org/TR/html4/loose.dtd'>"
"<html>"
"   <head>"
"       <title></title>"
"       <META HTTP-EQUIV='CACHE-CONTROL' CONTENT='NO-CACHE'>"
"       <meta http-equiv='Content-Type' content='text/html; charset=utf-8'>"
"       <style type='text/css'>"
"           p, li { white-space: pre-wrap; }"
"           td {padding-top: 10px;}"
"           th {background-color: darkBlue; color: white;}"
"           h1 {color: #1E5E8F; font-size: 1.2em;}"
"           h2 {color: #226AA0; font-size: 1em;}"
"           body {color: #626262; font-size: 1em;}"
"           .t {background: url(\"%1/frame_top.png\") 0 0 repeat-x; width: 30em;}"
"           .b {background: url(\"%1/frame_bottom.png\") 0 100% repeat-x}"
"           .l {background: url(\"%1/frame_left.png\") 0 0 repeat-y}"
"           .r {background: url(\"%1/frame_right.png\") 100% 0 repeat-y}"
"           .bl {background: url(\"%1/frame_bottom_left.png\") 0 100% no-repeat}"
"           .br {background: url(\"%1/frame_bottom_right.png\") 100% 100% no-repeat}"
"           .tl {background: url(\"%1/frame_top_left.png\") 0 0 no-repeat}"
"           .tr {background: url(\"%1/frame_top_right.png\") 100% 0 no-repeat; padding:10px}"
"           .scale1{ display: inline-block; background: url(\"%1/scale.png\") 0 0.20em no-repeat;}"
"           .scale0{ display: inline-block; }"
"       </style>"
"   </head>"
"   <body style=' font-family:'Sans'; font-size:9pt; font-weight:400; font-style:normal;'>"
"       <p>${info}</p>"
"   </body>"
"</html>";

const QString CWpt::htmlFrame = ""
"<div class='t'><div class='b'><div class='l'><div class='r'><div class='bl'><div class='br'><div class='tl'><div class='tr'>"
"%1"
"</div></div></div></div></div></div></div></div>";

struct wpt_head_entry_t
{
    wpt_head_entry_t() : type(CWpt::eEnd), offset(0) {}
    qint32      type;
    quint32     offset;
    QByteArray  data;
};

QDataStream& operator >>(QDataStream& s, CWpt& wpt)
{
    QIODevice * dev = s.device();
    qint64      pos = dev->pos();

    char magic[9];
    s.readRawData(magic,9);

    if(strncmp(magic,"QLWpt   ",9))
    {
        dev->seek(pos);
        //         throw(QObject::tr("This is not waypoint data."));
        return s;
    }

    QList<wpt_head_entry_t> entries;

    while(1)
    {
        wpt_head_entry_t entry;
        s >> entry.type >> entry.offset;
        entries << entry;
        if(entry.type == CWpt::eEnd) break;
    }

    QList<wpt_head_entry_t>::iterator entry = entries.begin();
    while(entry != entries.end())
    {
        qint64 o = pos + entry->offset;
        dev->seek(o);
        s >> entry->data;

        switch(entry->type)
        {
            case CWpt::eBase:
            {
                QString icon;
                QString key;

                QDataStream s1(&entry->data, QIODevice::ReadOnly);
                s1.setVersion(QDataStream::Qt_4_5);

                s1 >> key;
                s1 >> wpt.sticky;
                s1 >> wpt.timestamp;
                s1 >> icon;
                s1 >> wpt.name;
                s1 >> wpt.comment;
                s1 >> wpt.lat;
                s1 >> wpt.lon;
                s1 >> wpt.ele;
                s1 >> wpt.prx;
                s1 >> wpt.link;
                s1 >> wpt.description;
                s1 >> wpt.urlname;
                s1 >> wpt.type;
                s1 >> wpt.parentWpt;
                s1 >> wpt.selected;

                if(!s1.atEnd())
                {
                    s1 >> wpt.dir;
                }
                else
                {
                    wpt.dir = WPT_NOFLOAT;
                }

                wpt.setIcon(icon);
                wpt.setKey(key);
                break;
            }

            case CWpt::eImage:
            {
                QDataStream s1(&entry->data, QIODevice::ReadOnly);
                s1.setVersion(QDataStream::Qt_4_5);
                CWpt::image_t img;

                wpt.images.clear();

                s1 >> img.offset;
                while(img.offset)
                {
                    wpt.images << img;
                    s1 >> img.offset;
                }

                QList<CWpt::image_t>::iterator image = wpt.images.begin();
                while(image != wpt.images.end())
                {
                    s1.device()->seek(image->offset);
                    s1 >> image->filePath;
                    s1 >> image->info;
                    s1 >> image->pixmap;
                    ++image;
                }
                break;
            }

            case CWpt::eGeoCache:
            {
                quint32 N, n;
                QDataStream s1(&entry->data, QIODevice::ReadOnly);
                s1.setVersion(QDataStream::Qt_4_5);
                wpt.geocache = CWpt::geocache_t();
                CWpt::geocache_t& cache = wpt.geocache;

                s1 >> (quint8&)cache.service;
                s1 >> cache.hasData;
                s1 >> cache.id;
                s1 >> cache.available;
                s1 >> cache.archived;
                s1 >> cache.difficulty;
                s1 >> cache.terrain;
                s1 >> cache.status;
                s1 >> cache.name;
                s1 >> cache.owner;
                s1 >> cache.ownerId;
                s1 >> cache.type;
                s1 >> cache.container;
                s1 >> cache.shortDesc;
                s1 >> cache.longDesc;
                s1 >> cache.hint;
                s1 >> cache.country;
                s1 >> cache.state;
                s1 >> cache.locale;

                s1 >> N;

                for(n = 0; n < N; n++)
                {
                    CWpt::geocachelog_t log;

                    s1 >> log.id;
                    s1 >> log.date;
                    s1 >> log.type;
                    s1 >> log.finderId;
                    s1 >> log.finder;
                    s1 >> log.text;

                    cache.logs << log;
                }

                s1 >> cache.exportBuddies;

                cache.hasData = true;

                break;
            }

            default:;
        }

        ++entry;
    }
    return s;
}


QDataStream& operator <<(QDataStream& s, CWpt& wpt)
{
    QList<wpt_head_entry_t> entries;

    //---------------------------------------
    // prepare base data
    //---------------------------------------
    wpt_head_entry_t entryBase;
    entryBase.type = CWpt::eBase;
    QDataStream s1(&entryBase.data, QIODevice::WriteOnly);
    s1.setVersion(QDataStream::Qt_4_5);

    s1 << wpt.getKey();
    s1 << wpt.sticky;
    s1 << wpt.timestamp;
    s1 << wpt.iconString;
    s1 << wpt.name;
    s1 << wpt.comment;
    s1 << wpt.lat;
    s1 << wpt.lon;
    s1 << wpt.ele;
    s1 << wpt.prx;
    s1 << wpt.link;
    s1 << wpt.description;
    s1 << wpt.urlname;
    s1 << wpt.type;
    s1 << wpt.getParentWpt();
    s1 << wpt.selected;
    s1 << wpt.dir;

    entries << entryBase;

    //---------------------------------------
    // prepare image data
    //---------------------------------------
    wpt_head_entry_t entryImage;
    entryImage.type = CWpt::eImage;
    QDataStream s2(&entryImage.data, QIODevice::WriteOnly);
    s2.setVersion(QDataStream::Qt_4_5);

    // write place holder for image offset
    QList<CWpt::image_t>::iterator image = wpt.images.begin();
    while(image != wpt.images.end())
    {
        s2 << (quint32)0;
        ++image;
    }
    // offset terminator
    s2 << (quint32)0;

    // write image data and store the actual offset
    image = wpt.images.begin();
    while(image != wpt.images.end())
    {
        image->offset = (quint32)s2.device()->pos();
        s2 << image->filePath;
        s2 << image->info;
        s2 << image->pixmap;
        ++image;
    }

    // finally write image offset table
    (quint32)s2.device()->seek(0);
    image = wpt.images.begin();
    while(image != wpt.images.end())
    {
        s2 << image->offset;
        ++image;
    }

    entries << entryImage;

    //---------------------------------------
    // prepare geocache data
    //---------------------------------------
    if(wpt.geocache.hasData)
    {

        wpt_head_entry_t entryGeoCache;
        entryGeoCache.type = CWpt::eGeoCache;
        QDataStream s3(&entryGeoCache.data, QIODevice::WriteOnly);
        s3.setVersion(QDataStream::Qt_4_5);

        CWpt::geocache_t& cache = wpt.geocache;

        s3 << (quint8)cache.service;
        s3 << cache.hasData;
        s3 << cache.id;
        s3 << cache.available;
        s3 << cache.archived;
        s3 << cache.difficulty;
        s3 << cache.terrain;
        s3 << cache.status;
        s3 << cache.name;
        s3 << cache.owner;
        s3 << cache.ownerId;
        s3 << cache.type;
        s3 << cache.container;
        s3 << cache.shortDesc;
        s3 << cache.longDesc;
        s3 << cache.hint;
        s3 << cache.country;
        s3 << cache.state;
        s3 << cache.locale;

        s3 << cache.logs.count();

        foreach(const CWpt::geocachelog_t& log, cache.logs)
        {
            s3 << log.id;
            s3 << log.date;
            s3 << log.type;
            s3 << log.finderId;
            s3 << log.finder;
            s3 << log.text;
        }

        s3 << cache.exportBuddies;

        entries << entryGeoCache;
    }
    //---------------------------------------
    // prepare terminator
    //---------------------------------------
    wpt_head_entry_t entryEnd;
    entryEnd.type = CWpt::eEnd;
    entries << entryEnd;

    //---------------------------------------
    //---------------------------------------
    // now start to actually write data;
    //---------------------------------------
    //---------------------------------------
    // write magic key
    s.writeRawData("QLWpt   ",9);

    // calculate offset table
    quint32 offset = entries.count() * 8 + 9;

    QList<wpt_head_entry_t>::iterator entry = entries.begin();
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


void operator >>(QFile& f, CWpt& wpt)
{
    f.open(QIODevice::ReadOnly);
    QDataStream s(&f);
    s.setVersion(QDataStream::Qt_4_5);
    s >> wpt;
    f.close();
}


void operator <<(QFile& f, CWpt& wpt)
{
    f.open(QIODevice::WriteOnly);
    QDataStream s(&f);
    s.setVersion(QDataStream::Qt_4_5);
    s << wpt;
    f.close();
}


CWpt::CWpt(QObject * parent)
: IItem(parent)
, selected(false)
, sticky(false)
, lat(1000)
, lon(1000)
, ele(WPT_NOFLOAT)
, prx(WPT_NOFLOAT)
, dir(WPT_NOFLOAT)
{
    setIcon("Small City");
}


CWpt::~CWpt()
{
    //    qDebug() << "CWpt::~CWpt()";
}


void CWpt::setIcon(const QString& str)
{
    iconString = str;
    iconPixmap = getWptIconByName(str);
}


QPixmap CWpt::getIcon() const
{
    if(geocache.hasData)
    {
        if(iconString == "Geocache Found")
        {
            QPixmap pixmap = getWptIconByName(geocache.type);
            QPainter p(&pixmap);
            p.drawPixmap(pixmap.width() - 10, pixmap.height() - 10,QPixmap(":/icons/cache/found8x8.png"));
            return pixmap;

        }
        else
        {
            return getWptIconByName(geocache.type);
        }
    }

    return iconPixmap;
}


bool CWpt::hasHiddenInformation()
{
    if(isGeoCache())
    {
        return !geocache.hint.isEmpty();
    }

    return false;
}


QString CWpt::getInfo()
{
    QString str /*= getName()*/;

    if(geocache.hasData)
    {
        str += QString("%4 (%1, D %2, T %3)").arg(geocache.container).arg(geocache.difficulty, 0,'f',1).arg(geocache.terrain, 0,'f',1).arg(geocache.name);
    }

    if(timestamp != 0x00000000 && timestamp != 0xFFFFFFFF)
    {
        if(str.count()) str += "\n";

        QString timezone = GPS_Timezone(lon, lat);
        QDateTime time = QDateTime::fromTime_t(timestamp);
        if(!timezone.isEmpty())
        {
#ifdef QK_QT5_TZONE
            time = time.toTimeZone(QTimeZone(timezone.toLatin1()));
#else
            time = TimeStamp(timestamp).toZone(timezone).toDateTime();
#endif
        }
        str += time.toString();
    }

    if(ele != WPT_NOFLOAT)
    {
        if(str.count()) str += "\n";
        QString val, unit;
        IUnit::self().meter2elevation(ele, val, unit);
        str += tr("elevation: %1 %2").arg(val).arg(unit);
    }

    if(dir != WPT_NOFLOAT)
    {
        if(str.count()) str += "\n";
        str += tr("direction: %1%2").arg(dir).arg(QChar(0260));
    }

    if(prx != WPT_NOFLOAT)
    {
        if(str.count()) str += "\n";
        QString val, unit;
        IUnit::self().meter2distance(prx, val, unit);
        str += tr("proximity: %1 %2").arg(val).arg(unit);
    }

    if(link.count())
    {
        if(str.count()) str += "\n";
        if(link.count() < 50)
        {
            str += link;
        }
        else
        {
            str += link.left(47) + "...";
        }
    }

    if(!parentWpt.isEmpty())
    {
        if(str.count()) str += "\n";
        str += tr("Parent: %1").arg(parentWpt);
    }

    if(description.count())
    {
        if(str.count()) str += "\n";

        if(description.count() < 200)
        {
            str += description;
        }
        else
        {
            str += description.left(197) + "...";
        }
    }
    else
    {
        QString cmt = comment;
        IItem::removeHtml(cmt);

        if(cmt.count())
        {
            if(str.count()) str += "\n";

            if(cmt.count() < 200)
            {
                str += cmt;
            }
            else
            {
                str += cmt.left(197) + "...";
            }
        }
    }
    return str;
}


const QString CWpt::filename(const QDir& dir)
{
    QDateTime ts;
    QString str;

    ts.setTime_t(timestamp);
    str  = ts.toString("yyyy.MM.dd_hh.mm.ss_");
    str += name;
    str += ".wpt";

    return dir.filePath(str);
}


QString CWpt::getEntry(const QString& tag, const QDomNode& parent)
{
    if(parent.namedItem(tag).isElement())
    {
        return parent.namedItem(tag).toElement().text();
    }

    return "";
}


QString CWpt::getEntryHtml(const QString& tag, const QDomNode& parent)
{
    const QDomNode node = parent.namedItem(tag);
    if(node.isElement())
    {
        const QDomElement& elem      = node.toElement();
        const QDomNamedNodeMap& attr = elem.attributes();

        if(attr.namedItem("html").nodeValue().toLocal8Bit().toLower() == "true")
        {
            return elem.text();
        }

        return "<p>" + elem.text() + "</p>";
    }

    return "";
}


void CWpt::loadGpxExt(const QDomNode& wpt)
{
    geocache = geocache_t();

    QDomNode gpxCache = wpt.namedItem("cache");
    if(gpxCache.isNull())
    {
        const QDomNode& extension = wpt.namedItem("extensions");

        gpxCache = extension.namedItem("cache");
        if(gpxCache.isNull())
        {
            return;
        }

        loadOcExt(gpxCache);
    }
    else
    {
        loadGcExt(gpxCache);
    }
}


void CWpt::loadGcExt(const QDomNode& gpxCache)
{
    geocache.service = eGC;
    const QDomNamedNodeMap& attr = gpxCache.attributes();
    geocache.id = attr.namedItem("id").nodeValue().toInt();

    geocache.archived   = attr.namedItem("archived").nodeValue().toLocal8Bit() == "True";
    geocache.available  = attr.namedItem("available").nodeValue().toLocal8Bit() == "True";
    if(geocache.archived)
    {
        geocache.status = "Archived";
    }
    else if(geocache.available)
    {
        geocache.status = "Available";
    }
    else
    {
        geocache.status = "Not Available";
    }

    geocache.name       = getEntry("name",gpxCache);
    geocache.owner      = getEntry("placed_by",gpxCache);
    geocache.type       = getEntry("type",gpxCache);
    geocache.container  = getEntry("container",gpxCache);
    geocache.difficulty = getEntry("difficulty",gpxCache).toFloat();
    geocache.terrain    = getEntry("terrain",gpxCache).toFloat();
    geocache.shortDesc  = getEntryHtml("short_description",gpxCache);
    geocache.longDesc   = getEntryHtml("long_description",gpxCache);
    geocache.hint       = getEntry("encoded_hints",gpxCache);
    geocache.country    = getEntry("country",gpxCache);
    geocache.state      = getEntry("state",gpxCache);

    const QDomNodeList& logs = gpxCache.toElement().elementsByTagName("log");
    uint N = logs.count();

    for(uint n = 0; n < N; ++n)
    {
        const QDomNode& log = logs.item(n);
        const QDomNamedNodeMap& attr = log.attributes();

        geocachelog_t geocachelog;
        geocachelog.id      = attr.namedItem("id").nodeValue().toUInt();
        geocachelog.date    = getEntry("date", log);
        geocachelog.type    = getEntry("type", log);
        if(log.namedItem("finder").isElement())
        {
            const QDomNamedNodeMap& attr = log.namedItem("finder").attributes();
            geocachelog.finderId = attr.namedItem("id").nodeValue();
        }

        geocachelog.finder  = getEntry("finder", log);
        geocachelog.text    = getEntryHtml("text", log);

        geocache.logs << geocachelog;

    }
    geocache.hasData = true;
}


void CWpt::loadTwoNavExt(const QDomNode& gpxCache)
{
    geocache.service = eGC;
    const QDomNamedNodeMap& attr = gpxCache.attributes();
    geocache.id = attr.namedItem("id").nodeValue().toInt();
    geocache.archived   = attr.namedItem("archived").nodeValue().toLocal8Bit() == "True";
    geocache.available  = attr.namedItem("available").nodeValue().toLocal8Bit() == "True";
    if(geocache.archived)
    {
        geocache.status = "Archived";
    }
    else if(geocache.available)
    {
        geocache.status = "Available";
    }
    else
    {
        geocache.status = "Not Available";
    }

    geocache.name       = attr.namedItem("groundspeak:name").nodeValue().toLocal8Bit();
    geocache.owner      = attr.namedItem("groundspeak:placed_by").nodeValue().toLocal8Bit();
    geocache.container  = attr.namedItem("groundspeak:container").nodeValue().toLocal8Bit();
    geocache.type       = attr.namedItem("groundspeak:type").nodeValue().toLocal8Bit();
    geocache.difficulty  = attr.namedItem("groundspeak:difficulty").nodeValue().toFloat();
    geocache.terrain    = attr.namedItem("groundspeak:terrain").nodeValue().toFloat();

    geocache.hint       = attr.namedItem("groundspeak:encoded_hints").nodeValue().toFloat();
    geocache.country    = attr.namedItem("groundspeak:country").nodeValue().toFloat();
    geocache.state      = attr.namedItem("groundspeak:state").nodeValue().toFloat();

    geocache.shortDesc  = getEntryHtml("groundspeak:short_description",gpxCache);
    geocache.longDesc   = getEntryHtml("groundspeak:long_description",gpxCache);

    const QDomNodeList& logs = gpxCache.toElement().elementsByTagName("groundspeak:log");
    uint N = logs.count();

    for(uint n = 0; n < N; ++n)
    {
        const QDomNode& log = logs.item(n);
        const QDomNamedNodeMap& attr = log.attributes();

        geocachelog_t geocachelog;
        geocachelog.id      = attr.namedItem("id").nodeValue().toUInt();
        geocachelog.date    = attr.namedItem("date").nodeValue();
        geocachelog.type    = attr.namedItem("type").nodeValue();
        if(log.namedItem("groundspeak:finder").isElement())
        {
            const QDomNamedNodeMap& attr = log.namedItem("groundspeak:finder").attributes();
            geocachelog.finderId = attr.namedItem("id").nodeValue();
        }

        geocachelog.finder  = getEntry("groundspeak:finder", log);
        geocachelog.text    = getEntryHtml("groundspeak:text", log);

        geocache.logs << geocachelog;

    }
    geocache.hasData = true;

}


void CWpt::loadOcExt(const QDomNode& gpxCache)
{
    geocache.service = eOC;
    const QDomNamedNodeMap& attr = gpxCache.attributes();
    geocache.id = attr.namedItem("id").nodeValue().toInt();
    geocache.status = attr.namedItem("status").nodeValue();
    if(geocache.status == "Available")
    {
        geocache.available = true;
        geocache.archived  = false;
    }
    else
    {
        geocache.available = false;
        geocache.archived  = true;
    }

    geocache.name       = getEntry("name",gpxCache);
    geocache.owner      = getEntry("owner",gpxCache);
    {
        const QDomNamedNodeMap& attr = gpxCache.namedItem("owner").attributes();
        geocache.ownerId = attr.namedItem("userid").nodeValue();
    }

    geocache.type       = getEntry("type",gpxCache);
    geocache.container  = getEntry("container",gpxCache);
    geocache.difficulty = getEntry("difficulty",gpxCache).toFloat();
    geocache.terrain    = getEntry("terrain",gpxCache).toFloat();
    geocache.shortDesc  = getEntryHtml("short_description",gpxCache);
    geocache.longDesc   = getEntryHtml("long_description",gpxCache);
    geocache.hint       = getEntry("encoded_hints",gpxCache);
    geocache.country    = getEntry("country",gpxCache);
    geocache.state      = getEntry("state",gpxCache);
    geocache.locale     = getEntry("locale",gpxCache);

    const QDomNodeList& logs = gpxCache.toElement().elementsByTagName("log");
    uint N = logs.count();

    for(uint n = 0; n < N; ++n)
    {
        const QDomNode& log = logs.item(n);
        const QDomNamedNodeMap& attr = log.attributes();

        geocachelog_t geocachelog;
        geocachelog.id      = attr.namedItem("id").nodeValue().toUInt();
        geocachelog.date    = getEntry("date", log);
        geocachelog.type    = getEntry("type", log);
        if(log.namedItem("finder").isElement())
        {
            const QDomNamedNodeMap& attr = log.namedItem("finder").attributes();
            geocachelog.finderId = attr.namedItem("id").nodeValue();
        }

        geocachelog.finder  = getEntry("finder", log);
        geocachelog.text    = getEntryHtml("text", log);

        geocache.logs << geocachelog;

    }
    geocache.hasData = true;
}


void CWpt::setEntry(const QString& tag, const QString& val, QDomDocument& gpx, QDomElement& parent)
{
    if(!val.isEmpty())
    {
        QDomElement element = gpx.createElement(tag);
        parent.appendChild(element);
        QDomText text = gpx.createTextNode(val);
        element.appendChild(text);
    }
}


void CWpt::setEntryHtml(const QString& tag, const QString& val, QDomDocument& gpx, QDomElement& parent)
{
    if(!val.isEmpty())
    {
        QDomElement element = gpx.createElement(tag);
        parent.appendChild(element);
        QDomText text = gpx.createCDATASection(val);
        element.appendChild(text);
        element.setAttribute("html","True");
    }
}


QString CWpt::insertBuddies(const QString& html)
{
    QString _html_ = html;

    showBuddies(true);

    buddy_t buddy;
    foreach(buddy, buddies)
    {
        foreach(const QString& pos, buddy.pos)
        {
            _html_.replace(pos, QString("%1 (<b><i style='color: black;'>%2</i></b>)").arg(pos).arg(buddy.name));
        }
    }

    showBuddies(false);

    return _html_;
}


void CWpt::saveGpxExt(QDomNode& wpt, bool isExport)
{
    if(!geocache.hasData)
    {
        return;
    }

    QDomDocument gpx = wpt.ownerDocument();

    if(geocache.service == eGC)
    {
        QDomElement gpxCache = gpx.createElement("groundspeak:cache");
        saveGcExt(gpxCache, isExport);
        wpt.appendChild(gpxCache);
    }
    else if(geocache.service == eOC)
    {
        QDomElement extensions;

        if(wpt.namedItem("extensions").isElement())
        {
            extensions = wpt.namedItem("extensions").toElement();
        }
        else
        {
            extensions = gpx.createElement("extensions");
        }

        QDomElement gpxCache = gpx.createElement("cache");
        extensions.appendChild(gpxCache);
        saveOcExt(gpxCache, isExport);

        wpt.appendChild(extensions);
    }

}


void CWpt::saveGcExt(QDomElement& gpxCache, bool isExport)
{
    QString str;
    QDomDocument gpx       = gpxCache.ownerDocument();

    gpxCache.setAttribute("xmlns:groundspeak", "http://www.groundspeak.com/cache/1/0");
    gpxCache.setAttribute("id", geocache.id);
    gpxCache.setAttribute("archived", geocache.archived ? "True" : "False");
    gpxCache.setAttribute("available", geocache.available ? "True" : "False");

    setEntry("groundspeak:name", geocache.name, gpx, gpxCache);
    setEntry("groundspeak:placed_by", geocache.owner, gpx, gpxCache);
    setEntry("groundspeak:type", geocache.type, gpx, gpxCache);
    setEntry("groundspeak:container", geocache.container, gpx, gpxCache);
    if(geocache.difficulty == int(geocache.difficulty))
    {
        str.sprintf("%1.0f", geocache.difficulty);
    }
    else
    {
        str.sprintf("%1.1f", geocache.difficulty);
    }
    setEntry("groundspeak:difficulty", str, gpx, gpxCache);
    if(geocache.terrain == int(geocache.terrain))
    {
        str.sprintf("%1.0f", geocache.terrain);
    }
    else
    {
        str.sprintf("%1.1f", geocache.terrain);
    }
    setEntry("groundspeak:terrain", str, gpx, gpxCache);
    setEntryHtml("groundspeak:short_description", geocache.shortDesc, gpx, gpxCache);

    if(isExport && geocache.exportBuddies)
    {
        setEntryHtml("groundspeak:long_description", insertBuddies(geocache.longDesc), gpx, gpxCache);
    }
    else
    {
        setEntryHtml("groundspeak:long_description", geocache.longDesc, gpx, gpxCache);
    }

    setEntry("groundspeak:encoded_hints", geocache.hint, gpx, gpxCache);

    if(!geocache.logs.isEmpty())
    {
        QDomElement gpxLogs = gpx.createElement("groundspeak:logs");
        gpxCache.appendChild(gpxLogs);

        foreach(const geocachelog_t& log, geocache.logs)
        {
            QDomElement gpxLog = gpx.createElement("groundspeak:log");
            gpxLogs.appendChild(gpxLog);

            gpxLog.setAttribute("id", log.id);
            setEntry("groundspeak:date", log.date, gpx, gpxLog);
            setEntry("groundspeak:type", log.type, gpx, gpxLog);

            QDomElement finder = gpx.createElement("groundspeak:finder");
            gpxLog.appendChild(finder);

            QDomText _finder_ = gpx.createCDATASection(log.finder);
            finder.appendChild(_finder_);
            finder.setAttribute("id", log.finderId);

            setEntryHtml("groundspeak:text", log.text, gpx, gpxLog);
        }
    }
}


void CWpt::saveTwoNavExt(QDomElement& gpxCache, bool isExport)
{
    QString str;
    QDomDocument gpx  = gpxCache.ownerDocument();

    gpxCache.setAttribute("xmlns:groundspeak", "http://www.groundspeak.com/cache/1/0");
    gpxCache.setAttribute("id", geocache.id);
    gpxCache.setAttribute("archived", geocache.archived ? "True" : "False");
    gpxCache.setAttribute("available", geocache.available ? "True" : "False");
    gpxCache.setAttribute("groundspeak:name", geocache.name);
    gpxCache.setAttribute("groundspeak:placed_by", geocache.owner);
    gpxCache.setAttribute("groundspeak:type", geocache.type);
    gpxCache.setAttribute("groundspeak:container", geocache.container);
    if(geocache.difficulty == int(geocache.difficulty))
    {
        str.sprintf("%1.0f", geocache.difficulty);
    }
    else
    {
        str.sprintf("%1.1f", geocache.difficulty);
    }
    gpxCache.setAttribute("groundspeak:difficulty", str);

    if(geocache.terrain == int(geocache.terrain))
    {
        str.sprintf("%1.0f", geocache.terrain);
    }
    else
    {
        str.sprintf("%1.1f", geocache.terrain);
    }

    gpxCache.setAttribute("groundspeak:terrain", str);
    gpxCache.setAttribute("groundspeak:encoded_hints", geocache.hint);

    setEntryHtml("groundspeak:short_description", geocache.shortDesc, gpx, gpxCache);

    if(isExport && geocache.exportBuddies)
    {
        setEntryHtml("groundspeak:long_description", insertBuddies(geocache.longDesc), gpx, gpxCache);
    }
    else
    {
        setEntryHtml("groundspeak:long_description", geocache.longDesc, gpx, gpxCache);
    }

    if(!geocache.logs.isEmpty())
    {
        QDomElement gpxLogs = gpx.createElement("groundspeak:logs");
        gpxCache.appendChild(gpxLogs);

        foreach(const geocachelog_t& log, geocache.logs)
        {
            QDomElement gpxLog = gpx.createElement("groundspeak:log");
            gpxLogs.appendChild(gpxLog);

            gpxLog.setAttribute("id", log.id);
            gpxLog.setAttribute("date", log.date);
            gpxLog.setAttribute("type", log.type);

            QDomElement finder = gpx.createElement("groundspeak:finder");
            gpxLog.appendChild(finder);

            QDomText _finder_ = gpx.createCDATASection(log.finder);
            finder.appendChild(_finder_);
            finder.setAttribute("id", log.finderId);

            setEntryHtml("groundspeak:text", log.text, gpx, gpxLog);
        }
    }

}


void CWpt::saveOcExt(QDomElement& gpxCache, bool isExport)
{
    QString str;
    QDomDocument gpx       = gpxCache.ownerDocument();

    gpxCache.setAttribute("id", geocache.id);
    gpxCache.setAttribute("status", geocache.status);
    setEntry("name", geocache.name, gpx, gpxCache);
    setEntry("owner", geocache.owner, gpx, gpxCache);
    gpxCache.namedItem("owner").toElement().setAttribute("userid", geocache.ownerId);
    setEntry("locale", geocache.locale, gpx, gpxCache);
    setEntry("state", geocache.state, gpx, gpxCache);
    setEntry("country", geocache.country, gpx, gpxCache);

    setEntry("type", geocache.type, gpx, gpxCache);
    setEntry("container", geocache.container, gpx, gpxCache);
    str.sprintf("%1.1f", geocache.difficulty);
    setEntry("difficulty", str, gpx, gpxCache);
    str.sprintf("%1.1f", geocache.terrain);
    setEntry("terrain", str, gpx, gpxCache);
    setEntryHtml("short_description", geocache.shortDesc, gpx, gpxCache);

    if(isExport && geocache.exportBuddies)
    {
        setEntryHtml("long_description", insertBuddies(geocache.longDesc), gpx, gpxCache);
    }
    else
    {
        setEntryHtml("long_description", geocache.longDesc, gpx, gpxCache);
    }

    setEntry("encoded_hints", geocache.hint, gpx, gpxCache);

    if(!geocache.logs.isEmpty())
    {
        QDomElement gpxLogs = gpx.createElement("logs");
        gpxCache.appendChild(gpxLogs);

        foreach(const geocachelog_t& log, geocache.logs)
        {
            QDomElement gpxLog = gpx.createElement("log");
            gpxLogs.appendChild(gpxLog);

            gpxLog.setAttribute("id", log.id);
            setEntry("date", log.date, gpx, gpxLog);
            setEntry("type", log.type, gpx, gpxLog);

            QDomElement finder = gpx.createElement("finder");
            gpxLog.appendChild(finder);

            QDomText _finder_ = gpx.createCDATASection(log.finder);
            finder.appendChild(_finder_);
            finder.setAttribute("id", log.finderId);

            setEntryHtml("text", log.text, gpx, gpxLog);
        }
    }
}


QString CWpt::getExtInfo(bool showHidden)
{
    QDir dirWeb(QDir::home().filePath(CONFIGDIR  "WebStuff"));
    QString info = tr("No additional information.");

    if(geocache.hasData)
    {
        info  = "<h1>" + geocache.name + " by " + geocache.owner + "</h1>";
        info += "<h2>" + geocache.shortDesc + "</h2>";
        info += "<p>";
        info += tr("<div><b>Type:</b> %1 <b>Container:</b> %2 ").arg(geocache.type).arg(geocache.container);
        info += tr("<b>D:</b> %1 <b>T:</b> %2 ").arg(htmlScale(geocache.difficulty)).arg(htmlScale(geocache.terrain));
        info += "</div></p>";

        if(showHidden && !geocache.hint.isEmpty())
        {
            info += "<p><b>Hint:</b> " + geocache.hint + "</p>";
        }

        info += geocache.longDesc;

        foreach(const geocachelog_t& log, geocache.logs)
        {
            int tzoffset;
            QString tmp;
            QDateTime date = IDB::parseTimestamp(log.date, tzoffset);

            tmp  = "<div style='color: black; font-weight: bold;'>";
            tmp += date.date().toString() + " " + log.finder + " " + log.type;
            tmp += "</div>";
            tmp += log.text;

            info += "<p>" + htmlFrame.arg(tmp) + "</p>";
        }

    }

    QString cpytext = html.arg(QUrl::fromLocalFile(dirWeb.path()).toString());
    cpytext = cpytext.replace("${info}", info);
    cpytext.replace("&deg;",QChar(0260));
    //    qDebug() << cpytext;

    return cpytext;
}


QString CWpt::htmlScale(float val)
{
    return QString("<div class='scale1' style='width: %1px;'>&nbsp;</div><div class='scale0' style='width: %2px;'>&nbsp;</div>").arg(val*16).arg((5-val)*16);
}


static QRegExp rx("([NS]{1}[\\s:]*[0-9]+[\\s\260]*[0-9.,]+[\\s,/-]*[EWO]{1}[\\s:]*[0-9]+[\\s\260]*[0-9.,]+)");

void CWpt::showBuddies(bool show)
{
    if(show)
    {
        bool isKnown;

        int p           = 0;
        quint32 cnt     = 0;
        QString html    = getExtInfo(false);

        while ((p = rx.indexIn(html, p)) != -1)
        {
            buddy_t buddy;

            QString str = rx.cap(1);
            p += rx.matchedLength();

            buddy.lon = -1000.0;
            buddy.lat = -1000.0;

            QString str1 = str;
            str1.replace(",",".");
            str1.replace(". "," ");
            str1.replace("/","");
            str1.replace("-","");
            str1.replace(":","");
            str1.replace("O","E");
            if(str1.endsWith("."))
            {
                str1 = str1.left(str1.size() - 1);
            }
            GPS_Math_Str_To_Deg(str1, buddy.lon, buddy.lat, true);

            if(buddy.lon == -1000.0 || buddy.lat == -1000.0)
            {
                continue;
            }

            buddy.lon *= (float) DEG_TO_RAD;
            buddy.lat *= (float) DEG_TO_RAD;

            isKnown = false;
            for(int i = 0; i < buddies.count(); i++)
            {
                buddy_t& b = buddies[i];
                if(b.lon == buddy.lon && b.lat == buddy.lat)
                {
                    isKnown = true;
                    b.pos << str;
                }
            }

            if(!isKnown)
            {
                buddy.pos << str;
                buddy.name = QString("%1%2").arg(++cnt,2,10,QChar('0')).arg(name.mid(2));
                buddies << buddy;
            }
        }
    }
    else
    {
        buddies.clear();
    }
}


bool CWpt::hasBuddies()
{
    int p = 0;
    QString html = getExtInfo(false);

    return rx.indexIn(html, p) != -1;
}
