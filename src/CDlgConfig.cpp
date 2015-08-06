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
#include "config.h"
#include "CDlgConfig.h"
#include "CResources.h"
#include "IDevice.h"
#include "CUnitImperial.h"
#include "CUnitNautic.h"
#include "CUnitMetric.h"
#include "CMapTDB.h"
#include "CDlgSetupGarminIcons.h"
#include "GeoMath.h"

#include <QtGui>
#include <QNetworkProxy>
#include <QColorDialog>
#include <QFileDialog>
#include <QFontDialog>

#define XSTR(x) STR(x)
#define STR(x) #x

CDlgConfig::CDlgConfig(QWidget * parent)
: QDialog(parent)
{
#if defined(Q_OS_MAC)
    this->setParent(qApp->focusWidget());
    this->setWindowModality(Qt::WindowModal);
    this->setWindowFlags(Qt::Sheet);
#endif
    setupUi(this);
    connect(toolFont,SIGNAL(clicked()),this,SLOT(slotSelectFont()));
    connect(toolWptTextColor,SIGNAL(clicked()),this,SLOT(slotSelectWptTextColor()));
    fillTypeCombo();

    connect(toolPathGeoDB, SIGNAL(clicked()),this,SLOT(slotSelectPathGeoDB()));
    connect(checkUseGeoDB, SIGNAL(clicked(bool)), groupSaveWks, SLOT(setEnabled(bool)));
    connect(checkGeoDBSaveOnExit, SIGNAL(clicked(bool)), spinGeoDBMinutes, SLOT(setEnabled(bool)));

    connect(toolPathMapCache, SIGNAL(clicked()), this, SLOT(slotSelectPathMapCache()));

    connect(checkBRouterLocal, SIGNAL(stateChanged(int)), this, SLOT(slotCheckBRouterLocal(int)));
    connect(toolButtonBRouterProfilePath, SIGNAL(clicked()), this, SLOT(slotSelectPathBRouterProfiles()));
    connect(pushBRouterDefaultLocal, SIGNAL(clicked()), this, SLOT(slotPushBRouterDefaultLocal()));
    connect(pushBRouterDefaultOnline, SIGNAL(clicked()), this, SLOT(slotPushBRouterDefaultOnline()));

    const char ** tz = tblTimezone;
    while(*tz)
    {
        comboTimeZone->addItem(*tz);
        tz++;
    }

}


CDlgConfig::~CDlgConfig()
{

}


