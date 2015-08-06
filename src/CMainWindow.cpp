/**********************************************************************************************
    Copyright (C) 2008-2009 Oliver Eichler oliver.eichler@gmx.de

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

#include <QMenu>
#include <QSplitter>
#include <QMessageBox>
#include <QPrinter>
#include <QPrintDialog>
#include <QFileDialog>
#include <QStatusBar>
#include <QMenuBar>
#include <QDesktopWidget>
#include "CMainWindow.h"
#include "CMegaMenu.h"
#include "CCanvas.h"
#include "CCopyright.h"
#include "CResources.h"
#include "CMapDB.h"
#include "CWptDB.h"
#include "CTrackDB.h"
#include "CSearchDB.h"
#include "CDiaryDB.h"
#include "CRouteDB.h"
#include "CDlgConfig.h"
#include "CQlb.h"
#include "CGpx.h"
#include "tcxreader.h"
#include "CTabWidget.h"
#include "CLiveLogDB.h"
#include "COverlayDB.h"
#include "CActions.h"
#include "CMenus.h"
#include "IDevice.h"
#include "CDlgScreenshot.h"
#include "CDlgLoadOnlineMap.h"
#include "CUndoStack.h"
#include "WptIcons.h"
#include "CAppOpts.h"
#include "CMap3D.h"
#include "CDlgExport.h"
#include "CGeoDB.h"
#ifdef HAS_DBUS
#include "CDBus.h"
#endif
#include "CGridDB.h"
#include "CDlgSetupGrid.h"
#include "CSettings.h"
#include "version.h"

#include <QtGui>
#ifndef WIN32
#include <unistd.h>
#endif
#ifdef WIN32
#include <io.h>
#endif

#include "config.h"

CMainWindow * theMainWindow = 0;

CMainWindow::CMainWindow()
: geodb(0)
, modified(false)
, crashed(false)
{
    theMainWindow = this;
    groupProvidedMenu = 0;
    setupMenu = 0;
    setObjectName("MainWidget");
    setWindowTitle("QLandkarte GT");
    setAcceptDrops(true);

    initWptIcons();

    resources = new CResources(this);

    // setup splitter views
    mainSplitter = new QSplitter(Qt::Horizontal,this);
    setCentralWidget(mainSplitter);

    leftSplitter = new QSplitter(Qt::Vertical,this);
    mainSplitter->addWidget(leftSplitter);

    rightSplitter = new QSplitter(Qt::Vertical,this);
    mainSplitter->addWidget(rightSplitter);

    setCentralWidget(mainSplitter);

    canvasTab = new CTabWidget(this);
    rightSplitter->addWidget(canvasTab);

    canvas = new CCanvas(this);
    canvasTab->addTab(canvas,tr("Map"));

    actionGroupProvider = new CMenus(this);

    megaMenu = new CMegaMenu(canvas);
    leftSplitter->addWidget(megaMenu);

    tmpTabWidget = new QTabWidget(this);
    tmpTabWidget->setTabsClosable(true);
    tmpTabWidget->setTabPosition(QTabWidget::South);;
    tmpTabWidget->hide();
    rightSplitter->addWidget(tmpTabWidget);
    connect(tmpTabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(slotTabCloseRequest(int)));

    connect(leftSplitter, SIGNAL(splitterMoved(int, int)), megaMenu, SLOT(slotSplitterMoved(int, int)));

    CActions *actions = actionGroupProvider->getActions();
    canvas->addAction(actions->getAction("aZoomIn"));
    canvas->addAction(actions->getAction("aZoomOut"));
    canvas->addAction(actions->getAction("aMoveLeft"));
    canvas->addAction(actions->getAction("aMoveRight"));
    canvas->addAction(actions->getAction("aMoveUp"));
    canvas->addAction(actions->getAction("aMoveDown"));
    addAction(actions->getAction("aRedo"));
    addAction(actions->getAction("aUndo"));

    actionGroupProvider->addAction(CMenus::MainMenu, "aSwitchToMap");
    actionGroupProvider->addAction(CMenus::MainMenu, "aSwitchToWpt");
    actionGroupProvider->addAction(CMenus::MainMenu, "aSwitchToTrack");
    actionGroupProvider->addAction(CMenus::MainMenu, "aSwitchToRoute");
    actionGroupProvider->addAction(CMenus::MainMenu, "aSwitchToLiveLog");
    actionGroupProvider->addAction(CMenus::MainMenu, "aSwitchToOverlay");
    actionGroupProvider->addAction(CMenus::MainMenu, "aSwitchToMainMore");
    actionGroupProvider->addAction(CMenus::MainMenu, "aClearAll");
    actionGroupProvider->addAction(CMenus::MainMenu, "aUploadAll");
    actionGroupProvider->addAction(CMenus::MainMenu, "aDownloadAll");

    actionGroupProvider->addAction(CMenus::MapMenu, "aSwitchToMain");
    actionGroupProvider->addAction(CMenus::MapMenu, "aMoveArea");
    actionGroupProvider->addAction(CMenus::MapMenu, "aZoomArea");
    actionGroupProvider->addAction(CMenus::MapMenu, "aCenterMap");
    actionGroupProvider->addAction(CMenus::MapMenu, "aSelectArea");
    actionGroupProvider->addAction(CMenus::MapMenu, "aEditMap");
    actionGroupProvider->addAction(CMenus::MapMenu, "aSearchMap");
    actionGroupProvider->addAction(CMenus::MapMenu, "aSwitchToMap3D");
    actionGroupProvider->addAction(CMenus::MapMenu, "aUploadMap");

    actionGroupProvider->addAction(CMenus::Map3DMenu, "aCloseMap3D");
    actionGroupProvider->addAction(CMenus::Map3DMenu, "aMap3DMode");
    actionGroupProvider->addAction(CMenus::Map3DMenu, "aMap3DFPVMode");
    actionGroupProvider->addAction(CMenus::Map3DMenu, "aMap3DLighting");
    actionGroupProvider->addAction(CMenus::Map3DMenu, "aMap3DTrackMode");

    actionGroupProvider->addAction(CMenus::WptMenu, "aSwitchToMain");
    actionGroupProvider->addAction(CMenus::WptMenu, "aMoveArea");
    actionGroupProvider->addAction(CMenus::WptMenu, "aZoomArea");
    actionGroupProvider->addAction(CMenus::WptMenu, "aCenterMap");
    actionGroupProvider->addAction(CMenus::WptMenu, "aNewWpt");
    actionGroupProvider->addAction(CMenus::WptMenu, "aSelWpt");
#ifdef HAS_EXIF
    actionGroupProvider->addAction(CMenus::WptMenu, "aImageWpt");
#endif
    actionGroupProvider->addAction(CMenus::WptMenu, "aUploadWpt");
    actionGroupProvider->addAction(CMenus::WptMenu, "aDownloadWpt");

    actionGroupProvider->addAction(CMenus::TrackMenu, "aSwitchToMain");
    actionGroupProvider->addAction(CMenus::TrackMenu, "aMoveArea");
    actionGroupProvider->addAction(CMenus::TrackMenu, "aZoomArea");
    actionGroupProvider->addAction(CMenus::TrackMenu, "aCenterMap");
    actionGroupProvider->addAction(CMenus::TrackMenu, "aCombineTrack");
    actionGroupProvider->addAction(CMenus::TrackMenu, "aCutTrack");
    actionGroupProvider->addAction(CMenus::TrackMenu, "aSelTrack");
    actionGroupProvider->addAction(CMenus::TrackMenu, "aUploadTrack");
    actionGroupProvider->addAction(CMenus::TrackMenu, "aDownloadTrack");
    actionGroupProvider->addAction(CMenus::TrackMenu, "aTrackPurgeSelection");

    actionGroupProvider->addAction(CMenus::LiveLogMenu, "aSwitchToMain");
    actionGroupProvider->addAction(CMenus::LiveLogMenu, "aMoveArea");
    actionGroupProvider->addAction(CMenus::LiveLogMenu, "aZoomArea");
    actionGroupProvider->addAction(CMenus::LiveLogMenu, "aCenterMap");
    actionGroupProvider->addAction(CMenus::LiveLogMenu, "aLiveLog");
    actionGroupProvider->addAction(CMenus::LiveLogMenu, "aLockMap");
    actionGroupProvider->addAction(CMenus::LiveLogMenu, "aAddWpt");

    actionGroupProvider->addAction(CMenus::OverlayMenu, "aSwitchToMain");
    actionGroupProvider->addAction(CMenus::OverlayMenu, "aMoveArea");
    actionGroupProvider->addAction(CMenus::OverlayMenu, "aZoomArea");
    actionGroupProvider->addAction(CMenus::OverlayMenu, "aCenterMap");
    actionGroupProvider->addAction(CMenus::OverlayMenu, "aText");
    actionGroupProvider->addAction(CMenus::OverlayMenu, "aTextBox");
    actionGroupProvider->addAction(CMenus::OverlayMenu, "aSwitchToOverlayDistance");
    actionGroupProvider->addAction(CMenus::OverlayMenu, "aSwitchToOverlayArea");

    actionGroupProvider->addAction(CMenus::OverlayDistanceMenu, "aBackToOverlay");
    actionGroupProvider->addAction(CMenus::OverlayDistanceMenu, "aMoveArea");
    actionGroupProvider->addAction(CMenus::OverlayDistanceMenu, "aZoomArea");
    actionGroupProvider->addAction(CMenus::OverlayDistanceMenu, "aCenterMap");
    actionGroupProvider->addAction(CMenus::OverlayDistanceMenu, "aCombineDistOvl");
    actionGroupProvider->addAction(CMenus::OverlayDistanceMenu, "aDistance");

    actionGroupProvider->addAction(CMenus::OverlayAreaMenu, "aBackToOverlay");
    actionGroupProvider->addAction(CMenus::OverlayAreaMenu, "aMoveArea");
    actionGroupProvider->addAction(CMenus::OverlayAreaMenu, "aZoomArea");
    actionGroupProvider->addAction(CMenus::OverlayAreaMenu, "aCenterMap");
    actionGroupProvider->addAction(CMenus::OverlayAreaMenu, "aArea");

    actionGroupProvider->addAction(CMenus::MainMoreMenu, "aSwitchToMain");
    actionGroupProvider->addAction(CMenus::MainMoreMenu, "aMoveArea");
    actionGroupProvider->addAction(CMenus::MainMoreMenu, "aZoomArea");
    actionGroupProvider->addAction(CMenus::MainMoreMenu, "aCenterMap");
#ifdef HAS_DBUS
    actionGroupProvider->addAction(CMenus::MainMoreMenu, "aOcm");
#endif
    actionGroupProvider->addAction(CMenus::RouteMenu, "aSwitchToMain");
    actionGroupProvider->addAction(CMenus::RouteMenu, "aMoveArea");
    actionGroupProvider->addAction(CMenus::RouteMenu, "aZoomArea");
    actionGroupProvider->addAction(CMenus::RouteMenu, "aCenterMap");
    actionGroupProvider->addAction(CMenus::RouteMenu, "aUploadRoute");
    actionGroupProvider->addAction(CMenus::RouteMenu, "aDownloadRoute");

    connect(actionGroupProvider, SIGNAL(stateChanged()),megaMenu , SLOT(switchState()));

    switchState();

    QWidget * wtmp      = new QWidget(this);
    QVBoxLayout * ltmp  = new QVBoxLayout(wtmp);
    wtmp->setMinimumHeight(1);
    wtmp->setLayout(ltmp);
    leftSplitter->addWidget(wtmp);

    summary = new QLabel(wtmp);
    summary->setWordWrap(true);
    summary->setAlignment(Qt::AlignJustify|Qt::AlignTop);
    //
    ltmp->addWidget(summary);
    ltmp->addWidget(new QLabel(tr("<b>GPS Device:</b>"), wtmp));

    comboDevice = new QComboBox(wtmp);
    connect(resources, SIGNAL(sigDeviceChanged()), this, SLOT(slotDeviceChanged()));
    connect(comboDevice, SIGNAL(currentIndexChanged(int)), this, SLOT(slotCurrentDeviceChanged(int)));
    slotDeviceChanged();

    ltmp->addWidget(comboDevice);

    tabbar = new QTabWidget(canvas);
    leftSplitter->addWidget(tabbar);

    leftSplitter->setCollapsible(0, true);
    leftSplitter->setCollapsible(1, true);
    leftSplitter->setCollapsible(2, false);

    statusCoord = new QLabel(this);
    statusBar()->addPermanentWidget(statusCoord);

    SETTINGS;
    crashed     = cfg.value("mainWidget/crashed",false).toBool();
    cfg.setValue("mainWidget/crashed",true);

    quadraticZoom = new QCheckBox(theMainWindow->getCanvas());
    quadraticZoom->setText(tr("quadratic zoom"));
    quadraticZoom->setChecked(cfg.value("maps/quadraticZoom", false).toBool());
    theMainWindow->statusBar()->insertPermanentWidget(1,quadraticZoom);

    pathData    = cfg.value("path/data","./").toString();

    griddb      = new CGridDB(this);
    mapdb       = new CMapDB(tabbar, this);
    wptdb       = new CWptDB(tabbar, this);
    trackdb     = new CTrackDB(tabbar, this);
    routedb     = new CRouteDB(tabbar, this);
    overlaydb   = new COverlayDB(tabbar, this);
    livelogdb   = new CLiveLogDB(tabbar, this);
    diarydb     = new CDiaryDB(canvasTab, this);
    searchdb    = new CSearchDB(tabbar, this);


    if(resources->useGeoDB())
    {
        geodb       = new CGeoDB(tabbar, this);
    }

    canvas->setupDelayed();

    connect(searchdb, SIGNAL(sigChanged()), canvas, SLOT(update()));
    connect(wptdb, SIGNAL(sigChanged()), canvas, SLOT(update()));
    connect(trackdb, SIGNAL(sigChanged()), canvas, SLOT(update()));
    connect(livelogdb, SIGNAL(sigChanged()), canvas, SLOT(update()));
    connect(overlaydb, SIGNAL(sigChanged()), canvas, SLOT(update()));
    connect(tabbar, SIGNAL(currentChanged(int)), this, SLOT(slotToolBoxChanged(int)));
    connect(routedb, SIGNAL(sigChanged()), this, SLOT(update()));
    connect(mapdb, SIGNAL(sigChanged()), this, SLOT(update()));

    connect(mapdb, SIGNAL(sigModified(const QString&)), this, SLOT(slotModified()));
    connect(wptdb, SIGNAL(sigModified(const QString&)), this, SLOT(slotModified()));
    connect(trackdb, SIGNAL(sigModified(const QString&)), this, SLOT(slotModified()));
    connect(diarydb, SIGNAL(sigModified(const QString&)), this, SLOT(slotModified()));
    connect(overlaydb, SIGNAL(sigModified(const QString&)), this, SLOT(slotModified()));
    connect(routedb, SIGNAL(sigModified(const QString&)), this, SLOT(slotModified()));

    if(geodb)
    {
        geodb->gainFocus();
    }
    else
    {
        searchdb->gainFocus();
    }

    // restore last session position and size of mainWidget
    {
        if ( cfg.contains("mainWidget/geometry"))
        {
            QRect r = cfg.value("mainWidget/geometry").toRect();
            //qDebug() << r << QDesktopWidget().screenGeometry();
            if (r.isValid() && QDesktopWidget().screenGeometry().intersects(r))
                setGeometry(r);
            else
                showMaximized();
        }
        else
        {
            setGeometry(0,0,800,600);
        }
    }

    // restore last session settings
    QList<int> sizes = mainSplitter->sizes();
    sizes[0] = 200;
    sizes[1] = 600;

    mainSplitter->setSizes(sizes);
    sizes = leftSplitter->sizes();
    sizes[0] = 200;
    sizes[1] = 200;
    sizes[2] = 200;

    leftSplitter->setSizes(sizes);

    if( cfg.contains("mainWidget/mainSplitter") )
    {
        mainSplitter->restoreState(cfg.value("mainWidget/mainSplitter",mainSplitter->saveState()).toByteArray());
    }
    if( cfg.contains("mainWidget/leftSplitter") )
    {
        leftSplitter->restoreState(cfg.value("mainWidget/leftSplitter",leftSplitter->saveState()).toByteArray());
    }

    sizes.clear();
    sizes << 40 << 60;

    rightSplitter->setSizes(sizes);

    if( cfg.contains("mainWidget/rightSplitter") )
    {
        rightSplitter->restoreState(cfg.value("mainWidget/rightSplitter",rightSplitter->saveState()).toByteArray());
    }

    connect(&CMapDB::self(), SIGNAL(sigChanged()), this, SLOT(slotDataChanged()));
    connect(&CWptDB::self(), SIGNAL(sigChanged()), this, SLOT(slotDataChanged()));
    connect(&CTrackDB::self(), SIGNAL(sigChanged()), this, SLOT(slotDataChanged()));
    connect(&COverlayDB::self(), SIGNAL(sigChanged()), this, SLOT(slotDataChanged()));
    connect(&CDiaryDB::self(), SIGNAL(sigChanged()), this, SLOT(slotDataChanged()));
    connect(&CRouteDB::self(), SIGNAL(sigChanged()), this, SLOT(slotDataChanged()));

    slotDataChanged();

    connect(summary, SIGNAL(linkActivated(const QString&)),this,SLOT(slotOpenLink(const QString&)));

    canvas->setMouseMode(CCanvas::eMouseMoveArea);
    megaMenu->switchByKeyWord("Main");

    if (qlOpts->monitor != -1)
    {
        snRead = new QSocketNotifier(qlOpts->monitor, QSocketNotifier::Read, this);
        connect(snRead, SIGNAL(activated(int)), this, SLOT(slotReloadArgs()));
    }

    mostRecent = cfg.value("geodata/mostRecent",QStringList()).toStringList();

    if(!qlOpts->arguments.isEmpty())
    {
        CSearchDB::self().clear();
        CMapDB::self().clear();
        CWptDB::self().clear();
        CTrackDB::self().clear();
        CDiaryDB::self().clear();
        COverlayDB::self().clear();
        CRouteDB::self().clear();
        clear();

        foreach(QString arg, qlOpts->arguments)
        {
            loadData(arg, QString());
        }
    }
    modified = false;
    setTitleBar();

    setupMenuBar();
    connect(actionGroupProvider, SIGNAL(stateChanged()), this, SLOT(switchState()));

    megaMenu->slotSplitterMoved(leftSplitter->sizes()[0], 1);

#ifdef HAS_DBUS
    dbus = new CDBus(qApp);
#endif

    connect(&CTrackDB::self(), SIGNAL(sigHighlightTrack(CTrack *)), canvas, SLOT(slotHighlightTrack(CTrack*)));
    connect(&CTrackDB::self(), SIGNAL(sigChanged()), canvas, SLOT(slotTrackChanged()));
    connect(&soapHttp, SIGNAL(responseReady(const QtSoapMessage &)),this, SLOT(slotGetResponse(const QtSoapMessage &)));

}


void CMainWindow::slotReloadArgs()
{
    char c;
    int i;
#ifdef WIN32
                                 // read char
    i = _read(qlOpts->monitor, &c, 1);
#else
                                 // read char
    i = read(qlOpts->monitor, &c, 1);
#endif

    if(i != 1)
    {
        delete snRead;
        return;
    }

    CWptDB::self().clear();
    CTrackDB::self().clear();

    foreach(QString arg, qlOpts->arguments)
    {
        loadData(arg, QString());
    }

}


CMainWindow::~CMainWindow()
{

    SETTINGS;
    cfg.setValue("mainWidget/mainSplitter",mainSplitter->saveState());
    cfg.setValue("mainWidget/leftSplitter",leftSplitter->saveState());
    cfg.setValue("mainWidget/rightSplitter",rightSplitter->saveState());
    cfg.setValue("mainWidget/geometry", geometry());
    cfg.setValue("mainWidget/crashed",false);
    cfg.setValue("path/data",pathData);
    cfg.setValue("geodata/mostRecent", mostRecent);
    cfg.setValue("maps/quadraticZoom", quadraticZoom->isChecked());

    canvas = 0;
}


void CMainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if(event->mimeData()->hasUrls())
    {
        QList<QUrl> urls = event->mimeData()->urls();
        QFileInfo fi(urls[0].path());
        QString ext = fi.suffix().toUpper();

        if ( (ext == "QLB") || (ext == "GPX") || (ext == "TCX") )
        {
            event->acceptProposedAction();
        }

    }
}


void CMainWindow::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    QUrl url;

    foreach(url, urls)
    {
        QString filename = url.toLocalFile();
        QString filter;
        loadData(filename, filter);
    }

    event->acceptProposedAction();
}


void CMainWindow::setTitleBar()
{
    if(wksFile.isEmpty())
    {
        setWindowTitle(QString("QLandkarte GT") + (modified ? " *" : ""));
    }
    else
    {
        setWindowTitle(QString("QLandkarte GT - ") + QFileInfo(wksFile).fileName() + (modified ? " *" : ""));
    }
}


void CMainWindow::clearAll()
{
    QMessageBox::StandardButton res = QMessageBox::question(0, tr("Clear all..."), tr("This will erase all workspace data like waypoints and tracks."), QMessageBox::Ok|QMessageBox::Cancel, QMessageBox::Ok);

    if(res == QMessageBox::Ok)
    {
        IDB::signalsOff();

        CSearchDB::self().clear();
        CMapDB::self().clear();
        CWptDB::self().clear();
        CTrackDB::self().clear();
        CDiaryDB::self().clear();
        COverlayDB::self().clear();
        CRouteDB::self().clear();
        clear();

        IDB::signalsOn();
    }
}


void CMainWindow::clear()
{
    modified = false;
    wksFile.clear();
    setTitleBar();
}


void CMainWindow::setPositionInfo(const QString& info)
{
    statusCoord->setText(info);
}


void CMainWindow::switchState()
{
    if (groupProvidedMenu)
    {
        groupProvidedMenu->clear();
        actionGroupProvider->addActionsToMenu(groupProvidedMenu);
    }
}


void CMainWindow::setupMenuBar()
{
    QMenu * menu;

    menuMostRecent = new QMenu(tr("Load most recent..."),this);
    menuMostRecent->setIcon(QIcon(":/icons/iconFileLoad16x16.png"));

    QString recent;
    foreach(recent, mostRecent)
    {
        menuMostRecent->addAction(recent, this, SLOT(slotLoadRecent()));
    }

    menu = new QMenu(this);
    menu->setTitle(tr("&File"));
    menu->addAction(QIcon(":/icons/iconOpenMap16x16.png"),tr("Load Map"),this,SLOT(slotLoadMapSet()));
    menu->addAction(QIcon(":/icons/iconOpenMap16x16.png"),tr("Load Online Map"),this,SLOT(slotLoadOnlineMapSet()));
    menu->addSeparator();
    menu->addAction(QIcon(":/icons/iconFileLoad16x16.png"),tr("Load Geo Data"),this,SLOT(slotLoadData()), Qt::CTRL + Qt::Key_L);
    menu->addAction(QIcon(":/icons/iconFileSave16x16.png"),tr("Save Geo Data"),this,SLOT(slotSaveData()), Qt::CTRL + Qt::Key_S);
    menu->addAction(QIcon(":/icons/iconFileExport16x16.png"),tr("Export Geo Data"),this,SLOT(slotExportData()), Qt::CTRL + Qt::Key_X);
    menu->addAction(QIcon(":/icons/iconFileAdd16x16.png"),tr("Add Geo Data"),this,SLOT(slotAddData()), Qt::ALT + Qt::Key_A);
    menu->addMenu(menuMostRecent);
    menu->addSeparator();
    menu->addAction(QIcon(":/icons/iconScreenshot16x16.png"),tr("Device Screenshot ..."),this,SLOT(slotScreenshot()));
    menu->addAction(QIcon(":/icons/iconRaster16x16.png"),tr("Save map as image ..."),this,SLOT(slotSaveImage()));
    menu->addAction(QIcon(":/icons/iconPrint16x16.png"),tr("Print Map ..."),this,SLOT(slotPrint()), Qt::CTRL + Qt::Key_P);
    //menu->addAction(QIcon(":/icons/iconPrint16x16.png"),tr("Print Diary ..."),this,SLOT(slotPrintPreview()));
    menu->addSeparator();
    menu->addAction(QIcon(":/icons/iconUnknown16x16.png"),tr("Toggle toolview"),this,SLOT(slotToggleToolView()), Qt::CTRL + Qt::Key_T);
#if defined(Q_OS_MAC)
    menu->addAction(QIcon(":/icons/iconExit16x16.png"),("Exit"),this,SLOT(close()), Qt::CTRL + Qt::Key_Q);
#else
    menu->addAction(QIcon(":/icons/iconExit16x16.png"),tr("Exit"),this,SLOT(close()), Qt::CTRL + Qt::Key_Q);
#endif
    menuBar()->addMenu(menu);

    menu = new QMenu(this);
    actionGroupProvider->addActionsToMenu(menu,CMenus::MenuBarMenu,CMenus::EditMenu);
    menu->setTitle(tr("&Edit"));
    menuBar()->addMenu(menu);

    menu = new QMenu(this);
    actionGroupProvider->addActionsToMenu(menu,CMenus::MenuBarMenu,CMenus::MapMenu);
    menu->setTitle(tr("&Map"));
    menuBar()->addMenu(menu);

    menu = new QMenu(this);
    actionGroupProvider->addActionsToMenu(menu,CMenus::MenuBarMenu,CMenus::WptMenu);
    menu->setTitle(tr("&Waypoint"));
    menuBar()->addMenu(menu);

    menu = new QMenu(this);
    actionGroupProvider->addActionsToMenu(menu,CMenus::MenuBarMenu,CMenus::TrackMenu);
    menu->setTitle(tr("&Track"));
    menuBar()->addMenu(menu);

    menu = new QMenu(this);
    actionGroupProvider->addActionsToMenu(menu,CMenus::MenuBarMenu,CMenus::RouteMenu);
    menu->setTitle(tr("&Route"));
    menuBar()->addMenu(menu);

    menu = new QMenu(this);
    actionGroupProvider->addActionsToMenu(menu,CMenus::MenuBarMenu,CMenus::LiveLogMenu);
    menu->setTitle(tr("&Live Log"));
    menuBar()->addMenu(menu);

    menu = new QMenu(this);
    actionGroupProvider->addActionsToMenu(menu,CMenus::MenuBarMenu,CMenus::OverlayMenu);
    menu->setTitle(tr("&Overlay"));
    menuBar()->addMenu(menu);

#ifdef HAS_DBUS
    // note: currently, the More menu holds only one entry if DBus is enabled...
    menu = new QMenu(this);
    actionGroupProvider->addActionsToMenu(menu,CMenus::MenuBarMenu,CMenus::MainMoreMenu);
    menu->setTitle(tr("Mor&e"));
    menuBar()->addMenu(menu);
#endif

    menu = new QMenu(this);
    menu->setTitle(tr("&Setup"));
#if defined(Q_OS_MAC)
    menu->addAction(QIcon(":/icons/iconConfig16x16.png"),"&Preferences",this,SLOT(slotConfig()));
#else
    menu->addAction(QIcon(":/icons/iconConfig16x16.png"),tr("&General"),this,SLOT(slotConfig()));
#endif
    menuBar()->addMenu(menu);

    menu = new QMenu(this);
#if defined(Q_OS_MAC)
    menu->setTitle(("&Help"));
    menu->addAction(QIcon(":/icons/iconHelp16x16.png"),("http://FAQ"),this,SLOT(slotFAQ()));
    menu->addAction(QIcon(":/icons/iconHelp16x16.png"),("http://Help"),this,SLOT(slotHelp()));
    menu->addAction(QIcon(":/icons/iconHelp16x16.png"),("http://Support"),this,SLOT(slotSupport()));
    menu->addSeparator();
    menu->addAction(QIcon(":/icons/iconGlobe16x16.png"),("About &QLandkarte GT"),this,SLOT(slotCopyright()));
#else
    menu->setTitle(tr("&Help"));
    menu->addAction(QIcon(":/icons/iconHelp16x16.png"),tr("http://FAQ"),this,SLOT(slotFAQ()));
    menu->addAction(QIcon(":/icons/iconHelp16x16.png"),tr("http://Help"),this,SLOT(slotHelp()));
    menu->addAction(QIcon(":/icons/iconHelp16x16.png"),tr("http://Support"),this,SLOT(slotSupport()));
    menu->addSeparator();
    menu->addAction(QIcon(":/icons/iconGlobe16x16.png"),tr("About &QLandkarte GT"),this,SLOT(slotCopyright()));
#endif
    menuBar()->addMenu(menu);
    slotUpdate();
}


void CMainWindow::closeEvent(QCloseEvent * e)
{
    if(!modified)
    {
        e->accept();
        return;
    }

    if(!(CResources::self().useGeoDB() && CResources::self().saveGeoDBOnExit()))
    {
        if (maybeSave())
        {
            e->accept();
        }
        else
        {
            e->ignore();
        }
    }
    else
    {
        e->accept();
    }
}


void CMainWindow::slotLoadMapSet()
{
    SETTINGS;

    QString filter   = cfg.value("maps/filter","").toString();
    QString filename = QFileDialog::getOpenFileName( 0, tr("Select map...")
        ,CResources::self().pathMaps
        ,"All (*.*);;Map Collection (*.qmap);;Garmin (*.tdb);;BirdsEye (*.jnx);;TwoNav (*.rmap);;Magellan (*.rmp)"
        , &filter
        , FILE_DIALOG_FLAGS
        );
    if(filename.isEmpty()) return;

    CResources::self().pathMaps = QFileInfo(filename).absolutePath();
    CMapDB::self().openMap(filename, false, *canvas);

    cfg.setValue("maps/filter",filter);
}


void CMainWindow::slotLoadOnlineMapSet()
{
    CDlgLoadOnlineMap dlg;
    dlg.exec();

    QString filename = dlg.selectedfile;

    if(filename.isEmpty()) return;

    CResources::self().pathMaps = QFileInfo(filename).absolutePath();
    CMapDB::self().openMap(filename, false, *canvas);
}


void CMainWindow::slotCopyright()
{
    CCopyright dlg;
    dlg.exec();
}


void CMainWindow::slotToolBoxChanged(int idx)
{
    QString key = tabbar->widget(idx)->objectName();
    megaMenu->switchByKeyWord(key);
}


void CMainWindow::slotConfig()
{
    CDlgConfig dlg(this);
    dlg.exec();
}


void CMainWindow::slotLoadData()
{
    QString formats = getGeoDataFormats();

    SETTINGS;
    QString filter   = cfg.value("geodata/filter","").toString();
    QStringList filenames = QFileDialog::getOpenFileNames( 0, tr("Select input files")
        ,pathData
        ,formats
        ,&filter
        , FILE_DIALOG_FLAGS
        );

    if(filenames.isEmpty()) return;

    if(modified)
    {
        if(!maybeSave())
        {
            return;
        }
    }

    IDB::signalsOff();

    CMapDB::self().clear();
    CWptDB::self().clear();
    CTrackDB::self().clear();
    CRouteDB::self().clear();
    CDiaryDB::self().clear();
    COverlayDB::self().clear();

    QString filename;
    foreach(filename, filenames)
    {
        if (loadData(filename, filter)) addRecent(filename);
    }

    IDB::signalsOn();

    wksFile = filename;

    modified = false;
    setTitleBar();

    cfg.setValue("geodata/filter",filter);
}


void CMainWindow::slotAddData()
{
    QString formats = getGeoDataFormats();

    SETTINGS;
    int i;
    QString filename;
    QString filter   = cfg.value("geodata/filter","").toString();
    QStringList filenames = QFileDialog::getOpenFileNames( 0, tr("Select input files")
        ,pathData
        ,formats
        ,&filter
        , FILE_DIALOG_FLAGS
        );

    for (i = 0; i < filenames.size(); ++i)
    {
        filename = filenames.at(i);
        if(filename.isEmpty()) return;

        QString tmp = wksFile;
        if (loadData(filename, filter)) addRecent(filename);

        wksFile = tmp;

        modified = true;
        setTitleBar();

        cfg.setValue("geodata/filter",filter);
    }

}


bool CMainWindow::loadData(const QString& filename, const QString& filter)
{
    QTemporaryFile tmpfile;
    bool success = false;
    bool loadGPXData = false;
    QFileInfo fileInfo(filename);
    QString ext = fileInfo.suffix().toUpper();

    pathData = fileInfo.absolutePath();

    qDebug() << filter;

    try
    {
        if(ext == "QLB")
        {
            CQlb qlb(this);
            qlb.load(filename);
            CMapDB::self().loadQLB(qlb, false);
            CWptDB::self().loadQLB(qlb, false);
            CTrackDB::self().loadQLB(qlb, false);
            CRouteDB::self().loadQLB(qlb, false);
            CDiaryDB::self().loadQLB(qlb, false);
            COverlayDB::self().loadQLB(qlb, false);
        }
        else if(ext == "GPX")
        {
            CGpx gpx(this, CGpx::eQlgtExport);
            gpx.load(filename);
            CMapDB::self().loadGPX(gpx);
            CWptDB::self().loadGPX(gpx);
            CTrackDB::self().loadGPX(gpx);
            CRouteDB::self().loadGPX(gpx);
            CDiaryDB::self().loadGPX(gpx);
            COverlayDB::self().loadGPX(gpx);
        }
        else if(ext == "TCX")
        {
            TcxReader tcxReader(this);
            if (!tcxReader.read(filename))
            {
                throw(tcxReader.errorString());
            }
            else
            {
                //emit CTrackDB::self().sigChanged(); //??
                QRectF r = CTrackDB::self().getBoundingRectF();
                if (!r.isNull ())
                {
                    CMapDB::self().getMap().zoom(r.left() * DEG_TO_RAD, r.top() * DEG_TO_RAD, r.right() * DEG_TO_RAD, r.bottom() * DEG_TO_RAD);
                }
            }
        }
        else
        {

            tmpfile.open();
            loadGPXData = false;

            if(filter.startsWith("TwoNav"))
            {
                loadGPXData = convertData("compegps", filename, "gpx,garminextensions", tmpfile.fileName());
            }
            else if(ext == "LOC")
            {
                loadGPXData = convertData("geo", filename, "gpx,garminextensions", tmpfile.fileName());
            }
            else if(ext == "GDB")
            {

                loadGPXData = convertData("gdb", filename, "gpx,garminextensions", tmpfile.fileName());
            }
            else if(ext == "KML")
            {
                loadGPXData = convertData("kml", filename, "gpx,garminextensions", tmpfile.fileName());
            }
            else if(ext == "PLT" || ext == "WPT" || ext == "RTE")
            {
                loadGPXData = convertData("ozi", filename, "gpx,garminextensions", tmpfile.fileName());
            }
            else if(ext == "TK1")
            {
                loadGPXData = convertData("wbt-tk1", filename, "gpx,garminextensions", tmpfile.fileName());
            }
            else if(ext =="CSV")
            {
                loadGPXData = convertData("unicsv", filename, "gpx,garminextensions", tmpfile.fileName());
            }

            if (!loadGPXData)
            {
                QMessageBox::critical(0,tr("Convert error"),"Error in data conversion?",QMessageBox::Ok,QMessageBox::NoButton);
            }

            CGpx gpx(this, CGpx::eQlgtExport);
            gpx.load(tmpfile.fileName());
            CMapDB::self().loadGPX(gpx);
            CWptDB::self().loadGPX(gpx);
            CTrackDB::self().loadGPX(gpx);
            CRouteDB::self().loadGPX(gpx);
            CDiaryDB::self().loadGPX(gpx);
            COverlayDB::self().loadGPX(gpx);

        }
        wksFile = filename;
        success = true;
    }
    catch(const QString& msg)
    {
        wksFile.clear();
        QMessageBox:: critical(this,tr("Error"), msg, QMessageBox::Cancel, QMessageBox::Cancel);
    }
    return success;
}


bool CMainWindow::convertData(const QString& inFormat, const QString& inFile, const QString& outFormat, const QString& outFile)
{
    QString program = GPSBABEL;
    QStringList arguments;

    if(inFormat == "unicsv")
    {
        arguments /* << "-t" */ << "-w" << "-r" << "-i" << inFormat << "-f" << inFile << "-o" << outFormat << "-F" << outFile;
    }
    else
    {
        arguments << "-t"  << "-w" << "-r" << "-i" << inFormat << "-f" << inFile << "-o" << outFormat << "-F" << outFile;
    }

    QProcess *babelProcess = new QProcess(this);
    babelProcess->start(program, arguments);
    if (!babelProcess->waitForStarted())
    {
        return false;
    }

    if (!babelProcess->waitForFinished())
    {
        return false;
    }

    return babelProcess->exitCode() == 0;
}


