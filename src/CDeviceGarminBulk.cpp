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

#include "CDeviceGarminBulk.h"
#include "CGpx.h"
#include "CWptDB.h"
#include "CTrackDB.h"
#include "CTrack.h"
#include "CRouteDB.h"
#include "CRoute.h"
#include "CMainWindow.h"
#include "CCanvas.h"
#include "CSettings.h"
#include "CDlgDeviceExportPath.h"

#include <QtGui>
#include <QtXml>
#include <QMessageBox>
#include <QFileDialog>


CDeviceGarminBulk::CDeviceGarminBulk(QObject * parent)
: IDevice("Garmin Mass Storage", parent)
{

}


CDeviceGarminBulk::~CDeviceGarminBulk()
{

}

void CDeviceGarminBulk::slotDevice(const QString& dev)
{
        qDebug() << "!!!!!!!!!!!!!!!!" << dev << "!!!!!!!!!!!!!!!!!!!!!!!!!";
}

void CDeviceGarminBulk::readDeviceXml(const QString& filename)
{
    QFile file(filename);
    file.open(QIODevice::ReadOnly);

    QDomDocument dom;
    dom.setContent(&file);

    QDomElement device              = dom.firstChildElement("Device");
    QDomElement MassStorageMode     = device.firstChildElement("MassStorageMode");
    const QDomNodeList& DataTypes   = MassStorageMode.elementsByTagName("DataType");

    for(int i = 0; i < DataTypes.size(); i++)
    {
        QDomNode dataType       = DataTypes.at(i);
        QDomElement Name        = dataType.firstChildElement("Name");
        QDomElement File        = dataType.firstChildElement("File");
        QDomElement Location    = File.firstChildElement("Location");
        QDomElement Path        = Location.firstChildElement("Path");

        qDebug() << Name.text().simplified() << Path.text().simplified();

        QString name = Name.text().simplified();

        if(name == "GPSData")
        {
            pathGpx = Path.text().simplified();
        }
        else if(name == "UserDataSync")
        {
            pathGpx = Path.text().simplified();
        }
        else if(name == "GeotaggedPhotos")
        {
            pathPictures = Path.text().simplified();
        }
        else if(name == "GeocachePhotos")
        {
            pathSpoilers = Path.text().simplified();
        }

    }

}


bool CDeviceGarminBulk::aquire(QDir& dir)
{
    QMessageBox::StandardButton res;

    SETTINGS;
    QString path = cfg.value("device/path","").toString();
    dir.setPath(path);

    pathRoot     = "";
    pathPictures = "";
    pathGpx      = "Garmin/GPX";
    pathSpoilers = "";

    if(path.endsWith("Garmin"))
    {
        dir.cdUp();
    }

    if(dir.exists("Garmin/GarminDevice.xml"))
    {
        readDeviceXml(dir.absoluteFilePath("Garmin/GarminDevice.xml"));
        if(!dir.exists(pathGpx) && dir.exists("Garmin/GPX"))
        {
            pathGpx = "Garmin/GPX";
        }
    }

    if(!dir.exists() || !dir.exists(pathGpx))
    {
        while(1)
        {
            pathRoot     = "";
            pathPictures = "";
            pathGpx      = "Garmin/GPX";
            pathSpoilers = "";

            path = QFileDialog::getExistingDirectory(0, tr("Path to Garmin device..."), dir.absolutePath());
            if(path.isEmpty())
            {
                return false;
            }
            dir.setPath(path);

            if(path.endsWith("Garmin"))
            {
                dir.cdUp();
            }

            if(dir.exists("Garmin/GarminDevice.xml"))
            {
                readDeviceXml(dir.absoluteFilePath("Garmin/GarminDevice.xml"));
                if(!dir.exists(pathGpx) && dir.exists("Garmin/GPX"))
                {
                    pathGpx = "Garmin/GPX";
                }
            }

            if(!dir.exists(pathGpx))
            {
                res = QMessageBox::critical(0, tr("Missing..."), tr("The selected path must have a subdirectory '%1'. Should I create the path?\n\n%2").arg(pathGpx).arg(dir.absoluteFilePath(pathGpx)), QMessageBox::Abort|QMessageBox::Yes, QMessageBox::Abort);
                if(res == QMessageBox::Yes)
                {
                    dir.mkpath(pathGpx);
                }
                else
                {
                    continue;
                }
            }

            if(!pathPictures.isEmpty() && !dir.exists(pathPictures))
            {
                res = QMessageBox::critical(0, tr("Missing..."), tr("The selected path must have a subdirectory '%1'. Should I create the path?\n\n%2").arg(pathPictures).arg(dir.absoluteFilePath(pathPictures)), QMessageBox::Abort|QMessageBox::Yes, QMessageBox::Abort);
                if(res == QMessageBox::Yes)
                {
                    dir.mkpath(pathPictures);
                }
                else
                {
                    continue;
                }
            }

            if(!pathSpoilers.isEmpty() && !dir.exists(pathSpoilers))
            {

                res = QMessageBox::critical(0, tr("Missing..."), tr("The selected path must have a subdirectory '%1. Should I create the path?\n\n%2").arg(pathSpoilers).arg(dir.absoluteFilePath(pathSpoilers)), QMessageBox::Abort|QMessageBox::Yes, QMessageBox::Abort);
                if(res == QMessageBox::Yes)
                {
                    dir.mkpath(pathSpoilers);
                }
                else
                {
                    continue;
                }
            }

            break;
        }
    }
    pathRoot = dir.absolutePath();
    cfg.setValue("device/path", pathRoot);
    return true;
}