#ifdef QK_QT5_PORT
int CDlgConfig::exec()
#else
void CDlgConfig::exec()
#endif
{
    CResources& resources = CResources::self();

    checkProxy->setChecked(resources.m_useHttpProxy);
    lineProxyURL->setText(resources.m_httpProxy);
    lineProxyPort->setText(QString("%1").arg(resources.m_httpProxyPort));

    /*
    lineProxyUser->setText(resources.m_httpProxyUser);
    lineProxyPwd->setText(resources.m_httpProxyPwd);
    */

    labelFont->setFont(resources.m_mapfont);
    if(resources.unit->type == "metric")
    {
        radioMetric->setChecked(true);
    }
    else if(resources.unit->type == "nautic")
    {
        radioNautic->setChecked(true);
    }
    else if(resources.unit->type == "imperial")
    {
        radioImperial->setChecked(true);
    }

    checkPlaySound->setChecked(resources.m_playSound);
    checkFlipMouseWheel->setChecked(resources.m_flipMouseWheel);
    checkShowProfilePreview->setChecked(resources.m_showTrackProfile);
    checkShowTrackEleInfo->setChecked(resources.m_showTrackEleInfo);
    checkShowNorth->setChecked(resources.m_showNorth);
    checkShowClock->setChecked(resources.m_showClock);
    checkShowScale->setChecked(resources.m_showScale);
    checkTooltip->setChecked(resources.m_showToolTip);
    checkElementInfo->setChecked(resources.m_showElementInfo);
    checkShowZoomLevel->setChecked(resources.m_showZoomLevel);
    checkAntiAliasing->setChecked(resources.m_useAntiAliasing);
    checkReducePoiIcons->setChecked(resources.m_reducePoiIcons);

    spinZoomLevelThresholdPois->setValue(resources.m_zoomLevelThresholdPois);
    spinZoomLevelThresholdPoiLabels->setValue(resources.m_zoomLevelThresholdPoiLabels);

    checkUseGeoDB->setChecked(resources.m_useGeoDB);
    groupSaveWks->setEnabled(resources.m_useGeoDB);
    checkGeoDBSaveOnExit->setChecked(resources.m_saveGeoDBOnExit);
    spinGeoDBMinutes->setValue(resources.m_saveGeoDBMinutes);
    labelPathGeoDB->setText(resources.m_pathGeoDB.absolutePath());

    comboDevice->addItem(tr(""),"");
    comboDevice->addItem(tr("QLandkarte M"), "QLandkarteM");
    comboDevice->addItem(tr("Garmin"), "Garmin");
    comboDevice->addItem(tr("Garmin Mass Storage"), "Garmin Mass Storage");
    comboDevice->addItem(tr("Magellan"), "Magellan");
    comboDevice->addItem(tr("TwoNav"), "TwoNav");
    comboDevice->addItem(tr("NMEA"), "NMEA");

    checkWatchDog->setChecked(resources.m_watchdogEnabled);

    comboDevBaudRate->addItem("4800");
    comboDevBaudRate->addItem("9600");
    comboDevBaudRate->addItem("19200");
    comboDevBaudRate->addItem("38400");
    comboDevBaudRate->addItem("57600");
    comboDevBaudRate->addItem("115200");
    comboDevBaudRate->setCurrentIndex(comboDevBaudRate->findText(resources.m_devBaudRate));
#ifdef HAS_GPSD
    comboDevice->addItem(tr("GPSD"), "GPSD");
#endif

    connect(comboDevice, SIGNAL(currentIndexChanged(int)), this, SLOT(slotCurrentDeviceChanged(int)));
    comboDevice->setCurrentIndex(comboDevice->findData(resources.m_devKey));

    connect(buttonGarminIcons, SIGNAL(clicked()), this, SLOT(slotSetupGarminIcons()));

    lineDevIPAddr->setText(resources.m_devIPAddress);
    lineDevIPPort->setText(QString::number(resources.m_devIPPort));
    lineDevSerialPort->setText(resources.m_devSerialPort);
#ifdef Q_OS_WIN32
    lineDevSerialPort->setToolTip(tr("Pass something like \"COM1\" or \"\\\\.\\COM13\" or \"\\\\.\\com13\" for serial Garmin devices or NMEA devices. For Garmin USB devices leave blank."));
#endif

    checkDownloadTrk->setChecked(IDevice::m_DownloadAllTrk);
    checkDownloadWpt->setChecked(IDevice::m_DownloadAllWpt);
    checkDownloadRte->setChecked(IDevice::m_DownloadAllRte);
    checkUploadTrk->setChecked(IDevice::m_UploadAllTrk);
    checkUploadWpt->setChecked(IDevice::m_UploadAllWpt);
    checkUploadRte->setChecked(IDevice::m_UploadAllRte);

    QPalette palette = labelWptTextColor->palette();
    palette.setColor(labelWptTextColor->foregroundRole(), resources.m_WptTextColor);
    labelWptTextColor->setPalette(palette);

    labelPathMapCache->setText(resources.m_pathMapCache.absolutePath());
    spinSizeMapCache->setValue(resources.m_sizeMapCache);
    spinExpireMapCache->setValue(resources.m_expireMapCache);

    comboTimeZone->setCurrentIndex(comboTimeZone->findText(resources.m_timezone));
    if(resources.m_tzMode == resources.eTZUtc)
    {
        radioTimeUtc->setChecked(true);
    }
    else if(resources.m_tzMode == resources.eTZLocal)
    {
        radioTimeLocal->setChecked(true);
    }
    else if(resources.m_tzMode == resources.eTZAuto)
    {
        radioTimeAuto->setChecked(true);
    }
    else if(resources.m_tzMode == resources.eTZSelected)
    {
        radioTimeZone->setChecked(true);
    }

    lineBRouterHost->setText(resources.m_brouterHost);
    lineBRouterPort->setText(resources.m_brouterPort);
    lineBRouterProfiles->setText(resources.m_brouterProfiles);
    labelBRouterProfiles->setText(resources.m_brouterProfiles);
    labelBRouterProfilePath->setText(resources.m_brouterProfilePath);
    checkBRouterLocal->setChecked(resources.m_brouterLocal);
    slotCheckBRouterLocal(resources.m_brouterLocal ? Qt::Checked : Qt::Unchecked);
#ifdef QK_QT5_PORT
    return QDialog::exec();
#else
    QDialog::exec();
#endif
}


