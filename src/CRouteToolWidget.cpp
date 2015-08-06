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
#include "CRouteToolWidget.h"
#include "CRoute.h"
#include "CRouteDB.h"
#include "IUnit.h"
#include "CMapDB.h"
#include "CDlgEditRoute.h"
#include "CResources.h"
#include "COverlayDB.h"
#include "COverlayDistance.h"
#include "CTrackDB.h"
#include "CTrack.h"
#include "CDlgConvertToTrack.h"
#include "CMegaMenu.h"
#include "version.h"
#include "CSettings.h"
#include "CMainWindow.h"

#include <QtGui>
#include <QtXml>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkProxy>
#include <QMessageBox>
#include <QMenu>

#define N_LINES 3

CRouteToolWidget::sortmode_e CRouteToolWidget::sortmode = CRouteToolWidget::eSortByName;

CRouteToolWidget::CRouteToolWidget(QTabWidget * parent)
: QWidget(parent)
, originator(false)
{
    setupUi(this);
    setObjectName("Routes");
    parent->addTab(this,QIcon(":/icons/iconRoute16x16.png"),"");
    parent->setTabToolTip(parent->indexOf(this), tr("Routes"));

    QString url;
    quint16 port;
    bool enableProxy;
    enableProxy = CResources::self().getHttpProxy(url,port);
    m_networkAccessManager = new QNetworkAccessManager(this);

    if(enableProxy)
    {
        m_networkAccessManager->setProxy(QNetworkProxy(QNetworkProxy::DefaultProxy,url,port));
    }

    connect(m_networkAccessManager,SIGNAL(finished(QNetworkReply*)),this,SLOT(slotRequestFinished(QNetworkReply*)));

    connect(&CRouteDB::self(), SIGNAL(sigModified(QString)), this, SLOT(slotDBChanged()));
    connect(&CRouteDB::self(), SIGNAL(sigChanged()), this, SLOT(slotDBChanged()));

    connect(listRoutes,SIGNAL(itemClicked(QListWidgetItem*) ),this,SLOT(slotItemClicked(QListWidgetItem*)));
    connect(listRoutes,SIGNAL(itemDoubleClicked(QListWidgetItem*) ),this,SLOT(slotItemDoubleClicked(QListWidgetItem*)));
    connect(listRoutes,SIGNAL(itemSelectionChanged()),this,SLOT(slotSelectionChanged()));
    connect(listRoutes,SIGNAL(customContextMenuRequested(const QPoint&)),this,SLOT(slotContextMenu(const QPoint&)));

    tabWidget->setTabIcon(eTabRoute, QIcon(":/icons/iconRoute16x16.png"));
    tabWidget->setTabIcon(eTabSetup, QIcon(":/icons/iconConfig16x16.png"));
    tabWidget->setTabIcon(eTabHelp, QIcon(":/icons/iconHelp16x16.png"));

    comboService->addItem("OpenRouteService (Europe)", CRoute::eOpenRouteService);
    comboService->addItem("MapQuest (World)", CRoute::eMapQuest);
    comboService->addItem("BRouter", CRoute::eBRouter);
    connect(comboService, SIGNAL(currentIndexChanged(int)), this, SLOT(slotServiceChanged(int)));

    // ------------ ORS Setup ------------
    comboORSPreference->addItem(tr("Fastest"), "Fastest");
    comboORSPreference->addItem(tr("Shortest"), "Shortest");
    comboORSPreference->addItem(tr("Bicycle"), "Bicycle");
    comboORSPreference->addItem(tr("Mountain bike"), "BicycleMTB");
    comboORSPreference->addItem(tr("Bicycle racer"), "BicycleRacer");
    comboORSPreference->addItem(tr("Bicycle safest"), "BicycleSafety");
    comboORSPreference->addItem(tr("Bicycle route"), "BicycleRoute");
    comboORSPreference->addItem(tr("Pedestrian"), "Pedestrian");

    comboORSLanguage->addItem(tr("English"), "en");
    comboORSLanguage->addItem(tr("German"), "de");
    comboORSLanguage->addItem(tr("Bulgarian"), "bg");
    comboORSLanguage->addItem(tr("Czech"), "cz");
    comboORSLanguage->addItem(tr("Dutch"), "nl");
    comboORSLanguage->addItem(tr("Croatian"), "hr");
    comboORSLanguage->addItem(tr("Hungarian"), "hu");
    comboORSLanguage->addItem(tr("Dutch (belgium)"), "nl_BE");
    comboORSLanguage->addItem(tr("Spanish"), "es");
    comboORSLanguage->addItem(tr("Esperanto"), "eo");
    comboORSLanguage->addItem(tr("Finnish"), "fi");
    comboORSLanguage->addItem(tr("French"), "fr");
    comboORSLanguage->addItem(tr("Italian"), "it");
    comboORSLanguage->addItem(tr("Portuguese (brazil)"), "pt_BR");
    comboORSLanguage->addItem(tr("Romanian"), "ro");
    comboORSLanguage->addItem(tr("Russian"), "ru");
    comboORSLanguage->addItem(tr("Svenska"), "se");
    comboORSLanguage->addItem(tr("Danish"), "dk");
    comboORSLanguage->addItem(tr("Turkish"), "tr");
    comboORSLanguage->addItem(tr("Catalan"), "ca");
    comboORSLanguage->addItem(tr("Japanese"), "ja");
    comboORSLanguage->addItem(tr("Norwegian"), "no");
    comboORSLanguage->addItem(tr("Vietnamese"), "vi");
    comboORSLanguage->addItem(tr("Norwegian-bokmal"), "nb");
    comboORSLanguage->addItem(tr("de - Rhenish"), "de-rheinl");
    comboORSLanguage->addItem(tr("de - Op Platt"), "de-opplat");
    comboORSLanguage->addItem(tr("de - Berlin dialect"), "de-berlin");
    comboORSLanguage->addItem(tr("de - Swabian"), "de-swabia");
    comboORSLanguage->addItem(tr("de - Ruhrpott"), "de-ruhrpo");
    comboORSLanguage->addItem(tr("de - great Austrian dialect"), "de-at-ooe");
    comboORSLanguage->addItem(tr("de - Bavarian"), "de-bay");

    // ------------ MQ Setup ------------
    comboMQPreference->addItem(tr("Fastest"), "fastest");
    comboMQPreference->addItem(tr("Shortest"), "shortest");
    comboMQPreference->addItem(tr("Bicycle"), "bicycle");
    comboMQPreference->addItem(tr("Pedestrian/pub. transp."), "multimodal");
    comboMQPreference->addItem(tr("Pedestrian"), "pedestrian");

    comboMQLanguage->addItem(tr("US English"), "en_US");
    comboMQLanguage->addItem(tr("British English"), "en_GB");
    comboMQLanguage->addItem(tr("Danish"), "da_DK");
    comboMQLanguage->addItem(tr("Dutch"), "nl_NL");
    comboMQLanguage->addItem(tr("French"), "fr_FR");
    comboMQLanguage->addItem(tr("German"), "de_DE");
    comboMQLanguage->addItem(tr("Italian"), "it_IT");
    comboMQLanguage->addItem(tr("Norwegian"), "no");
    comboMQLanguage->addItem(tr("Spanish"), "es");
    comboMQLanguage->addItem(tr("Swedish"), "sv_SE");

    SETTINGS;
    int langIdx;
    QString locale = QLocale::system().name().left(2);

    cfg.beginGroup("routing");
    comboService->setCurrentIndex(cfg.value("service", 0).toInt());
    slotServiceChanged(comboService->currentIndex());
    cfg.beginGroup("ORS");
    langIdx = comboORSLanguage->findData(locale);
    comboORSPreference->setCurrentIndex(cfg.value("preference", 0).toInt());
    checkORSAvoidHighways->setChecked(cfg.value("avoidHighways", false).toBool());
    checkORSAvoidTollways->setChecked(cfg.value("avoidTollways", false).toBool());
    comboORSLanguage->setCurrentIndex(cfg.value("language", langIdx).toInt());
    cfg.endGroup();
    cfg.beginGroup("MQ");
    langIdx = comboORSLanguage->findData(locale);
    comboMQPreference->setCurrentIndex(cfg.value("preference", 0).toInt());
    checkMQAvoidLimAccess->setChecked(cfg.value("avoidLimAccess", false).toBool());
    checkMQAvoidTollRoads->setChecked(cfg.value("avoidTollRoads", false).toBool());
    checkMQAvoidSeasonal->setChecked(cfg.value("avoidSeasonal", false).toBool());
    checkMQAvoidUnpaved->setChecked(cfg.value("avoidUnpaved", false).toBool());
    checkMQAvoidFerry->setChecked(cfg.value("avoidFerry", false).toBool());
    checkMQAvoidCountryBorder->setChecked(cfg.value("avoidCountryBorder", false).toBool());
    comboMQLanguage->setCurrentIndex(cfg.value("language", langIdx).toInt());
    cfg.endGroup();

    // ------------ BR Setup ------------
    connect(&CResources::self(), SIGNAL(sigBRouterChanged()), this, SLOT(slotBRProfilesChanged()));
    slotBRProfilesChanged();
    cfg.beginGroup("BR");
    comboBRPreference->setCurrentIndex(cfg.value("preference", 0).toInt());
    connect(comboBRPreference, SIGNAL(currentIndexChanged(int)), this, SLOT(slotBRPreferenceChanged(int)));
    cfg.endGroup();
    cfg.endGroup();

    m_networkAccessManager->setProxy(QNetworkProxy(QNetworkProxy::DefaultProxy));

    connect(m_networkAccessManager, SIGNAL(proxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)),
        this, SLOT(slotProxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)));

    timer = new QTimer(this);
    timer->setSingleShot(true);

    connect(toolSortAlpha, SIGNAL(clicked()), this, SLOT(slotDBChanged()));
    connect(toolSortTime, SIGNAL(clicked()), this, SLOT(slotDBChanged()));

    toolSortAlpha->setIcon(QPixmap(":/icons/iconDec16x16.png"));
    toolSortTime->setIcon(QPixmap(":/icons/iconTime16x16.png"));

    toolSortAlpha->setChecked(cfg.value("route/sortAlpha", true).toBool());
    toolSortTime->setChecked(cfg.value("route/sortTime", true).toBool());

    connect(timer, SIGNAL(timeout()), this, SLOT(slotTimeout()));
}


