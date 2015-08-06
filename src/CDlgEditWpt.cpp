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

#include "CDlgEditWpt.h"
#include "CWpt.h"
#include "CWptDB.h"
#include "WptIcons.h"
#include "GeoMath.h"
#include "CDlgWptIcon.h"
#include "CResources.h"
#include "CMapDB.h"
#include "IMap.h"
#include "IUnit.h"
#include "CSettings.h"

#include "config.h"

#include "CImageViewer.h"

#include <QtGui>
#include <QtNetwork>
#ifndef QK_QT5_PORT
#include <tzdata.h>
#endif
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>

CDlgEditWpt::CDlgEditWpt(CWpt &wpt, QWidget * parent)
: QDialog(parent)
, wpt(wpt)
, idxImg(0)
#ifdef HAS_DMTX
, enc(0)
#endif
, silentSpoilerQuery(false)
{
    setupUi(this);
    connect(pushAdd, SIGNAL(clicked()), this, SLOT(slotAddImage()));
    connect(pushDel, SIGNAL(clicked()), this, SLOT(slotDelImage()));
    connect(pushNext, SIGNAL(clicked()), this, SLOT(slotNextImage()));
    connect(pushPrev, SIGNAL(clicked()), this, SLOT(slotPrevImage()));
    connect(toolIcon, SIGNAL(clicked()), this, SLOT(slotSelectIcon()));
    connect(pushSaveBarcode, SIGNAL(clicked()), this, SLOT(slotSaveBarcode()));
    connect(pushUpdateBarcode, SIGNAL(clicked()), this, SLOT(slotUpdateBarcode()));
    connect(labelLink, SIGNAL(linkActivated(const QString&)),this,SLOT(slotOpenLink(const QString&)));
    connect(toolLink, SIGNAL(pressed()),this,SLOT(slotEditLink()));
    connect(checkHint, SIGNAL(toggled(bool)), this, SLOT(slotToggleHint(bool)));
    connect(pushCreateBuddies, SIGNAL(clicked()), this, SLOT(slotCreateBuddies()));
    connect(checkTransparent, SIGNAL(toggled(bool)), this, SLOT(slotTransparent(bool)));
    connect(pushSpoiler, SIGNAL(clicked()), this, SLOT(slotCollectSpoiler()));
    connect(toolEditInfo, SIGNAL(clicked()), this, SLOT(slotEditInfo()));

    labelUnitElevation->setText(IUnit::self().baseunit);
    labelUnitProximity->setText(IUnit::self().baseunit);

    connect(webView, SIGNAL(linkClicked( const QUrl&)), this, SLOT(slotOpenLink(const QUrl&)));

    if(wpt.isGeoCache())
    {
        toolIcon->setEnabled(false);
        lineName->setEnabled(false);
        linePosition->setEnabled(false);
        tabWidget->setCurrentIndex(1);
    }

#ifdef HAS_DMTX
    enc = dmtxEncodeCreate();
#endif
    SETTINGS;
    checkTransparent->setChecked(cfg.value("waypoint/transparent", false).toBool());

    imageSelect->setTransparent(true);
    name = wpt.getName();
    imageSelect->setWpt(&wpt);
    connect(imageSelect, SIGNAL(sigSelectImage(const CImageSelect::img_t&)), this, SLOT(slotSelectImage(const CImageSelect::img_t&)));

    if(wpt.isGeoCache())
    {
        labelSpolerHint->show();
        pushSpoiler->show();
        imageSelect->hide();
        checkTransparent->hide();
    }
    else
    {
        labelSpolerHint->hide();
        pushSpoiler->hide();
        imageSelect->show();
        checkTransparent->show();
    }

    labelImage->installEventFilter(this);

    networkAccessManager = new QNetworkAccessManager(this);
    networkAccessManager->setProxy(QNetworkProxy(QNetworkProxy::DefaultProxy));
    connect(networkAccessManager, SIGNAL(proxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)),this, SLOT(slotProxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)));
    connect(networkAccessManager,SIGNAL(finished(QNetworkReply*)),this,SLOT(slotRequestFinished(QNetworkReply*)));

}


CDlgEditWpt::~CDlgEditWpt()
{
    wpt.showBuddies(false);
#ifdef HAS_DMTX
    dmtxEncodeDestroy(&enc);
#endif

    SETTINGS;
    cfg.setValue("waypoint/transparent", checkTransparent->isChecked());

}


