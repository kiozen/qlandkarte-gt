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

#include <QMessageBox>
#include "CResources.h"
#include "CDeviceMagellan.h"
#include "CDeviceGarmin.h"
#include "CDeviceGarminBulk.h"
#include "CDeviceTwoNav.h"
#include "CDeviceQLandkarteM.h"
#include "CDeviceNMEA.h"
#ifdef HAS_GPSD
#include "CDeviceGPSD.h"
#endif
#include "CLiveLogDB.h"
#include "CUnitMetric.h"
#include "CUnitNautic.h"
#include "CUnitImperial.h"
#include "CMapTDB.h"
#include "config.h"
#include "CSettings.h"
#include "CCanvas.h"

#ifndef Q_OS_WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <unistd.h>
#endif

#include <QtGui>

#include "CMainWindow.h"
#include "CDlgProxy.h"

CResources * CResources::m_self = 0;

CResources::CResources(QObject * parent)
: QObject(parent)
, pathMaps("./")
, m_useHttpProxy(false)
, m_httpProxyPort(8080)
, time_offset(0)
, m_device(0)
, m_devIPPort(4242)
, m_flipMouseWheel(false)
, m_useGeoDB(true)
, m_saveGeoDBOnExit(false)
, m_saveGeoDBMinutes(5)
#ifndef Q_OS_MAC
, m_pathGeoDB(QDir::homePath())
#else
, m_pathGeoDB(QDir::home().filePath(CONFIGDIR))
#endif
, m_showTrackProfile(true)
, m_showTrackEleInfo(true)
, m_showNorth(true)
, m_showClock(true)
, m_showScale(true)
, m_showToolTip(true)
, m_showElementInfo(true)
, m_showZoomLevel(true)
, m_useAntiAliasing(true)
, m_reducePoiIcons(true)
, m_zoomLevelThresholdPois(5.0)
, m_zoomLevelThresholdPoiLabels(2.0)
, m_WptTextColor(Qt::black)
, m_pathMapCache(QDir::temp().filePath("qlandkartegt/cache"))
, m_sizeMapCache(100)
, m_expireMapCache(8)
, m_tzMode(eTZAuto)
, m_timezone("UTC")
// BRouter service being used by http://brouter.de/brouter-web
, m_brouterHost("h2096617.stratoserver.net")
, m_brouterPort("443")
, m_brouterProfiles("trekking,fastbike,car-test,safety,shortest,trekking-ignore-cr,trekking-steep,trekking-noferries,trekking-nosteps,moped,rail,river,vm-forum-liegerad-schnell,vm-forum-velomobil-schnell")
, m_brouterLocal(false)
{
    m_self = this;

    SETTINGS;

#ifndef Q_OS_MAC
    QString family      = cfg.value("environment/mapfont/family","Arial").toString();
#else
    // Qt on Mac OS X 10.6 sometimes fails to render, so use the Mac default font here...
    QString family      = cfg.value("environment/mapfont/family","Lucida Grande").toString();
#endif
    int size            = cfg.value("environment/mapfont/size",8).toInt();
    bool bold           = cfg.value("environment/mapfont/bold",false).toBool();
    bool italic         = cfg.value("environment/mapfont/italic",false).toBool();
    m_mapfont = QFont(family,size);
    m_mapfont.setBold(bold);
    m_mapfont.setItalic(italic);

    //m_doMetric        = cfg.value("environment/doMetric",true).toBool();
    m_flipMouseWheel    = cfg.value("environment/flipMouseWheel",m_flipMouseWheel).toBool();
    m_useGeoDB          = cfg.value("environment/GeoDB",m_useGeoDB).toBool();
    m_saveGeoDBOnExit   = cfg.value("environment/saveGeoDBOnExit",m_saveGeoDBOnExit).toBool();
    m_saveGeoDBMinutes  = cfg.value("environment/saveGeoDBMinutes",m_saveGeoDBMinutes).toUInt();
    m_pathGeoDB         = QDir(cfg.value("environment/pathGeoDB", m_pathGeoDB.absolutePath()).toString());

    m_useHttpProxy      = cfg.value("network/useProxy",m_useHttpProxy).toBool();
    m_httpProxy         = cfg.value("network/proxy/url",m_httpProxy).toString();
    m_httpProxyPort     = cfg.value("network/proxy/port",m_httpProxyPort).toUInt();

    if(m_useHttpProxy)
        QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::HttpProxy,m_httpProxy,m_httpProxyPort));

    else
        QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::NoProxy));

    m_devKey            = cfg.value("device/key",m_devKey).toString();
    m_devIPAddress      = cfg.value("device/ipAddr",m_devIPAddress).toString();
    m_devIPPort         = cfg.value("device/ipPort",m_devIPPort).toUInt();
    m_devSerialPort     = cfg.value("device/serialPort",m_devSerialPort).toString();
    m_devBaudRate       = cfg.value("device/baudRate",m_devBaudRate).toString();
    m_watchdogEnabled   = cfg.value("device/watchdog",m_watchdogEnabled).toBool();
    m_devType           = cfg.value("device/type",m_devType).toString();
    m_devCharset        = cfg.value("device/charset",m_devCharset).toString();

    emit sigDeviceChanged();

    m_playSound         = cfg.value("device/playSound",m_playSound).toBool();
    IDevice::m_DownloadAllTrk   = cfg.value("device/dnlTrk",IDevice::m_DownloadAllTrk).toBool();
    IDevice::m_DownloadAllWpt   = cfg.value("device/dnlWpt",IDevice::m_DownloadAllWpt).toBool();
    IDevice::m_DownloadAllRte   = cfg.value("device/dnlRte",IDevice::m_DownloadAllRte).toBool();
    IDevice::m_UploadAllWpt     = cfg.value("device/uplWpt",IDevice::m_UploadAllWpt).toBool();
    IDevice::m_UploadAllTrk     = cfg.value("device/uplTrk",IDevice::m_UploadAllTrk).toBool();
    IDevice::m_UploadAllRte     = cfg.value("device/uplRte",IDevice::m_UploadAllRte).toBool();

    pathMaps        = cfg.value("path/maps",pathMaps).toString();

    QString unittype = cfg.value("environment/unittype","metric").toString();
    if(unittype == "metric")
        unit = new CUnitMetric(this);
    else if(unittype == "nautic")
    {
        unit = new CUnitNautic(this);
    }
    else if(unittype == "imperial")
    {
        unit = new CUnitImperial(this);
    }
    else
    {
        qWarning("Unknown unit type. Using 'metric'");
        unit = new CUnitMetric(this);
    }

    m_showTrackProfile = cfg.value("environment/showTrackProfile",m_showTrackProfile).toBool();
    m_showTrackEleInfo = cfg.value("environment/showTrackEleInfo",m_showTrackEleInfo).toBool();
    m_showNorth        = cfg.value("environment/showNorth",m_showNorth).toBool();
    m_showClock        = cfg.value("environment/showClock",m_showClock).toBool();
    m_showScale        = cfg.value("environment/showScale",m_showScale).toBool();
    m_showToolTip      = cfg.value("environment/showToolTip",m_showToolTip).toBool();
    m_showElementInfo  = cfg.value("environment/showElementInfo", m_showElementInfo).toBool();
    m_showZoomLevel    = cfg.value("environment/showZoomLevel",m_showZoomLevel).toBool();
    m_useAntiAliasing  = cfg.value("environment/useAntiAliasing",m_useAntiAliasing).toBool();
    m_reducePoiIcons   = cfg.value("environment/reducePoiIcons",m_reducePoiIcons).toBool();

    m_zoomLevelThresholdPois      = cfg.value("environment/zoomLevelThresholdPois",m_zoomLevelThresholdPois).toDouble();
    m_zoomLevelThresholdPoiLabels = cfg.value("environment/zoomLevelThresholdPoiLabels",m_zoomLevelThresholdPoiLabels).toDouble();

    m_WptTextColor = QColor(cfg.value("environment/wptTextColor", m_WptTextColor.name()).toString());

    QDir dirWeb(QDir::home().filePath(CONFIGDIR));
    dirWeb.mkdir("WebStuff");
    dirWeb.cd("WebStuff");

    QPixmap(":/webstuff/frame_top.png").save(dirWeb.filePath("frame_top.png"));
    QPixmap(":/webstuff/frame_bottom.png").save(dirWeb.filePath("frame_bottom.png"));
    QPixmap(":/webstuff/frame_left.png").save(dirWeb.filePath("frame_left.png"));
    QPixmap(":/webstuff/frame_right.png").save(dirWeb.filePath("frame_right.png"));
    QPixmap(":/webstuff/frame_top_left.png").save(dirWeb.filePath("frame_top_left.png"));
    QPixmap(":/webstuff/frame_top_right.png").save(dirWeb.filePath("frame_top_right.png"));
    QPixmap(":/webstuff/frame_bottom_left.png").save(dirWeb.filePath("frame_bottom_left.png"));
    QPixmap(":/webstuff/frame_bottom_right.png").save(dirWeb.filePath("frame_bottom_right.png"));
    QPixmap(":/webstuff/scale.png").save(dirWeb.filePath("scale.png"));

    QString cacheFolder;
