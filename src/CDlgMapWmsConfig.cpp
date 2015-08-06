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

#include "CDlgMapWmsConfig.h"
#include "CMapWms.h"
#include "CMapDB.h"
#include "CResources.h"
#include "CMainWindow.h"

#include <QtGui>
#include <QtXml>
#include <QtNetwork>
#include <proj_api.h>

CDlgMapWmsConfig::CDlgMapWmsConfig(CMapWms &map)
: map(map)
{

    setupUi(this);

    QFileInfo fi(map.filename);
    labelFile->setText(tr("File: %1").arg(fi.fileName()));

    QTreeWidgetItem * item;
    item = new QTreeWidgetItem(treeMapConfig);
    item->setText(eColProperty, QString("Title"));
    item->setText(eColValue, map.name);
    item->setFlags(item->flags()|Qt::ItemIsEditable);

    item = new QTreeWidgetItem(treeMapConfig);
    item->setText(eColProperty, QString("Copyright"));
    item->setText(eColValue, map.copyright);
    item->setFlags(item->flags()|Qt::ItemIsEditable);

    item = new QTreeWidgetItem(treeMapConfig);
    item->setText(eColProperty, QString("ServerUrl"));
    item->setText(eColValue, map.urlstr);
    item->setFlags(item->flags()|Qt::ItemIsEditable);

    item = new QTreeWidgetItem(treeMapConfig);
    item->setText(eColProperty, QString("ImageFormat"));
    item->setText(eColValue, map.format);
    item->setFlags(item->flags()|Qt::ItemIsEditable);

    item = new QTreeWidgetItem(treeMapConfig);
    item->setText(eColProperty, QString("Layers"));
    item->setText(eColValue, map.layers);
    item->setFlags(item->flags()|Qt::ItemIsEditable);

    item = new QTreeWidgetItem(treeMapConfig);
    item->setText(eColProperty, QString("SRS"));
    item->setText(eColValue, map.srs);
    item->setFlags(item->flags()|Qt::ItemIsEditable);

    item = new QTreeWidgetItem(treeMapConfig);
    item->setText(eColProperty, QString("SizeX"));
    item->setText(eColValue, QString::number(map.xsize_px));
    item->setFlags(item->flags()|Qt::ItemIsEditable);

    item = new QTreeWidgetItem(treeMapConfig);
    item->setText(eColProperty, QString("SizeY"));
    item->setText(eColValue, QString::number(map.ysize_px));
    item->setFlags(item->flags()|Qt::ItemIsEditable);

    item = new QTreeWidgetItem(treeMapConfig);
    item->setText(eColProperty, QString("BlockSizeX"));
    item->setText(eColValue, QString::number(map.blockSizeX));
    item->setFlags(item->flags()|Qt::ItemIsEditable);

    item = new QTreeWidgetItem(treeMapConfig);
    item->setText(eColProperty, QString("BlockSizeY"));
    item->setText(eColValue, QString::number(map.blockSizeY));
    item->setFlags(item->flags()|Qt::ItemIsEditable);

    item = new QTreeWidgetItem(treeMapConfig);
    item->setText(eColProperty, QString("MaxZoomLevel"));
    if(map.maxZoomLevel != -1)
    {
        item->setText(eColValue, QString::number(map.maxZoomLevel));
    }
    item->setFlags(item->flags()|Qt::ItemIsEditable);

    if(pj_is_latlong(map.pjsrc))
    {
        item = new QTreeWidgetItem(treeMapConfig);
        item->setText(eColProperty, QString("UpperLeftX"));
        item->setText(eColValue, QString::number(map.xref1 * RAD_TO_DEG));
        item->setFlags(item->flags()|Qt::ItemIsEditable);

        item = new QTreeWidgetItem(treeMapConfig);
        item->setText(eColProperty, QString("UpperLeftY"));
        item->setText(eColValue, QString::number(map.yref1 * RAD_TO_DEG));
        item->setFlags(item->flags()|Qt::ItemIsEditable);

        item = new QTreeWidgetItem(treeMapConfig);
        item->setText(eColProperty, QString("LowerRightX"));
        item->setText(eColValue, QString::number(map.xref2 * RAD_TO_DEG));
        item->setFlags(item->flags()|Qt::ItemIsEditable);

        item = new QTreeWidgetItem(treeMapConfig);
        item->setText(eColProperty, QString("LowerRightY"));
        item->setText(eColValue, QString::number(map.yref2 * RAD_TO_DEG));
        item->setFlags(item->flags()|Qt::ItemIsEditable);
    }
    else
    {
        item = new QTreeWidgetItem(treeMapConfig);
        item->setText(eColProperty, QString("UpperLeftX"));
        item->setText(eColValue, QString::number(map.xref1, 'f'));
        item->setFlags(item->flags()|Qt::ItemIsEditable);

        item = new QTreeWidgetItem(treeMapConfig);
        item->setText(eColProperty, QString("UpperLeftY"));
        item->setText(eColValue, QString::number(map.yref1, 'f'));
        item->setFlags(item->flags()|Qt::ItemIsEditable);

        item = new QTreeWidgetItem(treeMapConfig);
        item->setText(eColProperty, QString("LowerRightX"));
        item->setText(eColValue, QString::number(map.xref2, 'f'));
        item->setFlags(item->flags()|Qt::ItemIsEditable);

        item = new QTreeWidgetItem(treeMapConfig);
        item->setText(eColProperty, QString("LowerRightY"));
        item->setText(eColValue, QString::number(map.yref2, 'f'));
        item->setFlags(item->flags()|Qt::ItemIsEditable);
    }

    treeMapConfig->resizeColumnToContents(eColProperty);

    accessManager = new QNetworkAccessManager(this);
    accessManager->setProxy(QNetworkProxy(QNetworkProxy::DefaultProxy));

    connect(accessManager,SIGNAL(finished(QNetworkReply*)),this,SLOT(slotRequestFinished(QNetworkReply*)));
    connect(accessManager, SIGNAL(proxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)),
        this, SLOT(slotProxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)));

    QNetworkRequest request;

    QUrl url(map.urlstr);