CRouteToolWidget::~CRouteToolWidget()
{
    SETTINGS;
    cfg.setValue("route/sortAlpha", toolSortAlpha->isChecked());
    cfg.setValue("route/sortTime", toolSortTime->isChecked());

    foreach(const QString& key, pendingRequests)
    {
        qDebug() << "dead entry in pending requests" << key;
    }
}


void CRouteToolWidget::slotServiceChanged(int idx)
{
    groupORS->hide();
    groupMQ->hide();
    groupBR->hide();
    labelCopyrightMapQuest->hide();
    labelCopyrightOpenRoute->hide();

    if(comboService->itemData(idx).toInt() == CRoute::eOpenRouteService)
    {
        groupORS->show();
        labelCopyrightOpenRoute->show();
    }
    else if(comboService->itemData(idx).toInt() == CRoute::eMapQuest)
    {
        groupMQ->show();
        labelCopyrightMapQuest->show();
    }
    else if(comboService->itemData(idx).toInt() == CRoute::eBRouter)
    {
        groupBR->show();
    }
}


void CRouteToolWidget::slotDBChanged()
{
    if(originator) return;

    if(toolSortAlpha->isChecked())
    {
        sortmode = eSortByName;
    }
    else if(toolSortTime->isChecked())
    {
        sortmode = eSortByTime;
    }

    QFontMetrics fm(listRoutes->font());
    QPixmap icon(16,N_LINES*fm.height());
    icon.fill(Qt::white);

    listRoutes->clear();
    listRoutes->setIconSize(icon.size());

    QListWidgetItem * highlighted = 0;

    CRouteDB::keys_t key;
    QList<CRouteDB::keys_t> keys = CRouteDB::self().keys();

    foreach(key, keys)
    {
        CRoute * route = CRouteDB::self().getRouteByKey(key.key);

        QListWidgetItem * item = new QListWidgetItem(listRoutes);

        icon.fill(Qt::transparent);
        QPainter p;
        p.begin(&icon);
        p.drawPixmap(0,0,route->getIcon());
        p.end();

        item->setText(route->getInfo());
        item->setData(Qt::UserRole, route->getKey());
        item->setIcon(icon);

        if(route->isHighlighted())
        {
            highlighted = item;
        }

        ++route;
    }

    if(highlighted)
    {
        listRoutes->setCurrentItem(highlighted);
    }
}


