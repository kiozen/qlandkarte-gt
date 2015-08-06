/**********************************************************************************************
    Copyright (C) 2011 Oliver Eichler oliver.eichler@gmx.de

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

#include "CMapQMAPExport.h"
#include "CMapDB.h"
#include "CMapSelectionRaster.h"
#include "CMapFile.h"
#include "config.h"
#include "CResources.h"
#include "CDlgProjWizzard.h"
#include "CSettings.h"

#include <QtGui>
#include <QFileDialog>
#include <QScrollBar>
#include <QMessageBox>

static bool tileIndexLessThan(const QPair<int, int> &i1, const QPair<int, int> &i2)
{
    if(i1.second < i2.second)
    {
        return true;
    }
    else if (i1.second == i2.second)
    {
        return i1.first < i2.first;
    }
    else
    {
        return false;
    }
}


// --------------------------------------------------------------------------------------------
CMapQMAPExport::CMapQMAPExport(const CMapSelectionRaster& mapsel, QWidget * parent)
: QDialog(parent)
, mapsel(mapsel)
, tainted(false)
, has_map2jnx(false)
, totalNumberOfStates(0)
{
    setupUi(this);

    connect(toolPath, SIGNAL(clicked()), this, SLOT(slotOutputPath()));
    connect(pushExport, SIGNAL(clicked()), this, SLOT(slotStart()));
    connect(pushCancel, SIGNAL(clicked()), this, SLOT(slotCancel()));
    connect(pushDetails, SIGNAL(clicked()), this, SLOT(slotDetails()));
    connect(radioQLM, SIGNAL(toggled(bool)), this, SLOT(slotQLMToggled(bool)));
    connect(radioJNX, SIGNAL(toggled(bool)), this, SLOT(slotBirdsEyeToggled(bool)));
    connect(radioGCM, SIGNAL(toggled(bool)), this, SLOT(slotGCMToggled(bool)));
    connect(radioRMAP, SIGNAL(toggled(bool)), this, SLOT(slotRMAPToggled(bool)));
    connect(radioRMP, SIGNAL(toggled(bool)), this, SLOT(slotRMPToggled(bool)));

    connect(&cmd, SIGNAL(readyReadStandardError()), this, SLOT(slotStderr()));
    connect(&cmd, SIGNAL(readyReadStandardOutput()), this, SLOT(slotStdout()));
    connect(&cmd, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(slotFinished(int,QProcess::ExitStatus)));

    connect(toolGeoTiffProjWizard, SIGNAL(clicked()), this, SLOT(slotSetupProj()));
    connect(toolGeoTiffFromMap, SIGNAL(clicked()), this, SLOT(slotSetupProjFromMap()));

    connect(toolMagellanCopyright, SIGNAL(clicked()), this, SLOT(slotSelectCopyright()));

    SETTINGS;
    labelPath->setText(cfg.value("path/export","./").toString());

    CMapDB::map_t mapData = CMapDB::self().getMapData(mapsel.mapkey);
    linePrefix->setText(QString("%1_%2_%3").arg(mapData.description).arg(mapsel.lon1 * RAD_TO_DEG).arg(mapsel.lat1 * RAD_TO_DEG));
    linePrefix->setCursorPosition(0);
    lineDescription->setText(mapData.description);
    lineDescription->setCursorPosition(0);

    radioRMAP->show();
    radioRMAP->setEnabled(true);

    comboRmapProjection->addItem("Mercator(WGS 84)", "+proj=merc +lon_0=0 +k=1 +x_0=0 +y_0=0 +datum=WGS84 +units=m +no_defs");
    comboRmapProjection->addItem("EPSG:4326, LongLat(WGS 84)", "+init=epsg:4326");
    comboRmapProjection->addItem("EPSG:31467, GK3 (DHDN)", "+init=epsg:31467");
    comboRmapProjection->addItem("EPSG:31468, GK4 (DHDN)", "+init=epsg:31468");

#ifdef WIN32
    path_map2jnx        = QCoreApplication::applicationDirPath()+QDir::separator()+"map2jnx.exe";
    QFile file_map2jnx(path_map2jnx);
    has_map2jnx         = file_map2jnx.exists();
    path_map2gcm        = QCoreApplication::applicationDirPath()+QDir::separator()+"map2gcm.exe";
    path_cache2gtiff    = QCoreApplication::applicationDirPath()+QDir::separator()+"cache2gtiff.exe";
    path_map2rmap       = QCoreApplication::applicationDirPath()+QDir::separator()+"map2rmap.exe";
    path_map2rmp        = QCoreApplication::applicationDirPath()+QDir::separator()+"map2rmp.exe";
#else
#if defined(Q_OS_MAC)
    // MacOS X: applications are stored in the bundle folder
    path_map2gcm        = QString("%1/Resources/map2gcm").arg(QCoreApplication::applicationDirPath().replace(QRegExp("MacOS$"), ""));
    path_map2jnx        = QString("%1/Resources/map2jnx").arg(QCoreApplication::applicationDirPath().replace(QRegExp("MacOS$"), ""));
    path_cache2gtiff    = QString("%1/Resources/cache2gtiff").arg(QCoreApplication::applicationDirPath().replace(QRegExp("MacOS$"), ""));
    path_map2rmap       = QString("%1/Resources/map2rmap").arg(QCoreApplication::applicationDirPath().replace(QRegExp("MacOS$"), ""));
    path_map2rmp        = QString("%1/Resources/map2rmp").arg(QCoreApplication::applicationDirPath().replace(QRegExp("MacOS$"), ""));
#else
    path_map2gcm        = "map2gcm";
    path_map2jnx        = MAP2JNX;
    path_cache2gtiff    = "cache2gtiff";
    path_map2rmap       = "map2rmap";
    path_map2rmp        = "map2rmp";
#endif
    QProcess proc1;
    proc1.start(path_map2jnx, QStringList());
    proc1.waitForFinished();
    has_map2jnx = proc1.error() == QProcess::UnknownError;
#endif
    groupBirdsEye->hide();
    groupJPEG->hide();
    groupDevice->hide();
    groupRMAP->hide();
    groupMagellanRmp->hide();

    spinJpegQuality->setValue(cfg.value("map/export/jnx/quality",75).toInt());
    comboJpegSubsampling->setCurrentIndex(comboJpegSubsampling->findText(cfg.value("map/export/jnx/subsampling","411").toString()));
    spinProductId->setValue(cfg.value("map/export/jnx/productid",0).toInt());
    lineProductName->setText(cfg.value("map/export/jnx/productname","BirdsEye").toString());
    lineCopyright->setText(cfg.value("map/export/jnx/copyright","None").toString());

    lineMagellanProvider->setText(cfg.value("map/export/rmp/provider", tr("Please enter a string")).toString());
    lineMagellanProduct->setText(cfg.value("map/export/rmp/product", tr("Please enter a string")).toString());
    copyright = cfg.value("map/export/rmp/copyright", tr("")).toString();
    labelMagellanCopyright->setText(QFileInfo(copyright).fileName());

    radioQLM->setChecked(cfg.value("map/export/qlm", true).toBool());
    radioGCM->setChecked(cfg.value("map/export/gcm", false).toBool());
    radioRMAP->setChecked(cfg.value("map/export/rmap", false).toBool());
    radioRMP->setChecked(cfg.value("map/export/rmp", false).toBool());

    if (has_map2jnx)
    {
        radioJNX->setChecked(cfg.value("map/export/jnx", false).toBool());
    }
    else
    {
        radioJNX->hide();
    }

    checkOverview2x->setChecked(cfg.value("map/export/over2x", true).toBool());
    checkOverview4x->setChecked(cfg.value("map/export/over4x", true).toBool());
    checkOverview8x->setChecked(cfg.value("map/export/over8x", true).toBool());
    checkOverview16x->setChecked(cfg.value("map/export/over16x", true).toBool());

    lineStreamingLevels->setText(cfg.value("map/export/stream/levels", "1 ").toString());
    if(lineStreamingLevels->text().isEmpty())
    {
        lineStreamingLevels->setText("1 ");
    }

    lineGeoTiffProjection->setText(cfg.value("map/export/qlm/proj","+proj=longlat +a=6378137.0000 +b=6356752.3142 +towgs84=0,0,0,0,0,0,0,0 +units=m  +no_defs").toString());
    lineGeoTiffProjection->setCursorPosition(0);

    checkProjection->setChecked(cfg.value("map/export/qlm/proj_enable").toBool());

    progressBar->setMinimum(0);
    progressBar->setMaximum(100);
    progressBar->setValue(0);
    progressBar->resize(300, progressBar->height());

    if(cfg.value("map/export/hidedetails", true).toBool())
    {
        textBrowser->hide();
    }
    else
    {
        textBrowser->show();
    }

    if(mapsel.subtype == IMapSelection::eGDAL)
    {
        labelWarnStream->hide();
        groupStreaming->hide();
    }
    else
    {
        labelWarnStream->show();
        groupStreaming->show();
    }

    QFont f = font();
    f.setFamily("Mono");
    textBrowser->setFont(f);

    adjustSize();

}


CMapQMAPExport::~CMapQMAPExport()
{
    SETTINGS;
    cfg.setValue("path/export",labelPath->text());

    cfg.setValue("map/export/qlm", radioQLM->isChecked());
    cfg.setValue("map/export/gcm", radioGCM->isChecked());
    cfg.setValue("map/export/jnx", radioJNX->isChecked());
    cfg.setValue("map/export/rmap", radioRMAP->isChecked());
    cfg.setValue("map/export/rmp", radioRMP->isChecked());

    cfg.setValue("map/export/jnx/quality", spinJpegQuality->value());
    cfg.setValue("map/export/jnx/subsampling", comboJpegSubsampling->currentText());
    cfg.setValue("map/export/jnx/productid", spinProductId->value());
    cfg.setValue("map/export/jnx/productname",lineProductName->text());
    cfg.setValue("map/export/jnx/copyright",lineCopyright->text());

    cfg.setValue("map/export/rmp/provider", lineMagellanProvider->text());
    cfg.setValue("map/export/rmp/product", lineMagellanProduct->text());
    cfg.setValue("map/export/rmp/copyright", copyright);

    cfg.setValue("map/export/over2x",checkOverview2x->isChecked());
    cfg.setValue("map/export/over4x",checkOverview4x->isChecked());
    cfg.setValue("map/export/over8x",checkOverview8x->isChecked());
    cfg.setValue("map/export/over16x",checkOverview16x->isChecked());

    cfg.setValue("map/export/stream/levels", lineStreamingLevels->text());
    cfg.setValue("map/export/qlm/proj", lineGeoTiffProjection->text());
    cfg.setValue("map/export/qlm/proj_enable", checkProjection->isChecked() );

    cfg.setValue("map/export/hidedetails", textBrowser->isHidden());
}


void CMapQMAPExport::slotSetupProj()
{
    CDlgProjWizzard dlg(*lineGeoTiffProjection, this);
    dlg.exec();
    lineGeoTiffProjection->setCursorPosition(0);
}


void CMapQMAPExport::slotSetupProjFromMap()
{
    IMap& map = CMapDB::self().getMap();
    lineGeoTiffProjection->setText(map.getProjection());
    lineGeoTiffProjection->setCursorPosition(0);
}


void CMapQMAPExport::slotBirdsEyeToggled(bool checked)
{
    if(checked)
    {
        groupBirdsEye->show();
        groupMagellanRmp->hide();
        groupJPEG->show();
        groupDevice->show();
        groupOptimization->hide();
        groupRMAP->hide();
    }
    else
    {
        groupBirdsEye->hide();
        groupMagellanRmp->hide();
        groupJPEG->hide();
        groupDevice->hide();
        groupOptimization->hide();
        groupRMAP->hide();
    }

    adjustSize();
}


void CMapQMAPExport::slotGCMToggled(bool checked)
{
    if(checked)
    {
        groupJPEG->show();
        groupMagellanRmp->hide();
        groupDevice->show();
        labelTileSelection->hide();
        groupOptimization->hide();
        groupRMAP->hide();

    }
    else
    {
        groupJPEG->hide();
        groupMagellanRmp->hide();
        groupDevice->hide();
        labelTileSelection->show();
        groupOptimization->hide();
        groupRMAP->hide();
    }

    adjustSize();
}


void CMapQMAPExport::slotQLMToggled(bool checked)
{
    if(checked)
    {
        groupJPEG->hide();
        groupMagellanRmp->hide();
        groupDevice->hide();
        labelTileSelection->show();
        groupOptimization->show();
        groupRMAP->hide();

    }
    else
    {
        groupJPEG->hide();
        groupMagellanRmp->hide();
        groupDevice->hide();
        labelTileSelection->show();
        groupOptimization->hide();
        groupRMAP->hide();
    }

    adjustSize();
}


void CMapQMAPExport::slotRMAPToggled(bool checked)
{
    if(checked)
    {
        groupJPEG->show();
        groupMagellanRmp->hide();
        groupDevice->hide();
        labelTileSelection->show();
        groupOptimization->hide();
        groupRMAP->show();

    }
    else
    {
        groupJPEG->hide();
        groupMagellanRmp->hide();
        groupDevice->hide();
        labelTileSelection->show();
        groupOptimization->hide();
        groupRMAP->hide();
    }

    adjustSize();
}


void CMapQMAPExport::slotRMPToggled(bool checked)
{
    if(checked)
    {
        groupJPEG->show();
        groupMagellanRmp->show();
        groupDevice->hide();
        labelTileSelection->show();
        groupOptimization->hide();
        groupRMAP->hide();

    }
    else
    {
        groupJPEG->hide();
        groupMagellanRmp->hide();
        groupDevice->hide();
        labelTileSelection->show();
        groupOptimization->hide();
        groupRMAP->hide();
    }

    adjustSize();
}


void CMapQMAPExport::slotSelectCopyright()
{
    copyright = QFileDialog::getOpenFileName(0, tr("Select copyright notice..."),"./", tr("text file (*.txt)"), 0, FILE_DIALOG_FLAGS);
    if(copyright.isEmpty())
    {
        labelMagellanCopyright->clear();
        return;
    }

    labelMagellanCopyright->setText(QFileInfo(copyright).fileName());

}


void CMapQMAPExport::slotStderr()
{
    QString str;
    textBrowser->setTextColor(Qt::red);

    str = cmd.readAllStandardError();

#ifndef WIN32
    if(str[0] == '\r')
    {
        textBrowser->moveCursor( QTextCursor::End, QTextCursor::MoveAnchor );
        textBrowser->moveCursor( QTextCursor::StartOfLine, QTextCursor::MoveAnchor );
        textBrowser->moveCursor( QTextCursor::End, QTextCursor::KeepAnchor );
        textBrowser->textCursor().removeSelectedText();

        str = str.split("\r").last();
    }
#endif

    textBrowser->insertPlainText(str);
    textBrowser->verticalScrollBar()->setValue(textBrowser->verticalScrollBar()->maximum());

    tainted = true;
}


void CMapQMAPExport::slotStdout()
{
    QString str;
    textBrowser->setTextColor(Qt::blue);
    str = cmd.readAllStandardOutput();

#ifndef WIN32
    if(str[0] == '\r')
    {
        textBrowser->moveCursor( QTextCursor::End, QTextCursor::MoveAnchor );
        textBrowser->moveCursor( QTextCursor::StartOfLine, QTextCursor::MoveAnchor );
        textBrowser->moveCursor( QTextCursor::End, QTextCursor::KeepAnchor );
        textBrowser->textCursor().removeSelectedText();

        str = str.split("\r").last();
    }
#endif

    progress(str);

    textBrowser->insertPlainText(str);
    textBrowser->verticalScrollBar()->setValue(textBrowser->verticalScrollBar()->maximum());
}


void CMapQMAPExport::progress(const QString& str)
{
    QRegExp re("^(0[0-9\\.]*).*$");

    output += str;
    QStringList lines = output.split("\n");

    if(re.exactMatch(lines.last()))
    {
        QString prog    = re.cap(1);
        int points      = prog.count('.');
        int zeros       = prog.count('0');
        int p = (zeros - 1) * 10 + (points%3) * 2.5 + ((points/3) == zeros ? 7.5 : 0);
        if(p > 100) p = 100;

        progressBar->setValue(p);
    }

}


void CMapQMAPExport::stdOut(const QString& str, bool gui)
{
    textBrowser->setTextColor(Qt::black);
    textBrowser->append(str);

    if(gui)
    {
        QPalette palette = labelStatus->palette();
        palette.setColor(labelStatus->foregroundRole(), Qt::black);
        labelStatus->setPalette(palette);
        labelStatus->setText(str.simplified());
    }
}


void CMapQMAPExport::stdErr(const QString& str, bool gui)
{
    textBrowser->setTextColor(Qt::red);
    textBrowser->append(str);

    if(gui)
    {
        QPalette palette = labelStatus->palette();
        palette.setColor(labelStatus->foregroundRole(), Qt::red);
        labelStatus->setPalette(palette);
        labelStatus->setText(str.simplified());
    }
}


void CMapQMAPExport::slotDetails()
{
    if(textBrowser->isHidden())
    {
        textBrowser->show();
    }
    else
    {
        textBrowser->hide();
    }

    adjustSize();
}


void CMapQMAPExport::slotCancel()
{
    if(cmd.state() != QProcess::NotRunning)
    {
        stdOut(tr("\nCanceled by user's request.\n"));

        cmd.kill();
        cmd.waitForFinished(1000);
    }
    else
    {
        reject();
    }
}


void CMapQMAPExport::slotStart()
{
    if(!states.isEmpty() || !state.isNull()) return;
    pushExport->setEnabled(false);
    pushCancel->setText(tr("Cancel"));
    textBrowser->clear();

    tainted = false;

    if(mapsel.subtype == IMapSelection::eGDAL)
    {
        startExportGDAL();
    }
    else if(mapsel.subtype == IMapSelection::eTMS || mapsel.subtype == IMapSelection::eWMS)
    {
        startExportStreaming();
    }
    else
    {
        stdErr(tr("Unknown map format."), true);
    }

    totalNumberOfStates = states.count();
    // start the statemachine
    setNextState();
}


void CMapQMAPExport::startExportStreaming()
{
    QStringList files;
    QDir tarPath(labelPath->text());
    QString prefix = linePrefix->text();
    CMapDB::map_t& map = CMapDB::self().knownMaps[mapsel.mapkey];
    // *********************************************
    // 1. step: Create GeoTiff from map tile cache
    // ---------------------------------------------
    CMapExportStateReadTileCache * state1 = new CMapExportStateReadTileCache(path_cache2gtiff, this);

    QStringList levels = lineStreamingLevels->text().split(" ",QString::SkipEmptyParts);

    foreach(const QString& level, levels)
    {
        CMapExportStateReadTileCache::job_t job;

        job.srcFile = map.filename;
        job.tarFile = IMapExportState::getTempFilename();
        job.level   = level.toInt();
        job.lon1    = mapsel.lon1 * RAD_TO_DEG;
        job.lat1    = mapsel.lat1 * RAD_TO_DEG;
        job.lon2    = mapsel.lon2 * RAD_TO_DEG;
        job.lat2    = mapsel.lat2 * RAD_TO_DEG;

        state1->addJob(job);
        files << job.tarFile;
    }
    states << state1;

    startExportCommon(files, tarPath, prefix);
}


void CMapQMAPExport::startExportGDAL()
{
    qDebug() << "CMapQMAPExport::slotStart()";

    CMapDB::map_t& map = CMapDB::self().knownMaps[mapsel.mapkey];
    QDir srcPath = QFileInfo(map.filename).absolutePath();
    QDir tarPath(labelPath->text());
    QString prefix = linePrefix->text();
    QSettings srcdef(map.filename, QSettings::IniFormat);

    int levels = srcdef.value("main/levels",0).toInt();

    // *********************************************
    // 1. step: cut defined areas from all map files
    // ---------------------------------------------
    CMapExportStateCutFiles * state1 =  new CMapExportStateCutFiles(this);
    // iterate over all map levels
    for(int level = 1; level <= levels; ++level)
    {
        // iterate over all files in a level
        QStringList filenames = srcdef.value(QString("level%1/files").arg(level),"").toString().split("|", QString::SkipEmptyParts);
        foreach(const QString& filename, filenames)
        {
            qDebug();
            CMapFile mapfile(srcPath.filePath(filename), this);
            if(!mapfile.ok)
            {
                QMessageBox::critical(0,tr("Error ..."), tr("Failed to read %1").arg(filename), QMessageBox::Abort,  QMessageBox::Abort);
                return QDialog::reject();
            }

            // transform the WGS84 points that define the selection into the map files projection system
            projPJ  pjWGS84 = pj_init_plus("+proj=longlat  +datum=WGS84 +no_defs");
            projXY p1,p2;
            p1.u = mapsel.lon1;
            p1.v = mapsel.lat1;
            pj_transform(pjWGS84,mapfile.pj,1,0,&p1.u,&p1.v,0);

            p2.u = mapsel.lon2;
            p2.v = mapsel.lat2;
            pj_transform(pjWGS84,mapfile.pj,1,0,&p2.u,&p2.v,0);
            pj_free(pjWGS84);

            // cacluate the selected area on the map
            QRectF maparea(QPointF(mapfile.xref1, mapfile.yref1), QPointF(mapfile.xref2, mapfile.yref2));
            QRectF selarea(QPointF(p1.u, p1.v), QPointF(p2.u, p2.v));
            QRectF intersect = selarea.intersected(maparea);

            if(intersect.isValid())
            {
                CMapExportStateCutFiles::job_t job;

                job.level  = level;
                job.xoff   = (intersect.left()   - mapfile.xref1) / mapfile.xscale;
                job.yoff   = (intersect.bottom() - mapfile.yref1) / mapfile.yscale;
                job.width  =  intersect.width()  / mapfile.xscale;
                if(job.width == 0)
                {
                    // no empty files
                    continue;
                }
                job.height = -intersect.height() / mapfile.yscale;
                if (job.height == 0)
                {
                    // no empty files
                    continue;
                }

                job.tarFile = IMapExportState::getTempFilename();
                job.srcFile = mapfile.filename;

                qDebug() << job.level << job.srcFile << ">>" << job.tarFile;
                qDebug() << job.xoff << job.yoff << job.width << job.height;

                state1->addJob(job);
            }
        }
    }
    states << state1;

    // *********************************************
    // 2. step: combine files on a level
    // ---------------------------------------------
    CMapExportStateCombineFiles * state2 = new CMapExportStateCombineFiles(this);
    for(int level = 1; level <= levels; level++)
    {
        CMapExportStateCombineFiles::job_t job;
        job.tarFile = IMapExportState::getTempFilename();
        foreach(const CMapExportStateCutFiles::job_t& j, state1->getJobs())
        {
            if(j.level == level)
            {
                job.srcFile << j.tarFile;
            }
        }

        if(!job.srcFile.isEmpty())
        {
            state2->addJob(job);
        }
    }
    states << state2;

    // *********************************************
    // 3. step: move 8 bit files into rgb color space
    // ---------------------------------------------
    QStringList files;
    CMapExportStateConvColor * state3 = new CMapExportStateConvColor(this);
    foreach(const CMapExportStateCombineFiles::job_t& j, state2->getJobs())
    {
        CMapExportStateConvColor::job_t job;
        job.srcFile = j.tarFile;
        job.tarFile = IMapExportState::getTempFilename();
        state3->addJob(job);

        files << job.tarFile;
    }
    states << state3;

    startExportCommon(files, tarPath, prefix);
}


void CMapQMAPExport::startExportCommon(QStringList& srcFiles, QDir& tarPath, const QString& prefix)
{
    // *********************************************
    // 4. step: reproject files
    // ---------------------------------------------
    int cnt = 0;

    QString projection("EPSG:4326");
    if(radioQLM->isChecked())
    {
        if(checkProjection->isChecked())
        {
            projection = lineGeoTiffProjection->text();
        }
        else
        {
            projection.clear();
        }
    }
    else if(radioRMAP->isChecked())
    {
        projection = comboRmapProjection->itemData(comboRmapProjection->currentIndex()).toString();
    }
    else if(radioRMP->isChecked())
    {
        projection = "+init=epsg:4326";
    }

    CMapExportStateReproject * state4 = new CMapExportStateReproject(projection, this);
    foreach(const QString& srcFile, srcFiles)
    {
        CMapExportStateReproject::job_t job;
        job.srcFile = srcFile;

        if(radioQLM->isChecked())
        {
            job.tarFile = tarPath.filePath(QString("%1_%2.tif").arg(prefix).arg(cnt++));
            QFile::remove(job.tarFile);
        }
        else
        {
            job.tarFile = IMapExportState::getTempFilename();
        }
        state4->addJob(job);
    }
    states << state4;

    if(radioQLM->isChecked())
    {
        // *********************************************
        // 5. step: add overview levels
        // ---------------------------------------------

        QStringList overviews;
        if(checkOverview2x->isChecked())  overviews << "2";
        if(checkOverview4x->isChecked())  overviews << "4";
        if(checkOverview8x->isChecked())  overviews << "8";
        if(checkOverview16x->isChecked()) overviews << "16";

        CMapExportStateOptimize * state5 = new CMapExportStateOptimize(this);
        foreach(const CMapExportStateReproject::job_t& j, state4->getJobs())
        {
            CMapExportStateOptimize::job_t job;
            job.overviews   = overviews;
            job.srcFile     = j.tarFile;
            state5->addJob(job);
        }
        states << state5;
    }
    else if(radioGCM->isChecked())
    {
        // *********************************************
        // 5. step: convert Geotiff to Garmin Custom Map
        // ---------------------------------------------
        QString tileFile;
        QList< QPair<int, int> > keys = mapsel.selTiles.keys();
        if(keys.count())
        {
            QPair<int, int> key;

            qSort(keys.begin(), keys.end(), tileIndexLessThan);
            key = keys.last();
            int xmax = key.first  + 1;

            QString index;
            foreach(key, keys)
            {
                if(mapsel.selTiles[key] == false)
                {
                    index += QString("%1 ").arg(key.second * xmax + key.first);
                }
            }

            QFile file(IMapExportState::getTempFilename());
            file.open(QIODevice::WriteOnly);
            file.write(index.toLatin1());
            file.flush();
            file.close();
            tileFile = file.fileName();
        }

        CMapExportStateGCM * state5 = new CMapExportStateGCM(path_map2gcm, this);

        CMapExportStateGCM::job_t job;

        job.jpegQuality = QString::number(spinJpegQuality->value());
        job.jpegSubSmpl = comboJpegSubsampling->currentText();
        job.zOrder      = QString::number(spinZOrder->value());
        job.tileFile    = tileFile;

        foreach(const CMapExportStateReproject::job_t& j, state4->getJobs())
        {
            job.srcFile << j.tarFile;

        }
        job.tarFile     = tarPath.filePath(QString("%1.kmz").arg(prefix));

        state5->addJob(job);
        states << state5;
    }
    else if(radioJNX->isChecked())
    {
        // *********************************************
        // 5. step: convert Geotiff to Garmin JNX Map
        // ---------------------------------------------
        CMapExportStateJNX * state5 = new CMapExportStateJNX(path_map2jnx, this);

        CMapExportStateJNX::job_t job;

        job.jpegQuality = QString::number(spinJpegQuality->value());
        job.jpegSubSmpl = comboJpegSubsampling->currentText();
        job.zOrder      = QString::number(spinZOrder->value());
        job.productId   = QString::number(spinProductId->value());
        job.productName = lineProductName->text();
        job.description = lineDescription->text();
        job.copyright   = lineCopyright->text();

        foreach(const CMapExportStateReproject::job_t& j, state4->getJobs())
        {
            job.srcFile << j.tarFile;

        }
        job.tarFile     = tarPath.filePath(QString("%1.jnx").arg(prefix));

        state5->addJob(job);
        states << state5;
    }

    else if(radioRMAP->isChecked())
    {
        // *********************************************
        // 5. step: add overview levels
        // ---------------------------------------------

        QStringList overviews;
        overviews << "2" << "4" << "8" << "16";

        CMapExportStateOptimize * state5 = new CMapExportStateOptimize(this);
        foreach(const CMapExportStateReproject::job_t& j, state4->getJobs())
        {
            CMapExportStateOptimize::job_t job;
            job.overviews   = overviews;
            job.srcFile     = j.tarFile;
            state5->addJob(job);
        }
        states << state5;

        // *********************************************
        // 5. step: convert Geotiff to TwoNav RMAP
        // ---------------------------------------------
        CMapExportStateRMAP * state6 = new CMapExportStateRMAP(path_map2rmap, this);

        CMapExportStateRMAP::job_t job;

        job.jpegQuality = QString::number(spinJpegQuality->value());
        job.jpegSubSmpl = comboJpegSubsampling->currentText();

        foreach(const CMapExportStateOptimize::job_t& j, state5->getJobs())
        {
            job.srcFile << j.srcFile;

        }
        job.tarFile     = tarPath.filePath(QString("%1.rmap").arg(prefix));

        state6->addJob(job);
        states << state6;

    }
    else if(radioRMP->isChecked())
    {
        // *********************************************
        // 5. step: convert Geotiff to Garmin JNX Map
        // ---------------------------------------------
        CMapExportStateRMP * state5 = new CMapExportStateRMP(path_map2rmp, this);

        CMapExportStateRMP::job_t job;

        job.jpegQuality = QString::number(spinJpegQuality->value());
        job.jpegSubSmpl = comboJpegSubsampling->currentText();
        job.provider    = lineMagellanProvider->text();
        job.product     = lineMagellanProduct->text();
        job.copyright   = copyright;

        foreach(const CMapExportStateReproject::job_t& j, state4->getJobs())
        {
            job.srcFile << j.tarFile;

        }
        job.tarFile     = tarPath.filePath(QString("%1.rmp").arg(prefix));

        state5->addJob(job);
        states << state5;
    }

}


void CMapQMAPExport::setNextState()
{
    output.clear();

    if(!state.isNull()) state->deleteLater();

    if(states.isEmpty())
    {
        stdOut(tr("*** done ***\n"), true);
        if(tainted)
        {
            stdErr(tr("Warnings. See \"Details\" for more information.\n"), true);
        }
        progressBar->setValue(100);
        pushExport->setEnabled(true);
        pushCancel->setText(tr("Close"));
    }
    else
    {
        state = states.takeFirst();
        state->explain();
        state->nextJob(cmd);

        labelStep->setText(tr("Step %1/%2,").arg(totalNumberOfStates - states.count()).arg(totalNumberOfStates));
        labelJob->setText(tr("Job %1/%2 - ").arg(state->getJobIdx()).arg(state->getJobCnt()));
    }
}


void CMapQMAPExport::slotFinished(int exitCode, QProcess::ExitStatus status)
{
    output.clear();

    if(exitCode || status)
    {

        textBrowser->setTextColor(Qt::red);
        textBrowser->append(tr("!!! failed !!!\n"));

        labelStatus->setText(tr("Failed. See \"Details\" for more information."));

        qDeleteAll(states);
        states.clear();
        state->deleteLater();

        pushExport->setEnabled(true);
        pushCancel->setText(tr("Close"));
        return;
    }

    QApplication::processEvents();
    state->nextJob(cmd);
    labelJob->setText(tr("Job %1/%2 - ").arg(state->getJobIdx()).arg(state->getJobCnt()));
}


void CMapQMAPExport::slotOutputPath()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Select output path..."), labelPath->text(), FILE_DIALOG_FLAGS);
    if(path.isEmpty()) return;
    labelPath->setText(path);
}


// --------------------------------------------------------------------------------------------

quint32 IMapExportState::tmpFileCnt = 0;

IMapExportState::IMapExportState(CMapQMAPExport * parent)
: QObject(parent)
, gui(parent)
, jobIdx(0)
{

}


IMapExportState::~IMapExportState()
{

}


QString IMapExportState::getTempFilename()
{
    QTemporaryFile * tmp = new QTemporaryFile(QDir::temp().absoluteFilePath(QString("qlgt_%1.XXXXXX.tif").arg(tmpFileCnt++)));
    tmp->open();
    QString fn =  tmp->fileName();
    tmp->close();
    delete tmp;

    return fn;
}


// --------------------------------------------------------------------------------------------
CMapExportStateCutFiles::CMapExportStateCutFiles(CMapQMAPExport * parent)
: IMapExportState(parent)

{
}


CMapExportStateCutFiles::~CMapExportStateCutFiles()
{
    qDebug() << "~CMapExportStateCutFiles()";
}


void CMapExportStateCutFiles::explain()
{
    gui->stdOut(   "*************************************");
    gui->stdOut(tr("Cut area from files..."), true);
    gui->stdOut(   "-------------------------------------");
}


void CMapExportStateCutFiles::nextJob(QProcess& cmd)
{
    if(jobIdx < jobs.count())
    {
        job_t& job = jobs[jobIdx];

        bool isRgb;
        {
            CMapFile mapfile(job.srcFile, this);
            isRgb = mapfile.rasterBandCount != 1;
        }

        QStringList args;
        args << "-co" << "tiled=yes" << "-co" << "compress=DEFLATE";

        if(!isRgb)
        {
            args << "-expand" << "rgba";
        }

        args << "-srcwin";
        args << QString::number(job.xoff) << QString::number(job.yoff);
        args << QString::number(job.width) << QString::number(job.height);
        args << job.srcFile;
        args << job.tarFile;

        jobIdx++;

        gui->stdOut(GDALTRANSLATE " " +  args.join(" ") + "\n");
        cmd.start(GDALTRANSLATE, args);

    }
    else
    {
        gui->setNextState();
    }
}


// --------------------------------------------------------------------------------------------
CMapExportStateCombineFiles::CMapExportStateCombineFiles(CMapQMAPExport * parent)
: IMapExportState(parent)

{

}


CMapExportStateCombineFiles::~CMapExportStateCombineFiles()
{
    qDebug() << "~CMapExportStateCombineFiles()";

    foreach(const job_t& job, jobs)
    {
        foreach(const QString& file, job.srcFile)
        {
            QFile::remove(file);
        }
    }
}


void CMapExportStateCombineFiles::explain()
{
    gui->stdOut(   "*************************************");
    gui->stdOut(tr("Combine files for each level..."), true);
    gui->stdOut(   "-------------------------------------");
}


void CMapExportStateCombineFiles::nextJob(QProcess& cmd)
{
    if(jobIdx < jobs.count())
    {
        job_t& job = jobs[jobIdx];

        if(job.srcFile.count() == 1)
        {
            gui->stdOut("copy " + job.srcFile[0] + " ->" + job.tarFile + "\n");
            QFile::rename(job.srcFile[0], job.tarFile);
            jobIdx++;

            gui->slotFinished(0, QProcess::NormalExit);
            return;
        }

        QStringList args;
        args << "-co" << "tiled=yes" << "-co" << "compress=DEFLATE";
        args << job.srcFile;
        args << job.tarFile;

        jobIdx++;

        gui->stdOut(GDALWARP " " +  args.join(" ") + "\n");
        cmd.start(GDALWARP, args);
    }
    else
    {
        gui->setNextState();
    }
}


// --------------------------------------------------------------------------------------------
CMapExportStateConvColor::CMapExportStateConvColor(CMapQMAPExport * parent)
: IMapExportState(parent)

{

}


CMapExportStateConvColor::~CMapExportStateConvColor()
{
    qDebug() << "~CMapExportStateConvColor()";

    foreach(const job_t& job, jobs)
    {
        QFile::remove(job.srcFile);
    }

}


void CMapExportStateConvColor::explain()
{
    gui->stdOut(   "*************************************");
    gui->stdOut(tr("Reduce color bands to 3 (RGB)..."), true);
    gui->stdOut(   "-------------------------------------");
}


void CMapExportStateConvColor::nextJob(QProcess& cmd)
{
    if(jobIdx < jobs.count())
    {
        job_t& job = jobs[jobIdx];

        QStringList args;

        args << "-b" << "1";
        args << "-b" << "2";
        args << "-b" << "3";

        args << "-co" << "tiled=yes" << "-co" << "compress=jpeg" << "-co" << "jpeg_quality=100";

        args << job.srcFile;
        args << job.tarFile;

        jobIdx++;

        gui->stdOut(GDALTRANSLATE " " +  args.join(" ") + "\n");
        cmd.start(GDALTRANSLATE, args);

    }
    else
    {
        gui->setNextState();
    }
}


// --------------------------------------------------------------------------------------------
CMapExportStateReproject::CMapExportStateReproject(const QString& proj, CMapQMAPExport * parent)
: IMapExportState(parent)

, proj(proj)
{

}


CMapExportStateReproject::~CMapExportStateReproject()
{
    qDebug() << "~CMapExportStateReproject()";

    foreach(const job_t& job, jobs)
    {
        QFile::remove(job.srcFile);
    }

}


void CMapExportStateReproject::explain()
{
    gui->stdOut(   "*************************************");
    gui->stdOut(tr("Re-project files..."), true);
    gui->stdOut(   "-------------------------------------");
}


void CMapExportStateReproject::nextJob(QProcess& cmd)
{
    if(jobIdx < jobs.count())
    {
        job_t& job = jobs[jobIdx];

        if(proj.isEmpty())
        {
            gui->stdOut("copy " + job.srcFile[0] + " ->" + job.tarFile + "\n");
            QFile::rename(job.srcFile, job.tarFile);
            jobIdx++;

            gui->slotFinished(0, QProcess::NormalExit);
            return;
        }

        QString width, height;
        {
            CMapFile mapfile(job.srcFile, this);
            width   = QString("%1").arg(mapfile.xsize_px);
            height  = QString("%1").arg(mapfile.ysize_px);
        }

        QStringList args;
        args << "-t_srs" << proj;
        args << "-ts" << width << height;
        args << "-r" << "cubic";
        args << "-co" << "tiled=yes" << "-co" << "compress=jpeg" << "-co" << "jpeg_quality=100";
        args << job.srcFile;
        args << job.tarFile;

        jobIdx++;

        gui->stdOut(GDALWARP " " +  args.join(" ") + "\n");
        cmd.start(GDALWARP, args);

    }
    else
    {
        gui->setNextState();
    }
}


// --------------------------------------------------------------------------------------------
CMapExportStateOptimize::CMapExportStateOptimize(CMapQMAPExport * parent)
: IMapExportState(parent)

{

}


CMapExportStateOptimize::~CMapExportStateOptimize()
{
    qDebug() << "~CMapExportStateOptimize()";
}


void CMapExportStateOptimize::explain()
{
    gui->stdOut(   "*************************************");
    gui->stdOut(tr("Optimize files..."), true);
    gui->stdOut(   "-------------------------------------");
}


void CMapExportStateOptimize::nextJob(QProcess& cmd)
{
    if(jobIdx < jobs.count())
    {
        job_t& job = jobs[jobIdx];

        if(job.overviews.isEmpty())
        {
            gui->stdOut(tr("nothing to do\n"));
            jobIdx++;

            gui->slotFinished(0, QProcess::NormalExit);
            return;
        }

        QStringList args;
        args << "-r" << "cubic";
        args << "--config" << "COMPRESS_OVERVIEW" << "JPEG";
        args << job.srcFile;
        args += job.overviews;

        jobIdx++;

        gui->stdOut(GDALADDO " " +  args.join(" ") + "\n");
        cmd.start(GDALADDO, args);
    }
    else
    {
        gui->setNextState();
    }
}


// --------------------------------------------------------------------------------------------
CMapExportStateGCM::CMapExportStateGCM(const QString& app, CMapQMAPExport * parent)
: IMapExportState(parent)

, app(app)
{

}


CMapExportStateGCM::~CMapExportStateGCM()
{
    qDebug() << "~CMapExportStateGCM()";

    foreach(const job_t& job, jobs)
    {
        QFile::remove(job.tileFile);
        foreach(const QString& file, job.srcFile)
        {
            QFile::remove(file);
        }
    }
}


void CMapExportStateGCM::explain()
{
    gui->stdOut(   "*************************************");
    gui->stdOut(tr("Create Garmin Custom Map..."), true);
    gui->stdOut(   "-------------------------------------");
}


void CMapExportStateGCM::nextJob(QProcess& cmd)
{
    if(jobIdx < jobs.count())
    {
        job_t& job = jobs[jobIdx];

        QStringList args;
        args << "-q" << job.jpegQuality;
        args << "-s" << job.jpegSubSmpl;
        args << "-z" << job.zOrder;
        args << "-t" << job.tileFile;

        args += job.srcFile;
        args << job.tarFile;

        jobIdx++;

        gui->stdOut(app + " " +  args.join(" ") + "\n");
        cmd.start(app, args);

    }
    else
    {
        gui->setNextState();
    }
}


// --------------------------------------------------------------------------------------------
CMapExportStateJNX::CMapExportStateJNX(const QString& app, CMapQMAPExport * parent)
: IMapExportState(parent)

, app(app)
{

}


CMapExportStateJNX::~CMapExportStateJNX()
{
    qDebug() << "~CMapExportStateJNX()";

    foreach(const job_t& job, jobs)
    {
        foreach(const QString& file, job.srcFile)
        {
            QFile::remove(file);
        }
    }
}


void CMapExportStateJNX::explain()
{
    gui->stdOut(   "*************************************");
    gui->stdOut(tr("Create Garmin JNX Map..."), true);
    gui->stdOut(   "-------------------------------------");
}


void CMapExportStateJNX::nextJob(QProcess& cmd)
{
    if(jobIdx < jobs.count())
    {
        job_t& job = jobs[jobIdx];

        QStringList args;
        args << "-q" << job.jpegQuality;
        args << "-s" << job.jpegSubSmpl;
        args << "-p" << job.productId;
        args << "-m" << job.productName;
        args << "-n" << job.description;
        args << "-c" << job.copyright;
        args << "-z" << job.zOrder;
        args += job.srcFile;
        args << job.tarFile;

        jobIdx++;

        gui->stdOut(app + " " +  args.join(" ") + "\n");
        cmd.start(app, args);
    }
    else
    {
        gui->setNextState();
    }
}


// --------------------------------------------------------------------------------------------
CMapExportStateReadTileCache::CMapExportStateReadTileCache(const QString& app, CMapQMAPExport * parent)
: IMapExportState(parent)
, app(app)
{

}


CMapExportStateReadTileCache::~CMapExportStateReadTileCache()
{
    qDebug() << "~CMapExportStateReadTileCache()";

}


void CMapExportStateReadTileCache::explain()
{
    gui->stdOut(   "*************************************");
    gui->stdOut(tr("Create GeoTiff from map cache..."), true);
    gui->stdOut(   "-------------------------------------");
}


void CMapExportStateReadTileCache::nextJob(QProcess& cmd)
{
    if(jobIdx < jobs.count())
    {
        job_t& job = jobs[jobIdx];

        QStringList args;
        args << "-a" << QString::number(job.level)
            << QString::number(job.lon1, 'f', 12)
            << QString::number(job.lat1, 'f', 12)
            << QString::number(job.lon2, 'f', 12)
            << QString::number(job.lat2, 'f', 12);
        args << "-c" << CResources::self().getPathMapCache().absolutePath();
        args << "-i" << job.srcFile;
        args << "-o" << job.tarFile;

        jobIdx++;

        gui->stdOut(app + " " +  args.join(" ") + "\n");
        cmd.start(app, args);
    }
    else
    {
        gui->setNextState();
    }
}


// --------------------------------------------------------------------------------------------
CMapExportStateRMAP::CMapExportStateRMAP(const QString& app, CMapQMAPExport * parent)
: IMapExportState(parent)

, app(app)
{

}


CMapExportStateRMAP::~CMapExportStateRMAP()
{
    qDebug() << "~CMapExportStateRMAP()";

    foreach(const job_t& job, jobs)
    {
        foreach(const QString& file, job.srcFile)
        {
            QFile::remove(file);
        }
    }
}


void CMapExportStateRMAP::explain()
{
    gui->stdOut(   "*************************************");
    gui->stdOut(tr("Create TwoNav RMAP..."), true);
    gui->stdOut(   "-------------------------------------");
}


void CMapExportStateRMAP::nextJob(QProcess& cmd)
{
    if(jobIdx < jobs.count())
    {
        job_t& job = jobs[jobIdx];

        QStringList args;
        args << "-q" << job.jpegQuality;
        args << "-s" << job.jpegSubSmpl;
        args += job.srcFile;
        args << job.tarFile;

        jobIdx++;

        gui->stdOut(app + " " +  args.join(" ") + "\n");
        cmd.start(app, args);
    }
    else
    {
        gui->setNextState();
    }
}


// --------------------------------------------------------------------------------------------
CMapExportStateRMP::CMapExportStateRMP(const QString& app, CMapQMAPExport * parent)
: IMapExportState(parent)

, app(app)
{

}


CMapExportStateRMP::~CMapExportStateRMP()
{
    qDebug() << "~CMapExportStateRMP()";

    foreach(const job_t& job, jobs)
    {
        foreach(const QString& file, job.srcFile)
        {
            QFile::remove(file);
        }
    }
}


void CMapExportStateRMP::explain()
{
    gui->stdOut(   "*************************************");
    gui->stdOut(tr("Create Magellan RMP Map..."), true);
    gui->stdOut(   "-------------------------------------");
}


void CMapExportStateRMP::nextJob(QProcess& cmd)
{
    if(jobIdx < jobs.count())
    {
        job_t& job = jobs[jobIdx];

        QStringList args;
        args << "-q" << job.jpegQuality;
        args << "-s" << job.jpegSubSmpl;
        args << "-p" << job.provider;
        args << "-n" << job.product;
        if(!job.copyright.isEmpty())
        {
            args << "-c" << job.copyright;
        }
        args += job.srcFile;
        args << job.tarFile;

        jobIdx++;

        gui->stdOut(app + " " +  args.join(" ") + "\n");
        cmd.start(app, args);
    }
    else
    {
        gui->setNextState();
    }
}