void CDlgConfig::accept()
{
    CResources& resources = CResources::self();

    resources.m_useHttpProxy    = checkProxy->isChecked();
    resources.m_httpProxy       = lineProxyURL->text();
    resources.m_httpProxyPort   = lineProxyPort->text().toUInt();

    /*
    resources.m_httpProxyUser   = lineProxyUser->text();
    resources.m_httpProxyPwd    = lineProxyPwd->text();
    */

    if(resources.m_useHttpProxy)
        QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::HttpProxy,resources.m_httpProxy,resources.m_httpProxyPort));
    else
        QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::NoProxy));

    resources.m_mapfont         = labelFont->font();

    if(radioMetric->isChecked())
    {
        resources.unit = new CUnitMetric(&resources);
    }
    if(radioImperial->isChecked())
    {
        resources.unit = new CUnitImperial(&resources);
    }
    if(radioNautic->isChecked())
    {
        resources.unit = new CUnitNautic(&resources);
    }

    resources.m_flipMouseWheel  = checkFlipMouseWheel->isChecked();
    resources.m_showTrackProfile = checkShowProfilePreview->isChecked();
    resources.m_showTrackEleInfo = checkShowTrackEleInfo->isChecked();
    resources.m_showNorth       = checkShowNorth->isChecked();
    resources.m_showClock       = checkShowClock->isChecked();
    resources.m_showScale       = checkShowScale->isChecked();
    resources.m_showToolTip     = checkTooltip->isChecked();
    resources.m_showElementInfo = checkElementInfo->isChecked();
    resources.m_showZoomLevel   = checkShowZoomLevel->isChecked();
    resources.m_playSound       = checkPlaySound->isChecked();
    resources.m_useAntiAliasing = checkAntiAliasing->isChecked();
    resources.m_reducePoiIcons  = checkReducePoiIcons->isChecked();

    resources.m_zoomLevelThresholdPois      = spinZoomLevelThresholdPois->value();
    resources.m_zoomLevelThresholdPoiLabels = spinZoomLevelThresholdPoiLabels->value();

    resources.m_useGeoDB        = checkUseGeoDB->isChecked();
    resources.m_saveGeoDBOnExit = checkGeoDBSaveOnExit->isChecked();
    resources.m_saveGeoDBMinutes = spinGeoDBMinutes->value();
    resources.m_pathGeoDB       = QDir(labelPathGeoDB->text());

    resources.m_devKey          = comboDevice->itemData(comboDevice->currentIndex()).toString();
    resources.m_devIPAddress    = lineDevIPAddr->text();
    resources.m_devIPPort       = lineDevIPPort->text().toUShort();
    resources.m_devSerialPort   = lineDevSerialPort->text();
    resources.m_devBaudRate     = comboDevBaudRate->currentText();
    resources.m_watchdogEnabled = checkWatchDog->isChecked();
    resources.m_devType         = comboDevType->itemText(comboDevType->currentIndex());
    resources.m_devCharset      = comboDevCharset->itemText(comboDevCharset->currentIndex());

    if(resources.m_device)
    {
        delete resources.m_device;
        resources.m_device = 0;
    }

    emit resources.sigDeviceChanged();

    IDevice::m_DownloadAllTrk   = checkDownloadTrk->isChecked();
    IDevice::m_DownloadAllWpt   = checkDownloadWpt->isChecked();
    IDevice::m_DownloadAllRte   = checkDownloadRte->isChecked();
    IDevice::m_UploadAllWpt     = checkUploadWpt->isChecked();
    IDevice::m_UploadAllTrk     = checkUploadTrk->isChecked();
    IDevice::m_UploadAllRte     = checkUploadRte->isChecked();

    QPalette palette            = labelWptTextColor->palette();
    resources.m_WptTextColor    = palette.color(labelWptTextColor->foregroundRole());

    resources.m_pathMapCache    = QDir(labelPathMapCache->text());
    resources.m_sizeMapCache    = spinSizeMapCache->value();
    resources.m_expireMapCache  = spinExpireMapCache->value();

    resources.m_timezone        = comboTimeZone->currentText();
    if(radioTimeUtc->isChecked())
    {
        resources.m_tzMode = resources.eTZUtc;
    }
    else if(radioTimeLocal->isChecked())
    {
        resources.m_tzMode = resources.eTZLocal;
    }
    else if(radioTimeAuto->isChecked())
    {
        resources.m_tzMode = resources.eTZAuto;
    }
    else if(radioTimeZone->isChecked())
    {
        resources.m_tzMode = resources.eTZSelected;
    }

    resources.m_brouterHost = lineBRouterHost->text();
    resources.m_brouterPort = lineBRouterPort->text();
    bool bRouterChanged = resources.m_brouterProfiles.compare(lineBRouterProfiles->text())
            || resources.m_brouterProfilePath.compare(labelBRouterProfilePath->text())
            || resources.m_brouterLocal!=checkBRouterLocal->isChecked();
    resources.m_brouterLocal = checkBRouterLocal->isChecked();
    resources.m_brouterProfiles = lineBRouterProfiles->text().split(QRegExp("[,;| ]"),QString::SkipEmptyParts).join(",");
    resources.m_brouterProfilePath = labelBRouterProfilePath->text();
    if (bRouterChanged)
    {
        emit resources.sigBRouterChanged();
    }

    QDialog::accept();
}