void CRouteToolWidget::slotItemClicked(QListWidgetItem * item)
{
    originator = true;
    CRouteDB::self().highlightRoute(item->data(Qt::UserRole).toString());
    originator = false;
}


void CRouteToolWidget::slotItemDoubleClicked(QListWidgetItem * item)
{
    QString key = item->data(Qt::UserRole).toString();

    QRectF r = CRouteDB::self().getBoundingRectF(key);
    if (!r.isNull ())
    {
        CMapDB::self().getMap().zoom(r.left() * DEG_TO_RAD, r.top() * DEG_TO_RAD, r.right() * DEG_TO_RAD, r.bottom() * DEG_TO_RAD);
    }
}


void CRouteToolWidget::keyPressEvent(QKeyEvent * e)
{
    if(e->key() == Qt::Key_Delete)
    {
        slotDelete();
        e->accept();
    }
    else
    {
        QWidget::keyPressEvent(e);
    }
}


void CRouteToolWidget::slotContextMenu(const QPoint& pos)
{
    QListWidgetItem * item = listRoutes->currentItem();
    if(item)
    {
        QPoint p = listRoutes->mapToGlobal(pos);

        QMenu contextMenu;
        contextMenu.addAction(QPixmap(":/icons/iconEdit16x16.png"),tr("Edit"),this,SLOT(slotEdit()));
        contextMenu.addAction(QPixmap(":/icons/iconWizzard16x16.png"),tr("Calc. route"),this,SLOT(slotCalcRoute()));
        contextMenu.addSeparator();
        contextMenu.addAction(QPixmap(":/icons/iconDistance16x16.png"),tr("Make Overlay"),this,SLOT(slotToOverlay()));
        contextMenu.addAction(QPixmap(":/icons/iconTrack16x16.png"),tr("Make Track"),this,SLOT(slotToTrack()));
        contextMenu.addSeparator();
        contextMenu.addAction(QPixmap(":/icons/iconZoomArea16x16.png"),tr("Zoom to fit"),this,SLOT(slotZoomToFit()));
        contextMenu.addAction(QPixmap(":/icons/iconClear16x16.png"),tr("Reset"),this,SLOT(slotResetRoute()));
        contextMenu.addAction(QPixmap(":/icons/iconClear16x16.png"),tr("Delete"),this,SLOT(slotDelete()),Qt::CTRL + Qt::Key_Delete);
        contextMenu.exec(p);
    }
}