int CDlgEditWpt::exec()
{

    QString val, unit;
    toolIcon->setIcon(wpt.getIcon());
    toolIcon->setObjectName(wpt.getIconString());

    lineName->setText(wpt.getName());

    checkSticky->setChecked(wpt.sticky);

    QString pos;
    GPS_Math_Deg_To_Str(wpt.lon, wpt.lat, pos);
    linePosition->setText(pos);

    oldLon = wpt.lon;
    oldLat = wpt.lat;

    if(wpt.ele != WPT_NOFLOAT)
    {
        IUnit::self().meter2elevation(wpt.ele, val, unit);
        lineAltitude->setText(val);
    }
    if(wpt.prx != WPT_NOFLOAT)
    {
        IUnit::self().meter2elevation(wpt.prx, val, unit);
        lineProximity->setText(val);
    }

    if(wpt.images.count() != 0)
    {
        showImage(0);
        pushDel->setEnabled(true);
    }

    link = wpt.link;

    if(!link.isEmpty())
    {
        QString str;

        if(wpt.urlname.isEmpty())
        {

            if(link.count() < 50)
            {
                str = "<a href='" + link + "'>" + link + "</a>";
            }
            else
            {
                str = "<a href='" + link + "'>" + link.left(47) + "...</a>";
            }
        }
        else
        {
            str = "<a href='" + link + "'>" + wpt.urlname + "</a>";
        }
        labelLink->setText(str);
    }

    textComment->setHtml(wpt.getComment());
    lineDesc->setText(wpt.getDescription());

    slotUpdateBarcode();

    QString html = wpt.getExtInfo(checkHint->isChecked());

    if(wpt.isGeoCache())
    {
        //        checkExportBuddies->setChecked(wpt.geocache.exportBuddies);

        wpt.showBuddies(true);

        if(wpt.buddies.isEmpty())
        {
            listBuddies->hide();
            //            checkExportBuddies->hide();
            pushCreateBuddies->hide();
        }
        else
        {
            CWpt::buddy_t buddy;
            foreach(buddy, wpt.buddies)
            {
                QListWidgetItem * item = new QListWidgetItem();
                item->setText(buddy.name);
                item->setToolTip(*buddy.pos.begin());
                listBuddies->addItem(item);

                foreach(const QString& pos, buddy.pos)
                {
                    html.replace(pos, QString("%1 (<b><i style='color: black;'>%2</i></b>)").arg(pos).arg(buddy.name));
                }
            }
            listBuddies->sortItems();
            listBuddies->show();
            //            checkExportBuddies->show();
            pushCreateBuddies->show();
        }

        webView->setHtml(html);
        webView->page()->setLinkDelegationPolicy( QWebPage::DelegateAllLinks );
        checkHint->setEnabled(wpt.hasHiddenInformation());

    }
    else
    {
        listBuddies->hide();
        //        checkExportBuddies->hide();
        pushCreateBuddies->hide();
        checkHint->hide();

        if(!link.isEmpty())
        {
            webView->setUrl(link);
        }
    }

    QStringList caches;
    CWptDB::self().getListOfGeoCaches(caches);
    caches.prepend("");
    comboParentWpt->addItems(caches);

    if(wpt.parentWpt.isEmpty())
    {
        comboParentWpt->setCurrentIndex(0);
    }
    else
    {
        int idx = comboParentWpt->findText(wpt.parentWpt);
        if(idx < 0)
        {
            comboParentWpt->insertItem(1, wpt.parentWpt);
            comboParentWpt->setCurrentIndex(1);
        }
        else
        {
            comboParentWpt->setCurrentIndex(idx);
        }

    }

    lineName->setFocus();

    if(wpt.isGeoCache() && !wpt.link.isEmpty() && wpt.images.isEmpty())
    {
        silentSpoilerQuery = true;
        triggerSpoilerDownload();
    }

    return QDialog::exec();
}


