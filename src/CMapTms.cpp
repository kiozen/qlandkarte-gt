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

#include "CMapTms.h"
#include "CMapDB.h"
#include "GeoMath.h"
#include "CResources.h"
#include "CDiskCache.h"
#include "CDiskCacheZip.h"
#include "CMainWindow.h"
#include "CDlgMapTmsConfig.h"
#include "CMapSelectionRaster.h"
#include "CSettings.h"

#include <QtGui>
#include <QtNetwork>
#include <QtXml>
#include <QtScript>
#include <QMessageBox>
#include <QCheckBox>
#include <QStatusBar>

#include <iostream>

CMapTms::CMapTms(const QString& key, CCanvas *parent)
: IMap(eTMS,key,parent)
, zoomFactor(1.0)
, x(0)
, y(0)
, xscale( 1.19432854652)
, yscale(-1.19432854652)
, needsRedrawOvl(true)
, lastTileLoaded(false)
, status(0)
, minZoomLevel(1)
, maxZoomLevel(18)
{
    SETTINGS;

    CMapDB::map_t mapData = CMapDB::self().getMapData(key);

    if(key.endsWith(".tms"))
    {
        readConfigFromFile(key, parent);
    }
    else
    {

        copyright   = mapData.copyright;
        layers.resize(1);
        layers[0].strUrl        = mapData.filename;
        layers[0].minZoomLevel  = minZoomLevel;
        layers[0].maxZoomLevel  = maxZoomLevel;
    }

    pjsrc = pj_init_plus("+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext +no_defs");
    oSRS.importFromProj4(getProjection());

    char * ptr = pj_get_def(pjsrc,0);
    qDebug() << "tms:" << ptr;

    x       = cfg.value("tms/lon", 12.098133).toDouble() * DEG_TO_RAD;
    y       = cfg.value("tms/lat", 49.019233).toDouble() * DEG_TO_RAD;
    zoomidx = cfg.value("tms/zoomidx",15).toInt();

    lon1 = xref1   = -40075016/2;
    lat1 = yref1   =  40075016/2;
    lon2 = xref2   =  40075016/2;
    lat2 = yref2   = -40075016/2;
    pj_transform(pjsrc,pjtar,1,0,&lon1,&lat1,0);
    pj_transform(pjsrc,pjtar,1,0,&lon2,&lat2,0);

    accessManager = new QNetworkAccessManager(this);
    accessManager->setProxy(QNetworkProxy(QNetworkProxy::DefaultProxy));

    connect(accessManager,SIGNAL(finished(QNetworkReply*)),this,SLOT(slotRequestFinished(QNetworkReply*)));
    connect(accessManager, SIGNAL(proxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)),
        this, SLOT(slotProxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)));

    if ( layers[0].strUrl.startsWith("file") )
    {
        diskCache = new CDiskCacheZip(this);
    }
    else
    {
        diskCache = new CDiskCache(true, this);
    }

    status = new QLabel(theMainWindow->getCanvas());
    theMainWindow->statusBar()->insertPermanentWidget(0,status);
    theMainWindow->getCheckBoxQuadraticZoom()->hide();

    zoom(zoomidx);

    if(parent)
    {
        resize(parent->size());
    }

}


CMapTms::~CMapTms()
{
    QString pos;
    SETTINGS;

    cfg.setValue("tms/lon", x * RAD_TO_DEG);
    cfg.setValue("tms/lat", y * RAD_TO_DEG);
    cfg.setValue("tms/zoomidx",zoomidx);

    midU = rect.center().x();
    midV = rect.center().y();
    convertPt2Rad(midU, midV);

    if(pjsrc) pj_free(pjsrc);

    delete status;

    theMainWindow->getCheckBoxQuadraticZoom()->show();
}


