/**********************************************************************************************
    Copyright (C) 2008 Oliver Eichler oliver.eichler@gmx.de

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

#include "CDlgEditMapLevel.h"
#include "CCreateMapQMAP.h"
#include <QtGui>
#include <QFileDialog>
#include <gdal.h>

#include "config.h"

CDlgEditMapLevel::CDlgEditMapLevel(QTreeWidgetItem * item,  const QString& path, QWidget * parent)
: QDialog(parent)
, item(item)
, mapPath(path)
{
    setupUi(this);

    spinZoom->setValue(item->data(0,CCreateMapQMAP::eZoom).toInt());
    listFiles->addItems(item->text(CCreateMapQMAP::eFiles).split("; ",QString::SkipEmptyParts));

    toolFiles->setIcon(QPixmap(":/icons/iconFileLoad16x16.png"));
    connect(toolFiles, SIGNAL(clicked()), this, SLOT(slotSelectFiles()));

    toolAdd->setIcon(QPixmap(":/icons/iconFileAdd16x16.png"));
    connect(toolAdd, SIGNAL(clicked()), this, SLOT(slotAdd()));

    toolDel->setIcon(QPixmap(":/icons/iconFileDel16x16.png"));
    connect(toolDel, SIGNAL(clicked()), this, SLOT(slotDel()));

    toolUp->setIcon(QPixmap(":/icons/iconUpload16x16.png"));
    connect(toolUp, SIGNAL(clicked()), this, SLOT(slotUp()));

    toolDown->setIcon(QPixmap(":/icons/iconDownload16x16.png"));
    connect(toolDown, SIGNAL(clicked()), this, SLOT(slotDown()));

    connect(listFiles, SIGNAL(itemSelectionChanged()), this, SLOT(slotListChanged()));
}


CDlgEditMapLevel::~CDlgEditMapLevel()
{
}


void CDlgEditMapLevel::accept()
{
    QString str;
    QList<QListWidgetItem *> files = listFiles->findItems(".*", Qt::MatchRegExp);
    QListWidgetItem * file;
    foreach(file,files)
    {
        str += file->text();
        str += "; ";
    }

    item->setText(CCreateMapQMAP::eFiles, str);
    item->setData(0,CCreateMapQMAP::eZoom,spinZoom->value());

    QDialog::accept();
}


void CDlgEditMapLevel::slotSelectFiles()
{
    int gdalver = QString(GDALVersionInfo("VERSION_NUM")).toInt();
    QStringList files;
    if(gdalver >= 1800)
    {
        files = QFileDialog::getOpenFileNames(0, tr("Select <b>all</b> files for that level."), mapPath, "All (*.*);;GeoTiff (*.tif *.tiff);;OziMap (*.map)", 0, FILE_DIALOG_FLAGS);
    }
    else
    {
        files = QFileDialog::getOpenFileNames(0, tr("Select <b>all</b> files for that level."), mapPath, "All (*.*);;GeoTiff (*.tif *.tiff)", 0, FILE_DIALOG_FLAGS);
    }
    if(files.isEmpty()) return;

    listFiles->clear();

    QDir dir(mapPath);
    QString file;
    foreach(file,files)
    {
        listFiles->addItem(dir.relativeFilePath(file));
    }
}


void CDlgEditMapLevel::slotListChanged()
{
    QListWidgetItem * item = listFiles->currentItem();
    if(item != 0)
    {
        toolDel->setEnabled(true);
        toolUp->setEnabled(true);
        toolDown->setEnabled(true);
    }
    else
    {
        toolDel->setEnabled(false);
        toolUp->setEnabled(false);
        toolDown->setEnabled(false);
    }

}


void CDlgEditMapLevel::slotAdd()
{
    int gdalver = QString(GDALVersionInfo("VERSION_NUM")).toInt();
    QStringList files;
    if(gdalver >= 1800)
    {
        files = QFileDialog::getOpenFileNames(0, tr("Select <b>all</b> files for that level."), mapPath, "All (*.*);;GeoTiff (*.tif *.tiff);;OziMap (*.map)", 0, FILE_DIALOG_FLAGS);
    }
    else
    {
        files = QFileDialog::getOpenFileNames(0, tr("Select <b>all</b> files for that level."), mapPath, "All (*.*);;GeoTiff (*.tif *.tiff)", 0, FILE_DIALOG_FLAGS);
    }

    if(files.isEmpty()) return;

    QDir dir(mapPath);
    QString file;
    foreach(file,files)
    {
        listFiles->addItem(dir.relativeFilePath(file));
    }
}


void CDlgEditMapLevel::slotDel()
{
    QListWidgetItem * item = listFiles->currentItem();
    if(item != 0)
    {
        delete item;
    }
}


void CDlgEditMapLevel::slotUp()
{
    QListWidgetItem * item = listFiles->currentItem();
    if(item)
    {
        int row = listFiles->row(item);
        if(row == 0) return;
        listFiles->takeItem(row);
        row = row - 1;
        listFiles->insertItem(row,item);
        listFiles->setCurrentItem(item);
    }
}


void CDlgEditMapLevel::slotDown()
{
    QListWidgetItem * item = listFiles->currentItem();
    if(item)
    {
        int row = listFiles->row(item);
        if(row == (listFiles->count() - 1)) return;
        listFiles->takeItem(row);
        row = row + 1;
        listFiles->insertItem(row,item);
        listFiles->setCurrentItem(item);
    }
}
