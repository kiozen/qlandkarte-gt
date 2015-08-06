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

#include "CCreateMapGeoTiff.h"
#include "CCreateMapGridTool.h"
#include "CDlgProjWizzard.h"
#include "CMainWindow.h"
#include "CMapDB.h"
#include "GeoMath.h"

#include "config.h"
#include "CSettings.h"

#include <proj_api.h>
#ifdef __MINGW32__
#undef LP
#endif
#include <ogr_spatialref.h>
#include <gdal_priv.h>

#include <QtGui>
#include <QMessageBox>
#include <QFileDialog>
#include <QScrollBar>

CCreateMapGeoTiff * CCreateMapGeoTiff::m_self = 0;

CCreateMapGeoTiff::CCreateMapGeoTiff(QWidget * parent)
: QWidget(parent)
, refcnt(0)
, state(eNone)
, path("./")
, closemap(false)
{
    m_self = this;
    setupUi(this);

    toolReload->setIcon(QPixmap(":/icons/iconReload16x16.png"));
    toolOutFile->setIcon(QPixmap(":/icons/iconFileSave16x16.png"));

    helpStep1->setHelp(tr("Load Raster Map"),
        tr("This dialog allows you to georeference raster map files. As pre-requisite you need a set of reference points and the projection for those points. You will get best results if the projection of the points is also the projection of the map. In most cases this is mercator. It is recommended to shift the reference point to WGS84 datum, right from the beginning."));
    helpStep2->setHelp(tr("Add Reference Points"),
        trUtf8("The next stage is to add known reference points. Simply add reference points to the map and enter their latitude / longitude (WGS84) or the easting and northing [m] in the table. Next you move the point to the correct location on the map.\n\ncoordinate formats:\n\xe2\x80\xa2 \"N49\xc2\xb0 10.234 E12\xc2\xb0 01.456\" (dd mm.mmm)\n\xe2\x80\xa2 \"46.575377   12.193172\"  (dd.dddddd)\n\xe2\x80\xa2 \"285000 5162000\""));
    helpStep3->setHelp(tr("Reference Map"),
        tr("Now QLandkarte GT will reference your file with the help of the GDAL tools. Watch the progress in the output browser."));

    connect(pushOpenFile, SIGNAL(clicked()), this, SLOT(slotOpenFile()));
    connect(toolReload, SIGNAL(clicked()), this, SLOT(slotReload()));
    connect(toolOutFile, SIGNAL(clicked()), this, SLOT(slotOutFile()));
    connect(comboMode, SIGNAL(currentIndexChanged(int)), this, SLOT(slotModeChanged(int)));
    connect(pushAddRef, SIGNAL(clicked()), this, SLOT(slotAddRef()));
    connect(pushDelRef, SIGNAL(clicked()), this, SLOT(slotDelRef()));
    connect(pushLoadRef, SIGNAL(clicked()), this, SLOT(slotLoadRef()));
    connect(pushSaveRef, SIGNAL(clicked()), this, SLOT(slotSaveRef()));
    connect(pushGridTool, SIGNAL(clicked()), this, SLOT(slotGridTool()));
    connect(treeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(slotSelectionChanged()));
    connect(treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(slotItemDoubleClicked(QTreeWidgetItem*)));
    connect(treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(slotItemChanged(QTreeWidgetItem*, int)));
    connect(pushGoOn, SIGNAL(clicked()), this, SLOT(slotGoOn()));
    connect(&cmd, SIGNAL(readyReadStandardError()), this, SLOT(slotStderr()));
    connect(&cmd, SIGNAL(readyReadStandardOutput()), this, SLOT(slotStdout()));
    connect(&cmd, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(slotFinished(int,QProcess::ExitStatus)));
    connect(pushClearAll, SIGNAL(clicked()), this, SLOT(slotClearAll()));
    connect(toolProjWizard, SIGNAL(clicked()), this, SLOT(slotProjWizard()));
    connect(toolGCPProjWizard, SIGNAL(clicked()), this, SLOT(slotGCPProjWizard()));

    SETTINGS;
    lineMapProjection->setText(cfg.value("create/mapproj","+proj=merc +ellps=WGS84 +datum=WGS84 +no_defs").toString());
    lineGCPProjection->setText(cfg.value("create/gcpproj","+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs").toString());
    check2x->setChecked(cfg.value("create/overview/2", false).toBool());
    check4x->setChecked(cfg.value("create/overview/4", false).toBool());
    check8x->setChecked(cfg.value("create/overview/8", false).toBool());
    check16x->setChecked(cfg.value("create/overview/16", false).toBool());
    check32x->setChecked(cfg.value("create/overview/32", false).toBool());

    comboMode->addItem(tr("square pixels (2 Ref. Pts.)"), eSquare);
    comboMode->addItem(tr("linear (3 Ref. Pts.)"), eLinear);
    comboMode->addItem(tr("quadratic (6 Ref. Pts.)"), eQuadratic);
    comboMode->addItem(tr("thin plate (4 Ref. Pts.)"), eThinPlate);
    comboMode->setCurrentIndex(1);

    toolProjWizard->setIcon(QPixmap(":/icons/iconWizzard16x16.png"));
    toolGCPProjWizard->setIcon(QPixmap(":/icons/iconWizzard16x16.png"));
    pushOpenFile->setIcon(QPixmap(":/icons/iconFileLoad16x16.png"));

    theMainWindow->getCanvas()->setMouseMode(CCanvas::eMouseMoveRefPoint);
    theMainWindow->getCanvas()->installEventFilter(this);

    tabWidget->setTabEnabled(1, false);
    tabWidget->setTabEnabled(2, false);
    tabWidget->setCurrentIndex(0);
}


