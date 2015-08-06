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
#ifndef CROUTETOOLWIDGET_H
#define CROUTETOOLWIDGET_H

#include <QWidget>
#include <QSet>
#include <QtNetwork>
#include "CRoute.h"
#include "ui_IRouteToolWidget.h"

class CRoute;
class QDomDocument;
class QDomElement;
class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

class CRouteToolWidget : public QWidget, private Ui::IRouteToolWidget
{
    Q_OBJECT
        public:
        CRouteToolWidget(QTabWidget * parent);
        virtual ~CRouteToolWidget();

        enum sortmode_e
        {
            eSortByName
            ,eSortByTime
        };

        static sortmode_e  getSortMode(){return sortmode;}

    protected:
        void keyPressEvent(QKeyEvent * e);

    private slots:
        void slotDBChanged();
        void slotItemClicked(QListWidgetItem * item);
        void slotItemDoubleClicked(QListWidgetItem * item);
        void slotContextMenu(const QPoint& pos);
        void slotEdit();
        void slotDelete();
        void slotCalcRoute();
        void slotResetRoute();

        void slotRequestFinished(QNetworkReply* );
        void slotSelectionChanged();

        void slotToOverlay();
        void slotToTrack();
        void slotZoomToFit();

        void slotTimeout();
        void slotServiceChanged(int);
        void slotBRPreferenceChanged(int);
        void slotBRProfilesChanged();

        void slotProxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*);

    private:
        void startOpenRouteService(CRoute& rte);
        void addOpenLSWptList(QDomDocument& xml, QDomElement& WayPointList, CRoute& rte);
        void addOpenLSPos(QDomDocument& xml, QDomElement& Point, CRoute::pt_t& pos);

        void startMapQuest(CRoute& rte);
        void addMapQuestLocations(QDomDocument& xml, QDomElement& locations, CRoute& rte);

        void startBRouterService(CRoute& rte);

        bool originator;

        enum tab_e
        {
            eTabRoute = 0
            ,eTabSetup = 1
            ,eTabHelp = 2
        };

        static const QString gml_ns;
        static const QString xls_ns;
        static const QString xsi_ns;
        static const QString sch_ns;
        static const QString xlink_ns;
        static const QString schemaLocation;

        QNetworkAccessManager * m_networkAccessManager;
        QSet<QString> knownLocale;
        QTimer * timer;
        static sortmode_e sortmode;

        QMap<QNetworkReply*, QString> pendingRequests;
};
#endif                           //CROUTETOOLWIDGET_H
