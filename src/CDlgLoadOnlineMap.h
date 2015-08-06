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

#ifndef CDLGLOADONLINEMAP_H
#define CDLGLOADONLINEMAP_H

#include <QDialog>
#include <qtsoap.h>
#include "ui_IDlgLoadOnlineMap.h"

class QNetworkAccessManager;
class QNetworkReply;

class CDlgLoadOnlineMap : public QDialog, private Ui::IDlgLoadOnlineMap
{
    Q_OBJECT;
    public:
        CDlgLoadOnlineMap();
        virtual ~CDlgLoadOnlineMap();
        QString selectedfile;

    public slots:
        void accept();
        void slotWebServiceResponse(const QtSoapMessage &message);
        void slotTargetPath();

    private:
        static const QString text;
        QDir tempDir;
        QtSoapHttpTransport soapHttp;
        bool saveToDisk(const QString &filename, QString data);
        void getMapList();
};
#endif