void CDlgEditWpt::accept()
{
    if(lineName->text().isEmpty())
    {
        QMessageBox::warning(0,tr("Error"),tr("You must provide a waypoint identifier."),QMessageBox::Ok,QMessageBox::NoButton);
        return;
    }
    if(linePosition->text().isEmpty())
    {
        QMessageBox::warning(0,tr("Error"),tr("You must provide a waypoint position."),QMessageBox::Ok,QMessageBox::NoButton);
        return;
    }

    if(!GPS_Math_Str_To_Deg(linePosition->text(), wpt.lon, wpt.lat))
    {
        return;
    }
    wpt.setIcon(toolIcon->objectName());
    wpt.setName(lineName->text());
    wpt.setDescription(lineDesc->text());
    wpt.setComment(textComment->toHtml());
    wpt.sticky      = checkSticky->isChecked();

    wpt.ele         = lineAltitude->text().isEmpty() ? WPT_NOFLOAT : IUnit::self().elevation2meter(lineAltitude->text());

    // change elevation if position has changed and DEM data is present
    if(oldLon != wpt.lon || oldLat != wpt.lat)
    {
        float ele = CMapDB::self().getDEM().getElevation(wpt.lon * DEG_TO_RAD, wpt.lat * DEG_TO_RAD);
        if(ele != WPT_NOFLOAT) wpt.ele = ele;
    }

    wpt.prx         = lineProximity->text().isEmpty() ? WPT_NOFLOAT : IUnit::self().elevation2meter(lineProximity->text());
    wpt.link        = link;

    if(!lineDistance->text().isEmpty() && !lineBearing->text().isEmpty())
    {
        double bearing  = lineBearing->text().toDouble() * DEG_TO_RAD;
        double distance = lineDistance->text().toDouble();

        projXY pt1, pt2;
        pt1.u       = wpt.lon * DEG_TO_RAD;
        pt1.v       = wpt.lat * DEG_TO_RAD;
        pt2         = GPS_Math_Wpt_Projection(pt1, distance, bearing);

        CWpt * wpt2 = new CWpt(&CWptDB::self());
        wpt2->lon   = pt2.u * RAD_TO_DEG;
        wpt2->lat   = pt2.v * RAD_TO_DEG;
        wpt2->setName(wpt.name + tr("(proj.)"));
        wpt2->setIcon(wpt.iconString);

        float ele = CMapDB::self().getDEM().getElevation(pt2.u, pt2.v);
        if(ele != WPT_NOFLOAT) wpt2->ele = ele;

        CWptDB::self().addWpt(wpt2,false);
    }

    wpt.parentWpt = comboParentWpt->currentText();

    emit CWptDB::self().sigModified(wpt.getKey());

    if(wpt.getName() != name)
    {
        CWptDB::self().setNewWptName(wpt.getName());
    }

    QDialog::accept();
}


void CDlgEditWpt::slotSelectIcon()
{
    CDlgWptIcon dlg(*toolIcon);
    dlg.exec();
}


void CDlgEditWpt::slotAddImage()
{
    SETTINGS;
    QString path = cfg.value("path/images", "./").toString();

    QString filename = QFileDialog::getOpenFileName( 0, tr("Select image file")
        ,path
        ,"Image (*)"
        ,0
        , FILE_DIALOG_FLAGS
        );
    if(filename.isEmpty()) return;

    QFileInfo fi(filename);
    cfg.setValue("path/images", fi.absolutePath());

    QString info =  QInputDialog::getText( this, tr("Add comment ..."), tr("comment"), QLineEdit::Normal, QFileInfo(filename).fileName());

    CWpt::image_t img;
    img.info = info;
    img.pixmap = QPixmap(filename);
    wpt.images.push_back(img);
    showImage(wpt.images.count() - 1);

    pushDel->setEnabled(true);

}


void CDlgEditWpt::slotDelImage()
{
    wpt.images.removeAt(idxImg);
    while(idxImg >= wpt.images.count()) --idxImg;
    showImage(idxImg);

    pushDel->setEnabled(wpt.images.count() != 0);
}


void CDlgEditWpt::slotNextImage()
{
    showImage(idxImg + 1);
}


void CDlgEditWpt::slotPrevImage()
{
    showImage(idxImg - 1);
}


void CDlgEditWpt::showImage(int idx)
{
    if(idx < 0) idx = 0;

    if(idx < wpt.images.count())
    {
        idxImg = idx;

        CWpt::image_t& img = wpt.images[idx];
        if(wpt.isGeoCache())
        {
            QPixmap tmp = img.pixmap.scaledToWidth(300,Qt::SmoothTransformation);
            labelImage->setMinimumSize(tmp.size());
            labelImage->setPixmap(tmp);
        }
        else
        {
            QPixmap tmp = img.pixmap.scaledToWidth(150,Qt::SmoothTransformation);
            labelImage->setMinimumSize(tmp.size());
            labelImage->setPixmap(tmp);
        }
        labelInfo->setText(img.info);

        pushNext->setEnabled(idx < (wpt.images.count() - 1) && wpt.images.count() != 1);
        pushPrev->setEnabled(idx > 0);
    }
    else
    {
        labelImage->setText(tr("no image"));
        labelInfo->setText("");
    }
}


