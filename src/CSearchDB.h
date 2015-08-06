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
#ifndef CSEARCHDB_H
#define CSEARCHDB_H

#include "IDB.h"
#include "CSearch.h"

#include <QMap>
#include <QtNetwork>

class QNetworkReply;

/// search database
class CSearchDB : public IDB
{
    Q_OBJECT
        public:
        virtual ~CSearchDB();

        //         struct result_t
        //         {
        //             qreal   lon;
        //             qreal   lat;
        //             QString query;
        //         };

        static CSearchDB& self(){return *m_self;}

        enum hosts_t
        {
            eOpenRouteService
            ,eMapQuest
            ,eGoogle
            ,eNoHost
        };

        void draw(QPainter& p, const QRect& rect, bool& needsRedraw);

        /// start a query with given string
        void search(const QString& str, hosts_t host);
        /// get iterator access to track point list
        QMap<QString,CSearch*>::iterator begin(){return results.begin();}
        /// get iterator access to track point list
        QMap<QString,CSearch*>::iterator end(){return results.end();}

        CSearch * getResultByKey(const QString& key);

        void delResults(const QStringList& keys);

        void loadGPX(CGpx& gpx){}
        void saveGPX(CGpx& gpx, const QStringList& ){}

        void loadQLB(CQlb& qlb, bool newKey){}
        void saveQLB(CQlb& qlb){}

        void upload(const QStringList&){}
        void download(){}

        void clear();

        void add(const QString& label, double lon, double lat);

        void selSearchByKey(const QString& key);

        bool contains(const QString& key){return results.contains(key);}

        signals:
        void sigStatus(const QString& msg);
        void sigFinished();

    private slots:
        void slotRequestFinished(QNetworkReply * reply);
        void slotRequestFinishedGoogle(QByteArray& data);
        void slotRequestFinishedOpenRouteService(QByteArray& data);
        void slotRequestFinishedMapQuest(QByteArray& data);
        void slotProxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*);

    private:
        friend class CMainWindow;

        void startGoogle(const QString& str);
        void startOpenRouteService(const QString& str);
        void startMapQuest(const QString& str);

        CSearchDB(QTabWidget * tb, QObject * parent);
        static CSearchDB * m_self;
        static const QString xls_ns;
        static const QString sch_ns;
        static const QString gml_ns;
        static const QString xlink_ns;
        static const QString xsi_ns;
        static const QString schemaLocation;

        CSearch tmpResult;
        QNetworkAccessManager networkAccessManager;
        QMap<QString,CSearch*> results;

        QMap<QNetworkReply*, hosts_t> pendingRequests;
};
#endif                           //CSEARCHDB_H
