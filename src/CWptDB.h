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
#ifndef CWPTDB_H
#define CWPTDB_H

#include "IDB.h"

#include <QString>
#include <QMap>
#include <QStringList>
#include <QPointF>

#ifdef HAS_EXIF
#include <libexif/exif-data.h>
#endif

#ifdef HAS_EXIF
#include <libexif/exif-data.h>
#endif

class CWptToolWidget;
class CWpt;

/// waypoint database
class CWptDB : public IDB
{
    Q_OBJECT;
    public:
        virtual ~CWptDB();

        static CWptDB& self(){return *m_self;}

        void draw(QPainter& p, const QRect& rect, bool& needsRedraw);

        /// get iterator access to track point list
        QMap<QString,CWpt*> ::iterator begin(){return wpts.begin();}
        /// get iterator access to track point list
        QMap<QString,CWpt*> ::iterator end(){return wpts.end();}

        struct keys_t{QString key; QString name; quint32 time; QString comment; QString icon; qreal lon; qreal lat; qreal d; bool isCache;};

        /// get all keys in the database
        QList<keys_t> keys();

        const QMap<QString,CWpt*>& getWpts(){return wpts;}

        /// create a new waypoint
        /**
            @param lon longitude in [rad]
            @param lat latitude in [rad]
            @param ele elevation in [m]

            @return A temporary pointer to the waypoint object or 0 if the waypoint dialog was canceled.
        */
        CWpt * newWpt(float lon, float lat, float ele, const QString& name);

        /// get pointer access to waypoint via it's key
        CWpt * getWptByKey(const QString& key);

        /// delete several waypoints by their keys
        void delWpt(const QStringList& keys, bool saveSticky = true);
        /// delete a waipoint by it's key
        void delWpt(const QString& key, bool silent = false, bool saveSticky = true);

        void setProxyDistance(const QStringList& keys, double dist);
        void setIcon(const QStringList& keys, const QString& iconName);
        void setParentWpt(const QStringList& keys, const QString& name);

        void loadGPX(CGpx& gpx);
        void saveGPX(CGpx& gpx, const QStringList& keys);
        void loadQLB(CQlb& qlb, bool newKey);
        void saveQLB(CQlb& qlb);

        void upload(const QStringList& keys);
        void download();
        void clear();
        void selWptByKey(const QString& key, bool selectMode);
        void selWptInRange(const QPointF& center, double radius);
        void makeVisible(const QStringList& keys);

        int count(){return wpts.count();}

        QString getNewWptName();
        void    setNewWptName(const QString& name){lastWptName = name;}

#ifdef HAS_EXIF
        enum exifMode_e
        {
            eExifModeSmall,
            eExifModeLarge,
            eExifModeOriginal,
            eExifModeLink
        };

        void createWaypointsFromImages();
        void createWaypointsFromImages(const QStringList& files, exifMode_e mode);

        struct exifGPS_t
        {
            exifGPS_t(ExifByteOrder exif_byte_order): lon(0.0), lat(0.0), ele(1e25f), dir(1e25f), orient(1), lon_sign(1), lat_sign(1), byte_order(exif_byte_order) {}
            double lon;
            double lat;
            double ele;
            double dir;
            int    orient;

            int lon_sign;
            int lat_sign;

            quint32 timestamp;

            ExifByteOrder byte_order;
        };

        void createWaypointsFromImages(const QStringList& files, exifMode_e mode, const QString& filename, quint32 timestamp);
        void createWaypointsFromImages(const QStringList& files, exifMode_e mode, const QString& filename, double lon, double lat);
        void createWaypointsFromImages(const QStringList& files, exifMode_e mode, qint32 offset);

        void addWptFromExif(const exifGPS_t& exif, exifMode_e mode, const QString& filename);
#endif

        static bool keyLessThanAlpha(CWptDB::keys_t&  s1, CWptDB::keys_t&  s2);
        static bool keyLessThanComment(CWptDB::keys_t&  s1, CWptDB::keys_t&  s2);
        static bool keyLessThanIcon(CWptDB::keys_t&  s1, CWptDB::keys_t&  s2);
        static bool keyLessThanDistance(CWptDB::keys_t&  s1, CWptDB::keys_t&  s2);
        static bool keyLessThanTime(CWptDB::keys_t&  s1, CWptDB::keys_t&  s2);

        bool getShowNames(){return showNames;}
        void setShowNames(bool yes){showNames = yes;  emitSigChanged();}

        void getListOfGeoCaches(QStringList& caches);

        bool contains(const QString& key){return wpts.contains(key);}

    private slots:
        void slotModified();

    private:
        friend class CMainWindow;
        friend class CDlgEditWpt;
        friend class CMouseMoveWpt;

        CWptDB(QTabWidget * tb, QObject * parent);
        void addWpt(CWpt * wpt, bool silent);

        static CWptDB * m_self;

        QMap<QString,CWpt*> wpts;

        bool showNames;

        QString lastWptName;
};
#endif                           //CWPTDB_H