CCreateMapGeoTiff::~CCreateMapGeoTiff()
{
    if(closemap) CMapDB::self().closeMap();
    if(theMainWindow->getCanvas()) theMainWindow->getCanvas()->setMouseMode(CCanvas::eMouseMoveArea);
    m_self = 0;

    SETTINGS;
    cfg.setValue("create/overview/2", check2x->isChecked());
    cfg.setValue("create/overview/4", check4x->isChecked());
    cfg.setValue("create/overview/8", check8x->isChecked());
    cfg.setValue("create/overview/16", check16x->isChecked());
    cfg.setValue("create/overview/32", check32x->isChecked());

}


bool CCreateMapGeoTiff::eventFilter(QObject * watched, QEvent * event)
{
    if(watched == theMainWindow->getCanvas())
    {
        if(event->type() == QEvent::KeyPress)
        {
            int idx;
            QTreeWidgetItem * item;
            QKeyEvent * e = (QKeyEvent*)event;

            if(e->key() != Qt::Key_N && e->key() != Qt::Key_B)
            {
                return QWidget::eventFilter(watched, event);
            }

            item = treeWidget->currentItem();
            idx = treeWidget->indexOfTopLevelItem(item);
            if(idx < 0)
            {
                return QWidget::eventFilter(watched, event);
            }

            if(e->key() == Qt::Key_N)
            {
                idx++;
            }
            else
            {
                idx--;
            }

            if(idx < 0)
            {
                return QWidget::eventFilter(watched, event);
            }

            if(idx >= treeWidget->topLevelItemCount())
            {
                return QWidget::eventFilter(watched, event);
            }

            item = treeWidget->topLevelItem(idx);
            treeWidget->setCurrentItem(item);
            treeWidget->scrollToItem(item);
            slotItemDoubleClicked(item);

        }
    }

    return QWidget::eventFilter(watched, event);
}


void CCreateMapGeoTiff::selRefPointByKey(const quint32 key)
{
    if(refpts.contains(key))
    {
        refpt_t& pt = refpts[key];
        treeWidget->scrollToItem(pt.item);
        treeWidget->setCurrentItem(pt.item);
    }
}


int CCreateMapGeoTiff::getNumberOfGCPs()
{
    int n = 0;
    int mode = comboMode->itemData(comboMode->currentIndex()).toInt();
    switch(mode)
    {
        case 1:
            n = 3;
            break;
        case 2:
            n = 6;
            break;
        case -2:
            n = 2;
            break;
        case -1:
            n = 4;
            break;
    }
    return n;
}


void CCreateMapGeoTiff::enableStep2()
{
    tabWidget->setTabEnabled(1, true);
    toolOutFile->setEnabled(true);
    toolReload->setEnabled(true);
    treeWidget->setEnabled(true);
    helpStep2->setEnabled(true);
    pushAddRef->setEnabled(true);
    pushLoadRef->setEnabled(true);
    pushGridTool->setEnabled(true);

}


