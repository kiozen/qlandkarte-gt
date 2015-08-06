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

#include "CSearchDB.h"
#include "CSearchToolWidget.h"
#include "CResources.h"
#include "CMainWindow.h"
#include "CCanvas.h"
#include "CMapDB.h"
#include "IMap.h"

#include <QtGui>
#include <QtXml>
#include <QtNetwork>
#include <QNetworkProxy>

CSearchDB * CSearchDB::m_self;

CSearchDB::CSearchDB(QTabWidget * tb, QObject * parent)
: IDB(IDB::eTypeSrc, tb, parent)
, tmpResult(0)
{
    m_self      = this;
    toolview    = new CSearchToolWidget(tb);

    networkAccessManager.setProxy(QNetworkProxy(QNetworkProxy::DefaultProxy));

    connect(&networkAccessManager,SIGNAL(finished(QNetworkReply*)),this,SLOT(slotRequestFinished(QNetworkReply*)));
    connect(&networkAccessManager, SIGNAL(proxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)),
        this, SLOT(slotProxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)));
}


CSearchDB::~CSearchDB()
{

}


void CSearchDB::clear()
{
    if(results.isEmpty()) return;
    results.clear();
    emitSigChanged();
}


void CSearchDB::search(const QString& str, hosts_t host)
{
    emit sigStatus(tr("start searching..."));
    switch(host)
    {
        case eOpenRouteService:
            startOpenRouteService(str);
            break;
            //        case eMapQuest:
            //            startMapQuest(str);
            //            break;
        case eGoogle:
            startGoogle(str);
            break;
        default:
            emit sigStatus(tr("Unknown host."));
            emit sigFinished();
            emitSigChanged();
    }
}


void CSearchDB::startGoogle(const QString& str)
{
    tmpResult.setName(str);

    QString addr = str;

    QUrl url("http://maps.googleapis.com");
    url.setPath("/maps/api/geocode/xml");
#ifdef QK_QT5_PORT
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("address",addr.replace(" ","+"));
    urlQuery.addQueryItem("sensor","false");
    url.setQuery(urlQuery);
#else
    url.addQueryItem("address",addr.replace(" ","+"));
    url.addQueryItem("sensor","false");
#endif
    QNetworkRequest request;

    request.setUrl(url);
    QNetworkReply * reply = networkAccessManager.get(request);
    pendingRequests[reply] = eGoogle;
}


const QString CSearchDB::xls_ns = "http://www.opengis.net/xls";
const QString CSearchDB::sch_ns = "http://www.ascc.net/xml/schematron";
const QString CSearchDB::gml_ns = "http://www.opengis.net/gml";
const QString CSearchDB::xlink_ns = "http://www.w3.org/1999/xlink";
const QString CSearchDB::xsi_ns = "http://www.w3.org/2001/XMLSchema-instance";
const QString CSearchDB::schemaLocation = "http://www.opengis.net/xls http://schemas.opengis.net/ols/1.1.0/LocationUtilityService.xsd";