void CDeviceGarminBulk::createDayPath(const QDir& root, const QString& what)
{

    QDir dir = root;
    QString subdir;
    dir.cd(pathGpx);

    CDlgDeviceExportPath dlg(what, dir, subdir, CDlgDeviceExportPath::eDirectory, 0);
    dlg.exec();

    pathDay = dir.absoluteFilePath(subdir);
    if(!dir.exists(pathDay))
    {
        dir.mkpath(pathDay);
    }
}


void CDeviceGarminBulk::uploadWpts(const QList<CWpt*>& wpts)
{
    //QMessageBox::information(0,tr("Error..."), tr("Garmin Mass Storage: Upload waypoints is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
    QDir dir;
    if(!aquire(dir))
    {
        return;
    }

    createDayPath(dir, tr("waypoints"));

    QStringList keys;
    foreach(CWpt* wpt, wpts)
    {
        if(wpt == 0)
        {
            continue;
        }

        keys << wpt->getKey();
        if(!wpt->images.isEmpty())
        {
            if(wpt->isGeoCache() && !pathSpoilers.isEmpty())
            {
                int cnt = 1;
                QString name = wpt->getName();
                quint32 size = name.size();
                QString path = QString("%1/%2/%3").arg(name.at(size-1)).arg(name.at(size -2)).arg(name);

                dir.cd(pathSpoilers);
                dir.mkpath(path + "/Spoilers");
                dir.cd(path);

                foreach(const CWpt::image_t& img, wpt->images)
                {
                    QString fn = img.info;
                    if(fn.isEmpty())
                    {
                        fn = QString("pix%1.jpg").arg(cnt++);
                    }
                    if(!fn.endsWith("jpg"))
                    {
                        fn += ".jpg";
                    }
                    if(fn.contains("Spoiler"))
                    {
                        fn = "Spoilers/" + fn;
                    }

                    img.pixmap.save(dir.absoluteFilePath(fn));
                }
            }
            else
            {
                if(!pathPictures.isEmpty())
                {
                    dir.cd(pathPictures);

                    CWpt::image_t img   = wpt->images.first();
                    QString fn          = img.filename;
                    if(fn.isEmpty())
                    {
                        fn = wpt->getName() + ".jpg";
                    }

                    img.pixmap.save(dir.absoluteFilePath(fn));
                    wpt->link = pathPictures + "/" + fn;
                }
            }
            dir.cd(pathRoot);
        }
    }

    dir.cd(pathDay);
    CGpx gpx(this, CGpx::eCleanExport);
    CWptDB::self().saveGPX(gpx, keys);
    try
    {
        gpx.save(dir.absoluteFilePath("MyWaypoints.gpx"));
    }
    catch(const QString& msg)
    {
        QMessageBox::critical(0,tr("Error"), msg, QMessageBox::Cancel, QMessageBox::Cancel);
    }

    dir.cd(pathRoot);
    theMainWindow->getCanvas()->setFadingMessage(tr("Upload waypoints finished!"));

}


void CDeviceGarminBulk::downloadWpts(QList<CWpt*>& /*wpts*/)
{
    QStringList files;
    QStringList subdirs;
    QDir dir;
    if(!aquire(dir))
    {
        return;
    }

    dir.cd(pathGpx);

    subdirs = dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
    subdirs << pathGpx;
    foreach(const QString& subdir, subdirs)
    {
        dir.cd(subdir);

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

        dir.cdUp();
    }

    const QMap<QString,CWpt*>& wpts = CWptDB::self().getWpts();
    foreach(CWpt * wpt, wpts)
    {
        if(wpt->isGeoCache())
        {
            QString name = wpt->getName();
            quint32 size = name.size();
            QString path = QString("%1/%2/%3").arg(name.at(size-1)).arg(name.at(size -2)).arg(name);

            dir.cd(pathRoot);
            dir.cd(pathSpoilers);
            if(dir.exists(path))
            {
                dir.cd(path);

                QStringList files = dir.entryList(QStringList("*.jpg"), QDir::Files);
                foreach(const QString& file, files)
                {
                    CWpt::image_t img;
                    img.pixmap.load(dir.absoluteFilePath(file));
                    if(!img.pixmap.isNull())
                    {
                        img.filename    = file;
                        img.info        = file;
                        wpt->images << img;
                    }
                }

                dir.cd("Spoilers");
                files = dir.entryList(QStringList("*.jpg"), QDir::Files);
                foreach(const QString& file, files)
                {
                    CWpt::image_t img;
                    img.pixmap.load(dir.absoluteFilePath(file));
                    if(!img.pixmap.isNull())
                    {
                        img.filename    = file;
                        img.info        = file;
                        wpt->images << img;
                    }
                }
            }

        }
        else
        {
            dir.cd(pathRoot);
            if(wpt->link.startsWith(pathPictures))
            {
                CWpt::image_t img;
                img.pixmap.load(dir.absoluteFilePath(wpt->link));
                if(!img.pixmap.isNull())
                {
                    img.filename    = QFileInfo(wpt->link).fileName();
                    img.info        = wpt->getComment();
                    wpt->images << img;
                }
            }
        }
    }

    dir.cd(pathRoot);
    theMainWindow->getCanvas()->setFadingMessage(tr("Download waypoints finished!"));
}