bool CMainWindow::maybeSave()
{
    QMessageBox::StandardButton ret;
    ret = QMessageBox::warning(this, tr("Save geo data?"),
        tr("The loaded data has been modified.\n"
        "Do you want to save your changes?"),
        QMessageBox::Save | QMessageBox::Discard| QMessageBox::Cancel);

    if (ret == QMessageBox::Save)
    {
        if(wksFile.isEmpty())
        {
            slotSaveData();
        }
        else
        {
            SETTINGS;
            QString filter = cfg.value("geodata/filter","").toString();
            saveData(wksFile, filter);
        }
        return true;
    }
    else if (ret == QMessageBox::Cancel)
    {
        return false;
    }
    return true;
}


void CMainWindow::slotSaveData()
{
    SETTINGS;

    QString filter =cfg.value("geodata/filter","").toString();

    QString filename = QFileDialog::getSaveFileName( 0, tr("Select output file")
        ,pathData
        ,"QLandkarte (*.qlb);;GPS Exchange (*.gpx)"
        ,&filter
        , FILE_DIALOG_FLAGS
        );

    if(filename.isEmpty()) return;

    cfg.setValue("geodata/filter",filter);
    saveData(filename, filter);
    addRecent(filename);
}


void CMainWindow::slotExportData()
{
    SETTINGS;

    bool haveGPSBabel = isGPSBabel();

    QString formats;
    if(haveGPSBabel)
    {
        //formats = "All supported files (*.qlb *.gpx *.tcx *.loc *.gdb *.kml *.plt *.rte *.wpt);;QLandkarte (*.qlb);;GPS Exchange (*.gpx);;TCX TrainingsCenterExchange (*.tcx);;Geocaching.com - EasyGPS (*.loc);;Mapsource (*.gdb);;Google Earth (*.kml);;Ozi Track (*.plt);;Ozi Route (*.rte);;Ozi Waypoint (*.wpt)";
        formats = "All supported files (*.gpx *.plt *.rte *.wpt);;GPS Exchange (*.gpx);;OziExplorer (*.plt *.rte *.wpt)";
    }
    else
    {
        formats = "All supported files (*.gpx);;GPS Exchange (*.gpx)";
    }

    QString filter = cfg.value("geodata/filter","").toString();
    QString filename = QFileDialog::getSaveFileName( 0, tr("Select output file")
        ,pathData
        ,formats
        ,&filter
        , FILE_DIALOG_FLAGS
        );

    if(filename.isEmpty()) return;

    cfg.setValue("geodata/filter",filter);

    saveData(filename, filter, true);
    addRecent(filename);
}


