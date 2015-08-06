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

#ifndef CDLGMAPWMSCONFIG_H
#define CDLGMAPWMSCONFIG_H

#include <QDialog>
#include <QtNetwork>
#include "ui_IDlgMapWmsConfig.h"

class CMapWms;
class QDomDocument;
class QDomElement;
class QNetworkAccessManager;
class QNetworkReply;

class CDlgMapWmsConfig : public QDialog, private Ui::IDlgMapWmsConfig
{
    Q_OBJECT;
    public:
        CDlgMapWmsConfig(CMapWms& map);
        virtual ~CDlgMapWmsConfig();

        void accept();

    private slots:
        void slotRequestFinished(QNetworkReply* reply);
        void slotProxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*);

    private:
        void updateEntry(QDomDocument& dom, QTreeWidgetItem* item, QDomElement& elem, const QString& tag);

        CMapWms& map;

        enum col_e{eColProperty, eColValue};

        QNetworkAccessManager * accessManager;
};
#endif                           //CDLGMAPWMSCONFIG_H