void CRouteToolWidget::slotEdit()
{
    QListWidgetItem * item = listRoutes->currentItem();
    if(item == 0) return;

    QString key     = item->data(Qt::UserRole).toString();
    CRoute* route   = CRouteDB::self().getRouteByKey(key);
    if(route == 0) return;

    CDlgEditRoute dlg(*route, this);
    dlg.exec();
}


void CRouteToolWidget::slotDelete()
{
    QStringList keys;
    QListWidgetItem * item;
    const QList<QListWidgetItem*>& items = listRoutes->selectedItems();
    foreach(item,items)
    {
        keys << item->data(Qt::UserRole).toString();
    }
    originator = true;
    CRouteDB::self().delRoutes(keys);
    originator = false;

    slotDBChanged();
}


void CRouteToolWidget::slotCalcRoute()
{

    QListWidgetItem * item;
    QList<QListWidgetItem *> items = listRoutes->selectedItems();

    SETTINGS;
    cfg.beginGroup("routing");
    cfg.setValue("service", comboService->currentIndex());
    cfg.beginGroup("ORS");
    cfg.setValue("preference", comboORSPreference->currentIndex());
    cfg.setValue("avoidHighways", checkORSAvoidHighways->isChecked());
    cfg.setValue("avoidTollways", checkORSAvoidTollways->isChecked());
    cfg.setValue("language", comboORSLanguage->currentIndex());
    cfg.endGroup();
    cfg.beginGroup("MQ");
    cfg.setValue("preference", comboMQPreference->currentIndex());
    cfg.setValue("avoidLimAccess", checkMQAvoidLimAccess->isChecked());
    cfg.setValue("avoidTollRoads", checkMQAvoidTollRoads->isChecked());
    cfg.setValue("avoidSeasonal", checkMQAvoidSeasonal->isChecked());
    cfg.setValue("avoidUnpaved", checkMQAvoidUnpaved->isChecked());
    cfg.setValue("avoidFerry", checkMQAvoidFerry->isChecked());
    cfg.setValue("avoidCountryBorder", checkMQAvoidCountryBorder->isChecked());
    cfg.setValue("language", comboMQLanguage->currentIndex());
    cfg.endGroup();
    cfg.endGroup();

    originator = true;
    foreach(item, items)
    {
        QString key     = item->data(Qt::UserRole).toString();
        CRoute* route   = CRouteDB::self().getRouteByKey(key);
        if(route == 0) return;

        route->setCalcPending();
        route->reset();

        qint32 service = comboService->itemData(comboService->currentIndex()).toInt();

        if(service == CRoute::eOpenRouteService)
        {
            startOpenRouteService(*route);
        }
        else if(service == CRoute::eMapQuest)
        {
            startMapQuest(*route);
        }
        else if(service == CRoute::eBRouter)
        {
            startBRouterService(*route);
        }
    }

    originator = false;
    slotDBChanged();
}


const QString CRouteToolWidget::gml_ns = "http://www.opengis.net/gml";
const QString CRouteToolWidget::xls_ns = "http://www.opengis.net/xls";
const QString CRouteToolWidget::xsi_ns = "http://www.w3.org/2001/XMLSchema-instance";
const QString CRouteToolWidget::sch_ns = "http://www.ascc.net/xml/schematron";
const QString CRouteToolWidget::xlink_ns = "http://www.w3.org/1999/xlink";
const QString CRouteToolWidget::schemaLocation = "http://www.opengis.net/xls http://schemas.opengis.net/ols/1.1.0/RouteService.xsd";

void CRouteToolWidget::startOpenRouteService(CRoute& rte)
{
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
    root.setAttribute("xls:lang", comboORSLanguage->itemData(comboORSLanguage->currentIndex()).toString());

    QDomElement requestHeader = xml.createElement("xls:RequestHeader");
    root.appendChild(requestHeader);

    QDomElement Request = xml.createElement("xls:Request");
    root.appendChild(Request);

    Request.setAttribute("methodName", "RouteRequest");
    Request.setAttribute("requestID", rte.getKey());
    Request.setAttribute("version", "1.1");

    QDomElement DetermineRouteRequest = xml.createElement("xls:DetermineRouteRequest");
    Request.appendChild(DetermineRouteRequest);

    DetermineRouteRequest.setAttribute("distanceUnit", "KM");

    QDomElement RoutePlan = xml.createElement("xls:RoutePlan");
    DetermineRouteRequest.appendChild(RoutePlan);

    QDomElement RoutePreference = xml.createElement("xls:RoutePreference");
    RoutePlan.appendChild(RoutePreference);

    QDomText _RoutePreference_ = xml.createTextNode(comboORSPreference->itemData(comboORSPreference->currentIndex()).toString());
    RoutePreference.appendChild(_RoutePreference_);

    QDomElement WayPointList = xml.createElement("xls:WayPointList");
    RoutePlan.appendChild(WayPointList);

    addOpenLSWptList(xml, WayPointList, rte);

    QDomElement AvoidList = xml.createElement("xls:AvoidList");
    RoutePlan.appendChild(AvoidList);
    if(checkORSAvoidHighways->isChecked())
    {
        QDomElement AvoidFeature = xml.createElement("xls:AvoidFeature");
        AvoidList.appendChild(AvoidFeature);

        QDomText _AvoidFeature_ = xml.createTextNode("Highway");
        AvoidFeature.appendChild(_AvoidFeature_);
    }

    if(checkORSAvoidTollways->isChecked())
    {
        QDomElement AvoidFeature = xml.createElement("xls:AvoidFeature");
        AvoidList.appendChild(AvoidFeature);

        QDomText _AvoidFeature_ = xml.createTextNode("Tollway");
        AvoidFeature.appendChild(_AvoidFeature_);
    }

    QDomElement RouteInstructionsRequest = xml.createElement("xls:RouteInstructionsRequest");
    RouteInstructionsRequest.setAttribute("provideGeometry", "1");
    DetermineRouteRequest.appendChild(RouteInstructionsRequest);

    QByteArray array;
    QTextStream out(&array, QIODevice::WriteOnly);
    out.setCodec("UTF-8");
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << xml.toString() << endl;

    QUrl url("http://openls.geog.uni-heidelberg.de");
    url.setPath("/qlandkarte/route");

    QNetworkRequest request;

    request.setUrl(url);

    QNetworkReply* reply = m_networkAccessManager->post(request, array);
    pendingRequests[reply] = rte.getKey();

    timer->start(20000);

}


