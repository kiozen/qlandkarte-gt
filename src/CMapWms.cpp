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

#include "CMapWms.h"
#include "CCanvas.h"
#include "CMainWindow.h"
#include "CResources.h"
#include "CDiskCache.h"
#include "CDlgMapWmsConfig.h"
#include "CMapSelectionRaster.h"
#include "CSettings.h"

#include <QtGui>
#include <QtXml>
#include <QtNetwork>
#include <QMessageBox>
#include <QCheckBox>
#include <QUrl>
#include <QStatusBar>

CMapWms::CMapWms(const QString &key, const QString &filename, CCanvas *parent)
: IMap(eWMS,key,parent)
, xsize_px(0)
, ysize_px(0)
, xscale(1.0)
, yscale(1.0)
, xref1(0)
, yref1(0)
, xref2(0)
, yref2(0)
, x(0)
, y(0)
, zoomFactor(1.0)
, status(0)
, quadraticZoom(0)
, needsRedrawOvl(true)
, lastTileLoaded(false)
, maxZoomLevel(-1)
{

    IMap::filename = filename;

    qDebug() << filename;

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

    QDomElement gdal        =  dom.firstChildElement("GDAL_WMS");
    QDomElement service     = gdal.firstChildElement("Service");
    QDomElement datawindow  = gdal.firstChildElement("DataWindow");

    name        = service.firstChildElement("Title").text();
    urlstr      = service.firstChildElement("ServerUrl").text();
    format      = service.firstChildElement("ImageFormat").text();
    layers      = service.firstChildElement("Layers").text();
    srs         = service.firstChildElement("SRS").text();
    version     = service.firstChildElement("Version").text();
    copyright   = service.firstChildElement("Copyright").text();
    projection  = gdal.firstChildElement("Projection").text().toLower();
    blockSizeX  = gdal.firstChildElement("BlockSizeX").text().toUInt();
    blockSizeY  = gdal.firstChildElement("BlockSizeY").text().toUInt();

    if(gdal.firstChildElement("MaxZoomLevel").isElement())
    {
        maxZoomLevel = gdal.firstChildElement("MaxZoomLevel").text().toInt();
    }

    if(srs.isEmpty())
    {
        srs = projection;
    }

    if(projection.isEmpty())
    {
        projection = srs;
    }

    projection = projection.toLower();
    if(projection.startsWith("epsg"))
    {
        QString str = QString("+init=%1").arg(projection);
        pjsrc = pj_init_plus(str.toLocal8Bit());
        qDebug() << "wms:" << str.toLocal8Bit();
    }
    else
    {
        pjsrc = pj_init_plus(projection.toLocal8Bit());
        qDebug() << "wms:" << projection.toLocal8Bit();
    }

    if(pjsrc == 0)
    {
        QMessageBox::critical(parent, tr("Error..."), tr("Unknown projection %1").arg(projection.toLatin1().data()), QMessageBox::Abort, QMessageBox::Abort);
        return;
    }
    oSRS.importFromProj4(getProjection());

    xsize_px    = datawindow.firstChildElement("SizeX").text().toInt();
    ysize_px    = datawindow.firstChildElement("SizeY").text().toInt();

    if (pj_is_latlong(pjsrc))
    {
        xref1   = datawindow.firstChildElement("UpperLeftX").text().toDouble()  * DEG_TO_RAD;
        yref1   = datawindow.firstChildElement("UpperLeftY").text().toDouble()  * DEG_TO_RAD;
        xref2   = datawindow.firstChildElement("LowerRightX").text().toDouble() * DEG_TO_RAD;
        yref2   = datawindow.firstChildElement("LowerRightY").text().toDouble() * DEG_TO_RAD;
    }
    else
    {
        xref1   = datawindow.firstChildElement("UpperLeftX").text().toDouble();
        yref1   = datawindow.firstChildElement("UpperLeftY").text().toDouble();
        xref2   = datawindow.firstChildElement("LowerRightX").text().toDouble();
        yref2   = datawindow.firstChildElement("LowerRightY").text().toDouble();
    }

    xscale      = (xref2 - xref1) / xsize_px;
    yscale      = (yref2 - yref1) / ysize_px;

    x           = xref1 + (xref2 - xref1) / 2;
    y           = yref1 + (yref2 - yref1) / 2;

    quadraticZoom = theMainWindow->getCheckBoxQuadraticZoom();

    status = new QLabel(theMainWindow->getCanvas());
    theMainWindow->statusBar()->insertPermanentWidget(0,status);

    accessManager = new QNetworkAccessManager(this);
    accessManager->setProxy(QNetworkProxy(QNetworkProxy::DefaultProxy));

    connect(accessManager,SIGNAL(finished(QNetworkReply*)),this,SLOT(slotRequestFinished(QNetworkReply*)));
    connect(accessManager, SIGNAL(proxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)),
        this, SLOT(slotProxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)));

    diskCache = new CDiskCache(false, this);

    SETTINGS;
    cfg.beginGroup("wms/maps");
    cfg.beginGroup(getKey());

    x = cfg.value("lon", x).toDouble();
    y = cfg.value("lat", y).toDouble();

    cfg.endGroup();
    cfg.endGroup();

    zoom(zoomidx);

}