void CCreateMapGeoTiff::enableStep3(bool doEnable)
{
    tabWidget->setTabEnabled(2, doEnable);
    helpStep3->setEnabled(doEnable);
    pushGoOn->setEnabled(doEnable);
    textBrowser->setEnabled(doEnable);
    helpStep3->setEnabled(doEnable);
}


void CCreateMapGeoTiff::slotOpenFile()
{
    char str[1024];
    char * ptr = str;

    SETTINGS;
    path = QDir(cfg.value("path/create",path.path()).toString());

    QString filename = QFileDialog::getOpenFileName(0, tr("Open map file..."),path.path(), tr("Raw bitmaps (*.tif *.tiff *.png *.gif *.jpg)"), 0, FILE_DIALOG_FLAGS);
    if(filename.isEmpty()) return;

    CMapDB::self().openMap(filename, true, *theMainWindow->getCanvas());
    labelInputFile->setText(filename);

    QFileInfo fi(filename);
    path = QDir(fi.absolutePath());
    cfg.setValue("path/create",path.path());
    QString name = fi.baseName();

    labelOutputFile->setText(path.filePath(name + "_ref.tif"));

    theMainWindow->getCanvas()->move(CCanvas::eMoveCenter);

    GDALDataset * dataset = (GDALDataset*)GDALOpen(filename.toLocal8Bit(),GA_ReadOnly);
    if(dataset == 0) return;

    sizeMap = QSize(dataset->GetRasterXSize(), dataset->GetRasterYSize());
    rectSelArea = QRect(QPoint(0,0),sizeMap);

    QString proj = dataset->GetGCPProjection();
    if(!proj.isEmpty())
    {

        strncpy(str, dataset->GetGCPProjection(), sizeof(str));
        OGRSpatialReference oSRS;
        oSRS.importFromWkt(&ptr);
        oSRS.exportToProj4(&ptr);
        lineMapProjection->setText(ptr);

        gdalGCP2RefPt(dataset->GetGCPs(), dataset->GetGCPCount());
    }

    delete dataset;

    enableStep2();
}


void CCreateMapGeoTiff::slotOutFile()
{
    SETTINGS;
    path = QDir(cfg.value("path/create",path.path()).toString());

    QString filename = QFileDialog::getSaveFileName(0, tr("Save result as..."),path.filePath(labelOutputFile->text()), tr("GeoTiff (*.tif *.tiff)"), 0, FILE_DIALOG_FLAGS);
    if(filename.isEmpty()) return;

    labelOutputFile->setText(filename);

}


void CCreateMapGeoTiff::slotReload()
{
    char str[1024];
    char * ptr = str;

    QString filename = labelInputFile->text();
    CMapDB::self().openMap(filename, true, *theMainWindow->getCanvas());

    theMainWindow->getCanvas()->move(CCanvas::eMoveCenter);

    GDALDataset * dataset = (GDALDataset*)GDALOpen(filename.toLocal8Bit(),GA_ReadOnly);
    if(dataset == 0) return;

    rectSelArea     = QRect(0,0,dataset->GetRasterXSize(), dataset->GetRasterYSize());

    QString proj = dataset->GetGCPProjection();
    if(!proj.isEmpty())
    {
        strncpy(str, dataset->GetGCPProjection(), sizeof(str));
        OGRSpatialReference oSRS;
        oSRS.importFromWkt(&ptr);
        oSRS.exportToProj4(&ptr);
        lineMapProjection->setText(ptr);

        gdalGCP2RefPt(dataset->GetGCPs(), dataset->GetGCPCount());
    }
    delete dataset;

}


void CCreateMapGeoTiff::slotModeChanged(int)
{
    enableStep3(treeWidget->topLevelItemCount() >= getNumberOfGCPs());
}


