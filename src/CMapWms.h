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
#ifndef CMAPWMS_H
#define CMAPWMS_H

#include "IMap.h"

#include <QUrl>
#include <QHash>
#include <QQueue>
#include <QSet>
#include <QtNetwork>

class QCheckBox;
class QNetworkAccessManager;
class QNetworkReply;
class QLabel;
class CDiskCache;
class IMapSelection;

class CMapWms : public IMap
{
    Q_OBJECT;
    public:
        CMapWms(const QString& key, const QString& filename, CCanvas * parent);
        virtual ~CMapWms();

        void convertPt2M(double& u, double& v);
        void convertM2Pt(double& u, double& v);
        void convertPt2Pixel(double& u, double& v);

        void move(const QPoint& old, const QPoint& next);
        void zoom(bool zoomIn, const QPoint& p);
        void zoom(double lon1, double lat1, double lon2, double lat2);
        void zoom(qint32& level);
        void dimensions(double& lon1, double& lat1, double& lon2, double& lat2);
        void getArea_n_Scaling(projXY& p1, projXY& p2, float& my_xscale, float& my_yscale);
        QString getName(){return name;}

        void draw(QPainter& p);

        quint32 scalePixelGrid(quint32 nPixel);
        void select(IMapSelection& ms, const QRect& rect);

    private slots:
        void slotRequestFinished(QNetworkReply* reply);
        void slotProxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*);

    private:
        void convertPixel2M(double& u, double& v);
        void convertM2Pixel(double& u, double& v);

        friend class CDlgMapWmsConfig;
        struct request_t
        {
            bool operator==(const request_t& r) const{return reply == r.reply;}

            QUrl   url;
            QNetworkReply * reply;
            double lon;
            double lat;
            double zoomFactor;
        };

        void draw();
        void checkQueue();
        void addToQueue(request_t& req);
        void config();

        QString name;
        QString urlstr;
        QString format;
        QString layers;
        QString srs;
        QString projection;
        QString version;
        QString copyright;

        quint32 blockSizeX;
        quint32 blockSizeY;

        /// width in number of px
        quint32 xsize_px;
        /// height in number of px
        quint32 ysize_px;
        /// scale [px/m]
        double xscale;
        /// scale [px/m]
        double yscale;
        /// reference point [m] (left hand side of map)
        double xref1;
        /// reference point [m] (top of map)
        double yref1;
        /// reference point [m] (right hand side of map)
        double xref2;
        /// reference point [m] (bottom of map)
        double yref2;

        /// left of viewport
        double x;
        /// top of viewport
        double y;

        double zoomFactor;

        QLabel * status;
        QCheckBox * quadraticZoom;
        bool needsRedrawOvl;
        bool lastTileLoaded;

        QNetworkAccessManager * accessManager;
        QQueue<request_t> newRequests;
        QHash<QString,request_t> pendRequests;
        CDiskCache * diskCache;
        QSet<QString> seenRequest;

        qint32 maxZoomLevel;

};
#endif                           //CMAPWMS_H