CMapWms::~CMapWms()
{

    midU = rect.center().x();
    midV = rect.center().y();
    convertPt2Rad(midU, midV);

    if(pjsrc) pj_free(pjsrc);

    delete status;

    SETTINGS;
    cfg.beginGroup("wms/maps");
    cfg.beginGroup(getKey());

    cfg.setValue("lon", x);
    cfg.setValue("lat", y);

    cfg.endGroup();
    cfg.endGroup();

}


void CMapWms::convertPixel2M(double& u, double& v)
{
    u = xref1 + u * xscale;
    v = yref1 + v * yscale;
}


void CMapWms::convertM2Pixel(double& u, double& v)
{
    u = (u - xref1) / (xscale);
    v = (v - yref1) / (yscale);
}


void CMapWms::convertPt2M(double& u, double& v)
{
    u = x + u * xscale * zoomFactor;
    v = y + v * yscale * zoomFactor;
}


void CMapWms::convertM2Pt(double& u, double& v)
{
    u = floor((u - x) / (xscale * zoomFactor) + 0.5);
    v = floor((v - y) / (yscale * zoomFactor) + 0.5);
}


void CMapWms::convertPt2Pixel(double& u, double& v)
{
    convertPt2M(u,v);

    u = (u - xref1) / xscale;
    v = (v - yref1) / yscale;

    if(u < 0 || u > xsize_px)
    {
        u = -1;
        v = -1;
        return;
    }
    if(v < 0 || v > ysize_px)
    {
        u = -1;
        v = -1;
        return;
    }

}


void CMapWms::move(const QPoint& old, const QPoint& next)
{
    if(pjsrc == 0) return;
    needsRedraw     = true;
    needsRedrawOvl  = true;

    double xx = x, yy = y;
    convertM2Pt(xx, yy);

    // move top left point by difference
    xx += old.x() - next.x();
    yy += old.y() - next.y();

    convertPt2M(xx,yy);
    x = xx;
    y = yy;
    emit sigChanged();

    setAngleNorth();
}


void CMapWms::zoom(bool zoomIn, const QPoint& p0)
{
    qDebug() << "zoom" << zoomIn;

    projXY p1;
    if(pjsrc == 0) return;

    needsRedraw     = true;
    needsRedrawOvl  = true;

    // convert point to geo. coordinates
    p1.u = p0.x();
    p1.v = p0.y();
    convertPt2Rad(p1.u, p1.v);

    if(quadraticZoom->isChecked())
    {

        if(zoomidx > 1)
        {
            zoomidx = pow(2.0, ceil(log(zoomidx*1.0)/log(2.0)));
            zoomidx = zoomIn ? (zoomidx>>1) : (zoomidx<<1);
        }
        else
        {
            zoomidx += zoomIn ? -1 : 1;
        }
    }
    else
    {
        zoomidx += zoomIn ? -1 : 1;
    }

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
    blockSignals(false);
    emit sigChanged();
}


void CMapWms::zoom(double lon1, double lat1, double lon2, double lat2)
{
    if(pjsrc == 0) return;

    needsRedraw     = true;
    needsRedrawOvl  = true;

    double u[3];
    double v[3];
    double dU, dV;

    u[0] = lon1;
    v[0] = lat1;
    u[1] = lon2;
    v[1] = lat1;
    u[2] = lon1;
    v[2] = lat2;

    pj_transform(pjtar, pjsrc,3,0,u,v,0);
    dU = (u[1] - u[0]) / xscale;
    dV = (v[0] - v[2]) / yscale;

    int z1 = fabs(dU / size.width());
    int z2 = fabs(dV / size.height());

    zoomFactor = (z1 > z2 ? z1 : z2)  + 1;
    if(quadraticZoom->isChecked())
    {
        zoomFactor = zoomidx = pow(2.0, ceil(log(zoomFactor)/log(2.0)));
    }
    else
    {
        zoomidx = zoomFactor;
    }

    double u_ = lon1 + (lon2 - lon1)/2;
    double v_ = lat1 + (lat2 - lat1)/2;
    convertRad2Pt(u_,v_);
    move(QPoint(u_,v_), rect.center());

    emit sigChanged();

    qDebug() << "zoom:" << zoomFactor;
}