void CMainWindow::saveData(QString& fn, const QString& filter, bool exportFlag)
{
    QTemporaryFile tmpfile;
    tmpfile.open();
    QString filename = fn;
    QFileInfo fileInfo(filename);
    QString ext = fileInfo.suffix().toUpper();

    if(!exportFlag)
    {
        if ((filter == "GPS Exchange (*.gpx)") || (ext == "GPX"))
        {
            if (ext!="GPX")
            {
                filename += ".gpx";
                ext = "GPX";
            }
        }
        else
        {
            if(ext != "QLB")
            {
                filename += ".qlb";
            }
            ext = "QLB";
        }
    }
    else
    {
        if ((ext == "GPX") || (filter == "GPS Exchange (*.gpx)"))
        {
            if(ext != "GPX")
            {
                filename += ".gpx";
            }
            ext = "GPX";
        }
        else if ((ext == "PLT") || (ext == "RTE") || (ext == "WPT") || (filter == "OziExplorer (*.plt *.rte *.wpt)"))
        {
            if ((ext == "PLT") || (ext == "RTE") || (ext == "WPT"))
            {
                filename = filename.left(filename.length()-4);
            }
            ext = "OZI";
        }
        else
        {
            if(ext != "QLB")
            {
                filename += ".qlb";
            }
            ext = "QLB";
        }
    }

    fileInfo.setFile(filename);
    pathData = fileInfo.absolutePath();

    try
    {
        if(ext == "QLB")
        {
            CQlb qlb(this);
            CMapDB::self().saveQLB(qlb);
            CWptDB::self().saveQLB(qlb);
            CTrackDB::self().saveQLB(qlb);
            CRouteDB::self().saveQLB(qlb);
            CDiaryDB::self().saveQLB(qlb);
            COverlayDB::self().saveQLB(qlb);
            qlb.save(filename);
        }
        if(ext == "GPX" || ext == "OZI")
        {
            QStringList keysWpt, keysTrk, keysRte;

            if(exportFlag)
            {
                CDlgExport dlg(0, &keysWpt, &keysTrk, &keysRte);

                if( dlg.exec() == QDialog::Rejected)
                {
                    return;
                }
            }

            CGpx gpx(this, exportFlag ? CGpx::eCleanExport : CGpx::eQlgtExport);
            CMapDB::self().saveGPX(gpx, QStringList());
            CWptDB::self().saveGPX(gpx, keysWpt);
            CTrackDB::self().saveGPX(gpx, keysTrk);
            CRouteDB::self().saveGPX(gpx, keysRte);
            gpx.makeExtensions();
            CDiaryDB::self().saveGPX(gpx, QStringList());
            COverlayDB::self().saveGPX(gpx, QStringList());
            if (ext == "OZI")
            {
                gpx.save(tmpfile.fileName());
            }
            else
            {
                gpx.save(filename);
            }
        }
        if (ext == "OZI" )
        {
            convertData("gpx", tmpfile.fileName(),"ozi",filename);
        }

        wksFile  = filename;
        modified = false;
    }
    catch(const QString& msg)
    {
        wksFile.clear();
        QMessageBox:: critical(this,tr("Error"), msg, QMessageBox::Cancel, QMessageBox::Cancel);
        return;
    }

    setTitleBar();

    fn = filename;
}


