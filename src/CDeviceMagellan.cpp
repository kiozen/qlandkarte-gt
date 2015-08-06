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
#include "CDeviceMagellan.h"
#include "CSettings.h"
#include "CGpx.h"
#include "CWpt.h"
#include "CWptDB.h"
#include "CTrack.h"
#include "CTrackDB.h"
#include "CMainWindow.h"
#include "CCanvas.h"
#include "CDlgDeviceExportPath.h"

#include <QtGui>
#include <QMessageBox>
#include <QFileDialog>

CDeviceMagellan::CDeviceMagellan(QObject * parent)
: IDevice("Magellan", parent)
{

}


CDeviceMagellan::~CDeviceMagellan()
{

}


QString CDeviceMagellan::createDayPrefix(QDir& root, const QString& what)
{

    QString prefix;
    CDlgDeviceExportPath dlg(what, root, prefix, CDlgDeviceExportPath::eFilePrefix , 0);
    dlg.exec();

    return prefix;
}


bool CDeviceMagellan::aquire(QDir& dir)
{
    SETTINGS;
    QString path = cfg.value("device/path","").toString();
    dir.setPath(path);

    pathRoot = dir.absolutePath();
    pathTrk  = "Tracks";
    pathWpt  = "Waypoints";
    pathRts  = "Routes";
    pathGC   = "Geocaches";

    if(!dir.exists() || !dir.exists(pathTrk) || !dir.exists(pathWpt) || !dir.exists(pathRts))
    {
        while(1)
        {
            path = QFileDialog::getExistingDirectory(0, tr("Path to Magellan device..."), dir.absolutePath());
            if(path.isEmpty())
            {
                return false;
            }
            dir.setPath(path);

            if(!dir.exists() || !dir.exists(pathTrk) || !dir.exists(pathWpt) || !dir.exists(pathRts))
            {
                QMessageBox::information(0,tr("Error..."), tr("I need a path with 'Track', 'Waypoints', 'Routes' and 'Geocaches' as subdirectory"),QMessageBox::Retry,QMessageBox::Retry);
                continue;
            }

            break;
        }
    }

    pathRoot = dir.absolutePath();
    pathTrk  = dir.absoluteFilePath("Tracks");
    pathWpt  = dir.absoluteFilePath("Waypoints");
    pathRts  = dir.absoluteFilePath("Routes");
    pathGC   = dir.absoluteFilePath("Geocaches");

    cfg.setValue("device/path", pathRoot);
    return true;
}


void CDeviceMagellan::uploadWpts(const QList<CWpt*>& wpts)
{
    //    QMessageBox::information(0,tr("Error..."), tr("Magellan: Upload wapoints is not implemented."),QMessageBox::Abort,QMessageBox::Abort);

    QDir dir;
    if(!aquire(dir))
    {
        return;
    }

    dir.cd(pathWpt);

    QString prefix = createDayPrefix(dir, tr("waypoints"));

    QStringList keysWpt, keysGc;
    foreach(CWpt* wpt, wpts)
    {
        if(wpt->isGeoCache())
        {
            keysGc << wpt->getKey();
        }
        else
        {
            keysWpt << wpt->getKey();
        }
    }

    dir.cd(pathWpt);
    if(!keysWpt.isEmpty())
    {
        CGpx gpx(this, CGpx::eMagellan);
        CWptDB::self().saveGPX(gpx, keysWpt);
        try
        {
            gpx.save(dir.absoluteFilePath("%1_Waypoints.gpx").arg(prefix));
        }
        catch(const QString& msg)
        {
            QMessageBox::critical(0,tr("Error"), msg, QMessageBox::Cancel, QMessageBox::Cancel);
        }
    }

    dir.cd(pathGC);
    if(!keysGc.isEmpty())
    {
        CGpx gpx(this, CGpx::eMagellan);
        CWptDB::self().saveGPX(gpx, keysGc);
        try
        {
            gpx.save(dir.absoluteFilePath("%1_Geocaches.gpx").arg(prefix));
        }
        catch(const QString& msg)
        {
            QMessageBox::critical(0,tr("Error"), msg, QMessageBox::Cancel, QMessageBox::Cancel);
        }
    }
    theMainWindow->getCanvas()->setFadingMessage(tr("Upload waypoints finished!"));

}