void CSearchDB::startOpenRouteService(const QString& str)
{
    tmpResult.setName(str);

    QDomDocument xml;
    QDomElement root = xml.createElement("xls:XLS");
    xml.appendChild(root);

    root.setAttribute("xmlns:xls",xls_ns);
    root.setAttribute("xmlns:sch",sch_ns);
    root.setAttribute("xmlns:gml",gml_ns);
    root.setAttribute("xmlns:xlink",xlink_ns);
    root.setAttribute("xmlns:xsi",xsi_ns);
    root.setAttribute("xsi:schemaLocation",schemaLocation);
    root.setAttribute("version","1.1");
    //root.setAttribute("xls:lang", comboLanguage->itemData(comboLanguage->currentIndex()).toString());

    QDomElement requestHeader = xml.createElement("xls:RequestHeader");
    root.appendChild(requestHeader);

    QDomElement Request = xml.createElement("xls:Request");
    root.appendChild(Request);

    Request.setAttribute("methodName", "GeocodeRequest");
    Request.setAttribute("requestID", "12345");
    Request.setAttribute("version", "1.1");

    QDomElement GeocodeRequest = xml.createElement("xls:GeocodeRequest");
    Request.appendChild(GeocodeRequest);

    QDomElement Address = xml.createElement("xls:Address");
    GeocodeRequest.appendChild(Address);

    Address.setAttribute("countryCode", "DE");

    QDomElement freeFormAddress = xml.createElement("xls:freeFormAddress");
    Address.appendChild(freeFormAddress);

    QDomText _freeFormAddress_ = xml.createTextNode(str);
    freeFormAddress.appendChild(_freeFormAddress_);

    //    qDebug() << xml.toString();

    QByteArray array;
    QTextStream out(&array, QIODevice::WriteOnly);
    out.setCodec("UTF-8");
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << xml.toString() << endl;

    QUrl url("http://openls.geog.uni-heidelberg.de");
    url.setPath("/qlandkarte/geocode");

    QNetworkRequest request;

    request.setUrl(url);
    QNetworkReply * reply = networkAccessManager.post(request, array);
    pendingRequests[reply] = eOpenRouteService;

}


void CSearchDB::startMapQuest(const QString& str)
{

}