void CMainWindow::exportToOcm()
{
#ifdef HAS_DBUS
    int cnt = 0;
    QProcess ocm;
    QDBusMessage msg;
    QStringList keysWpt, keysTrk, keysRte;

    CDlgExport dlg(0, &keysWpt, &keysTrk, &keysRte);

    if( dlg.exec() == QDialog::Rejected)
    {
        return;
    }

    CGpx gpx(this, CGpx::eOcmExport);
    CWptDB::self().saveGPX(gpx, keysWpt);
    CTrackDB::self().saveGPX(gpx, keysTrk);
    CRouteDB::self().saveGPX(gpx, keysRte);
    gpx.makeExtensions();
    COverlayDB::self().saveGPX(gpx, QStringList());

    QDBusConnection dbus = QDBusConnection::sessionBus();
    QStringList serviceNames = dbus.interface()->registeredServiceNames();
    while(!serviceNames.contains("org.ocm.dbus"))
    {
        if(cnt > 2)
        {
            QMessageBox::warning(0,tr("Failed ..."), tr("Failed to start OCM."), QMessageBox::Abort);
            return;
        }

        //        qDebug() << "start ocm-gtk";
        ocm.startDetached("ocm-gtk");
        sleep(3);
        cnt++;
    }

    QTemporaryFile file;
    file.open();
    //file.close();
    gpx.save(file.fileName());

    msg = QDBusMessage::createMethodCall("org.ocm.dbus", "/org/ocm/dbus", "org.ocm.dbus", "ShowOCM");
    msg = dbus.call(msg);

    //    qDebug() << msg.errorName()  << msg.errorMessage();

    msg = QDBusMessage::createMethodCall("org.ocm.dbus", "/org/ocm/dbus", "org.ocm.dbus", "ImportGPX");
    msg << file.fileName();
    msg = dbus.call(msg);

    //    qDebug() << msg.errorName()  << msg.errorMessage();
#endif
}


