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

#include "CDlgLoadOnlineMap.h"
#include "CSettings.h"

#include <QtGui>
#include <QtNetwork>
#include <QFileDialog>

#ifndef WIN32
#include <unistd.h>
#include <pwd.h>
#ifdef Q_OS_MAC
#include "config.h"
#endif
#endif

const QString CDlgLoadOnlineMap::text =  QObject::tr(""
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
"       </style>"
"   </head>"
"   <body style=' font-family:'Sans'; font-size:9pt; font-weight:400; font-style:normal;'>"
"       <p>${info}</p>"
"   </body>"
"</html>"
"");

CDlgLoadOnlineMap::CDlgLoadOnlineMap()
{
    setupUi(this);

#ifndef Q_OS_WIN32
    const char *envCache = getenv("QLGT_LEGEND");

    if (envCache)
    {
        tempDir = envCache;
    }
    else
    {
#ifndef Q_OS_MAC
        struct passwd * userInfo = getpwuid(getuid());
        tempDir = QDir::homePath() + "/qlandkartegt-" + userInfo->pw_name + "/maps/";
#else
        tempDir = QDir::home().filePath(CONFIGDIR) + "/Maps/";
#endif
    }
#else
    tempDir = QDir::homePath() + "/qlandkarteqt/cache/";
#endif
    tempDir.mkpath(tempDir.path());

    SETTINGS;
    QString wmsPath     = cfg.value("wmsMaps/path","").toString();
    if (!wmsPath.isEmpty())
    {
        tempDir.setPath(wmsPath);
    }

    wmsTargetPath->setText(tempDir.absolutePath());

    connect(&soapHttp, SIGNAL(responseReady(const QtSoapMessage &)),this, SLOT(slotWebServiceResponse(const QtSoapMessage &)));
    connect(wmsButtonPath, SIGNAL(clicked()),this,SLOT(slotTargetPath()));

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    getMapList();
}


void CDlgLoadOnlineMap::getMapList()
{
    QtSoapMessage request;
    request.setMethod(QtSoapQName("getwmsmaps", "urn:qlandkartegt"));
    request.addMethodArgument("folder", "", "");
    soapHttp.setHost("www.qlandkarte.org");
    soapHttp.submitRequest(request, "/webservice/qlandkartegt.php");
}


CDlgLoadOnlineMap::~CDlgLoadOnlineMap()
{
    QApplication::restoreOverrideCursor();
}


void CDlgLoadOnlineMap::accept()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QList<QListWidgetItem *> items = mapList->selectedItems();
    for (int i=0; i < items.count();i++)
    {
        QListWidgetItem *item = items.at(i);
        QtSoapMessage request;
        request.setMethod(QtSoapQName("getwmslink", "urn:qlandkartegt"));
        request.addMethodArgument("link", "", item->data(Qt::UserRole).toString());
        soapHttp.setHost("www.qlandkarte.org");
        soapHttp.submitRequest(request, "/webservice/qlandkartegt.php");
        selectedfile = "";
        selectedfile = selectedfile.prepend(tempDir.absolutePath()+"/");
        selectedfile += item->data(Qt::UserRole + 1).toString();
    }
}


bool CDlgLoadOnlineMap::saveToDisk(const QString &filename, QString data)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly))
    {
        fprintf(stderr, "Could not open %s for writing: %s\n",
            qPrintable(filename),
            qPrintable(file.errorString()));
        return false;
    }

    file.write(data.toLocal8Bit());
    file.close();

    return true;
}


void CDlgLoadOnlineMap::slotWebServiceResponse(const QtSoapMessage &message)
{
    QString method(message.method().name().name());
    if (message.isFault())
    {
        qDebug("Error: %s", qPrintable(message.faultString().toString()));
    }
    else
    {
        if (method == "getwmsmapsResponse")
        {
            const QtSoapType &array = message.returnValue();
            QListWidgetItem *item;
            for (int i = 0; i < array.count(); ++i)
            {
                const QtSoapType &map = array[i];
                QString mapName(map["name"].toString().trimmed());
                if (mapName.endsWith(".xml") || mapName.endsWith(".tms"))
                {
                    QIcon icon      = mapName.endsWith(".xml") ? QIcon(":/icons/iconWMS22x22.png") : QIcon(":/icons/iconTMS22x22.png");
                    QString text    = QFileInfo(mapName).baseName();
                    text = text.replace(QRegExp("_")," ");

                    item = new QListWidgetItem(icon ,text);
                    item->setData(Qt::UserRole + 0,QUrl(map["link"].toString()));
                    item->setData(Qt::UserRole + 1, mapName);
                    mapList->addItem(item);
                }
            }
            mapList->sortItems();
        }

        if (method == "getwmslinkResponse")
        {
            QString data(message.returnValue().toString());
                                 // This _must_ come first
            data.replace(QRegExp("&amp;"), "&");
            data.replace(QRegExp("&lt;"), "<");
            data.replace(QRegExp("&gt;"), ">");
            data.replace(QRegExp("&quot;"), "\"");
            QDomDocument doc("mydocument");
            doc.setContent(data);
            saveToDisk(selectedfile,doc.toString());
            QDialog::accept();
        }

        if (method == "getlastversionResponse")
        {

        }
    }
    QApplication::restoreOverrideCursor();
}


void CDlgLoadOnlineMap::slotTargetPath()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Open Directory"), tempDir.absolutePath(), QFileDialog::ShowDirsOnly);
    if(!path.isEmpty())
    {
        tempDir.setPath(path);
        wmsTargetPath->setText(tempDir.absolutePath());
        SETTINGS;
        cfg.setValue("wmsMaps/path",path);
    }
}