#ifndef Q_OS_WIN32
    const char *envCache = getenv("QLGT_CACHE");

    if (envCache)
    {
        cacheFolder = envCache;
    }
    else
    {
#ifndef Q_OS_MAC
        struct passwd * userInfo = getpwuid(getuid());
        cacheFolder = QDir::tempPath() + "/qlandkarteqt-" + userInfo->pw_name + "/cache/";
#else
        // Mac OS X: points to /Users/<user name>/Library/Caches/QLandkarteGT/QLandkarteGT
#ifdef QK_QT5_PORT
        cacheFolder = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
#else
        cacheFolder = QDesktopServices::storageLocation(QDesktopServices::CacheLocation);
#endif
#endif
    }
#else
    cacheFolder = m_pathMapCache.absolutePath();
#endif

    m_pathMapCache = QDir(cfg.value("network/mapcache/path", cacheFolder).toString());
    m_sizeMapCache = cfg.value("network/mapcache/size", m_sizeMapCache).toInt();
    m_expireMapCache = cfg.value("network/mapcache/expire", m_expireMapCache).toInt();

    m_tzMode = (TimezoneMode_e)cfg.value("timezone/mode", m_tzMode).toInt();
    m_timezone = cfg.value("timezone/zone", m_timezone).toString();

    m_brouterHost = cfg.value("routing/BR/host", m_brouterHost).toString();
    m_brouterPort = cfg.value("routing/BR/port", m_brouterPort).toString();
    m_brouterProfiles = cfg.value("routing/BR/profiles", m_brouterProfiles).toString();
    m_brouterProfilePath = cfg.value("routing/BR/profilePath", m_brouterProfilePath).toString();
    m_brouterLocal = cfg.value("routing/BR/local", m_brouterLocal).toBool();
}