void CMainWindow::slotPrint()
{
    QPrinter printer;
    printer.setOrientation(QPrinter::Landscape);
    QPrintDialog dialog(&printer, this);
    dialog.setWindowTitle(tr("Print Map"));
    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    canvas->print(printer);
}


void CMainWindow::slotSaveImage()
{

    SETTINGS;
    QString pathData = cfg.value("path/data","./").toString();
    QString filter   = cfg.value("canvas/imagetype","Bitmap (*.png)").toString();

    QString filename = QFileDialog::getSaveFileName( 0, tr("Select output file")
        ,pathData
        ,"Bitmap (*.png)"
        ,&filter
        , FILE_DIALOG_FLAGS
        );

    if(filename.isEmpty()) return;

    QFileInfo fi(filename);
    if(fi.suffix().toLower() != "png")
    {
        filename += ".png";
    }

    CMap3D * map3d = qobject_cast<CMap3D*>(canvasTab->currentWidget());

    if(map3d)
    {
        map3d->slotSaveImage(filename);
    }
    else
    {
        QImage img(canvas->size(), QImage::Format_ARGB32);
        canvas->print(img);
        img.save(filename);
    }
    pathData = fi.absolutePath();
    cfg.setValue("path/data", pathData);
    cfg.setValue("canvas/imagetype", filter);

}