void CRouteToolWidget::addOpenLSWptList(QDomDocument& xml, QDomElement& WayPointList, CRoute& rte)
{

    QVector<CRoute::pt_t> wpts = rte.getPriRtePoints();

    QDomElement StartPoint = xml.createElement("xls:StartPoint");
    WayPointList.appendChild(StartPoint);
    addOpenLSPos(xml, StartPoint, wpts.first());

    if(wpts.size() > 2)
    {
        const int size = wpts.size() - 1;
        for(int i = 1; i < size; i++)
        {

            QDomElement ViaPoint = xml.createElement("xls:ViaPoint");
            WayPointList.appendChild(ViaPoint);
            addOpenLSPos(xml, ViaPoint, wpts[i]);
        }
    }

    QDomElement EndPoint = xml.createElement("xls:EndPoint");
    WayPointList.appendChild(EndPoint);
    addOpenLSPos(xml, EndPoint, wpts.last());
}


void CRouteToolWidget::addOpenLSPos(QDomDocument& xml, QDomElement& Parent, CRoute::pt_t& pt)
{
    QString lon, lat;
    QDomElement Position = xml.createElement("xls:Position");
    Parent.appendChild(Position);

    QDomElement Point = xml.createElement("gml:Point");
    Point.setAttribute("srsName", "EPSG:4326");
    Position.appendChild(Point);

    QDomElement Pos = xml.createElement("gml:pos");
    Point.appendChild(Pos);

    lon.sprintf("%1.8f", pt.lon);
    lat.sprintf("%1.8f", pt.lat);

    QDomText _Pos_ = xml.createTextNode(QString("%1 %2").arg(lon).arg(lat));
    Pos.appendChild(_Pos_);
}


void CRouteToolWidget::slotProxyAuthenticationRequired(const QNetworkProxy &prox, QAuthenticator *auth)
{
    QString user;
    QString pwd;

    CResources::self().getHttpProxyAuth(user,pwd);

    auth->setUser(user);
    auth->setPassword(pwd);
}


void CRouteToolWidget::slotRequestFinished(QNetworkReply* reply)
{
    QString key;
    if(pendingRequests.contains(reply))
    {
        key = pendingRequests.take(reply);
        //        qDebug() << "--------------------removed" << key << reply;
    }
    else
    {
        //        qDebug() << "--------------------reply not found" << reply;
    }

    if(reply->error() != QNetworkReply::NoError)
    {
        timer->stop();
        QMessageBox::warning(0,tr("Failed..."), tr("Bad response from server:\n%1").arg(reply->errorString()), QMessageBox::Abort);
        reply->deleteLater();
        return;
    }

    QByteArray res = reply->readAll();
    reply->deleteLater();

    if(res.isEmpty() || key.isEmpty())
    {
        return;
    }

    timer->stop();

    QDomDocument xml;
    xml.setContent(res);
    //    qDebug() << xml.toString();
    //    qDebug() << "key:" << key;

    qint32 service = comboService->itemData(comboService->currentIndex()).toInt();
    if(service == CRoute::eOpenRouteService)
    {
        QDomElement root        = xml.documentElement();
        QDomElement response    = root.firstChildElement("xls:Response");

        if(response.isNull())
        {
            QMessageBox::warning(0,tr("Failed..."), tr("Bad response from server:\n%1").arg(xml.toString()), QMessageBox::Abort);
            return;
        }

    }
    else if(service == CRoute::eMapQuest)
    {
        QDomElement response    = xml.firstChildElement("response");
        QDomElement info        = response.firstChildElement("info");
        QDomElement statusCode  = info.firstChildElement("statusCode");

        if(statusCode.isNull() || statusCode.text().toInt() != 0)
        {
            QMessageBox::warning(0,tr("Failed..."), tr("Bad response from server:\n%1").arg(xml.toString()), QMessageBox::Abort);
            return;
        }
    }
    else if (service == CRoute::eBRouter)
    {
        QDomElement response = xml.firstChildElement("gpx");
        if(response.isNull())
        {
            QMessageBox::warning(0,tr("Failed..."), tr("Bad response from server:\n%1").arg(xml.toString()), QMessageBox::Abort);
            return;
        }
    }

    CRouteDB::self().loadSecondaryRoute(key, xml, (CRoute::service_e)service);
}