CResources::~CResources()
{
    SETTINGS;

    cfg.setValue("path/maps",pathMaps);

    cfg.setValue("environment/mapfont/family",m_mapfont.family());
    cfg.setValue("environment/mapfont/size",m_mapfont.pointSize());
    cfg.setValue("environment/mapfont/bold",m_mapfont.bold());
    cfg.setValue("environment/mapfont/italic",m_mapfont.italic());

    cfg.setValue("environment/flipMouseWheel",m_flipMouseWheel);
    cfg.setValue("environment/GeoDB",m_useGeoDB);
    cfg.setValue("environment/saveGeoDBOnExit",m_saveGeoDBOnExit);
    cfg.setValue("environment/saveGeoDBMinutes",m_saveGeoDBMinutes);
    cfg.setValue("environment/pathGeoDB",m_pathGeoDB.absolutePath());
    cfg.setValue("network/useProxy",m_useHttpProxy);
    cfg.setValue("network/proxy/url",m_httpProxy);
    cfg.setValue("network/proxy/port",m_httpProxyPort);

    cfg.setValue("device/key",m_devKey);
    cfg.setValue("device/ipAddr",m_devIPAddress);
    cfg.setValue("device/ipPort",m_devIPPort);
    cfg.setValue("device/serialPort",m_devSerialPort);
    cfg.setValue("device/baudRate",m_devBaudRate);
    cfg.setValue("device/watchdog",m_watchdogEnabled);
    cfg.setValue("device/type",m_devType);
    cfg.setValue("device/charset",m_devCharset);

    cfg.setValue("device/dnlTrk",IDevice::m_DownloadAllTrk);
    cfg.setValue("device/dnlWpt",IDevice::m_DownloadAllWpt);
    cfg.setValue("device/dnlRte",IDevice::m_DownloadAllRte);
    cfg.setValue("device/uplWpt",IDevice::m_UploadAllWpt);
    cfg.setValue("device/uplTrk",IDevice::m_UploadAllTrk);
    cfg.setValue("device/uplRte",IDevice::m_UploadAllRte);
    cfg.setValue("device/playSound",m_playSound);

    cfg.setValue("environment/unittype",unit->type);
    cfg.setValue("environment/showTrackProfile",m_showTrackProfile);
    cfg.setValue("environment/showTrackEleInfo",m_showTrackEleInfo);
    cfg.setValue("environment/showNorth",m_showNorth);
    cfg.setValue("environment/showClock",m_showClock);
    cfg.setValue("environment/showScale",m_showScale);
    cfg.setValue("environment/showToolTip",m_showToolTip);
    cfg.setValue("environment/showElementInfo", m_showElementInfo);
    cfg.setValue("environment/showZoomLevel",m_showZoomLevel);
    cfg.setValue("environment/useAntiAliasing",m_useAntiAliasing);
    cfg.setValue("environment/reducePoiIcons",m_reducePoiIcons);

    cfg.setValue("environment/zoomLevelThresholdPois",m_zoomLevelThresholdPois);
    cfg.setValue("environment/zoomLevelThresholdPoiLabels",m_zoomLevelThresholdPoiLabels);

    cfg.setValue("environment/wptTextColor", m_WptTextColor.name());

    cfg.setValue("network/mapcache/path", m_pathMapCache.absolutePath());
    cfg.setValue("network/mapcache/size", m_sizeMapCache);
    cfg.setValue("network/mapcache/expire", m_expireMapCache);

    cfg.setValue("timezone/mode", m_tzMode);
    cfg.setValue("timezone/zone", m_timezone);

    cfg.setValue("routing/BR/host", m_brouterHost);
    cfg.setValue("routing/BR/port", m_brouterPort);
    cfg.setValue("routing/BR/profiles", m_brouterProfiles);
    cfg.setValue("routing/BR/profilePath", m_brouterProfilePath);
    cfg.setValue("routing/BR/local", m_brouterLocal);
}


