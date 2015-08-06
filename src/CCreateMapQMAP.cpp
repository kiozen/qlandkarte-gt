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

#include "CCreateMapQMAP.h"
#include "CDlgEditMapLevel.h"
#include "CMapFile.h"
#include "GeoMath.h"
#include "CMainWindow.h"
#include "CMapDB.h"
#include "CSettings.h"

#include "config.h"

#include <QtGui>
#include <QFileDialog>
#include <QMessageBox>

CCreateMapQMAP::CCreateMapQMAP(QWidget * parent)
: QWidget(parent)
, mapPath("./")
, width(0)
, height(0)
{
    setupUi(this);
    toolOpen->setIcon(QIcon(":/icons/iconOpenMap16x16.png"));
    toolNew->setIcon(QIcon(":/icons/iconNewMap16x16.png"));

    helpStep1->setHelp(tr("Define Map"),
        tr("You can edit or create QLandkarte GT map definition files (*.qmap). A map definition defines what files to show at a certain zoom level. The comment will be used to list the map collection as known map in the map tool view. You can choose between a linear or quadratic zoom level increment.\nOnce you created a map set you can attach DEM data to it via the context menu in the lefthand map tool view."));
    helpStep2->setHelp(tr("Add Maps"),
        tr("You can stack maps of different detail as layer. For each detail layer you can define the number of zoom levels. Several map files can be grouped into a detail layer. All map files in a layer must have the same projection and scale. You need at least one layer with one file."));

    SETTINGS;
    mapPath = cfg.value("path/maps",mapPath).toString();

    connect(toolOpen, SIGNAL(clicked()), this, SLOT(slotOpenMap()));
    connect(toolNew, SIGNAL(clicked()), this, SLOT(slotNewMap()));
    connect(treeLevels, SIGNAL(itemSelectionChanged()), this, SLOT(slotLevelSelectionChanged()));
    connect(pushAdd, SIGNAL(clicked()), this, SLOT(slotAdd()));
    connect(pushEdit, SIGNAL(clicked()), this, SLOT(slotEdit()));
    connect(pushDel, SIGNAL(clicked()), this, SLOT(slotDel()));
    connect(pushUp, SIGNAL(clicked()), this, SLOT(slotUp()));
    connect(pushDown, SIGNAL(clicked()), this, SLOT(slotDown()));
    connect(pushSave, SIGNAL(clicked()), this, SLOT(slotSaveMap()));

    tabWidget->setTabEnabled(1, false);
}


CCreateMapQMAP::~CCreateMapQMAP()
{
    SETTINGS;
    cfg.setValue("path/maps",mapPath);
}


void CCreateMapQMAP::slotOpenMap()
{
    QString filename = QFileDialog::getOpenFileName(0,tr("Select map definition file..."), mapPath,"QLandkarte map (*.qmap)", 0, FILE_DIALOG_FLAGS);

    if(filename.isEmpty()) return;
    mapPath = QFileInfo(filename).path();

    resetdlg();
    readqmap(filename);
}


void CCreateMapQMAP::slotNewMap()
{
    QString filename;

    filename = QFileDialog::getSaveFileName(0,tr("Define a map collection file..."), mapPath,"QLandkarte map (*.qmap)", 0, FILE_DIALOG_FLAGS);
    if(filename.isEmpty()) return;

    QFileInfo fi(filename);
    if(fi.suffix() != "qmap")
    {
        filename += ".qmap";
    }

    resetdlg();
    mapPath = QFileInfo(filename).path();
    labelCurrentMap->setText(filename);

    pushAdd->setEnabled(true);
    helpStep2->setEnabled(true);
    tabWidget->setTabEnabled(1, true);
}


void CCreateMapQMAP::slotSaveMap()
{
    QString filename = labelCurrentMap->text();
    if(filename.isEmpty())
    {
        filename = QFileDialog::getSaveFileName(0,tr("Define a map collection file..."), mapPath,"QLandkarte map (*.qmap)", 0, FILE_DIALOG_FLAGS);
        if(filename.isEmpty()) return;
    }

    writeqmap(filename);

    CMapDB::self().openMap(filename, false, *theMainWindow->getCanvas());
}


void CCreateMapQMAP::resetdlg()
{
    labelCurrentMap->clear();
    lineComment->clear();
    treeLevels->clear();

    pushEdit->setEnabled(false);
    pushDel->setEnabled(false);
    pushUp->setEnabled(false);
    pushDown->setEnabled(false);
}


