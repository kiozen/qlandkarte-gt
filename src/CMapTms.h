/**********************************************************************************************
    Copyright (C) 2012 Oliver Eichler oliver.eichler@gmx.de

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

#ifndef CMAPTMS_H
#define CMAPTMS_H

#include "IMap.h"
#include <QUrl>
#include <QHash>
#include <QQueue>
#include <QSet>
#include <QtCore>
#include <IDiskCache.h>
#include <QtNetwork>

class QNetworkAccessManager;
class QNetworkReply;
class QLabel;
class CDiskCache;

class CMapTms : public IMap
{
    Q_OBJECT;
    public:
        CMapTms(const QString& key, CCanvas *parent);
        virtual ~CMapTms();

        void convertPt2M(double& u, double& v);
        void convertM2Pt(double& u, double& v);

        void move(const QPoint& old, const QPoint& next);
        void zoom(bool zoomIn, const QPoint& p);
        void zoom(double lon1, double lat1, double lon2, double lat2);
        void zoom(qint32& level);
        void dimensions(double& lon1, double& lat1, double& lon2, double& lat2);
        void getArea_n_Scaling(projXY& p1, projXY& p2, float& my_xscale, float& my_yscale);

        void draw(QPainter& p);

        quint32 scalePixelGrid(quint32 nPixel);
        void select(IMapSelection& ms, const QRect& rect);

        QString getName(){return name;}

    public slots:
        void resize(const QSize& size);

    private slots:
        void slotRequestFinished(QNetworkReply* reply);
        void slotProxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*);

    private:
        struct layer_t
        {
            int minZoomLevel;
            int maxZoomLevel;
            QPixmap buffer;
            QString strUrl;
            QString script;

        };

        void readConfigFromFile(const QString& filename, QWidget * parent);
        QString createUrl(const layer_t &layer, int x, int y, int z);

        struct request_t
        {
            bool operator==(const request_t& r) const{return reply == r.reply;}

            QUrl url;
            QNetworkReply * reply;
            double lon;
            double lat;
            double zoomFactor;
            int layer;
        };

        void draw();
        void addToQueue(request_t& req);
        void checkQueue();
        void config();

        inline int lon2tile(double lon, int z)
        {
            return (int)(qRound(256*(lon + 180.0) / 360.0 * pow(2.0, z)));
        }

        inline int lat2tile(double lat, int z)
        {
            return (int)(qRound(256*(1.0 - log( tan(lat * M_PI/180.0) + 1.0 / cos(lat * M_PI/180.0)) / M_PI) / 2.0 * pow(2.0, z)));
        }

        inline double tile2lon(int x, int z)
        {
            return x / pow(2.0, z) * 360.0 - 180;
        }

        inline double tile2lat(int y, int z)
        {
            double n = M_PI - 2.0 * M_PI * y / pow(2.0, z);
            return 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
        }

        double zoomFactor;
        double x;
        double y;
        double xscale;
        double yscale;
        bool needsRedrawOvl;

        /// reference point [m] (left hand side of map)
        double xref1;
        /// reference point [m] (top of map)
        double yref1;
        /// reference point [m] (right hand side of map)
        double xref2;
        /// reference point [m] (bottom of map)
        double yref2;

        /// the longitude of the top left reference point [rad]
        double lon1;
        /// the latitude of the top left reference point [rad]
        double lat1;
        /// the longitude of the bottom right reference point [rad]
        double lon2;
        /// the latitude of the bottom right reference point [rad]
        double lat2;

        QString copyright;
        bool lastTileLoaded;

        QNetworkAccessManager * accessManager;
        QQueue<request_t> newRequests;
        QHash<QString,request_t> pendRequests;
        IDiskCache * diskCache;
        QSet<QString> seenRequest;

        QLabel * status;

        QVector<layer_t> layers;

        QString name;

        struct rawHeaderItem_t
        {
            QString name;
            QString value;
        };

        QList<rawHeaderItem_t> rawHeaderItems;

        int minZoomLevel;
        int maxZoomLevel;

};
#endif                           //CMAPTMS_H