void CCreateMapGeoTiff::slotAddRef()
{
    refpt_t& pt     = refpts[++refcnt];

    QPoint center   = theMainWindow->getCanvas()->rect().center();
    IMap& map       = CMapDB::self().getMap();
    pt.x            = center.x();
    pt.y            = center.y();
    map.convertPt2M(pt.x,pt.y);

    pt.item         = new QTreeWidgetItem();
    pt.item->setFlags(Qt::ItemIsEditable|Qt::ItemIsEnabled|Qt::ItemIsSelectable);
    pt.item->setData(eLabel,Qt::UserRole,refcnt);
    pt.item->setText(eLabel,tr("Ref %1").arg(refcnt));
    pt.item->setText(eLonLat,tr("<enter coord>"));
    pt.item->setText(eX,QString::number(pt.x));
    pt.item->setText(eY,QString::number(pt.y));

    treeWidget->addTopLevelItem(pt.item);

#ifdef QK_QT5_PORT
    treeWidget->header()->setSectionResizeMode(0,QHeaderView::Interactive);
#else
    treeWidget->header()->setResizeMode(0,QHeaderView::Interactive);
#endif
    for(int i=0; i < eMaxColumn - 1; ++i)
    {
        treeWidget->resizeColumnToContents(i);
    }

    enableStep3(treeWidget->topLevelItemCount() >= getNumberOfGCPs());
    pushSaveRef->setEnabled(treeWidget->topLevelItemCount() > 0);

    theMainWindow->getCanvas()->update();
}


void CCreateMapGeoTiff::addRef(double x, double y, double u, double v)
{
    refpt_t& pt     = refpts[++refcnt];
    pt.x            = x;
    pt.y            = y;
    pt.item         = new QTreeWidgetItem();
    pt.item->setFlags(Qt::ItemIsEditable|Qt::ItemIsEnabled|Qt::ItemIsSelectable);
    pt.item->setData(eLabel,Qt::UserRole,refcnt);
    pt.item->setText(eLabel,tr("Ref %1").arg(refcnt));
    pt.item->setText(eLonLat,tr(""));
    pt.item->setText(eX,QString::number(pt.x));
    pt.item->setText(eY,QString::number(pt.y));
    pt.item->setText(eLonLat,QString("%1 %2").arg(v,0,'f',6).arg(u,0,'f',6));

    treeWidget->addTopLevelItem(pt.item);

#ifdef QK_QT5_PORT
    treeWidget->header()->setSectionResizeMode(0,QHeaderView::Interactive);
#else
    treeWidget->header()->setResizeMode(0,QHeaderView::Interactive);
#endif
    for(int i=0; i < eMaxColumn - 1; ++i)
    {
        treeWidget->resizeColumnToContents(i);
    }

    enableStep3(treeWidget->topLevelItemCount() >= getNumberOfGCPs());
    pushSaveRef->setEnabled(treeWidget->topLevelItemCount() > 0);
}


void CCreateMapGeoTiff::keyPressEvent(QKeyEvent * e)
{
    if(e->key() == Qt::Key_Delete)
    {
        slotDelRef();
        e->accept();
    }
    else
    {
        QWidget::keyPressEvent(e);
    }
}


void CCreateMapGeoTiff::slotDelRef()
{
    QList<QTreeWidgetItem*> items = treeWidget->selectedItems();
    QTreeWidgetItem * item;

    foreach(item, items)
    {
        refpts.remove(item->data(eLabel,Qt::UserRole).toUInt());
        delete item;
    }
    enableStep3(treeWidget->topLevelItemCount() >= getNumberOfGCPs());
    pushSaveRef->setEnabled(treeWidget->topLevelItemCount() > 0);

    theMainWindow->getCanvas()->update();
}


void CCreateMapGeoTiff::slotLoadRef()
{

    QString filename = QFileDialog::getOpenFileName(0, tr("Load reference points..."),path.path(),"Ref. points (*.gcp *.tab)", 0, FILE_DIALOG_FLAGS);
    if(filename.isEmpty()) return;

    QFileInfo fi(filename);
    if(fi.suffix() == "gcp")
    {
        loadGCP(filename);
    }
    else if(fi.suffix() == "tab")
    {
        loadTAB(filename);
    }

#ifdef QK_QT5_PORT
    treeWidget->header()->setSectionResizeMode(0,QHeaderView::Interactive);
#else
    treeWidget->header()->setResizeMode(0,QHeaderView::Interactive);
#endif
    for(int i=0; i < eMaxColumn - 1; ++i)
    {
        treeWidget->resizeColumnToContents(i);
    }

    enableStep3(treeWidget->topLevelItemCount() >= getNumberOfGCPs());
    pushSaveRef->setEnabled(treeWidget->topLevelItemCount() > 0);

    theMainWindow->getCanvas()->update();

}