void CDlgEditWpt::slotOpenLink(const QUrl& url)
{

    QDesktopServices::openUrl(url);
}


void CDlgEditWpt::slotOpenLink(const QString& link)
{

    QDesktopServices::openUrl(QUrl(link));
}


void CDlgEditWpt::slotEditLink()
{
    bool ok = false;
    QString _link = QInputDialog::getText(0,tr("Edit link ..."),tr("Link: 'http://...'"),QLineEdit::Normal,link,&ok);
    if(ok)
    {
        link = _link;
        labelLink->setText(tr("None"));
    }

    if(!link.isEmpty())
    {
        QString str;
        str = "<a href='" + link + "'>" + link + "</a>";
        labelLink->setText(str);
    }
}


void CDlgEditWpt::slotSaveBarcode()
{

    if(labelBarcode->pixmap() == 0) return;

    QString filter;
    QString filename = QFileDialog::getSaveFileName( 0, tr("Select output file")
        ,QDir::home().path()
        ,"Bitmap (*.png);;"
        ,&filter
        , FILE_DIALOG_FLAGS
        );

    if(filename.isEmpty()) return;

    QFileInfo fi(filename);
    if(fi.suffix().toLower() != "png")
    {
        filename += ".png";
    }

    labelBarcode->pixmap()->save(filename);
}


void CDlgEditWpt::slotUpdateBarcode()
{
    QString barcode;

    barcode += tr("%1\n").arg(lineName->text());
    barcode += tr("%1\n").arg(linePosition->text());
    if(!link.isEmpty())
    {
        barcode += tr("%1\n").arg(link);
    }

    if(textComment->toPlainText().size())
    {
        barcode += textComment->toPlainText();
    }

#ifdef HAS_DMTX
    if(enc)
    {
        dmtxEncodeSetProp( enc, DmtxPropPixelPacking, DmtxPack32bppRGBX );
        dmtxEncodeSetProp( enc, DmtxPropWidth, 200 );
        dmtxEncodeSetProp( enc, DmtxPropHeight, 200 );

        barcode += "    ";
        barcode = barcode.replace(QChar(0260),' ');
        if(barcode.size() > 180)
        {
            barcode = barcode.left(177) + "...";
        }
        dmtxEncodeDataMatrix( enc, barcode.size(), (unsigned char*)barcode.toLatin1().data() );

        QImage curBarCode( enc->image->pxl, enc->image->width, enc->image->height, QImage::Format_RGB32 );
        labelBarcode->setPixmap(QPixmap::fromImage(curBarCode));
    }
    else
    {
        labelBarcode->setText("Failed!");
    }
#else
    labelBarcode->setPixmap(QPixmap(":/pics/DummyBarcode.png"));
    pushSaveBarcode->setEnabled(false);
#endif                       //HAS_DMTX

}


void CDlgEditWpt::slotToggleHint(bool show)
{
    QString html = wpt.getExtInfo(show);

    if(wpt.isGeoCache())
    {
        wpt.showBuddies(true);

        if(!wpt.buddies.isEmpty())
        {
            CWpt::buddy_t buddy;
            foreach(buddy, wpt.buddies)
            {
                foreach(const QString& pos, buddy.pos)
                {
                    html.replace(pos, QString("%1 (<b><i style='color: black;'>%2</i></b>)").arg(pos).arg(buddy.name));
                }
            }
        }
    }
    webView->setHtml(html);
}


void CDlgEditWpt::slotSelectImage(const CImageSelect::img_t& src)
{
    wpt.images.clear();

    CWpt::image_t tar;
    tar.filename    = src.filename;
    tar.info        = src.title;
    tar.pixmap      = src.img;

    wpt.images << tar;

    showImage(0);

    textComment->setText(src.title);

    pushDel->setEnabled(wpt.images.count() != 0);
}


void CDlgEditWpt::slotCreateBuddies()
{
    IMap& dem = CMapDB::self().getDEM();

    wpt.showBuddies(true);
    const QList<CWpt::buddy_t>& buddies = wpt.buddies;
    foreach(const CWpt::buddy_t& buddy, buddies)
    {
        CWpt * w = CWptDB::self().newWpt(buddy.lon, buddy.lat, dem.getElevation(buddy.lon, buddy.lat), buddy.name);
        w->setIcon("Civil");
        w->setParentWpt(wpt.getName());
    }

    wpt.showBuddies(false);
}