#ifdef QK_QT5_PORT
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("version", map.version);
    urlQuery.addQueryItem("service", "wms");
    urlQuery.addQueryItem("request", "GetCapabilities");
    url.setQuery(urlQuery);
#else
    url.addQueryItem("version", map.version);
    url.addQueryItem("service", "wms");
    url.addQueryItem("request", "GetCapabilities");
#endif
    request.setUrl(url);
    accessManager->get(request);

}


CDlgMapWmsConfig::~CDlgMapWmsConfig()
{

}


void CDlgMapWmsConfig::updateEntry(QDomDocument& dom, QTreeWidgetItem* item, QDomElement& elem, const QString& tag)
{
    if(elem.firstChildElement(tag).firstChild().isNull())
    {
        QDomElement prop = dom.createElement(tag);
        elem.appendChild(prop);

        QDomText val = dom.createTextNode(item->text(eColValue));
        prop.appendChild(val);

    }
    else
    {
        elem.firstChildElement(tag).firstChild().setNodeValue(item->text(eColValue));
    }
}


void CDlgMapWmsConfig::accept()
{
    QFile file(map.filename);
    file.open(QIODevice::ReadOnly);
    QDomDocument dom;
    dom.setContent(&file);
    file.close();

    QDomElement gdal        = dom.firstChildElement("GDAL_WMS");
    QDomElement service     = gdal.firstChildElement("Service");
    QDomElement datawindow  = gdal.firstChildElement("DataWindow");

    QList<QTreeWidgetItem*> items = treeMapConfig->findItems("*", Qt::MatchWildcard);
    foreach(QTreeWidgetItem* item, items)
    {
        if(item->text(eColProperty) == "Title")
        {
            updateEntry(dom, item, service, "Title");
        }
        else if(item->text(eColProperty) == "ServerUrl")
        {
            updateEntry(dom, item, service, "ServerUrl");
        }
        else if(item->text(eColProperty) == "ImageFormat")
        {
            updateEntry(dom, item, service, "ImageFormat");
        }
        else if(item->text(eColProperty) == "Layers")
        {
            updateEntry(dom, item, service, "Layers");
        }
        else if(item->text(eColProperty) == "SRS")
        {
            updateEntry(dom, item, service, "SRS");

            if(!gdal.firstChildElement("Projection").firstChild().isNull())
            {
                gdal.firstChildElement("Projection").firstChild().setNodeValue(item->text(eColValue));
            }
        }
        else if(item->text(eColProperty) == "Version")
        {
            updateEntry(dom, item, service, "Version");
        }
        else if(item->text(eColProperty) == "Copyright")
        {
            updateEntry(dom, item, service, "Copyright");
        }
        else if(item->text(eColProperty) == "BlockSizeX")
        {
            updateEntry(dom, item, gdal, "BlockSizeX");
        }
        else if(item->text(eColProperty) == "BlockSizeY")
        {
            updateEntry(dom, item, gdal, "BlockSizeY");
        }
        else if(item->text(eColProperty) == "MaxZoomLevel")
        {
            if(!item->text(eColValue).isEmpty())
            {
                updateEntry(dom, item, gdal, "MaxZoomLevel");
            }
        }
        else if(item->text(eColProperty) == "SizeX")
        {
            updateEntry(dom, item, datawindow, "SizeX");
        }
        else if(item->text(eColProperty) == "SizeY")
        {
            updateEntry(dom, item, datawindow, "SizeY");
        }
        else if(item->text(eColProperty) == "UpperLeftX")
        {
            updateEntry(dom, item, datawindow, "UpperLeftX");
        }
        else if(item->text(eColProperty) == "UpperLeftY")
        {
            updateEntry(dom, item, datawindow, "UpperLeftY");
        }
        else if(item->text(eColProperty) == "LowerRightX")
        {
            updateEntry(dom, item, datawindow, "LowerRightX");
        }
        else if(item->text(eColProperty) == "LowerRightY")
        {
            updateEntry(dom, item, datawindow, "LowerRightY");
        }

    }

    file.open(QIODevice::WriteOnly);
    QTextStream out(&file);
    dom.save(out, 2);
    file.close();
    QDialog::accept();

    CMapDB::self().reloadMap();
}


void CDlgMapWmsConfig::slotRequestFinished(QNetworkReply* reply)
{
    if(reply->error())
    {
        textCapabilities->setText(reply->errorString());
    }
    else
    {
        QDomDocument dom;
        dom.setContent(reply->readAll());

        textCapabilities->setPlainText(dom.toString());
    }
}


void CDlgMapWmsConfig::slotProxyAuthenticationRequired(const QNetworkProxy &prox, QAuthenticator *auth)
{
    QString user;
    QString pwd;

    CResources::self().getHttpProxyAuth(user,pwd);

    auth->setUser(user);
    auth->setPassword(pwd);
}