void CCreateMapGeoTiff::loadGCP(const QString& filename)
{

    QString gcpproj("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");

    QFile file(filename);
    file.open(QIODevice::ReadOnly);
    QString line = QString::fromUtf8(file.readLine());
    if(line.trimmed() == "#V1.0")
    {
        QRegExp re1("^-gcp\\s(-{0,1}[0-9]+)\\s(-{0,1}[0-9]+)\\s(.*)$");
        QRegExp re2("^-a_srs\\s(.*)$");
        QRegExp re3("^#gcpproj:\\s(.*)$");

        while(1)
        {
            if(re1.exactMatch(line))
            {
                refpt_t& pt     = refpts[++refcnt];
                pt.item         = new QTreeWidgetItem();

                pt.item->setFlags(Qt::ItemIsEditable|Qt::ItemIsEnabled|Qt::ItemIsSelectable);
                pt.item->setData(eLabel,Qt::UserRole,refcnt);
                pt.item->setText(eLabel,tr("Ref %1").arg(refcnt));

                pt.x = re1.cap(1).toDouble();
                pt.y = re1.cap(2).toDouble();
                pt.item->setText(eX,QString::number(pt.x));
                pt.item->setText(eY,QString::number(pt.y));
                pt.item->setText(eLonLat,re1.cap(3).trimmed());

                treeWidget->addTopLevelItem(pt.item);
            }
            else if(re2.exactMatch(line))
            {
                lineMapProjection->setText(re2.cap(1).trimmed());
            }
            else if(re3.exactMatch(line))
            {
                lineGCPProjection->setText(re3.cap(1).trimmed());
            }

            if (file.atEnd()) break;
            line = QString::fromUtf8(file.readLine());
        }
    }
    else
    {
        QRegExp re1("^-gcp\\s(.*)\\s(-{0,1}[0-9]+)\\s(-{0,1}[0-9]+)\\s(.*)$");
        QRegExp re2("^-proj\\s(.*)$");

        while(1)
        {
            if(re1.exactMatch(line))
            {
                refpt_t& pt     = refpts[++refcnt];
                pt.item         = new QTreeWidgetItem();

                pt.item->setFlags(Qt::ItemIsEditable|Qt::ItemIsEnabled|Qt::ItemIsSelectable);
                pt.item->setData(eLabel,Qt::UserRole,refcnt);
                QString label = re1.cap(4).trimmed();
                if(label.isEmpty())
                {
                    pt.item->setText(eLabel,tr("Ref %1").arg(refcnt));
                }
                else
                {
                    pt.item->setText(eLabel,label);
                }
                pt.item->setText(eLonLat,re1.cap(1));

                pt.x = re1.cap(2).toDouble();
                pt.y = re1.cap(3).toDouble();
                pt.item->setText(eX,QString::number(pt.x));
                pt.item->setText(eY,QString::number(pt.y));

                treeWidget->addTopLevelItem(pt.item);
            }
            else if(re2.exactMatch(line))
            {
                lineMapProjection->setText(re2.cap(1).trimmed());
            }

            if (file.atEnd()) break;
            line = QString::fromUtf8(file.readLine());
        }
    }

    file.close();
}


void CCreateMapGeoTiff::loadTAB(const QString& filename)
{
    char *pszTabWKT = 0;
    GDAL_GCP *gcpm;
    int n;

    double adfGeoTransform[6];
    adfGeoTransform[0] = 0.0;
    adfGeoTransform[1] = 1.0;
    adfGeoTransform[2] = 0.0;
    adfGeoTransform[3] = 0.0;
    adfGeoTransform[4] = 0.0;
    adfGeoTransform[5] = 1.0;

    if(GDALReadTabFile(filename.toLocal8Bit(),adfGeoTransform,&pszTabWKT,&n,&gcpm))
    {

        gdalGCP2RefPt(gcpm,n);

        char * ptr = 0;
        OGRSpatialReference oSRS;
        oSRS.importFromWkt(&pszTabWKT);
        oSRS.exportToProj4(&ptr);

        lineMapProjection->setText(ptr);
    }
}