void CCreateMapQMAP::mapData2Item(QTreeWidgetItem *& item)
{
    QStringList files = item->text(eFiles).split("; ",QString::SkipEmptyParts);
    QString     file;
    QString     projection;
    double      north   =  -90.0 * DEG_TO_RAD;
    double      west    = -180.0 * DEG_TO_RAD;
    double      south   =   90.0 * DEG_TO_RAD;
    double      east    =  180.0 * DEG_TO_RAD;

    foreach(file, files)
    {
        CMapFile map(QDir(mapPath).filePath(file),this);
        if(!map.ok)
        {
            QMessageBox::critical(this,tr("Error..."), tr("Failed to load file %1.").arg(file), QMessageBox::Ok, QMessageBox::Ok);
            delete item;
            return;
        }

        if(!projection.isEmpty() && projection != map.strProj)
        {
            QMessageBox::critical(this,tr("Error..."), tr("All maps in a level must have the same projection."), QMessageBox::Ok, QMessageBox::Ok);
            delete item;
            return;
        }
        projection = map.strProj;

        if(north < map.lat1) north = map.lat1;
        if(south > map.lat2) south = map.lat2;
        if(west  < map.lon1) west  = map.lon1;
        if(east  > map.lon2) east  = map.lon2;
    }

    item->setData(0,eProjection, projection);
    item->setData(0,eNorth, north);
    item->setData(0,eWest, west);
    item->setData(0,eSouth, south);
    item->setData(0,eEast, east);

    double a1,a2;
    projXY p1, p2, p4;
    p1.u = west;
    p1.v = north;
    p2.u = east;
    p2.v = north;
    p4.u = west;
    p4.v = south;

    item->setData(0,eWidth,  ::distance(p1, p2, a1, a2));
    item->setData(0,eHeight, ::distance(p1, p4, a1, a2));
}


void CCreateMapQMAP::processLevelList()
{
    double north   =  -90.0 * DEG_TO_RAD;
    double west    = -180.0 * DEG_TO_RAD;
    double south   =   90.0 * DEG_TO_RAD;
    double east    =  180.0 * DEG_TO_RAD;

    int level = 1;
    int zoom = 0;
    QList<QTreeWidgetItem*> items = treeLevels->findItems(".*",Qt::MatchRegExp);
    QTreeWidgetItem * item;
    foreach(item,items)
    {
        if(item->data(0,eNorth).toDouble() > north) north = item->data(0,eNorth).toDouble();
        if(item->data(0,eSouth).toDouble() < south) south = item->data(0,eSouth).toDouble();
        if(item->data(0,eWest).toDouble() > west) west = item->data(0,eWest).toDouble();
        if(item->data(0,eEast).toDouble() < east) east = item->data(0,eEast).toDouble();

        zoom += 1;
        item->setText(eMinZoom,QString("%1").arg(zoom));
        zoom += item->data(0,eZoom).toInt() - 1;
        item->setText(eMaxZoom,QString("%1").arg(zoom));
        item->setText(eLevel,QString("%1").arg(level++));
    }

    QString str;
    GPS_Math_Deg_To_Str(west * RAD_TO_DEG, north * RAD_TO_DEG, topLeft);
    str += tr("Top/left corner:\t%1\n").arg(topLeft);
    GPS_Math_Deg_To_Str(east * RAD_TO_DEG, south * RAD_TO_DEG, bottomRight);
    str += tr("Bottom/right corner:\t%1\n").arg(bottomRight);

    double a1,a2;
    projXY p1, p2, p4;
    p1.u = west;
    p1.v = north;
    p2.u = east;
    p2.v = north;
    p4.u = west;
    p4.v = south;

    width   = ::distance(p1, p2, a1, a2);
    height  = ::distance(p1, p4, a1, a2);

    str += tr("Width x Height [m] x [m]:\t %1 x %2").arg(width).arg(height);

    labelSummary->setText(str);
}


void CCreateMapQMAP::readqmap(const QString& filename)
{
    QSettings mapdef(filename,QSettings::IniFormat);
    mapdef.setIniCodec(QTextCodec::codecForName("UTF-8"));

    labelCurrentMap->setText(filename);

    mapdef.beginGroup("description");
    lineComment->setText(mapdef.value("comment","").toString());
    mapdef.endGroup();           // description

    int levels  = mapdef.value("main/levels",0).toInt();

    for(int i = 1; i <= levels; ++i)
    {
        mapdef.beginGroup(QString("level%1").arg(i));
        QTreeWidgetItem * item = new QTreeWidgetItem(treeLevels);
        item->setText(eLevel,QString("%1").arg(i));
        item->setText(eFiles,mapdef.value("files","").toString().replace("|","; "));
        item->setData(0,eZoom,mapdef.value("zoomLevelMax",-1).toInt() - mapdef.value("zoomLevelMin",-1).toInt() + 1);
        mapData2Item(item);

        mapdef.endGroup();       // level%1
    }
    processLevelList();

    checkQuadraticZoom->setChecked(mapdef.value("main/quadraticZoom",false).toBool());

    pushAdd->setEnabled(true);
    pushSave->setEnabled(treeLevels->topLevelItemCount());
    helpStep2->setEnabled(true);
    tabWidget->setTabEnabled(1, true);
}


