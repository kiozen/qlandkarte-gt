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

#include "CMapToolWidget.h"
#include "CMapDB.h"
#include "CMainWindow.h"
#include "GeoMath.h"
#include "CMapQMAPExport.h"
#include "CMapSelectionRaster.h"
#include "CGarminExport.h"
#include "CMapSelectionGarmin.h"
#include "CDlgMapTmsConfig.h"
#include "CSettings.h"

#include "config.h"

#include <QtGui>
#include <QFileDialog>
#include <QMessageBox>
#include <QShortcut>
#include <QMenu>

CMapToolWidget::CMapToolWidget(QTabWidget * parent)
: QWidget(parent)
, path("./")
{
    setupUi(this);
    setObjectName("Maps");
    parent->addTab(this,QIcon(":/icons/iconMap16x16.png"),"");
    parent->setTabToolTip(parent->indexOf(this), tr("Maps"));

    connect(&CMapDB::self(), SIGNAL(sigChanged()), this, SLOT(slotDBChanged()));

    contextMenuKnownMaps = new QMenu(this);
    actReload = contextMenuKnownMaps->addAction(QPixmap(":/icons/iconReload16x16.png"),tr("Reload map..."),this,SLOT(slotReload()));
    actAddDEM = contextMenuKnownMaps->addAction(QPixmap(":/icons/iconDEM16x16.png"),tr("Add DEM..."),this,SLOT(slotAddDEM()));
    actDelDEM = contextMenuKnownMaps->addAction(QPixmap(":/icons/iconNoDEM16x16.png"),tr("Del. DEM..."),this,SLOT(slotDelDEM()));
    actCfgMap = contextMenuKnownMaps->addAction(QPixmap(":/icons/iconInfo16x16.png"),tr("Info/Config"),this,SLOT(slotCfgMap()));
    actDelMap = contextMenuKnownMaps->addAction(QPixmap(":/icons/iconClear16x16.png"),tr("Delete"),this,SLOT(slotDeleteKnownMap()));
    actAddTMS = contextMenuKnownMaps->addAction(QPixmap(":/icons/iconAdd16x16.png"),tr("Add TMS map..."),this,SLOT(slotAddTmsMap()));;

    connect(treeKnownMapsStream,SIGNAL(customContextMenuRequested(const QPoint&)),this,SLOT(slotContextMenuKnownMaps(const QPoint&)));
    connect(treeKnownMapsStream,SIGNAL(itemClicked(QTreeWidgetItem*, int)),this,SLOT(slotKnownMapClicked(QTreeWidgetItem*, int)));
    connect(treeKnownMapsStream,SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)),this,SLOT(slotKnownMapDoubleClicked(QTreeWidgetItem*, int)));

    connect(treeKnownMapsRaster,SIGNAL(customContextMenuRequested(const QPoint&)),this,SLOT(slotContextMenuKnownMaps(const QPoint&)));
    connect(treeKnownMapsRaster,SIGNAL(itemClicked(QTreeWidgetItem*, int)),this,SLOT(slotKnownMapClicked(QTreeWidgetItem*, int)));
    connect(treeKnownMapsRaster,SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)),this,SLOT(slotKnownMapDoubleClicked(QTreeWidgetItem*, int)));

    connect(treeKnownMapsVector,SIGNAL(customContextMenuRequested(const QPoint&)),this,SLOT(slotContextMenuKnownMaps(const QPoint&)));
    connect(treeKnownMapsVector,SIGNAL(itemClicked(QTreeWidgetItem*, int)),this,SLOT(slotKnownMapClicked(QTreeWidgetItem*, int)));
    connect(treeKnownMapsVector,SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)),this,SLOT(slotKnownMapDoubleClicked(QTreeWidgetItem*, int)));

    contextMenuSelectedMaps = new QMenu(this);
    contextMenuSelectedMaps->addAction(QPixmap(":/icons/iconFileSave16x16.png"),tr("Export"),this,SLOT(slotExportMap()));
    contextMenuSelectedMaps->addAction(QPixmap(":/icons/iconClear16x16.png"),tr("Delete"),this,SLOT(slotDeleteSelectedMap()));
    connect(listSelectedMaps,SIGNAL(customContextMenuRequested(const QPoint&)),this,SLOT(slotContextMenuSelectedMaps(const QPoint&)));
    connect(listSelectedMaps,SIGNAL(itemDoubleClicked(QListWidgetItem*)),this,SLOT(slotSelectedMapClicked(QListWidgetItem*)));
    connect(listSelectedMaps,SIGNAL(itemClicked(QListWidgetItem*)),this,SLOT(slotSelectMap(QListWidgetItem*)));

    connect(pushExportMap, SIGNAL(clicked()), this, SLOT(slotExportMap()));

    tabWidget->setTabIcon(eTabStream, QIcon(":/icons/iconStream22x22.png"));
    tabWidget->setTabText(eTabStream,tr("Stream"));
    tabWidget->setTabIcon(eTabRaster, QIcon(":/icons/iconRaster22x22.png"));
    tabWidget->setTabText(eTabRaster,tr("Raster"));
    tabWidget->setTabIcon(eTabVector, QIcon(":/icons/iconVector22x22.png"));
    tabWidget->setTabText(eTabVector,tr("Vector"));

}