void CDeviceGarminBulk::uploadTracks(const QList<CTrack*>& trks)
{
    //QMessageBox::information(0,tr("Error..."), tr("Garmin Mass Storage: Upload tracks is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
    QDir dir;
    if(!aquire(dir))
    {
        return;
    }

    createDayPath(dir, tr("tracks"));

    QStringList keys;
    foreach(CTrack* trk, trks)
    {
        keys << trk->getKey();
    }

    dir.cd(pathDay);

    CGpx gpx(this, CGpx::eCleanExport);
    CTrackDB::self().saveGPX(gpx, keys);
    try
    {
        gpx.save(dir.absoluteFilePath("MyTracks.gpx"));
    }
    catch(const QString& msg)
    {
        QMessageBox:: critical(0,tr("Error"), msg, QMessageBox::Cancel, QMessageBox::Cancel);
    }
    dir.cd(pathRoot);
    theMainWindow->getCanvas()->setFadingMessage(tr("Upload tracks finished!"));
}


void CDeviceGarminBulk::downloadTracks(QList<CTrack*>& /*trks*/)
{
    QStringList files;
    QStringList subdirs;
    QDir dir;
    if(!aquire(dir))
    {
        return;
    }

    dir.cd(pathGpx);

    subdirs = dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
    subdirs << pathGpx;
    foreach(const QString& subdir, subdirs)
    {
        dir.cd(subdir);

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

        dir.cdUp();
    }

    dir.cd(pathRoot);
    theMainWindow->getCanvas()->setFadingMessage(tr("Download tracks finished!"));
}


void CDeviceGarminBulk::uploadRoutes(const QList<CRoute*>& rtes)
{
    QDir dir;
    if(!aquire(dir))
    {
        return;
    }

    createDayPath(dir, tr("routes"));

    QStringList keys;
    foreach(CRoute* rte, rtes)
    {
        keys << rte->getKey();
    }

    dir.cd(pathDay);

    CGpx gpx(this, CGpx::eCleanExport);
    CRouteDB::self().saveGPX(gpx, keys);
    try
    {
        gpx.save(dir.absoluteFilePath("MyRoutes.gpx"));
    }
    catch(const QString& msg)
    {
        QMessageBox:: critical(0,tr("Error"), msg, QMessageBox::Cancel, QMessageBox::Cancel);
    }
    dir.cd(pathRoot);
    theMainWindow->getCanvas()->setFadingMessage(tr("Upload routes finished!"));
}


void CDeviceGarminBulk::downloadRoutes(QList<CRoute*>& /*rtes*/)
{
    QStringList files;
    QStringList subdirs;

    QDir dir;
    if(!aquire(dir))
    {
        return;
    }

    dir.cd(pathGpx);

    subdirs = dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
    subdirs << pathGpx;
    foreach(const QString& subdir, subdirs)
    {
        dir.cd(subdir);

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

            CRouteDB::self().loadGPX(gpx);
        }

        dir.cdUp();
    }

    dir.cd(pathRoot);
    theMainWindow->getCanvas()->setFadingMessage(tr("Download routes finished!"));
}


void CDeviceGarminBulk::uploadMap(const QList<IMapSelection*>& /*mss*/)
{
    QMessageBox::information(0,tr("Error..."), tr("Garmin Mass Storage: Upload maps is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
}


void CDeviceGarminBulk::downloadScreenshot(QImage& /*image*/)
{
    QMessageBox::information(0,tr("Error..."), tr("Garmin Mass Storage: Download screenshots is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
}


void CDeviceGarminBulk::setLiveLog(bool on)
{
    QMessageBox::information(0,tr("Error..."), tr("Garmin Mass Storage: Live log is not implemented."),QMessageBox::Abort,QMessageBox::Abort);
}
