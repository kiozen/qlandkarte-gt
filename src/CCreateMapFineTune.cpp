/**********************************************************************************************
    Copyright (C) 2010 Oliver Eichler oliver.eichler@gmx.de

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

#include "CCreateMapFineTune.h"
#include "CMapDB.h"
#include "CMainWindow.h"
#include "CMapRaster.h"

#include "config.h"
#include "CSettings.h"

#include <QtGui>
#include <QFileDialog>
#include <gdal_priv.h>

CCreateMapFineTune::CCreateMapFineTune(QWidget * parent)
: QWidget(parent)
{
    setupUi(this);

    toolUp->setIcon(QIcon(":/icons/iconUpload16x16.png"));
    toolDown->setIcon(QIcon(":/icons/iconDownload16x16.png"));
    toolLeft->setIcon(QIcon(":/icons/iconLeft16x16.png"));
    toolRight->setIcon(QIcon(":/icons/iconRight16x16.png"));

    connect(pushOpenFile, SIGNAL(clicked()), this, SLOT(slotOpenFile()));
    connect(toolUp, SIGNAL(clicked()), this, SLOT(slotUp()));
    connect(toolDown, SIGNAL(clicked()), this, SLOT(slotDown()));
    connect(toolLeft, SIGNAL(clicked()), this, SLOT(slotLeft()));
    connect(toolRight, SIGNAL(clicked()), this, SLOT(slotRight()));

    connect(pushSave, SIGNAL(clicked()), this, SLOT(slotSave()));
    progressBar->hide();

}


CCreateMapFineTune::~CCreateMapFineTune()
{

}


void CCreateMapFineTune::slotOpenFile()
{
    SETTINGS;
    path = QDir(cfg.value("path/create",path.path()).toString());

    QString filename = QFileDialog::getOpenFileName(0, tr("Open map file..."),path.path(), tr("Referenced file (*.tif *.tiff *.png *.gif)"), 0, FILE_DIALOG_FLAGS);
    if(filename.isEmpty()) return;

    CMapDB::self().openMap(filename, false, *theMainWindow->getCanvas());

    IMap& map = CMapDB::self().getMap();

    QString proj = map.getProjection();
    if(proj.isEmpty())
    {
        toolUp->setEnabled(false);
        toolDown->setEnabled(false);
        toolLeft->setEnabled(false);
        toolRight->setEnabled(false);
        pushSave->setEnabled(false);
        labelInfile->setText("");
        labelOutfile->setText("");

    }
    else
    {
        toolUp->setEnabled(true);
        toolDown->setEnabled(true);
        toolLeft->setEnabled(true);
        toolRight->setEnabled(true);
        pushSave->setEnabled(true);
        labelInfile->setText(filename);

        QFileInfo fi(filename);

        labelOutfile->setText(fi.dir().filePath(fi.baseName() + "_tuned." + fi.completeSuffix()));
    }

}


void CCreateMapFineTune::slotUp()
{
    IMap& map = CMapDB::self().getMap();
    map.incYOffset(1);
}


void CCreateMapFineTune::slotDown()
{
    IMap& map = CMapDB::self().getMap();
    map.decYOffset(1);
}


void CCreateMapFineTune::slotLeft()
{
    IMap& map = CMapDB::self().getMap();
    map.decXOffset(1);
}


void CCreateMapFineTune::slotRight()
{
    IMap& map = CMapDB::self().getMap();
    map.incXOffset(1);
}


int CPL_STDCALL ProgressFunc(double dfComplete, const char *pszMessage, void *pProgressArg)
{
    CCreateMapFineTune * parent = (CCreateMapFineTune*)pProgressArg;
    parent->progressBar->setValue(dfComplete * 100);
    return TRUE;
}


void CCreateMapFineTune::slotSave()
{
    IMap& map = CMapDB::self().getMap();
    GDALDataset * srcds = map.getDataset();
    if(srcds == 0)
    {
        return;
    }

    double adfGeoTransform[6];
    double u1, v1, u2, v2;
    map.dimensions(u1, v1, u2, v2);
    map.convertRad2M(u1, v1);
    map.convertRad2M(u2, v2);

    srcds->GetGeoTransform( adfGeoTransform );
    adfGeoTransform[0] = u1;
    adfGeoTransform[3] = v1;

    progressBar->show();
    GDALDriver * driver = srcds->GetDriver();

    char **papszOptions = NULL;
    papszOptions = CSLSetNameValue( papszOptions, "TILED", "YES" );
    papszOptions = CSLSetNameValue( papszOptions, "COMPRESS", "DEFLATE" );

    GDALDataset * dstds = driver->CreateCopy(labelOutfile->text().toLocal8Bit(), srcds, false, papszOptions, ProgressFunc, this);
    if(dstds)
    {
        dstds->SetGeoTransform( adfGeoTransform );
        GDALClose(dstds);
    }
    CSLDestroy( papszOptions );

    progressBar->hide();
}