void CDlgConfig::slotCurrentDeviceChanged(int index)
{
    lineDevIPAddr->setEnabled(false);
    labelDevIPAddr->setEnabled(false);
    lineDevIPPort->setEnabled(false);
    labelDevIPPort->setEnabled(false);
    lineDevSerialPort->setEnabled(false);
    labelDevSerialPort->setEnabled(false);
    comboDevBaudRate->setEnabled(false);
    labelDevBaudRate->setEnabled(false);
    comboDevType->setEnabled(false);
    labelDevType->setEnabled(false);
    comboDevCharset->setEnabled(false);
    labelDevCharset->setEnabled(false);
    buttonGarminIcons->setEnabled(false);

    if(comboDevice->itemData(index) == "QLandkarteM")
    {
        lineDevIPAddr->setEnabled(true);
        labelDevIPAddr->setEnabled(true);
        lineDevIPPort->setEnabled(true);
        labelDevIPPort->setEnabled(true);
    }
    else if(comboDevice->itemData(index) == "Garmin")
    {
        comboDevType->setEnabled(true);
        lineDevSerialPort->setEnabled(true);
        labelDevSerialPort->setEnabled(true);
        labelDevType->setEnabled(true);
        comboDevCharset->setEnabled(true);
        labelDevCharset->setEnabled(true);
        buttonGarminIcons->setEnabled(true);
        fillTypeCombo();
        fillCharsetCombo();
    }
    else if(comboDevice->itemData(index) == "Mikrokopter")
    {
        lineDevSerialPort->setEnabled(true);
        labelDevSerialPort->setEnabled(true);
    }
    else if(comboDevice->itemData(index) == "NMEA")
    {
        lineDevSerialPort->setEnabled(true);
        labelDevSerialPort->setEnabled(true);
        comboDevBaudRate->setEnabled(true);
        labelDevBaudRate->setEnabled(true);
    }
}


void CDlgConfig::slotSelectFont()
{
    bool ok = false;
    QFont font = QFontDialog::getFont(&ok, labelFont->font(), this);
    if(ok)
    {
        labelFont->setFont(font);
    }

}


void CDlgConfig::fillTypeCombo()
{
    comboDevType->clear();

    CResources& resources = CResources::self();
    QRegExp regex("lib(.*)\\" XSTR(SHARED_LIB_EXT));
    QString file;
    QStringList files;
#if defined(Q_OS_MAC)
    // MacOS X: plug-ins are stored in the bundle folder
    QDir inst_dir(QCoreApplication::applicationDirPath()
        .replace(QRegExp("MacOS$"), "Resources/Drivers"));
#else
    QDir inst_dir(XSTR(PLUGINDIR));
#endif
    files = inst_dir.entryList(QString("*" XSTR(SHARED_LIB_EXT)).split(','));

    foreach(file,files)
    {
        if(regex.exactMatch(file))
        {
            comboDevType->addItem(regex.cap(1));
        }
    }
    comboDevType->setCurrentIndex(comboDevType->findText(resources.m_devType));

    if(files.isEmpty())
    {
#if defined(Q_OS_MAC)
        labelMessage->setText(tr("No plugins found. I expect them in: %1")
            .arg(QCoreApplication::applicationDirPath()
            .replace(QRegExp("MacOS$"), "Resources/Drivers")));
#else
        labelMessage->setText(tr("No plugins found. I expect them in: %1").arg(XSTR(PLUGINDIR)));
#endif
    }
    else
    {
        labelMessage->setText("");
    }
}