void CRouteToolWidget::slotResetRoute()
{
    QListWidgetItem * item;
    QList<QListWidgetItem *> items = listRoutes->selectedItems();

    foreach(item, items)
    {
        QString key     = item->data(Qt::UserRole).toString();
        CRouteDB::self().reset(key);
    }
}


void CRouteToolWidget::slotSelectionChanged()
{
    if(originator)
    {
        return;
    }
    if(listRoutes->hasFocus() && listRoutes->selectedItems().isEmpty())
    {
        CRouteDB::self().highlightRoute("");
    }
}


void CRouteToolWidget::slotToOverlay()
{

    QListWidgetItem * item;
    const QList<QListWidgetItem*>& items = listRoutes->selectedItems();
    foreach(item,items)
    {
        CRoute * route = CRouteDB::self().getRouteByKey(item->data(Qt::UserRole).toString());

        QList<COverlayDistance::pt_t> pts;

        int idx = 0;
        CRoute::pt_t rtept;
        QVector<CRoute::pt_t>& rtepts = route->getSecRtePoints().isEmpty() ? route->getPriRtePoints() : route->getSecRtePoints();
        foreach(rtept, rtepts)
        {
            COverlayDistance::pt_t pt;
            pt.u = rtept.lon * DEG_TO_RAD;
            pt.v = rtept.lat * DEG_TO_RAD;
            pt.idx = idx++;

            pts << pt;
        }

        COverlayDB::self().addDistance(route->getName(), tr("created from route"), 0.0, pts);
    }

    CMegaMenu::self().switchByKeyWord("Overlay");
}


void CRouteToolWidget::slotToTrack()
{

    QListWidgetItem * item;
    const QList<QListWidgetItem*>& items = listRoutes->selectedItems();
    foreach(item,items)
    {
        CRoute * route = CRouteDB::self().getRouteByKey(item->data(Qt::UserRole).toString());

        QVector<CRoute::pt_t>& rtepts = route->getSecRtePoints().isEmpty() ? route->getPriRtePoints() : route->getSecRtePoints();

        double dist, d, delta = 10.0, a1 , a2;
        projXY pt1, pt2, ptx;
        CTrack::pt_t pt;
        CDlgConvertToTrack::EleMode_e eleMode;

        CDlgConvertToTrack dlg(0);
        if(dlg.exec() == QDialog::Rejected)
        {
            return;
        }

        CTrack * track  = new CTrack(&CTrackDB::self());
        track->setName(route->getName());

        delta   = dlg.getDelta();
        eleMode = dlg.getEleMode();

        if(delta == -1)
        {

            for(int i = 0; i < rtepts.count(); ++i)
            {
                pt2 = rtepts[i];
                pt.lon = pt2.u;
                pt.lat = pt2.v;
                pt._lon = pt.lon;
                pt._lat = pt.lat;
                *track << pt;
            }
        }
        else
        {
            if((route->getDistance() / delta) > (MAX_TRACK_SIZE - rtepts.count()))
            {
                delta = route->getDistance() / (MAX_TRACK_SIZE - rtepts.count());
            }

            // 1st point
            pt1 = rtepts.first();
            pt.lon = pt1.u;
            pt.lat = pt1.v;
            pt._lon = pt.lon;
            pt._lat = pt.lat;
            *track << pt;

            pt1.u = pt1.u * DEG_TO_RAD;
            pt1.v = pt1.v * DEG_TO_RAD;

            // all other points
            for(int i = 1; i < rtepts.count(); ++i)
            {
                pt2 = rtepts[i];

                pt2.u = pt2.u * DEG_TO_RAD;
                pt2.v = pt2.v * DEG_TO_RAD;

                // all points from pt1 -> pt2, with 10m steps
                dist = ::distance(pt1, pt2, a1, a2);
                a1 *= DEG_TO_RAD;

                d = delta;
                while(d < dist)
                {
                    ptx = GPS_Math_Wpt_Projection(pt1, d, a1);
                    pt.lon = ptx.u * RAD_TO_DEG;
                    pt.lat = ptx.v * RAD_TO_DEG;
                    pt._lon = pt.lon;
                    pt._lat = pt.lat;

                    *track << pt;

                    d += delta;
                }

                // and finally the next point
                pt.lon = pt2.u * RAD_TO_DEG;
                pt.lat = pt2.v * RAD_TO_DEG;
                pt._lon = pt.lon;
                pt._lat = pt.lat;

                *track << pt;

                pt1 = pt2;
            }
        }

        if(eleMode == CDlgConvertToTrack::eLocal)
        {
            track->replaceElevationByLocal(true);
        }
        else if(eleMode == CDlgConvertToTrack::eRemote)
        {
            track->replaceElevationByRemote(true);
        }

        CTrackDB::self().addTrack(track, false);
    }
    CMegaMenu::self().switchByKeyWord("Tracks");

}