void CDlgEditWpt::slotTransparent(bool ok)
{
    imageSelect->setTransparent(ok);
}


void CDlgEditWpt::slotEditInfo()
{
    CWpt::image_t& img = wpt.images[idxImg];

    QString info =  QInputDialog::getText( this, tr("Add comment ..."), tr("comment"), QLineEdit::Normal, img.info);

    if(info.isEmpty())
    {
        return;
    }
    img.info = info;
    labelInfo->setText(info);

}


bool CDlgEditWpt::eventFilter(QObject *obj, QEvent *event)
{
    if(obj == labelImage)
    {
        if(event->type() == QEvent::MouseButtonPress && !wpt.images.isEmpty())
        {
            CImageViewer viewer(wpt.images, idxImg, this);
            viewer.exec();
        }
    }

    return QObject::eventFilter(obj, event);
}


void CDlgEditWpt::slotCollectSpoiler()
{
    if(!wpt.link.isEmpty())
    {
        if(!wpt.images.isEmpty())
        {
            if(QMessageBox::question(0, tr("Delete images..."), tr("Remove all other images first?"), QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes ) == QMessageBox::Yes)
            {
                wpt.images.clear();
                showImage(0);
                pushDel->setEnabled(false);
            }
        }
        triggerSpoilerDownload();
    }
}


void CDlgEditWpt::triggerSpoilerDownload()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    pendingRequests.clear();

    QNetworkRequest request;

    request.setUrl(wpt.link);
    networkAccessManager->get(request);
}


void CDlgEditWpt::slotProxyAuthenticationRequired(const QNetworkProxy &prox, QAuthenticator *auth)
{
    QString user;
    QString pwd;

    CResources::self().getHttpProxyAuth(user,pwd);

    auth->setUser(user);
    auth->setPassword(pwd);
}


void CDlgEditWpt::slotRequestFinished(QNetworkReply * reply)
{
    if(reply->error() != QNetworkReply::NoError)
    {
        return;
    }

    if(pendingRequests.contains(reply))
    {

        QString info = pendingRequests.take(reply);

        if(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 301)
        {
            QNetworkRequest request;
            request.setUrl(reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl());
            pendingRequests[networkAccessManager->get(request)] = info;
        }
        else if(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200)
        {
            QByteArray data = reply->readAll();
            CWpt::image_t img;
            img.info = info;
            img.pixmap.loadFromData(data);
            wpt.images.push_back(img);
            showImage(wpt.images.count() - 1);
            pushDel->setEnabled(true);
        }

        if(pendingRequests.isEmpty())
        {
            QApplication::restoreOverrideCursor();
        }

        return;
    }

    QString asw = reply->readAll();
    reply->deleteLater();

    static int cnt = 0;
    QFile f(QString("test%1.html").arg(cnt++));
    f.open(QIODevice::WriteOnly);
    f.write(asw.toUtf8(), asw.size());
    f.close();

    if(asw.isEmpty())
    {
        return;
    }

    QRegExp re0(".*Object moved to <a href=\"(.*)\".*");
    QRegExp re1(".*CachePageImages.*");
    QRegExp re2("(http://.*\\.jpg).*>(.*)</a>");

    re2.setMinimal(true);

    bool watchOut       = false;
    bool spoilerFound   = false;
    QStringList lines   = asw.split("\n");
    foreach(const QString& line, lines)
    {
        if(re0.exactMatch(line))
        {
            QString url  = re0.cap(1);

            QNetworkRequest request;

            request.setUrl(url);
            networkAccessManager->get(request);
            return;
        }
        else if(!watchOut && re1.exactMatch(line))
        {
            watchOut = true;
        }
        else if(watchOut)
        {
            int pos = 0;
            while ((pos = re2.indexIn(line, pos)) != -1)
            {
                spoilerFound = true;

                QString url  = re2.cap(1);
                QString text = re2.cap(2);

                QNetworkRequest request;

                request.setUrl(url);
                pendingRequests[networkAccessManager->get(request)] = text;

                pos += re2.matchedLength();
            }

            watchOut = false;
        }
    }

    if(!spoilerFound && !silentSpoilerQuery)
    {
        QApplication::restoreOverrideCursor();
        QMessageBox::information(0,tr("No spoilers..."), tr("No spoilers found."), QMessageBox::Ok);
    }

    silentSpoilerQuery = false;

}
