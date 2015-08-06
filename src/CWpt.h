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
#ifndef CWPT_H
#define CWPT_H

#include "IItem.h"

#include <QObject>
#include <QPixmap>
#include <QString>
#include <QList>
#include <QFile>
#include <QDir>
#include <QList>
#include <QSet>

#define WPT_NOFLOAT 1e25f

class IDB;
class QDomNode;
class QDomDocument;
class QDomElement;

/// waypoint data object
/**
    The idea of this waypoint data onject is to provide an appendable data format, that
    will fit all needs of the future. For the sake of backward compatibility <b>never append
    an existing functional data block</b>. Define a new one. <b>And never assume a block other
    than base to exist.</b> Use the type values from the offset table and read those functional
    data block you understand.

    Data structure for serialized waypoint objects:

<pre>
    "QLWpt   "\0                        // 9 bytes magic string
    qint32 type, quint32 offset         // offset table for functional data blocks
    ...        , ...                    // the type is one of CWpt:type_e, the offset
    ...        , ...                    // is the number of bytes right from the beginning
    0          , void                   // The table is terminated by eEnd (0)
    QByteArray 1                        // Each functional data block is stored in it's own
    ...                                 // byte array. You can randomly access these blocks
    QByteArray N                        // by seeking the offset and stream the data into
                                        // a QByteArray object.

    functional data block eBase:
    QString _key_;                      // unique key to identify the waypoint
    quint32 sticky;                     // the sticky flag (always keep waypoint)
    quint32 timestamp;                  // the creation timestamp of the waypoint
    QString icon;                       // the icon type string
    QString name;                       // waypoint name
    QString comment;                    // waypoint comment (HTML)
    float   lat;                        // latitude []
    float   lon;                        // longitude []
    float   altitude;                   // well, the altitude [m]
    float   proximity;                  // a radius for proximity alerts [m]

    functional data block eImage:
    quint32 offset 1                    // for each image an offset into the functional data block
    ...                                 // is stored.
    quint32 offset N                    //
    quint32 0                           // the offset table is terminated by a value of 0
    QString info1, QPixmap image1       // each image is stored as QPixmap (some kind of png format)
    ...          , ...                  // and an informational string.
    QString infoN, QPixmap imageN

</pre>
*/
class CWpt : public IItem
{
    Q_OBJECT;
    public:
        CWpt(QObject * parent);
        virtual ~CWpt();

        enum geocacheservice_e {eGC, eOC, eTC};

        struct geocachelog_t
        {
            geocachelog_t() : id(0){}
            quint32 id;
            QString date;
            QString type;
            QString finderId;
            QString finder;
            QString text;
        };

        struct geocache_t
        {
            geocache_t() : service(eOC), hasData(false), id(0), available(true), archived(false), difficulty(0), terrain(0), exportBuddies(false){}
            geocacheservice_e service;
            bool hasData;
            quint32 id;
            bool available;
            bool archived;
            float difficulty;
            float terrain;
            QString status;
            QString name;
            QString owner;
            QString ownerId;
            QString type;
            QString container;
            QString shortDesc;
            QString longDesc;
            QString hint;
            QString country;
            QString state;
            QString locale;
            QList<geocachelog_t> logs;
            bool exportBuddies;

        };

        const QString filename(const QDir& dir = CWpt::path);
        enum type_e {eEnd,eBase,eImage,eGeoCache};
        static QDir& getWptPath(){return path;}

        void setIcon(const QString& str);
        QString getInfo();

        QString getExtInfo(bool showHidden);

        QPixmap getIcon() const;

        const geocache_t& getGeocacheData(){return geocache;}

        void loadGpxExt(const QDomNode& wpt);
        void saveGpxExt(QDomNode& wpt, bool isExport);

        bool isGeoCache()const{return geocache.hasData;}
        bool isMovable(){return !(geocache.hasData||(bool)sticky);}
        bool isDeletable(){return !(sticky);}
        bool hasHiddenInformation();

        void showBuddies(bool show);
        bool hasBuddies();

        // eBase: base information
    private:
        friend QDataStream& operator >>(QDataStream& s, CWpt& wpt);
        friend QDataStream& operator <<(QDataStream& s, CWpt& wpt);
        friend class CDlgEditWpt;
        friend class CDlgWptEdit;
        friend class CWptDB;
        friend class CDeviceTwoNav;
        static QDir path;

        void loadGcExt(const QDomNode& gpxCache);
        void loadOcExt(const QDomNode& gpxCache);
        void loadTwoNavExt(const QDomNode& gpxCache);
        void saveGcExt(QDomElement& gpxCache, bool isExport);
        void saveOcExt(QDomElement& gpxCache, bool isExport);
        void saveTwoNavExt(QDomElement& gpxCache, bool isExport);

        void setEntry(const QString& tag, const QString& val, QDomDocument& gpx, QDomElement& parent);
        void setEntryHtml(const QString& tag, const QString& val, QDomDocument& gpx, QDomElement& parent);
        QString getEntry(const QString& tag, const QDomNode& parent);
        QString getEntryHtml(const QString& tag, const QDomNode& parent);
        QString htmlScale(float val);
        QString insertBuddies(const QString& html);

        geocache_t geocache;

        static const QString html;
        static const QString htmlFrame;

    public:
        quint32 selected;
        quint32 sticky;
        float   lat;             ///< [deg]
        float   lon;             ///< [deg]
        float   ele;             ///< [m]
        float   prx;             ///< [m]
        float   dir;             ///< [deg]
        QString link;
        QString urlname;
        QString type;

        struct image_t
        {
            quint32 offset;
            QString info;
            QPixmap pixmap;
            QString filePath;
            QString filename;
        };
        QList<image_t> images;

        struct buddy_t
        {
            QString name;
            QSet<QString> pos;
            float lon;
            float lat;
        };

        QList<buddy_t> buddies;

};

QDataStream& operator >>(QDataStream& s, CWpt& wpt);
QDataStream& operator <<(QDataStream& s, CWpt& wpt);

void operator >>(QFile& s, CWpt& wpt);
void operator <<(QFile& s, CWpt& wpt);
#endif                           //CWPT_H