void CMapTms::readConfigFromFile(const QString& filename, QWidget *parent)
{
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(parent, tr("Error..."), tr("Failed to open %1").arg(filename), QMessageBox::Abort, QMessageBox::Abort);
        return;
    }

    QString msg;
    int line, column;
    QDomDocument dom;
    if(!dom.setContent(&file, true, &msg, &line, &column))
    {
        file.close();
        QMessageBox::critical(parent, tr("Error..."), tr("Failed to read: %1\nline %2, column %3:\n %4").arg(filename).arg(line).arg(column).arg(msg), QMessageBox::Abort, QMessageBox::Abort);
        return;
    }
    file.close();

    QDomElement tms =  dom.firstChildElement("TMS");
    name            = tms.firstChildElement("Title").text();
    copyright       = tms.firstChildElement("Copyright").text();

    if(tms.firstChildElement("MaxZoomLevel").isElement())
    {
        maxZoomLevel = tms.firstChildElement("MaxZoomLevel").text().toInt();
    }

    if(tms.firstChildElement("MinZoomLevel").isElement())
    {
        minZoomLevel = tms.firstChildElement("MinZoomLevel").text().toInt();
    }

    const QDomNodeList& layerList = tms.elementsByTagName("Layer");
    uint N = layerList.count();
    layers.resize(N);

    for(uint n = 0; n < N; ++n)
    {
        const QDomNode& layerSingle = layerList.item(n);
        int idx = layerSingle.attributes().namedItem("idx").nodeValue().toInt();
        layers[idx].strUrl          = layerSingle.namedItem("ServerUrl").toElement().text();
        layers[idx].script          = layerSingle.namedItem("Script").toElement().text();
        layers[idx].minZoomLevel    = minZoomLevel;
        layers[idx].maxZoomLevel    = maxZoomLevel;

        if(layerSingle.firstChildElement("MinZoomLevel").isElement())
        {
            layers[idx].minZoomLevel = layerSingle.firstChildElement("MinZoomLevel").text().toInt();
        }

        if(layerSingle.firstChildElement("MaxZoomLevel").isElement())
        {
            layers[idx].maxZoomLevel = layerSingle.firstChildElement("MaxZoomLevel").text().toInt();
        }
    }

    const QDomElement& rawHeader    = tms.firstChildElement("RawHeader");
    const QDomNodeList& valueList   = rawHeader.elementsByTagName("Value");
    N = valueList.count();
    for(uint n = 0; n < N; ++n)
    {
        rawHeaderItem_t item;
        const QDomNode& valueSingle = valueList.item(n);
        item.name  = valueSingle.attributes().namedItem("name").nodeValue();
        item.value = valueSingle.toElement().text();
        rawHeaderItems << item;
    }

}


void CMapTms::resize(const QSize& size)
{
    IMap::resize(size);

    for(int i = 0; i < layers.size(); i++)
    {
        layer_t& layer = layers[i];
        layer.buffer = QPixmap(size);
    }
}


void CMapTms::convertPt2M(double& u, double& v)
{
    u = x + u * xscale * zoomFactor;
    v = y + v * yscale * zoomFactor;
}


void CMapTms::convertM2Pt(double& u, double& v)
{
    u = floor((u - x) / (xscale * zoomFactor) + 0.5);
    v = floor((v - y) / (yscale * zoomFactor) + 0.5);
}


void CMapTms::move(const QPoint& old, const QPoint& next)
{
    double xx = x, yy = y;
    convertM2Pt(xx, yy);

    // move top left point by difference
    xx += old.x() - next.x();
    yy += old.y() - next.y();

    convertPt2M(xx,yy);
    x = xx;
    y = yy;
    needsRedraw     = true;
    needsRedrawOvl  = true;

    setAngleNorth();
    emit sigChanged();
}