bool CResources::getHttpProxy(QString& url, quint16& port)
{

    const char *proxy;

    // unless a manual proxy is configured, use the environment settings "HTTP_PROXY" or "http_proxy"
    if (!m_useHttpProxy && ((proxy = getenv("HTTP_PROXY")) || (proxy = getenv("http_proxy"))))
    {
        QString theProxy(proxy);
        QRegExp re("^http://([^:]+):(\\d+)$", Qt::CaseInsensitive);
        if (re.indexIn(theProxy) != -1)
        {
            qDebug() << "http proxy host" << re.cap(1) << "port" << re.cap(2);
            url = re.cap(1);
            port = re.cap(2).toInt();
            return true;
        }
    }

    url  = m_httpProxy;
    port = m_httpProxyPort;

    return m_useHttpProxy;
}


void CResources::getHttpProxyAuth(QString& user, QString& pwd)
{
    CDlgProxy dlg(user, pwd, theMainWindow->getCanvas());
    dlg.exec();
}


IDevice * CResources::device()
{
    // purge device if the key does not match
    if(m_device && (m_device->getDevKey() != m_devKey))
    {
        qDebug() << m_device->getDevKey() << m_devKey;
        delete m_device;
        m_device = 0;
    }

    // allocate new device
    if(!m_device)
    {
        if(m_devKey == "QLandkarteM")
        {
            //m_device = new CDeviceTBDOE(m_devIPAddress,m_devIPPort,this);
            m_device = new CDeviceQLandkarteM(m_devIPAddress,m_devIPPort,this);
        }
        else if(m_devKey == "Garmin" && !m_devType.isEmpty())
        {
            m_device = new CDeviceGarmin(m_devType, m_devSerialPort, this);
        }
        else if(m_devKey == "NMEA")
        {
            m_device = new CDeviceNMEA(m_devSerialPort, m_devBaudRate, this, m_watchdogEnabled);
        }
#ifdef HAS_GPSD
        else if(m_devKey == "GPSD")
        {
            m_device = new CDeviceGPSD(this);
        }
#endif
        else if(m_devKey == "Garmin Mass Storage")
        {
            m_device = new CDeviceGarminBulk(this);
        }
        else if(m_devKey == "TwoNav")
        {
            m_device = new CDeviceTwoNav(this);
        }
        else if(m_devKey == "Magellan")
        {
            m_device = new CDeviceMagellan(this);
        }

        connect(m_device, SIGNAL(sigLiveLog(const CLiveLog&)), &CLiveLogDB::self(), SLOT(slotLiveLog(const CLiveLog&)));
    }

    // still no device?
    if(!m_device)
    {
        qWarning() << "no device";
        // TODO: would be nicer to open the setup dialog
        QMessageBox::critical(0,tr("No device."),tr("You have to select a device in Setup->Config->Device & Xfer"), QMessageBox::Abort, QMessageBox::Abort);
    }

    return m_device;
}


QString CResources::charset()
{
    if (m_devCharset.isNull() || m_devCharset.isEmpty())
        return "latin1";
    else
        return m_devCharset;
}

QStringList CResources::getBRouterProfiles()
{
    if (m_brouterLocal && !m_brouterProfilePath.isNull())
    {
        return readBRouterProfiles(m_brouterProfilePath);
    }
    else
    {
        return m_brouterProfiles.split(QRegExp("[,;| ]"),QString::SkipEmptyParts);
    }
}

QStringList CResources::readBRouterProfiles(QString path)
{
    QDir dir = QDir(path);
    QString profile;
    QStringList entries = dir.entryList();
    QStringList profiles = QStringList();
    foreach(profile,entries)
    {
        if(profile.endsWith(".brf"))
        {
            profiles.append(profile.left(profile.length()-4));
        }
    }
    return profiles;
}