void CRouteToolWidget::slotZoomToFit()
{
    QRectF r;

    const QList<QListWidgetItem*>& items = listRoutes->selectedItems();
    QList<QListWidgetItem*>::const_iterator item = items.begin();

    r = CRouteDB::self().getBoundingRectF((*item)->data(Qt::UserRole).toString());

    while(item != items.end())
    {
        r |= CRouteDB::self().getBoundingRectF((*item)->data(Qt::UserRole).toString());
        ++item;
    }

    if (!r.isNull ())
    {
        CMapDB::self().getMap().zoom(r.left() * DEG_TO_RAD, r.top() * DEG_TO_RAD, r.right() * DEG_TO_RAD, r.bottom() * DEG_TO_RAD);
    }
}


void CRouteToolWidget::slotTimeout()
{
    QMessageBox::warning(0,tr("Failed..."), tr("Route request timed out. Please try again later."), QMessageBox::Abort);
}


const QByteArray keyMapQuest = "Fmjtd%7Cluu2n16t2h%2Crw%3Do5-haya0";

void CRouteToolWidget::startMapQuest(CRoute& rte)
{
    QDomDocument xml;

    QDomElement route = xml.createElement("route");
    xml.appendChild(route);

    QDomElement locations = xml.createElement("locations");
    route.appendChild(locations);
    addMapQuestLocations(xml, locations, rte);

    QDomElement options = xml.createElement("options");
    route.appendChild(options);

    QDomElement shapeFormat = xml.createElement("shapeFormat");
    shapeFormat.appendChild(xml.createTextNode("raw"));
    options.appendChild(shapeFormat);

    QDomElement generalize = xml.createElement("generalize");
    generalize.appendChild(xml.createTextNode("0"));
    options.appendChild(generalize);

    QDomElement unit = xml.createElement("unit");
    unit.appendChild(xml.createTextNode("k"));
    options.appendChild(unit);

    QDomElement routeType = xml.createElement("routeType");
    routeType.appendChild(xml.createTextNode(comboMQPreference->itemData(comboMQPreference->currentIndex()).toString()));
    options.appendChild(routeType);

    QDomElement locale = xml.createElement("locale");
    locale.appendChild(xml.createTextNode(comboMQLanguage->itemData(comboMQLanguage->currentIndex()).toString()));
    options.appendChild(locale);

    QDomElement avoids = xml.createElement("avoids");
    if(checkMQAvoidLimAccess->isChecked())
    {
        QDomElement avoid = xml.createElement("avoid");
        avoid.appendChild(xml.createTextNode("Limited Access"));
        avoids.appendChild(avoid);
    }
    if(checkMQAvoidTollRoads->isChecked())
    {
        QDomElement avoid = xml.createElement("avoid");
        avoid.appendChild(xml.createTextNode("Toll road"));
        avoids.appendChild(avoid);
    }
    if(checkMQAvoidSeasonal->isChecked())
    {
        QDomElement avoid = xml.createElement("avoid");
        avoid.appendChild(xml.createTextNode("Approximate Seasonal Closure"));
        avoids.appendChild(avoid);
    }
    if(checkMQAvoidUnpaved->isChecked())
    {
        QDomElement avoid = xml.createElement("avoid");
        avoid.appendChild(xml.createTextNode("Unpaved"));
        avoids.appendChild(avoid);
    }
    if(checkMQAvoidFerry->isChecked())
    {
        QDomElement avoid = xml.createElement("avoid");
        avoid.appendChild(xml.createTextNode("Ferry"));
        avoids.appendChild(avoid);
    }
    if(checkMQAvoidCountryBorder->isChecked())
    {
        QDomElement avoid = xml.createElement("avoid");
        avoid.appendChild(xml.createTextNode("Country border crossing"));
        avoids.appendChild(avoid);
    }

    options.appendChild(avoids);

    QString xmlstr = xml.toString(0);
    qDebug() << xmlstr;
    xmlstr = xmlstr.replace("\n","");

    QUrl url("http://open.mapquestapi.com");
    url.setPath("directions/v2/route");

#ifdef QK_QT5_PORT
    QList< QPair<QString, QString> > queryItems;
    queryItems << QPair<QString, QString>("key", keyMapQuest);
    queryItems << QPair<QString, QString>("ambiguities", "ignore");
    queryItems << QPair<QString, QString>("inFormat", "xml");
    queryItems << QPair<QString, QString>("outFormat", "xml");
    queryItems << QPair<QString, QString>("xml", QUrl::toPercentEncoding(xmlstr));
    QUrlQuery urlQuery;
    urlQuery.setQueryItems(queryItems);
    url.setQuery(urlQuery);
#else
    QList< QPair<QByteArray, QByteArray> > queryItems;
    queryItems << QPair<QByteArray, QByteArray>(QByteArray("key"), keyMapQuest);
    queryItems << QPair<QByteArray, QByteArray>(QByteArray("ambiguities"), QByteArray("ignore"));
    queryItems << QPair<QByteArray, QByteArray>(QByteArray("inFormat"), QByteArray("xml"));
    queryItems << QPair<QByteArray, QByteArray>(QByteArray("outFormat"), QByteArray("xml"));
    queryItems << QPair<QByteArray, QByteArray>(QByteArray("xml"), QUrl::toPercentEncoding(xmlstr));
    url.setEncodedQueryItems(queryItems);
#endif
    //    qDebug() << url.toString();

    QNetworkRequest request;

    request.setUrl(url);
    m_networkAccessManager->get(request);

    QNetworkReply* reply = m_networkAccessManager->get(request);
    pendingRequests[reply] = rte.getKey();

    timer->start(20000);
}