void CMapTms::zoom(bool zoomIn, const QPoint& p0)
{
    projXY p1;

    // convert point to geo. coordinates
    p1.u = p0.x();
    p1.v = p0.y();
    convertPt2Rad(p1.u, p1.v);

    zoomidx += zoomIn ? -1 : 1;
    // sigChanged will be sent at the end of this function
    blockSignals(true);
    zoom(zoomidx);

    // convert geo. coordinates back to point
    convertRad2Pt(p1.u, p1.v);

    double xx = x, yy = y;
    convertM2Pt(xx, yy);

    // move top left point by difference point befor and after zoom
    xx += p1.u - p0.x();
    yy += p1.v - p0.y();

    // convert back to new top left geo coordinate
    convertPt2M(xx, yy);
    x = xx;
    y = yy;

    needsRedraw     = true;
    needsRedrawOvl  = true;
    blockSignals(false);
    emit sigChanged();
}


void CMapTms::zoom(double lon1, double lat1, double lon2, double lat2)
{
    double u[3];
    double v[3];
    double dU, dV;
    int i;

    u[0] = lon1;
    v[0] = lat1;
    u[1] = lon2;
    v[1] = lat1;
    u[2] = lon1;
    v[2] = lat2;

    pj_transform(pjtar, pjsrc,3,0,u,v,0);
    dU = fabs(u[1] - u[0]);
    dV = fabs(v[0] - v[2]);

    int z1 = dU / size.width();
    int z2 = dV / size.height();

    for(i=minZoomLevel-1; i < maxZoomLevel; ++i)
    {
        zoomFactor  = (1<<i);
        zoomidx     = i + 1;
        if(zoomFactor > z1 && zoomFactor > z2) break;
    }

    double u_ = lon1 + (lon2 - lon1)/2;
    double v_ = lat1 + (lat2 - lat1)/2;
    convertRad2Pt(u_,v_);
    move(QPoint(u_,v_), rect.center());

    needsRedraw     = true;
    needsRedrawOvl  = true;
    emit sigChanged();

    qDebug() << "zoom:" << zoomFactor;
}


void CMapTms::zoom(qint32& level)
{
    if(level > maxZoomLevel)
    {
        level = maxZoomLevel;
    }
    // no level less than 1
    if(level < minZoomLevel)
    {
        level = minZoomLevel;
    }
    zoomFactor = (1<<(level-1));
    needsRedraw     = true;
    needsRedrawOvl  = true;

    emit sigChanged();
}


void CMapTms::dimensions(double& lon1, double& lat1, double& lon2, double& lat2)
{
    lon1 = this->lon1;
    lat1 = this->lat1;
    lon2 = this->lon2;
    lat2 = this->lat2;
}


void CMapTms::getArea_n_Scaling(projXY& p1, projXY& p2, float& my_xscale, float& my_yscale)
{
    p1.u        = 0;
    p1.v        = 0;
    p2.u        = rect.width();
    p2.v        = rect.height();

    convertPt2Rad(p1.u, p1.v);
    convertPt2Rad(p2.u, p2.v);

    my_xscale   = xscale * zoomFactor;
    my_yscale   = yscale * zoomFactor;
}