CMapToolWidget::~CMapToolWidget()
{

}


void CMapToolWidget::slotDBChanged()
{
    IMap& basemap               = CMapDB::self().getMap();
    QString key                 = basemap.getKey();
    QTreeWidgetItem * selected  = 0;

    treeKnownMapsStream->clear();
    treeKnownMapsRaster->clear();
    treeKnownMapsVector->clear();

    const QMap<QString,CMapDB::map_t>& knownMaps = CMapDB::self().getKnownMaps();
    {
        QMap<QString,CMapDB::map_t>::const_iterator map = knownMaps.begin();
        while(map != knownMaps.end())
        {
            QTreeWidgetItem * item;
            if(map->type == IMap::eGarmin)
            {
                item = new QTreeWidgetItem(treeKnownMapsVector);
            }
            else if(map->type == IMap::eTMS)
            {
                item = new QTreeWidgetItem(treeKnownMapsStream);
            }
            else if(map->type == IMap::eWMS)
            {
                item = new QTreeWidgetItem(treeKnownMapsStream);
            }
            else
            {
                item = new QTreeWidgetItem(treeKnownMapsRaster);
            }

            item->setText(eName, map->description);
            item->setToolTip(eName, map->description);
            item->setData(eName, Qt::UserRole, map.key());

            QIcon icon = QIcon(":/icons/iconUnknown16x16.png");
            if(map->type == IMap::eRaster)
            {
                if(map->filename.toLower().endsWith("jnx"))
                {
                    icon = QIcon(":/icons/iconJNX22x22.png");
                    item->setToolTip(eType, tr("BirdsEye/JNX"));
                }
                else if(map->filename.toLower().endsWith("rmap"))
                {
                    icon = QIcon(":/icons/iconRMAP22x22.png");
                    item->setToolTip(eType, tr("TwoNav/RMAP"));
                }
                else if(map->filename.toLower().endsWith("rmp"))
                {
                    icon = QIcon(":/icons/iconRMP22x22.png");
                    item->setToolTip(eType, tr("Magellan/RMP"));
                }
                else
                {
                    icon = QIcon(":/icons/iconQMAP22x22.png");
                    item->setToolTip(eType, tr("map stack/QMAP"));
                }
            }
            else if(map->type == IMap::eGarmin)
            {
                icon = QIcon(":/icons/iconTDB22x22.png");
                item->setToolTip(eType, tr("Garmin/TDB/IMG"));
            }
            else if(map->type == IMap::eTMS)
            {
                icon = QIcon(":/icons/iconTMS22x22.png");
                item->setToolTip(eType, tr("tile server"));
            }
            else if(map->type == IMap::eWMS)
            {
                icon = QIcon(":/icons/iconWMS22x22.png");
                item->setToolTip(eType, tr("map server"));
            }
            else if(map->type == IMap::eNoMap)
            {
                icon = QIcon(":/icons/iconRaster22x22.png");
                item->setToolTip(eType, tr("various projections"));
            }

            item->setIcon(eType, icon);
            item->setData(eType, Qt::UserRole, map->type);

            if(map.key() == key)
            {
                selected = item;
                item->setIcon(eMode, QIcon(QIcon(":/icons/iconOk16x16.png")));
                item->setData(eMode, Qt::UserRole, eSelected);
                item->setToolTip(eMode, tr("selected map"));
            }
            else if(basemap.hasOverlayMap(map.key()))
            {
                item->setIcon(eMode, QIcon(QIcon(":/icons/iconOvlOk16x16.png")));
                item->setData(eMode, Qt::UserRole, eOverlayActive);
                item->setToolTip(eMode, tr("use a single click to deactivate map as overlay"));
            }
            else if(map->type == IMap::eGarmin)
            {
                item->setIcon(eMode, QIcon(QIcon(":/icons/iconOvl16x16.png")));
                item->setData(eMode, Qt::UserRole, eOverlay);
                item->setToolTip(eMode, tr("use a single click to activate map as overlay"));
            }
            else
            {
                item->setData(eMode, Qt::UserRole, eNoMode);
            }
            ++map;
        }
    }
    treeKnownMapsStream->sortItems(eName, Qt::AscendingOrder);
    treeKnownMapsVector->sortItems(eName, Qt::AscendingOrder);
    treeKnownMapsRaster->sortItems(eName, Qt::AscendingOrder);

    if(selected)
    {
        if(selected->data(eType, Qt::UserRole) == IMap::eGarmin)
        {
            treeKnownMapsVector->setCurrentItem(selected);
            tabWidget->setCurrentIndex(eTabVector);
        }
        else if(selected->data(eType, Qt::UserRole) == IMap::eTMS)
        {
            treeKnownMapsStream->setCurrentItem(selected);
            tabWidget->setCurrentIndex(eTabStream);
        }
        else if(selected->data(eType, Qt::UserRole) == IMap::eWMS)
        {
            treeKnownMapsStream->setCurrentItem(selected);
            tabWidget->setCurrentIndex(eTabStream);
        }
        else
        {
            treeKnownMapsRaster->setCurrentItem(selected);
            tabWidget->setCurrentIndex(eTabRaster);
        }
    }

    // adjust column sizes to fit
#ifdef QK_QT5_PORT
    treeKnownMapsStream->header()->setSectionResizeMode(0,QHeaderView::Interactive);
#else
    treeKnownMapsStream->header()->setResizeMode(0,QHeaderView::Interactive);
#endif
    for(int i=0; i < eMaxColumn - 1; ++i)
    {
        treeKnownMapsStream->resizeColumnToContents(i);
    }
#ifdef QK_QT5_PORT
    treeKnownMapsVector->header()->setSectionResizeMode(0,QHeaderView::Interactive);
#else
    treeKnownMapsVector->header()->setResizeMode(0,QHeaderView::Interactive);
#endif
    for(int i=0; i < eMaxColumn - 1; ++i)
    {
        treeKnownMapsVector->resizeColumnToContents(i);
    }

#ifdef QK_QT5_PORT
    treeKnownMapsRaster->header()->setSectionResizeMode(0,QHeaderView::Interactive);
#else
    treeKnownMapsRaster->header()->setResizeMode(0,QHeaderView::Interactive);
#endif
    for(int i=0; i < eMaxColumn - 1; ++i)
    {
        treeKnownMapsRaster->resizeColumnToContents(i);
    }

    listSelectedMaps->clear();
    const QMap<QString,IMapSelection*>& selectedMaps = CMapDB::self().getSelectedMaps();
    {
        QListWidgetItem * selected = 0;
        QMap<QString,IMapSelection*>::const_iterator map = selectedMaps.begin();
        while(map != selectedMaps.end())
        {
            QListWidgetItem * item = new QListWidgetItem(listSelectedMaps);

            item->setText((*map)->getDescription());
            //item->setText((*map)->getName());
            item->setData(Qt::UserRole, (*map)->getKey());

            if(IMapSelection::focusedMap == (*map)->getKey()) selected = item;
            ++map;
        }

#if defined(Q_OS_MAC)
        listSelectedMaps->setCurrentRow(0);
#endif
        if(selected) listSelectedMaps->setCurrentItem(selected);
        updateExportButton();
    }

}