void CDlgConfig::fillCharsetCombo()
{
    comboDevCharset->clear();

    CResources& resources = CResources::self();
    QList<QByteArray> allCodecs = QTextCodec::availableCodecs();
    QByteArray codec;

    qSort(allCodecs);
    foreach(codec, allCodecs)
    {
        comboDevCharset->addItem(codec);
    }
    comboDevCharset->setCurrentIndex(comboDevCharset->findText(resources.m_devCharset));
}


void CDlgConfig::slotSetupGarminIcons()
{
    CDlgSetupGarminIcons dlg;
    dlg.exec();
}


void CDlgConfig::slotSelectPathGeoDB()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Open Directory"), labelPathGeoDB->text(), QFileDialog::ShowDirsOnly);
    if(!path.isEmpty())
    {
        labelPathGeoDB->setText(path);
    }
}


void CDlgConfig::slotSelectWptTextColor()
{
    QPalette palette = labelWptTextColor->palette();
    QColor color = palette.color(labelWptTextColor->foregroundRole());

    color = QColorDialog::getColor(color);

    if(color.isValid())
    {
        palette.setColor(labelWptTextColor->foregroundRole(), color);
        labelWptTextColor->setPalette(palette);
    }

}


void CDlgConfig::slotSelectPathMapCache()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Open Directory"), labelPathMapCache->text(), QFileDialog::ShowDirsOnly);
    if(!path.isEmpty())
    {
        labelPathMapCache->setText(path);
    }
}

void CDlgConfig::slotCheckBRouterLocal(int state)
{
    switch(state) {
    case Qt::Checked:
    {
        lineBRouterProfiles->setVisible(false);
        labelBRouterProfiles->setVisible(true);
        labelBRouterProfileDir->setVisible(true);
        toolButtonBRouterProfilePath->setVisible(true);
        labelBRouterProfilePath->setVisible(true);
        if (!labelBRouterProfilePath->text().isEmpty())
        {
            labelBRouterProfiles->setText(CResources::self().readBRouterProfiles(labelBRouterProfilePath->text()).join(","));
        }
        else
        {
            labelBRouterProfiles->setText(lineBRouterProfiles->text());
        }
        break;
    }
    case Qt::Unchecked:
    {
        lineBRouterProfiles->setVisible(true);
        labelBRouterProfiles->setVisible(false);
        labelBRouterProfileDir->setVisible(false);
        toolButtonBRouterProfilePath->setVisible(false);
        labelBRouterProfilePath->setVisible(false);
        if (!labelBRouterProfiles->text().isEmpty())
        {
            lineBRouterProfiles->setText(labelBRouterProfiles->text());
        }
        break;
    }
    }
}

void CDlgConfig::slotSelectPathBRouterProfiles()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Open Directory"), labelPathMapCache->text(), QFileDialog::ShowDirsOnly);
    if(!path.isEmpty())
    {
        labelBRouterProfilePath->setText(path);
        labelBRouterProfiles->setText(CResources::self().readBRouterProfiles(path).join(","));
    }
}

void CDlgConfig::slotPushBRouterDefaultOnline()
{
    lineBRouterHost->setText(QString("h2096617.stratoserver.net"));
    lineBRouterPort->setText(QString("443"));
    lineBRouterProfiles->setText(QString("trekking,fastbike,car-test,safety,shortest,trekking-ignore-cr,trekking-steep,trekking-noferries,trekking-nosteps,moped,rail,river,vm-forum-liegerad-schnell,vm-forum-velomobil-schnell"));
}

void CDlgConfig::slotPushBRouterDefaultLocal()
{
    lineBRouterHost->setText(QString("127.0.0.1"));
    lineBRouterPort->setText(QString("17777"));
    lineBRouterProfiles->setText(QString("car-test,fastbike,moped,shortest,trekking"));
}