void CCreateMapGeoTiff::gdalGCP2RefPt(const GDAL_GCP* gcps, int n)
{
    int i;
    if (n) for(i=0;i<n;i++)
    {
        refpt_t& pt     = refpts[++refcnt];
        pt.item         = new QTreeWidgetItem();

        pt.item->setFlags(Qt::ItemIsEditable|Qt::ItemIsEnabled|Qt::ItemIsSelectable);

        pt.item->setData(eLabel,Qt::UserRole,refcnt);
        QString label = gcps[i].pszInfo;
        if(label.isEmpty())
        {
            pt.item->setText(eLabel,tr("Ref %1").arg(refcnt));
        }
        else
        {
            pt.item->setText(eLabel,label);
        }
        pt.item->setText(eLonLat,QString("%1 %2").arg(gcps[i].dfGCPX,0,'f',6).arg(gcps[i].dfGCPY,0,'f',6));

        pt.x = gcps[i].dfGCPPixel;
        pt.y = gcps[i].dfGCPLine;
        pt.item->setText(eX,QString::number(pt.x,'f',6));
        pt.item->setText(eY,QString::number(pt.y,'f',6));

        treeWidget->addTopLevelItem(pt.item);
    }
}


void CCreateMapGeoTiff::slotSaveRef()
{
    SETTINGS;
    QString filter = cfg.value("create/filter.out","Ref. points (*.gcp)").toString();

    QFileInfo fin(labelInputFile->text());
    QString base = fin.baseName();

    QString filename = QFileDialog::getSaveFileName(0, tr("Save reference points..."),path.filePath(base),"Ref. points (*.gcp);;Mapinfo (*.tab)", &filter, FILE_DIALOG_FLAGS);
    if(filename.isEmpty()) return;

    QFileInfo fi(filename);

    if((filter == "Ref. points (*.gcp)") && (fi.suffix() != "gcp"))
    {
        filename += ".gcp";
    }
    else if((filter == "Mapinfo (*.tab)") && (fi.suffix() != "tab"))
    {
        filename += ".tab";
    }

    if(filter == "Ref. points (*.gcp)")
    {
        saveGCP(filename);
    }
    else if(filter == "Mapinfo (*.tab)")
    {
        saveTAB(filename);
    }

    cfg.setValue("create/filter.out",filter);

}


void CCreateMapGeoTiff::saveGCP(const QString& filename)
{
    QFile file(filename);
    file.open(QIODevice::WriteOnly);

    file.write(QString("#V1.0\n").toUtf8());

    QString gcpproj = lineGCPProjection->text().trimmed();
    if(!gcpproj.isEmpty())
    {
        file.write(QString("#gcpproj: %1\n").arg(gcpproj).toUtf8());
    }

    QString mapproj = lineMapProjection->text().trimmed();
    if(mapproj.isEmpty())
    {
        mapproj = "+proj=merc +ellps=WGS84 +datum=WGS84 +no_defs";
    }

    QStringList args;
    args << "-a_srs";
    args << mapproj;
    args << "\n";
    file.write(args.join(" ").toUtf8());

    QMap<quint32,refpt_t>::iterator refpt = refpts.begin();
    while(refpt != refpts.end())
    {
        args.clear();
        args << "-gcp";
        args << QString::number(refpt->x,'f',0);
        args << QString::number(refpt->y,'f',0);
        args << refpt->item->text(eLonLat);
        args << "\n";
        file.write(args.join(" ").toUtf8());
        ++refpt;
    }
    file.close();
}


void CCreateMapGeoTiff::saveTAB(const QString& filename)
{
    QMessageBox::information(0,tr("Sorry..."),tr("No Mapinfo TAB file support yet."), QMessageBox::Abort, QMessageBox::Abort);
}


void CCreateMapGeoTiff::slotGridTool()
{
    rectSelArea = QRect(QPoint(0,0),sizeMap);
    CCreateMapGridTool * tool = new CCreateMapGridTool(this, parentWidget());
    theMainWindow->setTempWidget(tool, tr("Grid Tool"));
}


void CCreateMapGeoTiff::slotSelectionChanged()
{
    pushDelRef->setEnabled(treeWidget->currentItem());
}


void CCreateMapGeoTiff::slotItemDoubleClicked(QTreeWidgetItem * item)
{
    IMap& map   = CMapDB::self().getMap();
    refpt_t& pt = refpts[item->data(eLabel,Qt::UserRole).toUInt()];
    double x = pt.x;
    double y = pt.y;
    map.convertM2Pt(x,y);

    map.move(QPoint(x,y), theMainWindow->getCanvas()->rect().center());
    theMainWindow->getCanvas()->update();
}