void CDeviceMagellan::downloadWpts(QList<CWpt*>& wpts)
{
    //QMessageBox::information(0,tr("Error..."), tr("Magellan: Download wapoints is not implemented."),QMessageBox::Abort,QMessageBox::Abort);

    QStringList files;
    QDir dir;
    if(!aquire(dir))
    {
        return;
    }

    dir.cd(pathWpt);

    files = dir.entryList(QStringList("*gpx"));
    foreach(const QString& filename, files)
    {
        CGpx gpx(this, CGpx::eCleanExport);
        try
        {
            gpx.load(dir.absoluteFilePath(filename));
        }
        catch(const QString& msg)
        {
            QMessageBox::critical(0,tr("Error"), msg, QMessageBox::Ok, QMessageBox::Ok);
            continue;
        }
        CWptDB::self().loadGPX(gpx);
    }

    dir.cd(pathGC);

    files = dir.entryList(QStringList("*gpx"));
    foreach(const QString& filename, files)
    {
        CGpx gpx(this, CGpx::eCleanExport);
        try
        {
            gpx.load(dir.absoluteFilePath(filename));
        }
        catch(const QString& msg)
        {
            QMessageBox::critical(0,tr("Error"), msg, QMessageBox::Ok, QMessageBox::Ok);
            continue;
        }
        CWptDB::self().loadGPX(gpx);
    }

    theMainWindow->getCanvas()->setFadingMessage(tr("Download waypoints finished!"));
}


void CDeviceMagellan::uploadTracks(const QList<CTrack*>& trks)
{
    //    QMessageBox::information(0,tr("Error..."), tr("Magellan: Upload tracks is not implemented."),QMessageBox::Abort,QMessageBox::Abort);

    QDir dir;
    if(!aquire(dir))
    {
        return;
    }

    dir.cd(pathTrk);

    QString prefix = createDayPrefix(dir, tr("tracks"));

    foreach(CTrack* trk, trks)
    {
        QStringList keys(trk->getKey());

        CGpx gpx(this, CGpx::eMagellan);
        CTrackDB::self().saveGPX(gpx, keys);
        try
        {
            QCryptographicHash md5(QCryptographicHash::Md5);
            md5.addData(trk->getKey().toLatin1());
            QString hash = md5.result().toHex();

            gpx.save(dir.absoluteFilePath(QString("%1_%2.gpx").arg(prefix).arg(hash)));
        }
        catch(const QString& msg)
        {
            QMessageBox:: critical(0,tr("Error"), msg, QMessageBox::Cancel, QMessageBox::Cancel);
        }

    }

    theMainWindow->getCanvas()->setFadingMessage(tr("Upload tracks finished!"));
}


void CDeviceMagellan::downloadTracks(QList<CTrack*>& trks)
{
    //    QMessageBox::information(0,tr("Error..."), tr("Magellan: Download tracks is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
    QStringList files;
    QDir dir;
    if(!aquire(dir))
    {
        return;
    }

    dir.cd(pathTrk);

    files = dir.entryList(QStringList("*gpx"));
    foreach(const QString& filename, files)
    {
        CGpx gpx(this, CGpx::eCleanExport);
        try
        {
            gpx.load(dir.absoluteFilePath(filename));
        }
        catch(const QString& msg)
        {
            QMessageBox::critical(0,tr("Error"), msg, QMessageBox::Ok, QMessageBox::Ok);
            continue;
        }
        CTrackDB::self().loadGPX(gpx);
    }

    theMainWindow->getCanvas()->setFadingMessage(tr("Download tracks finished!"));
}


void CDeviceMagellan::uploadRoutes(const QList<CRoute*>& rtes)
{
    QMessageBox::information(0,tr("Error..."), tr("Magellan: Upload routes is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
}


void CDeviceMagellan::downloadRoutes(QList<CRoute*>& rtes)
{
    QMessageBox::information(0,tr("Error..."), tr("Magellan: Download routes is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
}


void CDeviceMagellan::uploadMap(const QList<IMapSelection*>& mss)
{
    QMessageBox::information(0,tr("Error..."), tr("Magellan: Upload maps is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
}


void CDeviceMagellan::setLiveLog(bool on)
{
    QMessageBox::information(0,tr("Error..."), tr("Magellan: Live log is not supported."),QMessageBox::Abort,QMessageBox::Abort);
}


void CDeviceMagellan::downloadScreenshot(QImage& image)
{
    QMessageBox::information(0,tr("Error..."), tr("Magellan: Screen shot is not supported."),QMessageBox::Abort,QMessageBox::Abort);
}