void CMainWindow::slotModified()
{
    modified = true;
    setTitleBar();
}


void CMainWindow::slotDataChanged()
{

    int c;
    QString str = tr("<div style='float: left;'><b>Workspace Summary (<a href='Clear'>clear</a> workspace):</b></div>");

    str += "<p>";
    c = CWptDB::self().count();
    if(c > 0)
    {
        if(c == 1)
        {
            str += tr("Currently there is %1 <a href='Waypoints'>waypoint</a>, ").arg(c);
        }
        else
        {
            str += tr("Currently there are %1 <a href='Waypoints'>waypoints</a>, ").arg(c);
        }
    }
    else
    {
        str += tr("There are no waypoints, ");
    }

    c = CTrackDB::self().count();
    if(c > 0)
    {
        if(c == 1)
        {
            str += tr(" %1 <a href='Tracks'>track</a>, ").arg(c);
        }
        else
        {
            str += tr(" %1 <a href='Tracks'>tracks</a>, ").arg(c);
        }
    }
    else
    {
        str += tr("no tracks, ");
    }

    c = CRouteDB::self().count();
    if(c > 0)
    {
        if(c == 1)
        {
            str += tr(" %1 <a href='Routes'>route</a> and ").arg(c);
        }
        else
        {
            str += tr(" %1 <a href='Routes'>routes</a> and ").arg(c);
        }
    }
    else
    {
        str += tr("no routes and ");
    }

    c = COverlayDB::self().count();
    if(c > 0)
    {
        if(c == 1)
        {
            str += tr(" %1 <a href='Overlay'>overlay</a>. ").arg(c);
        }
        else
        {
            str += tr(" %1 <a href='Overlay'>overlays</a>. ").arg(c);
        }
    }
    else
    {
        str += tr("no overlays. ");
    }

    //    c = CDiaryDB::self().count();
    //    if(c > 0)
    //    {
    //        str += tr("A <a href='Diary'>diary</a> is loaded.");
    //    }
    //    else
    //    {
    //        str += tr("The diary (<a href='Diary'>new</a>) is empty.");
    //    }

    str += "</p>";

    summary->setText(str);

}