void CCreateMapGeoTiff::slotItemChanged(QTreeWidgetItem * item, int column)
{
    refpt_t& pt = refpts[item->data(eLabel,Qt::UserRole).toUInt()];
    pt.x = item->text(eX).toDouble();
    pt.y = item->text(eY).toDouble();

    theMainWindow->getCanvas()->update();
}


void CCreateMapGeoTiff::slotGoOn()
{
    QStringList args;
    bool islonlat = false;

    tabWidget->setTabEnabled(2,true);
    tabWidget->setCurrentIndex(2);

    // get / store target projection
    QString gcpproj = lineGCPProjection->text();
    QString mapproj = lineMapProjection->text();
    if(mapproj.isEmpty())
    {
        mapproj = "+proj=merc +ellps=WGS84 +datum=WGS84 +no_defs";
    }
    islonlat = mapproj.contains("longlat");

    SETTINGS;
    cfg.setValue("create/mapproj",mapproj);

    args << "-a_srs" << mapproj;

    // add gcps
    double x1, x2, y1, y2, u1, u2, v1, v2;
    x1 = x2 = y1 = y2 = u1 = u2 = v1 = v2 = 0;

    QMap<quint32,refpt_t>::iterator refpt = refpts.begin();
    while(refpt != refpts.end())
    {
        float lon = 0, lat = 0;
        args << "-gcp";

        x1 = x2; x2 = refpt->x;
        y1 = y2; y2 = refpt->y;

        args << QString::number(refpt->x,'f',0);
        args << QString::number(refpt->y,'f',0);
        if(islonlat)
        {
            if(!GPS_Math_Str_To_Deg(refpt->item->text(eLonLat), lon, lat))
            {
                return;
            }
            //            qDebug() << "islonlat" << lon << lat;
        }
        else
        {
            if(!GPS_Math_Str_To_LongLat(refpt->item->text(eLonLat), lon, lat, gcpproj, mapproj))
            {
                return;
            }

            //            qDebug() << "!islonlat" << lon << lat;
        }

        u1 = u2; u2 = lon;
        v1 = v2; v2 = lat;

        args << QString::number(lon,'f',6);
        args << QString::number(lat,'f',6);
        ++refpt;
    }

    // as gdalwarp needs 3 GCPs at least we add an artificial one on two GCPs
                                 /* use square pixels */
    if ( treeWidget->topLevelItemCount() == 2 )
    {
        double adfGeoTransform[6];
        double dx, dy, dX, dY, delta;
        double a, b;

        dx =   x2 - x1;
        dy = -(y2 - y1);

        dX =   u2 - u1;
        dY =   v2 - v1;

        delta = dx * dx + dy * dy;

        if (delta < 1 )          /* pixel */
        {
            QMessageBox::warning(0,tr("Error ..."), tr("Reference points are too close."), QMessageBox::Abort, QMessageBox::Abort);
            return;
        }

        a = (dX * dx + dY * dy) / delta;
        b = (dY * dx - dX * dy) / delta;

        adfGeoTransform[0] = u2 - a * x2 - b * y2;
        adfGeoTransform[1] =  a;
        adfGeoTransform[2] = -b;
        adfGeoTransform[3] = v2 - b * x2 + a * y2;
        adfGeoTransform[4] =  b;
        adfGeoTransform[5] =  a;

        /* -gcp 0 0 x0 y0. Image size is not known here?. Let's hope that GCP ! = 0,0 */
        args << "-gcp";
        args << QString::number(0.,'f',6);
        args << QString::number(0.,'f',6);
        args << QString::number(adfGeoTransform[0],'f',6);
        args << QString::number(adfGeoTransform[3],'f',6);
    }

    tmpfile1 = new QTemporaryFile();
    tmpfile1->open();
    args << labelInputFile->text() << tmpfile1->fileName();

    textBrowser->clear();
    textBrowser->setTextColor(Qt::black);
    textBrowser->append(GDALTRANSLATE " " +  args.join(" ") + "\n");

    state = eTranslate;
    cmd.start(GDALTRANSLATE, args);

    enableStep3(true);
    tabWidget->setCurrentIndex(2);
}


void CCreateMapGeoTiff::slotStderr()
{
    textBrowser->setTextColor(Qt::red);
    textBrowser->append(cmd.readAllStandardError());
}


void CCreateMapGeoTiff::slotStdout()
{
    textBrowser->setTextColor(Qt::blue);
    QString str = cmd.readAllStandardOutput();

    textBrowser->insertPlainText(str);
    textBrowser->verticalScrollBar()->setValue(textBrowser->verticalScrollBar()->maximum());
}