void CMapToolWidget::slotKnownMapDoubleClicked(QTreeWidgetItem* item, int)
{
    QString key = item->data(eName, Qt::UserRole).toString();
    CMapDB::self().openMap(key);
}


void CMapToolWidget::slotKnownMapClicked(QTreeWidgetItem* item, int c)
{
    if(c == eMode)
    {
        QString key = item->data(eName, Qt::UserRole).toString();

        if(item->data(eMode, Qt::UserRole).toInt() == eOverlay)
        {
            CMapDB::self().getMap().addOverlayMap(key);
        }
        else if(item->data(eMode, Qt::UserRole).toInt() == eOverlayActive)
        {
            CMapDB::self().getMap().delOverlayMap(key);
        }

        emit sigChanged();
    }
}


void CMapToolWidget::slotSelectedMapClicked(QListWidgetItem* item)
{
    QString key = item->data(Qt::UserRole).toString();

    const QMap<QString,IMapSelection*>& selectedMaps = CMapDB::self().getSelectedMaps();
    if(selectedMaps.contains(key))
    {
        const IMapSelection * ms = selectedMaps[key];

        CMapDB::self().getMap().zoom(ms->lon1, ms->lat1, ms->lon2, ms->lat2);
        CMapDB::self().selSelectedMap(key);
    }

}