void CMapWms::zoom(qint32& level)
{
    if(pjsrc == 0) return;
    needsRedraw     = true;
    needsRedrawOvl  = true;

    if(maxZoomLevel > 0 && level > maxZoomLevel)
    {
        level = maxZoomLevel;
    }

    // no level less than 1
    if(level < 1)
    {
        zoomFactor  = 1.0 / - (level - 2);
        qDebug() << "zoom:" << zoomFactor;
        return;
    }
    zoomFactor = level;
    emit sigChanged();
    qDebug() << "zoom:" << zoomFactor;
}


void CMapWms::dimensions(double& lon1, double& lat1, double& lon2, double& lat2)
{
    if(pjsrc == 0) return;

    lon1 = xref1;
    lat1 = yref1;
    pj_transform(pjsrc,pjtar,1,0,&lon1,&lat1,0);

    lon2 = xref2;
    lat2 = yref2;
    pj_transform(pjsrc,pjtar,1,0,&lon2,&lat2,0);
}


void CMapWms::getArea_n_Scaling(projXY& p1, projXY& p2, float& my_xscale, float& my_yscale)
{
    if(pjsrc == 0) return;

    p1.u        = 0;
    p1.v        = 0;
    p2.u        = rect.width();
    p2.v        = rect.height();

    convertPt2Rad(p1.u, p1.v);
    convertPt2Rad(p2.u, p2.v);

    my_xscale   = xscale * zoomFactor;
    my_yscale   = yscale * zoomFactor;

}


void CMapWms::draw(QPainter& p)
{
    if(pjsrc == 0) return IMap::draw(p);

    // render map if necessary
    if(needsRedraw)
    {
        draw();
    }

    p.drawPixmap(0,0,pixBuffer);

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
        CCanvas::drawText(tr("%1 %2").arg(QChar(0x00A9)).arg(tr("Copyright notice is missing. Use <copyright> tag in <service> secton to supply a copyright notice.")), p, rect.bottomLeft() + QPoint(rect.width() / 2, -5) , QColor(Qt::darkBlue));
    }
    else
    {
        CCanvas::drawText(tr("%1 %2").arg(QChar(0x00A9)).arg(copyright), p, rect.bottomLeft() + QPoint(rect.width() / 2, -5) , QColor(Qt::darkBlue));
    }

}