void CMainWindow::slotOpenLink(const QString& link)
{
    if(link == "Diary")
    {
        //        CDiaryDB::self().openEditWidget();
    }
    else if(link == "Clear")
    {
        clearAll();
    }
    else
    {
        CMegaMenu::self().switchByKeyWord(link);
    }
}


void CMainWindow::slotCurrentDeviceChanged(int i)
{
    resources->m_devKey = comboDevice->itemData(i).toString();
}


void CMainWindow::slotDeviceChanged()
{
    QString devKey = resources->m_devKey;

    comboDevice->clear();
    comboDevice->addItem(tr(""),"");
    comboDevice->addItem(tr("QLandkarte M"), "QLandkarteM");
    comboDevice->addItem(resources->m_devType, "Garmin");
    comboDevice->addItem(tr("Garmin Mass Storage"), "Garmin Mass Storage");
    comboDevice->addItem(tr("Magellan"), "Magellan");
    comboDevice->addItem(tr("TwoNav"), "TwoNav");
    comboDevice->addItem(tr("NMEA"), "NMEA");
#ifdef HAS_GPSD
    comboDevice->addItem(tr("GPSD"), "GPSD");
#endif

    resources->m_devKey = devKey;
    comboDevice->setCurrentIndex(comboDevice->findData(resources->m_devKey));
}