void CCreateMapGeoTiff::slotFinished( int exitCode, QProcess::ExitStatus status)
{
    if(exitCode != 0)
    {
        textBrowser->append(tr("Failed!\n"));
        cleanupTmpFiles();
        return;
    }

    if(state == eTranslate)
    {
        IMap& map = CMapDB::self().getMap();
        state = eWarp;
        QStringList args;

        int mode = comboMode->itemData(comboMode->currentIndex()).toInt();
        if(mode > 0)
        {
            args << "-order" << QString::number(mode);
        }
        else if(mode == eThinPlate)
        {
            args << "-tps";
        }

        if(map.is32BitRgb())
        {
            args << "-r" << "cubic";
        }

        args << "-dstnodata" << "\"255\"";
        args << tmpfile1->fileName();

        tmpfile2 = new QTemporaryFile();
        tmpfile2->open();

        args << tmpfile2->fileName();

        textBrowser->setTextColor(Qt::black);
        textBrowser->append(GDALWARP " " +  args.join(" ") + "\n");

        cmd.start(GDALWARP, args);
        return;
    }
    if(state == eWarp)
    {
        state = eTile;

        QFile::remove(labelOutputFile->text());

        QStringList args;
        args << "-co" << "tiled=yes";
        args << "-co" << "blockxsize=256";
        args << "-co" << "blockysize=256";
        args << "-co" << "compress=deflate";
        args << tmpfile2->fileName();
        args << labelOutputFile->text();

        textBrowser->setTextColor(Qt::black);
        textBrowser->append(GDALTRANSLATE " " +  args.join(" ") + "\n");

        cmd.start(GDALTRANSLATE, args);
        return;
    }
    if(state == eTile)
    {
        IMap& map = CMapDB::self().getMap();
        state = eOverview;

        QStringList args;
        if(map.is32BitRgb())
        {
            args << "-r" << "cubic";
        }
        else
        {
            args << "-r" << "average";
        }
        args << "--config" << "COMPRESS_OVERVIEW" << "JPEG";
        args << labelOutputFile->text();

        if(check2x->isChecked())
        {
            args << "2";
        }
        if(check4x->isChecked())
        {
            args << "4";
        }
        if(check8x->isChecked())
        {
            args << "8";
        }
        if(check16x->isChecked())
        {
            args << "16";
        }
        if(check32x->isChecked())
        {
            args << "32";
        }

        if(args.count() > 6)
        {

            textBrowser->setTextColor(Qt::black);
            textBrowser->append(GDALADDO " " +  args.join(" ") + "\n");

            cmd.start(GDALADDO, args);
            return;
        }

    }
    cleanupTmpFiles();

    textBrowser->setTextColor(Qt::black);
    textBrowser->append(tr("--- finished ---\n"));

}


void CCreateMapGeoTiff::cleanupTmpFiles()
{
    delete tmpfile1;
    delete tmpfile2;
}


void CCreateMapGeoTiff::slotClearAll()
{
    CMapDB::self().closeMap();

    refpts.clear();
    refcnt      = 0;
    state       = eNone;
    closemap    = false;

    treeWidget->clear();
    textBrowser->clear();

    labelInputFile->clear();
    labelOutputFile->clear();

    toolOutFile->setEnabled(false);
    toolReload->setEnabled(false);
    treeWidget->setEnabled(false);
    helpStep2->setEnabled(false);
    pushAddRef->setEnabled(false);
    pushLoadRef->setEnabled(false);
    pushSaveRef->setEnabled(false);
    pushGridTool->setEnabled(false);
    pushDelRef->setEnabled(false);
    pushGoOn->setEnabled(false);

    textBrowser->setEnabled(false);
    helpStep3->setEnabled(false);
    tabWidget->setTabEnabled(1, false);
    enableStep3(false);
    tabWidget->setCurrentIndex(0);

    theMainWindow->getCanvas()->update();
}


void CCreateMapGeoTiff::slotProjWizard()
{
    CDlgProjWizzard dlg(*lineMapProjection, this);
    dlg.exec();
}


void CCreateMapGeoTiff::slotGCPProjWizard()
{
    CDlgProjWizzard dlg(*lineGCPProjection, this);
    dlg.exec();
}