void CSearchDB::slotRequestFinished(QNetworkReply * reply)
{
    hosts_t host = eNoHost;
    if(pendingRequests.contains(reply))
    {
        host = pendingRequests.take(reply);
    }

    if(reply->error() != QNetworkReply::NoError)
    {
        emit sigStatus(reply->errorString());
        emit sigFinished();
        emitSigChanged();
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    if(data.isEmpty() || host == eNoHost)
    {
        return;
    }

    switch(host)
    {
        case eOpenRouteService:
            slotRequestFinishedOpenRouteService(data);
            break;
        case eGoogle:
            slotRequestFinishedGoogle(data);
            break;
        default:
            emit sigStatus(tr("Unknown host."));
            emit sigFinished();
            emitSigChanged();
    }

    QApplication::restoreOverrideCursor();
}


void CSearchDB::slotRequestFinishedGoogle(QByteArray& data)
{
    QString stat;
    QDomDocument xml;
    QString status = tr("finished");

    xml.setContent(data);
    //    qDebug() << xml.toString();

    QDomElement root = xml.documentElement();

    if(root.tagName() != "GeocodeResponse")
    {
        status = tr("Unknown response");
        goto slotRequestFinishedGoogle_End;
    }

    stat = root.namedItem("status").toElement().text();
    if(stat != "OK")
    {
        status  = tr("Error: ");
        status += root.namedItem("error_message").toElement().text();
        goto slotRequestFinishedGoogle_End;
    }

    {
        QDomNodeList entries = root.elementsByTagName("result");
        const qint32 N = entries.size();
        if(N)
        {
            for(int i = 0; i < N; i++)
            {
                CSearch * item = new CSearch(this);

                QDomElement entry   = entries.item(i).toElement();
                QDomElement address = entry.namedItem("formatted_address").toElement();
                if(address.isElement())
                {
                    item->formatedText = address.text();
                }

                QDomNode geometry = entry.namedItem("geometry");
                QDomNode location = geometry.namedItem("location");
                QDomElement lng = location.namedItem("lng").toElement();
                QDomElement lat = location.namedItem("lat").toElement();

                QString strLng = lng.text();
                QString strLat = lat.text();

                item->lon   = strLng.toDouble();
                item->lat   = strLat.toDouble();
                item->setName(tmpResult.getName());
                results[item->getKey()] = item;
                theMainWindow->getCanvas()->move(item->lon, item->lat);

            }
        }
    }

    slotRequestFinishedGoogle_End:
    emit sigStatus(status);
    emit sigFinished();
    emitSigChanged();
}


void CSearchDB::slotRequestFinishedOpenRouteService(QByteArray& data)
{
    QDomDocument xml;
    QString status = tr("finished");

    xml.setContent(data);
    //    qDebug() << xml.toString();

    QDomElement root = xml.documentElement();
    QDomNodeList Results = root.elementsByTagName("xls:GeocodedAddress");
    const qint32 N = Results.size();

    if(N)
    {
        for(int i = 0; i < N; i++)
        {
            CSearch * item = new CSearch(this);

            QDomElement Result = Results.item(i).toElement();
            QDomElement Point = Result.firstChildElement("gml:Point").toElement();
            QString strpos = Point.firstChildElement("gml:pos").toElement().text();

            QDomElement Address = Result.firstChildElement("xls:Address").toElement();
            item->countryCode = Address.attribute("countryCode","");

            QDomElement StreetAddress = Address.firstChildElement("xls:StreetAddress").toElement();
            QDomElement Street = StreetAddress.firstChildElement("xls:Street").toElement();
            item->street = Street.attribute("officialName","");

            QDomNodeList places = Result.elementsByTagName("xls:Place");
            const qint32 M = places.size();
            for(int j = 0; j < M; j++)
            {
                QDomElement place = places.item(j).toElement();
                QString type = place.attribute("type","");

                if(type == "CountrySubdivision")
                {
                    item->country = place.text();
                }
                else if(type == "Municipality")
                {
                    item->municipal = place.text();
                }
            }

            QDomElement PostalCode = Address.firstChildElement("xls:PostalCode").toElement();
            item->postalCode = PostalCode.text();

            item->lon   = strpos.section(" ", 0, 0).toFloat();
            item->lat   = strpos.section(" ", 1, 1).toFloat();
            item->setName(tmpResult.getName());
            results[item->getKey()] = item;
            theMainWindow->getCanvas()->move(item->lon, item->lat);
        }
    }
    else
    {
        status = tr("no result");
    }

    emit sigStatus(status);
    emit sigFinished();
    emitSigChanged();

}


void CSearchDB::slotRequestFinishedMapQuest(QByteArray& data)
{
    QDomDocument xml;
    QString status = tr("finished");

    xml.setContent(data);
    qDebug() << xml.toString();

    QDomElement root = xml.documentElement();

    emit sigStatus(status);
    emit sigFinished();
    emitSigChanged();

}


void CSearchDB::slotProxyAuthenticationRequired(const QNetworkProxy &prox, QAuthenticator *auth)
{
    QString user;
    QString pwd;

    CResources::self().getHttpProxyAuth(user,pwd);

    auth->setUser(user);
    auth->setPassword(pwd);
}


CSearch * CSearchDB::getResultByKey(const QString& key)
{
    if(!results.contains(key)) return 0;

    return results[key];
}


void CSearchDB::draw(QPainter& p, const QRect& rect, bool& needsRedraw)
{
    IMap& map = CMapDB::self().getMap();

    QMap<QString,CSearch*>::const_iterator result = results.begin();
    while(result != results.end())
    {
        double u = (*result)->lon * DEG_TO_RAD;
        double v = (*result)->lat * DEG_TO_RAD;
        map.convertRad2Pt(u,v);

        if(rect.contains(QPoint(u,v)))
        {
            p.drawPixmap(u-8 , v-8, QPixmap(":/icons/iconBullseye16x16.png"));
            CCanvas::drawText((*result)->getInfo(), p, QPoint(u, v - 10));
        }

        ++result;
    }
}


void CSearchDB::delResults(const QStringList& keys)
{

    QString key;
    foreach(key, keys)
    {
        results.remove(key);
    }

    emitSigChanged();

}


void CSearchDB::add(const QString& label, double lon, double lat)
{
    CSearch * item = new CSearch(this);
    item->lon   = lon * RAD_TO_DEG;
    item->lat   = lat * RAD_TO_DEG;
    item->setName(label);

    results[item->getKey()] = item;

    emitSigChanged();
}


void CSearchDB::selSearchByKey(const QString& key)
{
    CSearchToolWidget * t = qobject_cast<CSearchToolWidget*>(toolview);
    if(t)
    {
        t->selSearchByKey(key);
        gainFocus();
    }
}