void CMapToolWidget::slotContextMenuKnownMaps(const QPoint& pos)
{
    QTreeWidgetItem * item      = 0;
    if(sender() == treeKnownMapsStream)
    {
        item = treeKnownMapsStream->currentItem();
        lastTreeWidget = treeKnownMapsStream;
    }
    else if(sender() == treeKnownMapsRaster)
    {
        item = treeKnownMapsRaster->currentItem();
        lastTreeWidget = treeKnownMapsRaster;
    }
    else if(sender() == treeKnownMapsVector)
    {
        item = treeKnownMapsVector->currentItem();
        lastTreeWidget = treeKnownMapsVector;
    }

    if(item)
    {
        IMap& dem       = CMapDB::self().getDEM();
        QString key     = item->data(eName, Qt::UserRole).toString();
        bool isBuiltIn  = CMapDB::self().isBuiltIn(key);

        IMap::maptype_e type = CMapDB::self().getMapData(key).type;

        if(item->data(eMode, Qt::UserRole).toInt() == eSelected)
        {
            actAddDEM->setEnabled(true);
            actDelDEM->setEnabled(dem.maptype == IMap::eDEM);
            actCfgMap->setEnabled(isBuiltIn && (type != IMap::eNoMap) ? false : true);
        }
        else
        {
            actAddDEM->setEnabled(false);
            actDelDEM->setEnabled(false);
            actCfgMap->setEnabled(false);
        }

        actDelMap->setEnabled(!isBuiltIn);
        actAddTMS->setVisible(lastTreeWidget == treeKnownMapsStream);

        QPoint p = lastTreeWidget->mapToGlobal(pos);
        contextMenuKnownMaps->exec(p);
    }
    else if(lastTreeWidget == treeKnownMapsStream)
    {
        actAddDEM->setEnabled(false);
        actDelDEM->setEnabled(false);
        actCfgMap->setEnabled(false);
        actDelMap->setEnabled(false);
        actAddTMS->setVisible(true);
        QPoint p = lastTreeWidget->mapToGlobal(pos);
        contextMenuKnownMaps->exec(p);
    }
}


void CMapToolWidget::slotContextMenuSelectedMaps(const QPoint& pos)
{
    if(listSelectedMaps->currentItem())
    {
        QPoint p = listSelectedMaps->mapToGlobal(pos);
        contextMenuSelectedMaps->exec(p);
    }
}


void CMapToolWidget::slotDeleteKnownMap()
{
    QStringList keys;
    QTreeWidgetItem * item;

    bool wasSelected = false;

    if(lastTreeWidget)
    {
        const QList<QTreeWidgetItem*>& items = lastTreeWidget->selectedItems();
        foreach(item,items)
        {
            if(item->data(eMode, Qt::UserRole).toInt() == eSelected)
            {
                wasSelected = true;
            }
            keys << item->data(eName, Qt::UserRole).toString();
        }
        CMapDB::self().delKnownMap(keys);
    }

    if(wasSelected)
    {
        CMapDB::self().openMap("NoMap");
    }
}