void CMapWms::draw()
{
    QImage img;
    lastTileLoaded = false;

    pixBuffer.fill(Qt::white);
    QPainter p(&pixBuffer);

    // convert top/left corner to abs. pixel in map
    double x1 = 0;
    double y1 = 0;
    convertPt2M(x1, y1);
    convertM2Pixel(x1, y1);

    // convert bottom/right corner to abs. pixel in map
    double x2 = rect.width();
    double y2 = rect.height();
    convertPt2M(x2, y2);
    convertM2Pixel(x2, y2);

    // quantify to smalles multiple of blocksize
    x1 = floor(x1/(blockSizeX * zoomFactor)) * blockSizeX * zoomFactor;
    y1 = floor(y1/(blockSizeY * zoomFactor)) * blockSizeY * zoomFactor;

    int n = 0;
    int m = 0;

    double cx;
    double cy;

    do
    {
        do
        {
            double p1x = x1 + n * blockSizeX * zoomFactor;
            double p1y = y1 + m * blockSizeY * zoomFactor;
            double p2x = x1 + (n + 1) * blockSizeX * zoomFactor;
            double p2y = y1 + (m + 1) * blockSizeY * zoomFactor;

            cx = p2x;
            cy = p2y;

            convertPixel2M(p1x, p1y);
            convertPixel2M(p2x, p2y);

            QUrl url(urlstr);
#ifdef QK_QT5_PORT
            QUrlQuery urlQuery;
            urlQuery.addQueryItem("request", "GetMap");
            urlQuery.addQueryItem("version", version);
            urlQuery.addQueryItem("layers", layers);
            urlQuery.addQueryItem("styles", "");
            urlQuery.addQueryItem("srs", srs);
            urlQuery.addQueryItem("format", format);
            urlQuery.addQueryItem("width", QString::number(blockSizeX));
            urlQuery.addQueryItem("height", QString::number(blockSizeY));
#else
            url.addQueryItem("request", "GetMap");
            url.addQueryItem("version", version);
            url.addQueryItem("layers", layers);
            url.addQueryItem("styles", "");
            url.addQueryItem("srs", srs);
            url.addQueryItem("format", format);
            url.addQueryItem("width", QString::number(blockSizeX));
            url.addQueryItem("height", QString::number(blockSizeY));
#endif

            if(pj_is_latlong(pjsrc))
            {
#ifdef QK_QT5_PORT
                urlQuery.addQueryItem("bbox", QString("%1,%2,%3,%4").arg(p1x*RAD_TO_DEG,0,'f').arg(p2y*RAD_TO_DEG,0,'f').arg(p2x*RAD_TO_DEG,0,'f').arg(p1y*RAD_TO_DEG,0,'f'));
#else
                url.addQueryItem("bbox", QString("%1,%2,%3,%4").arg(p1x*RAD_TO_DEG,0,'f').arg(p2y*RAD_TO_DEG,0,'f').arg(p2x*RAD_TO_DEG,0,'f').arg(p1y*RAD_TO_DEG,0,'f'));
#endif
            }
            else
            {
#ifdef QK_QT5_PORT
                urlQuery.addQueryItem("bbox", QString("%1,%2,%3,%4").arg(p1x,0,'f').arg(p2y,0,'f').arg(p2x,0,'f').arg(p1y,0,'f'));
#else
                url.addQueryItem("bbox", QString("%1,%2,%3,%4").arg(p1x,0,'f').arg(p2y,0,'f').arg(p2x,0,'f').arg(p1y,0,'f'));
#endif
            }

#ifdef QK_QT5_PORT
            url.setQuery(urlQuery);
#endif

//            qDebug() << url;
            request_t req;
            req.url         = url;
            req.lon         = p1x;
            req.lat         = p1y;
            req.zoomFactor  = zoomFactor;
            convertM2Rad(req.lon,req.lat);

            diskCache->restore(url.toString(), img);
            if(!img.isNull())
            {
                convertRad2Pt(req.lon,req.lat);
                p.drawImage(req.lon, req.lat,img);
            }
            else
            {
                addToQueue(req);
            }

            n++;
        }
        while(cx < x2);

        n = 0;
        m++;
    }
    while(cy < y2);

    checkQueue();
}


void CMapWms::addToQueue(request_t& req)
{
    if(!seenRequest.contains(req.url.toString()))
    {
        newRequests.enqueue(req);
        seenRequest << req.url.toString();
    }
}


void CMapWms::checkQueue()
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


void CMapWms::slotRequestFinished(QNetworkReply* reply)
{

    QString _url_ = reply->url().toString();
    if(pendRequests.contains(_url_))
    {
        request_t req = pendRequests[_url_];

        QString urlRedir = reply->header(QNetworkRequest::LocationHeader).toString();
        if(!urlRedir.isEmpty())
        {

            QUrl url(urlRedir);

            QNetworkRequest request;

            request.setUrl(url);
            req.reply = accessManager->get(request);

            pendRequests[url.toString()] = req;

        }
        else
        {
            QImage img;
            QPainter p(&pixBuffer);

            QByteArray buf = reply->readAll();
//            qDebug() << buf;

            // only take good responses
            if(!reply->error())
            {
                // read image data
                img.loadFromData(buf);
            }

            // always store image to cache, the cache will take care of NULL images
            diskCache->store(req.url.toString(), img);

            // only paint image if on current zoom factor
            if((req.zoomFactor == zoomFactor))
            {
                convertRad2Pt(req.lon, req.lat);
                p.drawImage(req.lon, req.lat, img);
            }
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


void CMapWms::slotProxyAuthenticationRequired(const QNetworkProxy &prox, QAuthenticator *auth)
{
    QString user;
    QString pwd;

    CResources::self().getHttpProxyAuth(user,pwd);

    auth->setUser(user);
    auth->setPassword(pwd);
}


void CMapWms::config()
{
    CDlgMapWmsConfig dlg(*this);
    dlg.exec();
}


quint32 CMapWms::scalePixelGrid(quint32 nPixel)
{
    return double(nPixel) / zoomFactor;
}


void CMapWms::select(IMapSelection& ms, const QRect& rect)
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