void CCreateMapQMAP::writeqmap(const QString& filename)
{
    QSettings mapdef(filename,QSettings::IniFormat);
    mapdef.setIniCodec(QTextCodec::codecForName("UTF-8"));
    mapdef.beginGroup("description");
    mapdef.setValue("bottomright",bottomRight);
    mapdef.setValue("topleft",topLeft);
    mapdef.setValue("width",width);
    mapdef.setValue("height",height);
    mapdef.setValue("comment",lineComment->text());
    mapdef.endGroup();           // description

    mapdef.beginGroup("home");
    mapdef.setValue("center",topLeft);
    mapdef.setValue("zoom",1);
    mapdef.endGroup();           // home

    mapdef.setValue("main/levels",treeLevels->topLevelItemCount());

    QList<QTreeWidgetItem*> items = treeLevels->findItems(".*",Qt::MatchRegExp);
    QTreeWidgetItem * item;
    foreach(item,items)
    {
        mapdef.beginGroup(QString("level%1").arg(item->text(eLevel).toInt()));
        mapdef.setValue("files",item->text(eFiles).replace("; ","|"));
        mapdef.setValue("zoomLevelMin", item->text(eMinZoom).toInt());
        mapdef.setValue("zoomLevelMax", item->text(eMaxZoom).toInt());

        mapdef.endGroup();       // level%i
    }
    mapdef.setValue("main/quadraticZoom",checkQuadraticZoom->isChecked());
}


void CCreateMapQMAP::slotLevelSelectionChanged()
{
    QTreeWidgetItem * item = treeLevels->currentItem();
    if(item)
    {
        pushEdit->setEnabled(true);
        pushDel->setEnabled(true);
        pushUp->setEnabled(true);
        pushDown->setEnabled(true);
    }
    else
    {
        pushEdit->setEnabled(false);
        pushDel->setEnabled(false);
        pushUp->setEnabled(false);
        pushDown->setEnabled(false);
    }
}


void CCreateMapQMAP::slotAdd()
{
    QTreeWidgetItem * item = new QTreeWidgetItem(treeLevels);
    item->setText(eLevel,QString("%1").arg(treeLevels->topLevelItemCount()));
    item->setData(0,eZoom,4);

    CDlgEditMapLevel dlg(item, mapPath, this);
    if(dlg.exec() == QDialog::Accepted)
    {
        mapData2Item(item);
        processLevelList();
    }
    else
    {
        delete item;
    }

    pushSave->setEnabled(treeLevels->topLevelItemCount());
}


void CCreateMapQMAP::slotEdit()
{
    QTreeWidgetItem * item = treeLevels->currentItem();
    if(item == 0) return;

    CDlgEditMapLevel dlg(item, mapPath, this);
    dlg.exec();
    mapData2Item(item);
    processLevelList();

}


void CCreateMapQMAP::slotDel()
{
    QTreeWidgetItem * item = treeLevels->currentItem();
    if(item == 0) return;
    delete item;

    processLevelList();
    pushSave->setEnabled(treeLevels->topLevelItemCount());
}


void CCreateMapQMAP::slotUp()
{
    int idx = 0;

    QTreeWidgetItem * item = treeLevels->currentItem();
    if(item == 0) return;

    idx = treeLevels->indexOfTopLevelItem(item);
    treeLevels->takeTopLevelItem(idx);
    idx = idx != 0 ? idx - 1 : idx;
    treeLevels->insertTopLevelItem(idx,item);

    treeLevels->setCurrentItem(item);
    processLevelList();
}


void CCreateMapQMAP::slotDown()
{
    int idx = 0;

    QTreeWidgetItem * item = treeLevels->currentItem();
    if(item == 0) return;

    idx = treeLevels->indexOfTopLevelItem(item);
    treeLevels->takeTopLevelItem(idx);
    idx = idx < treeLevels->topLevelItemCount() ? idx + 1 : idx;
    treeLevels->insertTopLevelItem(idx,item);

    treeLevels->setCurrentItem(item);
    processLevelList();
}