void CMapToolWidget::slotDeleteSelectedMap()
{
    QStringList keys;
    QListWidgetItem * item;
    const QList<QListWidgetItem*>& items = listSelectedMaps->selectedItems();
    foreach(item,items)
    {
        keys << item->data(Qt::UserRole).toString();
    }
    CMapDB::self().delSelectedMap(keys);

    updateExportButton();
}


void CMapToolWidget::slotSelectMap(QListWidgetItem* item)
{
    const QMap<QString,IMapSelection*>& selectedMaps = CMapDB::self().getSelectedMaps();
    QString key = item->data(Qt::UserRole).toString();
    if(selectedMaps.contains(key))
    {
        IMapSelection::focusedMap = key;
        theMainWindow->getCanvas()->update();
    }
    updateExportButton();
}


void CMapToolWidget::updateExportButton()
{
    pushExportMap->setEnabled(listSelectedMaps->currentItem() != 0);
}


void CMapToolWidget::slotExportMap()
{

    QListWidgetItem * item = listSelectedMaps->currentItem();
    if(item == 0) return;

    QString key = item->data(Qt::UserRole).toString();
    if(!CMapDB::self().getSelectedMaps().contains(key)) return;

    const QMap<QString,IMapSelection*>& selectedMaps = CMapDB::self().getSelectedMaps();
    const IMapSelection * ms = selectedMaps[key];
    if(ms->type == IMapSelection::eRaster)
    {
        QProcess proc1;
        proc1.start(GDALWARP " --version");
        proc1.waitForFinished();
        qDebug() << proc1.exitCode() << proc1.error() << proc1.errorString();
        bool haveGDALWarp = proc1.error() == QProcess::UnknownError;

        proc1.start(GDALTRANSLATE " --version");
        proc1.waitForFinished();
        qDebug() << proc1.exitCode() << proc1.error() << proc1.errorString();
        bool haveGDALTranslate = proc1.error() == QProcess::UnknownError;

        bool haveGDAL = haveGDALWarp && haveGDALTranslate;

        qDebug() << haveGDALWarp << haveGDALTranslate << haveGDAL;

        if(!haveGDAL)
        {
            QMessageBox::critical(0,tr("Error export maps..."), tr("You need to have the GDAL toolchain installed in your path."), QMessageBox::Abort, QMessageBox::Abort);
            return;
        }

        CMapQMAPExport dlg((CMapSelectionRaster&)*ms,this);
        dlg.exec();
    }
    if(ms->type == IMapSelection::eVector)
    {
        CGarminExport dlg(this);
        dlg.exportToFile((CMapSelectionGarmin&)*ms);
    }
}


void CMapToolWidget::slotAddDEM()
{
    SETTINGS;
    path = QDir(cfg.value("path/DEM",path.path()).toString());

    QString filename = QFileDialog::getOpenFileName(0, tr("Select DEM file..."),path.path(), tr("16bit Srtm Data (*.tif *.tiff *.hgt *.blx *.vrt)"), 0, FILE_DIALOG_FLAGS);
    if(filename.isEmpty()) return;

    QFileInfo fi(filename);
    path = QDir(fi.absolutePath());
    cfg.setValue("path/DEM",path.path());

    CMapDB::self().openDEM(filename);
}


void CMapToolWidget::slotDelDEM()
{
    IMap& dem = CMapDB::self().getDEM();
    if(dem.maptype == IMap::eDEM)
    {
        SETTINGS;
        cfg.setValue(QString("map/dem/%1").arg(CMapDB::self().getMap().getKey()), "");
        cfg.setValue(QString("map/dem/%1/ignoreWarning").arg(CMapDB::self().getMap().getKey()), false);
        dem.deleteLater();
    }
}


void CMapToolWidget::slotCfgMap()
{
    CMapDB::self().getMap().config();
}


void CMapToolWidget::slotAddTmsMap()
{
    CDlgMapTmsConfig dlg;
    dlg.exec();
}


void CMapToolWidget::slotReload()
{
    CMapDB::self().reloadMap();
}