void CRouteToolWidget::addMapQuestLocations(QDomDocument& xml, QDomElement& locations, CRoute& rte)
{
    QVector<CRoute::pt_t> wpts = rte.getPriRtePoints();
    foreach(const CRoute::pt_t& wpt, wpts)
    {
        QDomElement location = xml.createElement("location");
        location.appendChild(xml.createTextNode(QString("%1,%2").arg(wpt.lat).arg(wpt.lon)));
        locations.appendChild(location);
    }
}

void CRouteToolWidget::startBRouterService(CRoute& rte)
{
    // /brouter?lonlats=11.626453,48.298498|9.942884,49.798885&nogos=&profile=fastbike&alternativeidx=0&format=gpx

    QUrl url(QString("http://").append(CResources::self().getBRouterHost()));
    url.setPort(CResources::self().getBRouterPort());
    url.setPath("/brouter");

    QVector<CRoute::pt_t> wpts = rte.getPriRtePoints();
    bool isNext = false;
    QString lonlats;
    foreach(const CRoute::pt_t& wpt, wpts)
    {
        if (isNext)
        {
            lonlats.append(QString("|%1,%2").arg(wpt.lon).arg(wpt.lat));
        } else {
            lonlats = QString("%1,%2").arg(wpt.lon).arg(wpt.lat);
            isNext = true;
        }
    }

#ifdef QK_QT5_PORT
    QList< QPair<QString, QString> > queryItems;
    queryItems << QPair<QString, QString>("lonlats",lonlats.toLatin1());
    queryItems << QPair<QString, QString>("nogos", "");
    queryItems << QPair<QString, QString>("profile", comboBRPreference->itemData(comboBRPreference->currentIndex()).toString());
    queryItems << QPair<QString, QString>("alternativeidx", QString::number(rte.getRouteIdx()));
    queryItems << QPair<QString, QString>("format", "gpx");
    QUrlQuery urlQuery;
    urlQuery.setQueryItems(queryItems);
    url.setQuery(urlQuery);
#else
    QList< QPair<QByteArray, QByteArray> > queryItems;
    queryItems << QPair<QByteArray, QByteArray>(QByteArray("lonlats"),QByteArray(lonlats.toLatin1()));
    queryItems << QPair<QByteArray, QByteArray>(QByteArray("nogos"), QByteArray(""));
    queryItems << QPair<QByteArray, QByteArray>(QByteArray("profile"), comboBRPreference->itemData(comboBRPreference->currentIndex()).toByteArray());
    queryItems << QPair<QByteArray, QByteArray>(QByteArray("alternativeidx"), QVariant(rte.getRouteIdx()).toByteArray());
    queryItems << QPair<QByteArray, QByteArray>(QByteArray("format"), QByteArray("gpx"));
    url.setEncodedQueryItems(queryItems);
#endif

    QNetworkRequest request;

    request.setUrl(url);

    QNetworkReply* reply = m_networkAccessManager->get(request);
    pendingRequests[reply] = rte.getKey();

    timer->start(20000);
}

void CRouteToolWidget::slotBRPreferenceChanged(int idx)
{
    for(int i = 0; i<listRoutes->count(); i++)
    {
        QString key = listRoutes->item(i)->data(Qt::UserRole).toString();
        CRouteDB::self().getRouteByKey(key)->resetRouteIdx();
    }
    SETTINGS;
    cfg.setValue("routing/BR/preference", comboBRPreference->currentIndex());
}

void CRouteToolWidget::slotBRProfilesChanged()
{
    int currentIndex = -1;
    int i = 0;
    QString currentProfile = comboBRPreference->itemData(comboBRPreference->currentIndex()).toString();
    comboBRPreference->clear();
    foreach(const QString& key, CResources::self().getBRouterProfiles())
    {
        comboBRPreference->addItem(key,key);
        if (currentProfile.compare(key) == 0)
        {
            currentIndex = i;
        }
        i++;
    }
    if (currentIndex >= 0)
    {
        comboBRPreference->setCurrentIndex(currentIndex);
        SETTINGS;
        cfg.setValue("routing/BR/preference", currentIndex);
    }
}