void CMapTms::draw(QPainter& p)
{
    if(pjsrc == 0) return IMap::draw(p);

    // render map if necessary
    if(needsRedraw)
    {
        draw();
    }

    //p.drawPixmap(0,0,pixBuffer);
    for(int i = 0; i < layers.size(); i++)
    {
        layer_t& layer = layers[i];
        p.drawPixmap(0,0,layer.buffer);
    }

    // render overlay
    if(!ovlMap.isNull() && lastTileLoaded && !doFastDraw)
    {
        ovlMap->draw(size, needsRedrawOvl, p);
        needsRedrawOvl = false;
    }

    needsRedraw = false;

    if(CResources::self().showZoomLevel())
    {

        QString str;
        if(zoomFactor < 1.0)
        {
            str = tr("Overzoom x%1").arg(1/zoomFactor,0,'f',0);
        }
        else
        {
            str = tr("Zoom level x%1").arg(zoomFactor);
        }

        p.setPen(Qt::white);
        p.setFont(QFont("Sans Serif",14,QFont::Black));

        p.drawText(9  ,23, str);
        p.drawText(10 ,23, str);
        p.drawText(11 ,23, str);
        p.drawText(9  ,24, str);
        p.drawText(11 ,24, str);
        p.drawText(9  ,25, str);
        p.drawText(10 ,25, str);
        p.drawText(11 ,25, str);

        p.setPen(Qt::darkBlue);
        p.drawText(10,24,str);
    }

    p.setFont(QFont("Sans Serif",8,QFont::Black));
    if(copyright.isEmpty())
    {
        CCanvas::drawText(tr("%1 %2").arg(QChar(0x00A9)).arg(tr("Copyright notice is missing.")), p, rect.bottomLeft() + QPoint(rect.width() / 2, -5) , QColor(Qt::darkBlue));
    }
    else
    {
        CCanvas::drawText(tr("%1 %2").arg(QChar(0x00A9)).arg(copyright), p, rect.bottomLeft() + QPoint(rect.width() / 2, -5) , QColor(Qt::darkBlue));
    }

}


QString CMapTms::createUrl(const layer_t& layer, int x, int y, int z)
{
    if(layer.strUrl.startsWith("script"))
    {

        QString filename = layer.strUrl.mid(9);
        QFile scriptFile(filename);
        if (!scriptFile.open(QIODevice::ReadOnly))
        {
            return "";
        }
        QTextStream stream(&scriptFile);
        QString contents = stream.readAll();
        scriptFile.close();

        QScriptEngine engine;
        QScriptValue fun = engine.evaluate(contents, filename);

        if(engine.hasUncaughtException())
        {
            int line = engine.uncaughtExceptionLineNumber();
            qDebug() << "uncaught exception at line" << line << ":" << fun.toString();
        }

        QScriptValueList args;
        args << z << x << y;
        QScriptValue res = fun.call(QScriptValue(), args);

        //        qDebug() << "yyy" << res.toString();

        return res.toString();
    }
    else if(!layer.script.isEmpty())
    {
        QScriptEngine engine;
        QScriptValue fun = engine.evaluate(layer.script);
        QScriptValueList args;
        args << z << x << y;
        QScriptValue res = fun.call(QScriptValue(), args);

        //        qDebug() << "xxx" << res.toString();

        return res.toString();

    }

    return layer.strUrl.arg(z).arg(x).arg(y);
}


void CMapTms::draw()
{
    if(pjsrc == 0) return IMap::draw();

    QImage img;
    lastTileLoaded  = false;

    for(int i = 0; i < layers.size(); i++)
    {
        layer_t& layer = layers[i];

        layer.buffer.fill(Qt::transparent);
    }

    int z      = 18 - zoomidx;
    double lon = x;
    double lat = y;
    convertM2Rad(lon, lat);

    lon *= RAD_TO_DEG;
    lat *= RAD_TO_DEG;

    int x1      = lon2tile(lon, z) / 256;
    int y1      = lat2tile(lat, z) / 256;
    double xx1  = tile2lon(x1, z) * DEG_TO_RAD;
    double yy1  = tile2lat(y1, z) * DEG_TO_RAD;
    convertRad2Pt(xx1, yy1);

    int n = 0;
    int m = 0;

    double cx;
    double cy;

    do
    {
        do
        {
            double p1x = xx1 + n * 256;
            double p1y = yy1 + m * 256;
            double p2x = xx1 + (n + 1) * 256;
            double p2y = yy1 + (m + 1) * 256;

            cx = p2x;
            cy = p2y;

            convertPt2Rad(p1x, p1y);

            for(int i = 0; i < layers.size(); i++)
            {
                layer_t& layer = layers[i];

                if(zoomidx < layer.minZoomLevel || zoomidx > layer.maxZoomLevel)
                {
                    continue;
                }

                request_t req;
                req.url         = QUrl(createUrl(layer, x1 + n, y1 + m, z));
                req.lon         = p1x;
                req.lat         = p1y;
                req.zoomFactor  = zoomFactor;
                req.layer       = i;

                diskCache->restore(req.url.toString(), img);
                if(!img.isNull())
                {
                    convertRad2Pt(req.lon,req.lat);
                    QPainter p;
                    p.begin(&layer.buffer);
                    p.drawImage(req.lon, req.lat,img);
                    p.end();
                }
                else
                {
                    addToQueue(req);
                }
            }
            n++;
        }
        while(cx < rect.width());

        n = 0;
        m++;
    }
    while(cy < rect.height());

    checkQueue();
}