void CMainWindow::slotScreenshot()
{
    CDlgScreenshot dlg(this);
    dlg.exec();

}


void CMainWindow::slotLoadRecent()
{
    QAction * act = qobject_cast<QAction*>(sender());
    if(act)
    {
        QString filename = act->text();

        if(modified)
        {
            if(!maybeSave())
            {
                return;
            }
        }

        CMapDB::self().clear();
        CWptDB::self().clear();
        CTrackDB::self().clear();
        CRouteDB::self().clear();
        CDiaryDB::self().clear();
        COverlayDB::self().clear();

        if (loadData(filename,"")) addRecent(filename);

        wksFile = filename;

        modified = false;
        setTitleBar();
    }
}


void CMainWindow::addRecent(const QString& filename)
{
    int recentpos = mostRecent.indexOf(filename);
    if (recentpos > -1)
    {
        if (!recentpos) return;
        mostRecent.move(recentpos, 0);
    }
    else
    {
        mostRecent.prepend(filename);
    }

    if(mostRecent.count() >= 10)
    {
        mostRecent.removeLast();
    }

    menuMostRecent->clear();
    QString recent;
    foreach(recent, mostRecent)
    {
        menuMostRecent->addAction(recent, this, SLOT(slotLoadRecent()));
    }
}


void CMainWindow::setTempWidget(QWidget * w, const QString& label)
{
    tmpTabWidget->addTab(w, label);
    tmpTabWidget->show();
    tmpTabWidget->setCurrentWidget(w);

    connect(w, SIGNAL(destroyed(QObject*)), this, SLOT(slotItemDestroyed(QObject*)));
}


void CMainWindow::slotItemDestroyed(QObject *)
{
    if(tmpTabWidget->count() < 2)
    {
        tmpTabWidget->hide();
    }
}


void CMainWindow::slotTabCloseRequest(int i)
{
    QWidget * w = tmpTabWidget->widget(i);
    if(w)
    {
        w->deleteLater();
    }
}


void CMainWindow::slotFAQ()
{
    QDesktopServices::openUrl(QUrl("http://sourceforge.net/p/qlandkartegt/qlandkartegt/FAQ/"));
}


void CMainWindow::slotHelp()
{
    QDesktopServices::openUrl(QUrl("http://sourceforge.net/p/qlandkartegt/qlandkartegt/QLandkarte_GT/"));
}


void CMainWindow::slotSupport()
{
    QDesktopServices::openUrl(QUrl("http://www.qlandkarte.org/530747a0730821603/index.html"));
}


void CMainWindow::slotDownload()
{
    QDesktopServices::openUrl(QUrl("http://www.qlandkarte.org/530747a0720dfbb0d/index.html"));
}


void CMainWindow::slotUpdate()
{
    SETTINGS;
    int result = cfg.value("checkVersion", -1).toInt();
    if(result == -1)
    {
        QString msg = tr(
            "QLandkarte GT can query for new versions on start-up. If there is a new version available, it will display a short notice in the statusbar. "
            "To query for a new version QLandkarte GT has to connect to the server"
            "\n\n"
            "http://www.qlandkarte.org/webservice/qlandkartegt.php"
            "\n\n"
            "It won't transmit any private data other than needed for requesting a HTML page."
            "\n\n"
            "If you want QLandkarte GT to do this query now and in the future press 'Ok'. Else press 'No'."
            "\n\n"
            "You won't be bugged a second time unless you erase QLandkarte's configuration data."
            );

        QMessageBox::StandardButton r = QMessageBox::question(this, tr("Query for new version..."),msg,QMessageBox::Ok|QMessageBox::No, QMessageBox::Ok);
        if(r == QMessageBox::Ok)
        {
            cfg.setValue("checkVersion", 1);
        }
        else
        {
            cfg.setValue("checkVersion", 0);
            return;
        }
    }
    else if(result == 0)
    {
        return;
    }

    QtSoapMessage request;
    request.setMethod(QtSoapQName("getlastversion", "urn:qlandkartegt"));
    request.addMethodArgument("changelog", "", "0");
    soapHttp.setHost("www.qlandkarte.org");
    soapHttp.submitRequest(request, "/webservice/qlandkartegt.php");
}


void CMainWindow::slotGetResponse(const QtSoapMessage &message)
{
    if (message.isFault())
    {
        qDebug("Error: %s", qPrintable(message.faultString().toString()));
    }
    else
    {
        //qDebug("message : %s",qPrintable(message.toXmlString(1)));
        QString res = message.returnValue().toString();
        res = res.simplified();
        if (!res.isEmpty() && res != VER_STR)
        {
            statusBar()->showMessage(tr("New QLandkarte GT %1 available").arg(res),0);
            qDebug("Version is: %s", res.toLatin1().constData());
        }
    }
}


void CMainWindow::slotToggleToolView()
{
    qDebug() << "CMainWindow::slotToggleToolView()";
    if(leftSplitter->isHidden())
    {
        leftSplitter->show();
    }
    else
    {
        leftSplitter->hide();
    }
}


bool CMainWindow::isGPSBabel()
{
    bool haveGPSBabel = false;
    QProcess proc1;
    proc1.start(GPSBABEL " -V");
    proc1.waitForFinished();
    haveGPSBabel = proc1.error() == QProcess::UnknownError;
    return haveGPSBabel;
}


QString CMainWindow::getGeoDataFormats()
{

    bool haveGPSBabel = isGPSBabel();

    QString formats;
    if(haveGPSBabel)
    {
        formats = "All supported files (*.qlb *.gpx *.tcx *.loc *.gdb *.kml *.plt *.rte *.wpt *.tk1);;QLandkarte (*.qlb);;GPS Exchange (*.gpx);;TwoNav (*.trk *.rte *.wpt);;TCX TrainingsCenterExchange (*.tcx);;Geocaching.com - EasyGPS (*.loc);;Mapsource (*.gdb);;Google Earth (*.kml);;Ozi Track (*.plt);;Ozi Route (*.rte);;Ozi Waypoint (*.wpt);;Wintec WBT201/1000 (*.tk1);;Universal CSV (*.csv)";
    }
    else
    {
        formats = "All supported files (*.qlb *.gpx *.tcx);;QLandkarte (*.qlb);;GPS Exchange (*.gpx);;TCX TrainingsCenterExchange (*.tcx)";
    }
    return formats;
}