void CMapTms::addToQueue(request_t& req)
{
    if(!seenRequest.contains(req.url.toString()))
    {
        newRequests.enqueue(req);
        seenRequest << req.url.toString();
    }
}


void CMapTms::checkQueue()
{
    while(newRequests.size() && pendRequests.size() < 6)
    {
        request_t req = newRequests.dequeue();

        if(diskCache->contains(req.url.toString()) || (req.zoomFactor != zoomFactor))
        {
            checkQueue();
            return;
        }

        QNetworkRequest request;

        request.setUrl(req.url);
        foreach(const rawHeaderItem_t& item, rawHeaderItems)
        {
            request.setRawHeader(item.name.toLatin1(), item.value.toLatin1());
        }

        req.reply = accessManager->get(request);

        pendRequests[req.url.toString()] = req;
    }

    if(pendRequests.isEmpty() && newRequests.isEmpty())
    {
        status->setText(tr("Map loaded."));
        lastTileLoaded = true;
        seenRequest.clear();
    }
    else
    {
        status->setText(tr("Wait for %1 tiles.").arg(pendRequests.size() + newRequests.size()));
    }
}


void CMapTms::slotRequestFinished(QNetworkReply* reply)
{
    QString _url_ = reply->url().toString();

    if(pendRequests.contains(_url_))
    {
        QImage img;

        request_t& req = pendRequests[_url_];

        // only take good responses
        if(!reply->error())
        {
            // read image data
            img.loadFromData(reply->readAll());
        }

        // always store image to cache, the cache will take care of NULL images
        diskCache->store(_url_, img);

        // only paint image if on current zoom factor
        if((req.zoomFactor == zoomFactor))
        {
            convertRad2Pt(req.lon, req.lat);
            QPainter p;
            p.begin(&layers[req.layer].buffer);
            p.drawImage(req.lon, req.lat, img);
            p.end();
        }

        // pending request finished
        pendRequests.remove(_url_);
    }

    // debug output any error
    if(reply->error())
    {
        qDebug() << reply->errorString();
    }

    // delete reply object
    reply->deleteLater();

    // check for more requests
    checkQueue();

    // the map did change
    emit sigChanged();
}


void CMapTms::slotProxyAuthenticationRequired(const QNetworkProxy &prox, QAuthenticator *auth)
{
    QString user;
    QString pwd;

    CResources::self().getHttpProxyAuth(user,pwd);

    auth->setUser(user);
    auth->setPassword(pwd);
}


void CMapTms::config()
{
    CDlgMapTmsConfig dlg(*this);
    dlg.exec();
}


quint32 CMapTms::scalePixelGrid(quint32 nPixel)
{
    return double(nPixel) / zoomFactor;
}


void CMapTms::select(IMapSelection& ms, const QRect& rect)
{
    if(ms.type != IMapSelection::eRaster) return;

    CMapSelectionRaster& sel = (CMapSelectionRaster&)ms;

    sel.lon1 = rect.left();
    sel.lat1 = rect.top();
    convertPt2Rad(sel.lon1, sel.lat1);

    sel.lon2 = rect.right();
    sel.lat2 = rect.bottom();
    convertPt2Rad(sel.lon2, sel.lat2);
}
