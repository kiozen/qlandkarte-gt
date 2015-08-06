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

#include "CTrackEditWidget.h"
#include "CTrackStatProfileWidget.h"
#include "CTrackStatSpeedWidget.h"
#include "CTrackStatDistanceWidget.h"
#include "CTrackStatTraineeWidget.h"
                                 //Anfgen des Ext. Widgets
#ifdef GPX_EXTENSIONS
#include "CTrackStatExtensionWidget.h"
#endif
#include "CTrack.h"
#include "CTrackDB.h"
#include "CResources.h"
#include "GeoMath.h"
#include "CMainWindow.h"
#include "CTabWidget.h"
#include "IUnit.h"
#include "CMenus.h"
#include "CActions.h"
#include "CWptDB.h"
#include "CSettings.h"
#include "CPlot.h"
#include "CDlgMultiColorConfig.h"

#include <QtGui>
#include <QMessageBox>
#include <QMenu>
#include <QDesktopWidget>

#ifndef QK_QT5_TZONE
#include <tzdata.h>
#endif

#ifdef Q_OS_MAC
#include <CoreFoundation/CoreFoundation.h>

template<class T>
class CFType
{
    public:
        CFType(const T & t) : ref(t) { }
        ~CFType() { CFRelease(ref); }
        operator T() const { return ref; }

    private:
        T ref;
};

static QString CFStringToQString(const CFStringRef & str)
{
    CFIndex length = CFStringGetLength(str);
    const UniChar * chars = CFStringGetCharactersPtr(str);
    if (chars == 0)
    {
        UniChar buffer[length];
        CFStringGetCharacters(str, CFRangeMake(0, length), buffer);
        return QString(reinterpret_cast<const QChar *>(buffer), length);
    }
    else
    {
        return QString(reinterpret_cast<const QChar *>(chars), length);
    }
}
#endif

bool CTrackTreeWidgetItem::operator< ( const QTreeWidgetItem & other ) const
{
    const QString speed("/h");
    const QRegExp distance("(ft|ml|m|km)");
    double d1 = 0, d2 = 0;

    int sortCol = treeWidget()->sortColumn();
    QString str1 = text(sortCol);
    QString str2 = other.text(sortCol);

    if (str1.contains(speed) && str2.contains(speed))
    {
        d1 = IUnit::self().str2speed(str1);
        d2 = IUnit::self().str2speed(str2);
    }
    else if (str1.contains(distance) && str2.contains(distance))
    {
        d1 = IUnit::self().str2distance(str1);
        d2 = IUnit::self().str2distance(str2);
    }
    else
    {
        /* let's assume it's a double without any unit ... */
        d1 = str1.toDouble();
        d2 = str2.toDouble();
    }

    return d1 < d2;
}


CTrackEditWidget::CTrackEditWidget(QWidget * parent)
: QWidget(parent)
, originator(false)
#ifdef GPX_EXTENSIONS
, Vspace(0)
, tabstat(0)
, no_ext_info_stat(0)
#endif
{
    setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose,true);

#ifndef GPX_EXTENSIONS
    tabWidget->removeTab(eSetup);
#endif

    toolGraphDistance->setIcon(QIcon(":/icons/iconGraph16x16.png"));
    connect(toolGraphDistance, SIGNAL(clicked()), this, SLOT(slotToggleStatDistance()));

    toolGraphTime->setIcon(QIcon(":/icons/iconTime16x16.png"));
    connect(toolGraphTime, SIGNAL(clicked()), this, SLOT(slotToggleStatTime()));

    traineeGraph->setIcon(QIcon(":/icons/package_favorite.png"));
    connect(traineeGraph, SIGNAL(clicked()), this, SLOT(slotToggleTrainee()));

    QPixmap icon(16,8);
    for(int i=0; i < 17; ++i)
    {
        icon.fill(CTrack::lineColors[i]);
        comboColor->addItem(icon,"",QVariant(i));
    }

    connect(treePoints,SIGNAL(itemSelectionChanged()),this,SLOT(slotPointSelectionChanged()));
    connect(treePoints,SIGNAL(itemClicked(QTreeWidgetItem*,int)),this,SLOT(slotPointSelection(QTreeWidgetItem*)));

#ifdef GPX_EXTENSIONS
    //------------------------------------
    //add icon for extension & connect
    toolGraphExtensions->setIcon(QIcon(":/icons/iconExtensions16x16.png"));
    connect(toolGraphExtensions, SIGNAL(clicked()), this, SLOT(slotToggleExtensionsGraph()));

    //add icon for google maps
    toolGoogleMaps->setIcon(QIcon(":/icons/iconGoogleMaps16x16.png"));
    connect(toolGoogleMaps, SIGNAL(clicked()), this, SLOT(slotGoogleMaps()));

    //checkboxes to switch on/off standard columns
    connect(checkBox_num,SIGNAL(clicked(bool)),this,SLOT(slotSetColumns(bool)));
    connect(checkBox_tim,SIGNAL(clicked(bool)),this,SLOT(slotSetColumns(bool)));
    connect(checkBox_hig,SIGNAL(clicked(bool)),this,SLOT(slotSetColumns(bool)));
    connect(checkBox_dis,SIGNAL(clicked(bool)),this,SLOT(slotSetColumns(bool)));
    connect(checkBox_azi,SIGNAL(clicked(bool)),this,SLOT(slotSetColumns(bool)));
    connect(checkBox_ent,SIGNAL(clicked(bool)),this,SLOT(slotSetColumns(bool)));
    connect(checkBox_vel,SIGNAL(clicked(bool)),this,SLOT(slotSetColumns(bool)));
    connect(checkBox_suu,SIGNAL(clicked(bool)),this,SLOT(slotSetColumns(bool)));
    connect(checkBox_sud,SIGNAL(clicked(bool)),this,SLOT(slotSetColumns(bool)));
    connect(checkBox_pos,SIGNAL(clicked(bool)),this,SLOT(slotSetColumns(bool)));

#else
    toolGraphExtensions->hide();
    toolGoogleMaps->hide();
#endif

    CActions * actions = theMainWindow->getActionGroupProvider()->getActions();

    contextMenu = new QMenu(this);
    contextMenu->addAction(actions->getAction("aTrackPurgeSelection"));
    actSplit    = contextMenu->addAction(QPixmap(":/icons/iconEditCut16x16.png"),tr("Split"),this,SLOT(slotSplit()));

    connect(treePoints,SIGNAL(customContextMenuRequested(const QPoint&)),this,SLOT(slotContextMenu(const QPoint&)));
    connect(comboColor, SIGNAL(currentIndexChanged(int)), this, SLOT(slotColorChanged(int)));
    connect(lineName, SIGNAL(returnPressed()), this, SLOT(slotNameChanged()));
    connect(lineName, SIGNAL(textChanged(QString)), this, SLOT(slotNameChanged(QString)));
    connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(slotCurrentChanged(int)));
    connect(checkStages, SIGNAL(stateChanged(int)), this, SLOT(slotStagesChanged(int)));
    connect(textStages, SIGNAL(sigHighlightArea(QString)), this, SLOT(slotHighlightArea(QString)));
    connect(checkMultiColor, SIGNAL(toggled(bool)), this, SLOT(slotToggleMultiColor(bool)));
    connect(comboMultiColor, SIGNAL(currentIndexChanged(int)), this, SLOT(slotMultiColorMode(int)));
    connect(toolMuliColorConfig, SIGNAL(clicked()), this, SLOT(slotMultiColorConfig()));

    connect(&CTrackDB::self(), SIGNAL(sigModified(const QString&)), this, SLOT(slotStagesChanged()));
    connect(&CTrackDB::self(), SIGNAL(sigPointOfFocus(int)), this, SLOT(slotPointOfFocus(int)));
    connect(&CWptDB::self(),SIGNAL(sigChanged()),this,SLOT(slotStagesChanged()));
    connect(&CWptDB::self(),SIGNAL(sigModified(QString)),this,SLOT(slotStagesChanged()));

    CTrackFilterWidget * w = tabWidget->findChild<CTrackFilterWidget*>();
    if(w)
    {
        w->setTrackEditWidget(this);
    }

    SETTINGS;
    tabWidget->setCurrentIndex(cfg.value("TrackEditWidget/currentIndex",0).toUInt());
    checkCenterMap->setChecked((cfg.value("TrackEditWidget/centerMap",true).toBool()));

    QByteArray state = cfg.value("TrackEditWidget/trackpointlist").toByteArray();
    if(state.isEmpty())
    {
        treePoints->sortByColumn(eNum, Qt::AscendingOrder);
    }
    else
    {
        treePoints->header()->restoreState(state);
    }
}


CTrackEditWidget::~CTrackEditWidget()
{
    SETTINGS;
    cfg.setValue("TrackEditWidget/currentIndex",tabWidget->currentIndex());
    cfg.setValue("TrackEditWidget/centerMap",checkCenterMap->isChecked());
    cfg.setValue("TrackEditWidget/trackpointlist", treePoints->header()->saveState());

    if(!trackStatProfileDist.isNull())
    {
        delete trackStatProfileDist;
    }
    if(!trackStatSpeedDist.isNull())
    {
        delete trackStatSpeedDist;
    }
    if(!trackStatProfileTime.isNull())
    {
        delete trackStatProfileTime;
    }
    if(!trackStatSpeedTime.isNull())
    {
        delete trackStatSpeedTime;
    }
    if(!trackStatDistanceTime.isNull())
    {
        delete trackStatDistanceTime;
    }
    if(!trackStatTrainee.isNull())
    {
        delete trackStatTrainee;
    }
#ifdef GPX_EXTENSIONS
    //delete all extension tabs and reset tab status
    int num_of_tabs  = trackStatExtensions.size();
    for(int i=0; i < num_of_tabs; ++i)
    {
        if (trackStatExtensions[i])
        {
            delete trackStatExtensions[i];
        }
    }
    tabstat = 0;
    trackStatExtensions.clear();
#endif

}


void CTrackEditWidget::keyPressEvent(QKeyEvent * e)
{
    if(track.isNull()){e->ignore(); return;}

    switch (e->key())
    {
        case Qt::Key_Delete:
        {
            slotPurge();
            break;
        }

        case Qt::Key_Left:
        {
            QList<QTreeWidgetItem*> items = treePoints->selectedItems();
            if (items.begin() != items.end())
            {
                originator = true;
                track->setPointOfFocus((*items.begin())->data(0,Qt::UserRole).toInt(), CTrack::eNoErase, true);
                originator = false;
            }
            break;
        }

        case Qt::Key_Right:
        {
            QList<QTreeWidgetItem*> items = treePoints->selectedItems();
            if (items.begin() != items.end())
            {
                originator = true;
                track->setPointOfFocus((*(--items.end()))->data(0,Qt::UserRole).toInt(), CTrack::eNoErase, true);
                originator = false;
            }
            break;
        }

        default:
            e->ignore();
    }
}


void CTrackEditWidget::resizeEvent(QResizeEvent * e)
{
    QWidget::resizeEvent(e);

    if(oldSize.width() != e->size().width())
    {
        updateStages();
    }

    oldSize = e->size();
}


void CTrackEditWidget::slotContextMenu(const QPoint& pos)
{
    int cnt = treePoints->selectedItems().count();
    if(cnt > 0)
    {

        actSplit->setEnabled(cnt == 1);

        QPoint p = treePoints->mapToGlobal(pos);
        contextMenu->exec(p);
    }

}


void CTrackEditWidget::slotResetAllZoom()
{
    if(!trackStatProfileDist.isNull())
    {
        trackStatProfileDist->getPlot()->resetZoom();
    }
    if(!trackStatSpeedDist.isNull())
    {
        trackStatSpeedDist->getPlot()->resetZoom();
    }
    if(!trackStatProfileTime.isNull())
    {
        trackStatProfileTime->getPlot()->resetZoom();
    }
    if(!trackStatSpeedTime.isNull())
    {
        trackStatSpeedTime->getPlot()->resetZoom();
    }
    if(!trackStatDistanceTime.isNull())
    {
        trackStatDistanceTime->getPlot()->resetZoom();
    }
    if(!trackStatTrainee.isNull())
    {
        trackStatTrainee->getPlot()->resetZoom();
    }
}


void CTrackEditWidget::slotSplit()
{
    QList<QTreeWidgetItem *> items = treePoints->selectedItems();

    if(items.isEmpty())
    {
        return;
    }

    int idx = items.first()->text(eNum).toInt();

    CTrackDB::self().splitTrack(idx);
}


void CTrackEditWidget::slotSetTrack(CTrack * t)
{
    if(originator) return;

    treePoints->clear();

    if(track)
    {
        disconnect(track, SIGNAL(sigChanged()), this, SLOT(slotUpdate()));
        disconnect(track, SIGNAL(sigNeedUpdate()), this, SLOT(slotUpdate()));
        disconnect(track, SIGNAL(destroyed(QObject*)), this, SLOT(deleteLater()));

#ifdef GPX_EXTENSIONS
        //delete all extension tabs and reset tab status
        int num_of_tabs  = trackStatExtensions.size();
        for(int i=0; i < num_of_tabs; ++i)
        {
            if (trackStatExtensions[i])
            {
                delete trackStatExtensions[i];
            }
        }
        tabstat = 0;
        trackStatExtensions.clear();
        toolGraphExtensions->setChecked(false);

        //delete extensions columns in table
        //get names and number of extensions
                                 //Anzahl der Extensions
        QList<QString> names_of_ext = track->tr_ext.set.toList();
        int num_of_ext              = names_of_ext.size();
        for(int i=0; i < num_of_ext; ++i)
        {
            int number_of_column = eMaxColumn + i;
            //remove extensions columns - removeItemWidget is not sufficient ...
            treePoints->hideColumn(number_of_column);
            treePoints->removeItemWidget(treePoints->headerItem(), number_of_column);
        }

        //delete checkboxes & spacer
        for(int i=0; i < c_boxes.size(); ++i)
        {
            delete c_boxes[i];   //remove checkboxes
        }
        c_boxes.clear();         //empty qlist

        //remove spacer
        verticalLayout_Extensions->removeItem(Vspace);
        gridLayout_Extensions->removeWidget(label);

        //------------------------------------------------------------------------------------
#endif
    }

    track = t;
    if(track.isNull())
    {
        close();
        return;
    }

    //----------------------------------------------------------------------------------------------
    //----------------------------------------------------------------------------------------------
    //----------------------------------------------------------------------------------------------

    //for each extension: create a checkbox and a column and link them together
#ifdef GPX_EXTENSIONS

    //get names and number of extensions
                                 //Anzahl der Extensions
    QList<QString> names_of_ext = track->tr_ext.set.toList();
    int num_of_ext              = names_of_ext.size();

    if (num_of_ext)
    {
        gridLayout_Extensions->removeWidget(label);

        //create checkboxes
        for(int i=0; i < num_of_ext; ++i)
        {
            //Namen der Extentions pro i in Variable
            QString name        = names_of_ext[i];
            //column number
            int number_of_column = eMaxColumn + i;
            //Name des objects
            QString obj_name    = QString("%1").arg(number_of_column);

            //Check box generieren, checken und mit Name versehen
            QCheckBox *CheckBoxMake;
            CheckBoxMake = new QCheckBox(name);
            CheckBoxMake->setChecked(true);
            CheckBoxMake->setObjectName(obj_name);
            //CheckBoxMake->setToolTip(obj_name);
            c_boxes.insert(i, CheckBoxMake);
            verticalLayout_Extensions->addWidget(CheckBoxMake, i, 0);

            //add columns
            treePoints->headerItem()->setText(number_of_column, name);
            treePoints->showColumn(number_of_column);

            //connect checkbox with column
            connect(CheckBoxMake ,SIGNAL(clicked(bool)),this,SLOT(slotSetColumnsExt(bool)));
        }

        //QSpacerItem *Vspace;
        Vspace = new QSpacerItem(20, 2, QSizePolicy::Minimum, QSizePolicy::Expanding);
        verticalLayout_Extensions->addItem(Vspace);

    }
    else
    {
        if (no_ext_info_stat == 0)
        {
            QString lname = "no_ext_info";
            label = new QLabel(lname);
            label->setObjectName(lname);
            label->setEnabled(false);
            QFont font;
            font.setBold(true);
            font.setItalic(false);
            font.setWeight(75);
            font.setStyleStrategy(QFont::PreferDefault);
            label->setFont(font);
            label->setAlignment(Qt::AlignCenter);
            //keine extensions Elemente in dieser Datei
            label->setText(tr("no extensions elements in this file"));
            verticalLayout_Extensions->addWidget(label);
            no_ext_info_stat = 1;
        }
        else
        {
            verticalLayout_Extensions->removeWidget(label);
            no_ext_info_stat = 0;
        }
    }

    //----------------------------------------------------------------------------------------------
    //----------------------------------------------------------------------------------------------
    //----------------------------------------------------------------------------------------------
#endif

    QApplication::setOverrideCursor(Qt::WaitCursor);

    bool on;
    int id;
    QList<CTrack::multi_color_item_t> multiColorItems;
    track->getMultiColor(on, id, multiColorItems);

    track->blockSignals(true);
    comboMultiColor->clear();
    foreach(const CTrack::multi_color_item_t& item, multiColorItems)
    {
        comboMultiColor->addItem(item.name,item.id);
    }
    comboMultiColor->setCurrentIndex(id);
    checkMultiColor->setChecked(on);
    track->blockSignals(false);

    QList<CTrack::pt_t>& trkpts           = track->getTrackPoints();
    QList<CTrack::pt_t>::iterator trkpt   = trkpts.begin();
    while(trkpt != trkpts.end())
    {
        trkpt->editItem = 0;
        ++trkpt;
    }

    connect(track,SIGNAL(sigChanged()), this, SLOT(slotUpdate()));
    connect(track, SIGNAL(sigNeedUpdate()), this, SLOT(slotUpdate()));
    connect(track,SIGNAL(destroyed(QObject*)), this, SLOT(deleteLater()));

    slotUpdate();

    SETTINGS;
    // restore last session position and size of TrackEditWidget
    if ( cfg.contains("TrackEditWidget/geometry"))
    {
        QRect r = cfg.value("TrackEditWidget/geometry").toRect();

        if (r.isValid() && QDesktopWidget().screenGeometry().intersects(r))
        {
            tabWidget->setGeometry(r);
        }
    }
    else
    {
        cfg.setValue("TrackEditWidget/geometry",tabWidget->geometry());
    }

    treePoints->setUpdatesEnabled(false);

#ifdef GPX_EXTENSIONS
    for(int i=0; i < eMaxColumn+num_of_ext-1; ++i)
#else
        for(int i=0; i < eMaxColumn; ++i)
#endif
    {
        treePoints->resizeColumnToContents(i);
    }
    treePoints->setUpdatesEnabled(true);

    QApplication::restoreOverrideCursor();

    slotStagesChanged();
}


void CTrackEditWidget::slotUpdate()
{
    int i;

    if (track->hasTraineeData())
    {
        traineeGraph->setEnabled(true);
    }
    else
    {
        traineeGraph->setEnabled(false);
        if (!trackStatTrainee.isNull())
        {
            delete trackStatTrainee;
        }
    }

#ifdef GPX_EXTENSIONS
    //get names and number of extensions
                                 //Anzahl der Extensions
    QList<QString> names_of_ext = track->tr_ext.set.toList();
    int num_of_ext              = names_of_ext.size();

    if (num_of_ext == 0)
    {
        toolGraphExtensions->setEnabled(false);
    }
    else
    {
        toolGraphExtensions->setEnabled(true);
    }
#endif

    if(originator) return;

    lineName->setText(track->getName());
    comboColor->setCurrentIndex(track->getColorIdx());

    treePoints->setUpdatesEnabled(false);
    treePoints->setSelectionMode(QAbstractItemView::MultiSelection);

    QString str, val, unit;
    CTrackTreeWidgetItem * focus    = 0;
    QList<CTrack::pt_t>& trkpts           = track->getTrackPoints();
    QList<CTrack::pt_t>::iterator trkpt   = trkpts.begin();

#ifdef Q_OS_MAC
    // work around https://bugreports.qt.nokia.com/browse/QTBUG-21678
    CFType<CFLocaleRef> loc(CFLocaleCopyCurrent());
    CFType<CFDateFormatterRef> df(CFDateFormatterCreate(NULL, loc, kCFDateFormatterNoStyle, kCFDateFormatterNoStyle));
    CFDateFormatterSetFormat(df, CFSTR("EEE MMM d HH:mm:ss yyyy"));
#endif

    treePoints->setUpdatesEnabled(false);
    treePoints->blockSignals(true);
    treePoints->model()->blockSignals(true);

    QString timezone = track->getTimezone();

    while(trkpt != trkpts.end())
    {
        CTrackTreeWidgetItem * item;
        if ( !trkpt->editItem )
        {
            item = new CTrackTreeWidgetItem(treePoints);
            item->setTextAlignment(eNum,Qt::AlignLeft);
            item->setTextAlignment(eAltitude,Qt::AlignRight);
            item->setTextAlignment(eDelta,Qt::AlignRight);
            item->setTextAlignment(eAzimuth,Qt::AlignRight);
            item->setTextAlignment(eDistance,Qt::AlignRight);
            item->setTextAlignment(eAscend,Qt::AlignRight);
            item->setTextAlignment(eDescend,Qt::AlignRight);
            item->setTextAlignment(eSpeed,Qt::AlignRight);

            trkpt->editItem = item;
            trkpt->flags.setChanged(true);
        }

        if ( !trkpt->flags.isChanged() )
        {
            ++trkpt;
            continue;
        }

        item = (CTrackTreeWidgetItem *)trkpt->editItem.data();
        item->setData(0, Qt::UserRole, trkpt->idx);

        // gray shade deleted items
        if(trkpt->flags & CTrack::pt_t::eDeleted)
        {
            //item->setFlags((item->flags() & ~Qt::ItemIsEnabled) | Qt::ItemIsTristate);
#ifdef GPX_EXTENSIONS
            for(i = 0; i < (eMaxColumn + num_of_ext); ++i)
#else
                for(i = 0; i < eMaxColumn; ++i)
#endif
            {
                item->setForeground(i,QBrush(Qt::gray));
            }
        }
        else
        {
            //item->setFlags(item->flags() | Qt::ItemIsEnabled | Qt::ItemIsTristate);
#ifdef GPX_EXTENSIONS
            for(i = 0; i < (eMaxColumn + num_of_ext); ++i)
#else
                for(i = 0; i < eMaxColumn; ++i)
#endif
            {
                item->setForeground(i,QBrush(Qt::black));
            }
        }

        // temp. store item of user focus
        if(trkpt->flags & CTrack::pt_t::eFocus)
        {
            focus = item;
        }

        if(trkpt->flags & CTrack::pt_t::eSelected)
        {
            if ( !item->isSelected() )
            {
                item->setSelected(true);
            }
        }
        else
        {
            if ( item->isSelected() )
            {
                item->setSelected(false);
            }
        }

        // point number
        item->setText(eNum,QString::number(trkpt->idx));

        // timestamp
        if(trkpt->timestamp != 0x00000000 && trkpt->timestamp != 0xFFFFFFFF)
        {
            QDateTime time = QDateTime::fromTime_t(trkpt->timestamp);
            if(!timezone.isEmpty())
            {
#ifdef QK_QT5_TZONE
                time = time.toTimeZone(QTimeZone(timezone.toLatin1()));
#else
                time = TimeStamp(trkpt->timestamp).toZone(timezone).toDateTime();
#endif
            }

#ifdef Q_OS_MAC
            CFType<CFDateRef> cfdate(CFDateCreate(NULL, time.toTime_t() - kCFAbsoluteTimeIntervalSince1970));
            CFType<CFStringRef> cfstr(CFDateFormatterCreateStringWithDate(NULL, df, cfdate));
            str = CFStringToQString(cfstr);
#else
            str = time.toString();
#endif
        }
        else
        {
            str = "-";
        }

        item->setText(eTime,str);

        // altitude
        if(trkpt->ele != WPT_NOFLOAT)
        {
            IUnit::self().meter2elevation(trkpt->ele, val, unit);
            str = tr("%1 %2").arg(val).arg(unit);
        }
        else
        {
            str = "-";
        }
        item->setText(eAltitude,str);

        // delta
        IUnit::self().meter2distance(trkpt->delta, val, unit);
        item->setText(eDelta, tr("%1 %2").arg(val).arg(unit));

        // azimuth
        if(trkpt->azimuth != WPT_NOFLOAT)
        {
            const QChar degreeChar(0260);
            str.sprintf("%1.0f",trkpt->azimuth);
            str.append(degreeChar);
        }
        else
        {
            str = "-";
        }
        item->setText(eAzimuth,str);

        // distance
        IUnit::self().meter2distance(trkpt->distance, val, unit);
        item->setText(eDistance, tr("%1 %2").arg(val).arg(unit));
        IUnit::self().meter2elevation(trkpt->ascend, val, unit);
        item->setText(eAscend, tr("%1 %2").arg(val).arg(unit));
        IUnit::self().meter2elevation(trkpt->descend, val, unit);
        item->setText(eDescend, tr("%1 %2").arg(val).arg(unit));

        // speed
        if(trkpt->speed != WPT_NOFLOAT)
        {
            IUnit::self().meter2speed(trkpt->speed, val, unit);
            str = tr("%1 %2").arg(val).arg(unit);
        }
        else
        {
            str = "-";
        }
        item->setText(eSpeed,str);

        // position
        GPS_Math_Deg_To_Str(trkpt->lon, trkpt->lat, str);
        item->setText(ePosition,str);

#ifdef GPX_EXTENSIONS
        //fill cells of tracklist with extensions
        for(int i=0; i < num_of_ext; ++i)
        {
            int col = eMaxColumn+i;

            //name of the extension
            QString nam = names_of_ext[i];

            //value of the extension
            QString val = trkpt->gpx_exts.getValue(nam);

            if (val == "")
            {
                val = "-";;
            }

            //insert
            item->setText(col, val);
            //format
            item->setTextAlignment(col, Qt::AlignRight);

        }
#endif
        //--------------------------------------------------------------------------------------------------
        //--------------------------------------------------------------------------------------------------
        //--------------------------------------------------------------------------------------------------

        trkpt->flags.setChanged(false);
        ++trkpt;
    }

    treePoints->model()->blockSignals(false);
    treePoints->blockSignals(false);
    treePoints->setUpdatesEnabled(true);

    // adjust column sizes to fit

#ifdef QK_QT5_PORT
    treePoints->header()->setSectionResizeMode(0,QHeaderView::Interactive);
#else
    treePoints->header()->setResizeMode(0,QHeaderView::Interactive);
#endif

    // scroll to item of user focus
    if(focus)
    {
        //treePoints->setCurrentItem(focus);
        treePoints->scrollToItem(focus);
    }
    treePoints->setSelectionMode(QAbstractItemView::ExtendedSelection);
}


void CTrackEditWidget::slotPointSelectionChanged()
{
    if(track.isNull() || originator) return;

    if(treePoints->selectionMode() == QAbstractItemView::MultiSelection) return;

    //    qDebug() << Q_FUNC_INFO;

    qint32 old_b = -1;
    qint32 old_e = -1;
    qint32 new_b = -1;
    qint32 new_e = -1;
    qint32 i = -1;

    // reset previous selections
    QList<CTrack::pt_t>& trkpts           = track->getTrackPoints();
    QList<CTrack::pt_t>::iterator trkpt   = trkpts.begin();
    while(trkpt != trkpts.end())
    {
        i++;
        if ((trkpt->flags & CTrack::pt_t::eSelected) != 0)
        {
            if (old_b < 0)
            {
                old_b = i;
            }
            old_e = i;
        }
        trkpt->flags &= ~CTrack::pt_t::eFocus;
        trkpt->flags &= ~CTrack::pt_t::eSelected;
        ++trkpt;
    }

    // set eSelected flag for selected points
    QList<QTreeWidgetItem*> items = treePoints->selectedItems();
    QList<QTreeWidgetItem*>::const_iterator item = items.begin();
    while(item != items.end())
    {
        quint32 idxTrkPt = (*item)->data(0,Qt::UserRole).toUInt();
        //trkpts[idxTrkPt].flags |= CTrack::pt_t::eFocus;
        trkpts[idxTrkPt].flags |= CTrack::pt_t::eSelected;
        if (new_b < 0)
        {
            new_b = idxTrkPt;
        }
        new_e = idxTrkPt;
        ++item;
    }
    if(!items.isEmpty())
    {
        quint32 idxTrkPt = items.last()->data(0,Qt::UserRole).toUInt();
        trkpts[idxTrkPt].flags |= CTrack::pt_t::eFocus;
    }

    originator = true;
    if (items.begin() != items.end())
    {
        if (old_e != new_e)
        {
            track->setPointOfFocus(new_e, CTrack::eNoErase, checkCenterMap->isChecked());
        }
        else
        {
            track->setPointOfFocus(new_b, CTrack::eNoErase, checkCenterMap->isChecked());
        }
    }
    //track->rebuild(false);
    track->emitSigNeedUpdate();
    originator = false;
}


void CTrackEditWidget::slotPointSelection(QTreeWidgetItem * item)
{
    if(track.isNull()) return;

    originator = true;
    track->setPointOfFocus(item->data(0,Qt::UserRole).toInt(), CTrack::eNoErase, true);
    originator = false;
}


void CTrackEditWidget::slotPurge()
{
    QList<CTrack::pt_t>& trkpts                     = track->getTrackPoints();
    QList<QTreeWidgetItem*> items                   = treePoints->selectedItems();
    QList<QTreeWidgetItem*>::const_iterator item    = items.begin();

    while(item != items.end())
    {
        quint32 idxTrkPt = (*item)->data(0,Qt::UserRole).toUInt();
        if(trkpts[idxTrkPt].flags & CTrack::pt_t::eDeleted)
        {
            trkpts[idxTrkPt].flags &= ~CTrack::pt_t::eDeleted;
        }
        else
        {
            trkpts[idxTrkPt].flags |= CTrack::pt_t::eDeleted;
        }

        ++item;
    }
    track->rebuild(false);
}


void CTrackEditWidget::slotShowProfile()
{
    if(trackStatProfileDist.isNull())
    {
        if(trackStatSpeedDist.isNull())
        {
            trackStatSpeedDist = new CTrackStatSpeedWidget(ITrackStat::eOverDistance, this);
            theMainWindow->getCanvasTab()->setTabPosition(QTabWidget::South);
            theMainWindow->getCanvasTab()->addTab(trackStatSpeedDist, tr("Speed/Dist."));
        }

        if(trackStatDistanceTime.isNull())
        {
            trackStatDistanceTime = new CTrackStatDistanceWidget(this);
            theMainWindow->getCanvasTab()->setTabPosition(QTabWidget::South);
            theMainWindow->getCanvasTab()->addTab(trackStatDistanceTime, tr("Dist./Time"));
        }

        trackStatProfileDist = new CTrackStatProfileWidget(ITrackStat::eOverDistance, this);
        theMainWindow->getCanvasTab()->setTabPosition(QTabWidget::South);
        theMainWindow->getCanvasTab()->addTab(trackStatProfileDist, tr("Profile/Dist."));
        toolGraphDistance->toggle();
    }

    theMainWindow->getCanvasTab()->setCurrentWidget(trackStatProfileDist);
}


void CTrackEditWidget::slotToggleStatDistance()
{

    if(trackStatSpeedDist.isNull())
    {
        trackStatSpeedDist = new CTrackStatSpeedWidget(ITrackStat::eOverDistance, this);
        theMainWindow->getCanvasTab()->setTabPosition(QTabWidget::South);
        theMainWindow->getCanvasTab()->addTab(trackStatSpeedDist, tr("Speed/Dist."));
    }
    else
    {
        delete trackStatSpeedDist;
    }

    if(trackStatDistanceTime.isNull())
    {
        trackStatDistanceTime = new CTrackStatDistanceWidget(this);
        theMainWindow->getCanvasTab()->setTabPosition(QTabWidget::South);
        theMainWindow->getCanvasTab()->addTab(trackStatDistanceTime, tr("Dist./Time"));
    }
    else
    {
        delete trackStatDistanceTime;
    }

    if(trackStatProfileDist.isNull())
    {
        trackStatProfileDist = new CTrackStatProfileWidget(ITrackStat::eOverDistance, this);
        theMainWindow->getCanvasTab()->setTabPosition(QTabWidget::South);
        theMainWindow->getCanvasTab()->addTab(trackStatProfileDist, tr("Profile/Dist."));
    }
    else
    {
        delete trackStatProfileDist;
    }
}


void CTrackEditWidget::slotToggleStatTime()
{
    if(trackStatSpeedTime.isNull())
    {
        trackStatSpeedTime = new CTrackStatSpeedWidget(ITrackStat::eOverTime, this);
        theMainWindow->getCanvasTab()->setTabPosition(QTabWidget::South);
        theMainWindow->getCanvasTab()->addTab(trackStatSpeedTime, tr("Speed/Time"));
    }
    else
    {
        delete trackStatSpeedTime;
    }

    if(trackStatProfileTime.isNull())
    {
        trackStatProfileTime = new CTrackStatProfileWidget(ITrackStat::eOverTime, this);
        theMainWindow->getCanvasTab()->setTabPosition(QTabWidget::South);
                                 //Tab hinzufgen
        theMainWindow->getCanvasTab()->addTab(trackStatProfileTime, tr("Profile/Time"));
    }
    else
    {
        delete trackStatProfileTime;
    }
}


void CTrackEditWidget::slotToggleTrainee()
{
    if(trackStatTrainee.isNull())
    {
        trackStatTrainee = new CTrackStatTraineeWidget(ITrackStat::eOverDistance, this);
        theMainWindow->getCanvasTab()->setTabPosition(QTabWidget::South);
        theMainWindow->getCanvasTab()->addTab(trackStatTrainee, tr("Trainee"));
    }
    else
    {
        delete trackStatTrainee;
    }
}


#ifdef GPX_EXTENSIONS
//method to show & hide the extensions graphs
void CTrackEditWidget::slotToggleExtensionsGraph()
{
    //get names und number of extensions
    QList<QString> names_of_ext = track->tr_ext.set.toList();
    int num_of_ext              = names_of_ext.size();

    if (tabstat == 0)
    {
        for(int i=0; i < num_of_ext; ++i)
        {
            QString name = names_of_ext[i];
            QPointer<CTrackStatExtensionWidget> tab;

            //Create tab containing plot over time
            tab = new CTrackStatExtensionWidget(ITrackStat::eOverTime, this, name);

            //add tab
            theMainWindow->getCanvasTab()->addTab(tab, name+"/t");
                                 //add Tab index to list for further handling
            trackStatExtensions.insert(i, tab);
        }
        connect(theMainWindow->getCanvasTab(),SIGNAL(tabCloseRequested(int)),this,SLOT(slotKillTab(int)));
        tabstat = 1;
    }
    else
    {
        //delete all extension tabs and reset tab status
        int num_of_tabs  = trackStatExtensions.size();
        for(int i=0; i < num_of_tabs; ++i)
        {
            if (trackStatExtensions[i])
            {
                delete trackStatExtensions[i];
            }
        }
        disconnect(theMainWindow->getCanvasTab(),SIGNAL(tabCloseRequested(int)), this, SLOT(slotKillTab(int)));
        tabstat = 0;
        trackStatExtensions.clear();
    }

    //Tab Settings
    theMainWindow->getCanvasTab()->setTabPosition(QTabWidget::South);
    theMainWindow->getCanvasTab()->setMovable(true);

    theMainWindow->getCanvasTab()->setTabsClosable(true);
}
#endif

#ifdef GPX_EXTENSIONS
void CTrackEditWidget::slotSetColumns(bool checked)
{
    //who made the signal
    QString name_std_is = (QObject::sender()->objectName());

    //0. checkBox_num - number
    if (name_std_is == "checkBox_num")
    {
        if(checked == true) {CTrackEditWidget::treePoints->showColumn(eNum);}
        else                {CTrackEditWidget::treePoints->hideColumn(eNum);}
    }
    //1. checkBox_tim - time
    else if (name_std_is == "checkBox_tim")
    {
        if(checked == true) {CTrackEditWidget::treePoints->showColumn(eTime);}
        else                {CTrackEditWidget::treePoints->hideColumn(eTime);}
    }
    //2 .checkBox_hig - hight
    else if (name_std_is == "checkBox_hig")
    {
        if(checked == true) {CTrackEditWidget::treePoints->showColumn(eAltitude);}
        else                {CTrackEditWidget::treePoints->hideColumn(eAltitude);}
    }
    //3 .checkBox_dis - distance
    else if (name_std_is == "checkBox_dis")
    {
        if(checked == true) {CTrackEditWidget::treePoints->showColumn(eDelta);}
        else                {CTrackEditWidget::treePoints->hideColumn(eDelta);}
    }
    //4 .checkBox_azi - azimuth
    else if (name_std_is == "checkBox_azi")
    {
        if(checked == true) {CTrackEditWidget::treePoints->showColumn(eAzimuth);}
        else                {CTrackEditWidget::treePoints->hideColumn(eAzimuth);}
    }
    //5 .checkBox_ent - entfernung
    else if (name_std_is == "checkBox_ent")
    {
        if(checked == true) {CTrackEditWidget::treePoints->showColumn(eDistance);}
        else                {CTrackEditWidget::treePoints->hideColumn(eDistance);}
    }
    //6 .checkBox_vel - velocity
    else if (name_std_is == "checkBox_vel")
    {
        if(checked == true) {CTrackEditWidget::treePoints->showColumn(eSpeed);}
        else                {CTrackEditWidget::treePoints->hideColumn(eSpeed);}
    }
    //7 .checkBox_suu - summ up
    else if (name_std_is == "checkBox_suu")
    {
        if(checked == true) {CTrackEditWidget::treePoints->showColumn(eAscend);}
        else                {CTrackEditWidget::treePoints->hideColumn(eAscend);}
    }
    //8 .checkBox_sud - summ down
    else if (name_std_is == "checkBox_sud")
    {
        if(checked == true) {CTrackEditWidget::treePoints->showColumn(eDescend);}
        else                {CTrackEditWidget::treePoints->hideColumn(eDescend);}
    }
    //9 .checkBox_pos - position
    else if (name_std_is == "checkBox_pos")
    {
        if(checked == true) {CTrackEditWidget::treePoints->showColumn(ePosition);}
        else                {CTrackEditWidget::treePoints->hideColumn(ePosition);}
    }
    //sender unknown -> nothing happens
    else
    {
    }
}
#endif

#ifdef GPX_EXTENSIONS
void CTrackEditWidget::slotSetColumnsExt(bool checked)
{
    //who's sender of checkbox signal
    QString nameis = (QObject::sender()->objectName());
    int col = nameis.toInt();    //use object name as column #

    //on or off
    if(checked == true) {CTrackEditWidget::treePoints->showColumn(col);}
    else                {CTrackEditWidget::treePoints->hideColumn(col);}

}
#endif

void CTrackEditWidget::slotGoogleMaps()
{
    QString str, outp;
    int count = 0;
    int pnts = 0;
    int every_this = 0;

    QList<CTrack::pt_t>& trkpts           = track->getTrackPoints();
    QList<CTrack::pt_t>::iterator trkpt   = trkpts.begin();

    pnts = trkpts.size();
    if (pnts <= 25)
    {

        while(trkpt != trkpts.end())
        {

            GPS_Math_Deg_To_Str(trkpt->lon, trkpt->lat, str);
            if (count == 0)     {outp += str;}
            else                {outp += "+to:"+str;}
            trkpt++;
            count++;
        }
    }
    else
    {

        every_this = (pnts/25);
        int icount = 0;
        int multi = 0;

        while(trkpt != trkpts.end())
        {
            if (count == every_this)
            {

                GPS_Math_Deg_To_Str(trkpt->lon, trkpt->lat, str);
                if (icount == 0)        {outp += str;}
                else                {outp += "+to:"+str;}
                //every_this += every_this;
                icount++;
                multi = icount+1;
            }
            else if (count == every_this*multi)
            {
                GPS_Math_Deg_To_Str(trkpt->lon, trkpt->lat, str);
                if (icount == 0)        {outp += str;}
                else                {outp += "+to:"+str;}
                //every_this += every_this;
                icount++;
                multi = icount+1;
            }

            //trkpt->idx = trkpt->idx+every_this;
            trkpt++;
            count++;
        }

    }
    /*
    QMessageBox msgBox;
    msgBox.setText("text");
    msgBox.exec();
    */
    QDesktopServices::openUrl(QUrl("http://maps.google.com/maps?f=h&saddr=&daddr="+outp, QUrl::TolerantMode));
}


void CTrackEditWidget::slotKillTab(int index)
{
    if (index != 0)
    {
        theMainWindow->getCanvasTab()->removeTab(index);
    }
}


void CTrackEditWidget::slotColorChanged(int idx)
{
    if(track.isNull()) return;

    int _idx_ = track->getColorIdx();
    if(_idx_ != comboColor->currentIndex())
    {
        track->setColor(comboColor->currentIndex());
        track->rebuild(true);
    }
}


void CTrackEditWidget::slotNameChanged(const QString& name)
{
    if(track.isNull()) return;
    QString _name_ = track->getName();
    QPalette palette = lineName->palette();
    if(_name_ != name)
    {
        palette.setColor(QPalette::Base, QColor(255, 128, 128));
    }
    else
    {
        palette.setColor(QPalette::Base, QColor(255, 255, 255));
    }
    lineName->setPalette(palette);
}


void CTrackEditWidget::slotNameChanged()
{
    if(track.isNull()) return;

    QString  name  = lineName->text();
    QString _name_ = track->getName();

    QPalette palette = lineName->palette();

    if(_name_ != name)
    {
        track->setName(name);
        track->rebuild(true);

        palette.setColor(QPalette::Base, QColor(128, 255, 128));
    }

    lineName->setPalette(palette);
}


void CTrackEditWidget::slotReset()
{
    if(track.isNull()) return;
    track->reset();
    track->rebuild(true);
    track->slotScaleWpt2Track();
}


void CTrackEditWidget::slotDelete()
{
    if(track.isNull()) return;
    if(QMessageBox::warning(0,tr("Remove track points ...")
        ,tr("You are about to remove hidden track points permanently. If you press 'yes', all information will be lost.")
        ,QMessageBox::Yes|QMessageBox::No) == QMessageBox::No)
    {
        return;
    }

    QList<CTrack::pt_t>& trkpts = track->getTrackPoints();
    QList<CTrack::pt_t>::iterator trkpt, end;
    track->setupIterators(trkpt, end);

    originator = true;
    while(trkpt != end && trkpt != trkpts.end())
    {
        if(trkpt->flags & CTrack::pt_t::eDeleted)
        {
            delete trkpt->editItem;
            trkpt = trkpts.erase(trkpt);
        }
        else
        {
            ++trkpt;
        }
    }
    originator = false;

    track->rebuild(true);
    track->slotScaleWpt2Track();
}


void CTrackEditWidget::slotCurrentChanged(int idx)
{
    if(idx == eStages)
    {
        updateStages();
    }
}


void CTrackEditWidget::slotStagesChanged(int state)
{
    if(track.isNull() || originator) return;
    track->setDoScaleWpt2Track((Qt::CheckState)state);
}


void CTrackEditWidget::slotStagesChanged()
{
    textStages->clear();
    if(track.isNull() || originator) return;

    // get waypoints near track
    originator = true;
    checkStages->setCheckState(track->getDoScaleWpt2Track());
    originator = false;

    const QList<CTrack::wpt_t>& wpts = track->getStageWaypoints();
    if(wpts.isEmpty())
    {
        tabWidget->setTabEnabled(eStages, false);
        return;
    }

    updateStages();

}


void CTrackEditWidget::slotPointOfFocus(const int idx)
{
    int cnt = 0;

    const QList<CTrack::wpt_t>& wpts = track->getStageWaypoints();
    if(idx < 0 || wpts.isEmpty() || track.isNull() || !CResources::self().showTrackProfileEleInfo())
    {
        textStages->slotHighlightArea("");
        if(trackStatProfileDist)
        {
            trackStatProfileDist->getPlot()->slotHighlightSection(WPT_NOFLOAT,WPT_NOFLOAT);
        }
        return;
    }

    double x1 = 0;
    double x2 = 0;

    foreach(const CTrack::wpt_t& wpt, wpts)
    {
        //        qDebug() << idx << wpt.trkpt.idx;

        x2 = wpt.trkpt.distance;

        if(idx <= wpt.trkpt.idx)
        {
            textStages->slotHighlightArea(QString("stage%1").arg(cnt));
            if(trackStatProfileDist)
            {
                //                double x = track->getTrackPoints()[idx].distance;
                trackStatProfileDist->getPlot()->slotHighlightSection(x1,x2);
            }
            return;
        }

        x1 = x2;
        cnt++;
    }

    if(idx >= wpts.last().trkpt.idx)
    {
        textStages->slotHighlightArea(QString("stage%1").arg(cnt));

        if(trackStatProfileDist && track)
        {
            //            double x = track->getTrackPoints()[idx].distance;
            trackStatProfileDist->getPlot()->slotHighlightSection(x1,track->getTrackPoints().last().distance);
        }

    }
}


void CTrackEditWidget::slotHighlightArea(const QString& key)
{
    if(lastStageKey == key)
    {
        return;
    }

    lastStageKey = key;

    if(trackStatProfileDist)
    {
        if(stages.contains(key))
        {
            stage_t& stage = stages[key];
            trackStatProfileDist->getPlot()->slotHighlightSection(stage.x1, stage.x2);
        }
        else
        {
            trackStatProfileDist->getPlot()->slotHighlightSection(WPT_NOFLOAT, WPT_NOFLOAT);
        }
    }
}


void CTrackEditWidget::slotToggleMultiColor(bool on)
{
    comboColor->setEnabled(!on);
    comboMultiColor->setEnabled(on);
    toolMuliColorConfig->setEnabled(on);

    if(track.isNull()) return;
    track->setMultiColor(on, comboMultiColor->currentIndex());
}


void CTrackEditWidget::slotMultiColorMode(int idx)
{
    if(track.isNull()) return;
    track->setMultiColor(checkMultiColor->isChecked(), idx);
}


void CTrackEditWidget::slotMultiColorConfig()
{
    if(track.isNull()) return;

    CTrack::multi_color_setup_t& setup = track->getMultiColorSetup(comboMultiColor->currentIndex());
    CDlgMultiColorConfig dlg(setup);
    if(dlg.exec() == QDialog::Rejected)
    {
        return;
    }

    track->rebuildColorMap();

    SETTINGS;
    cfg.beginGroup("MultiColorTrack");
    setup.save(cfg);
    cfg.endGroup();

}


#define CHAR_PER_LINE 120
#define ROOT_FRAME_MARGIN 5
#define BASE_FONT_SIZE  9

void CTrackEditWidget::updateStages()
{
    textStages->clear();

    if(track.isNull()) return;

    const QList<CTrack::wpt_t>& wpts = track->getStageWaypoints();

    if(wpts.isEmpty()) return;

    tabWidget->setTabEnabled(eStages, true);

    QTextDocument * doc = new QTextDocument(textStages);
    doc->setTextWidth(textStages->size().width() - 20);
    QFontMetrics fm(QFont(textStages->font().family(),BASE_FONT_SIZE));
    int w = doc->textWidth();
    int pointSize = ((BASE_FONT_SIZE * (w - 2 * ROOT_FRAME_MARGIN)) / (CHAR_PER_LINE *  fm.width("X")));
    if(pointSize == 0) return;
    if(pointSize > 12) pointSize = 12;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    QFont f = textStages->font();
    f.setPointSize(pointSize);
    textStages->setFont(f);

    // copied from CDiaryEdit
    QTextCharFormat fmtCharStandard;
    fmtCharStandard.setFont(f);

    QTextCharFormat fmtCharShade;
    fmtCharShade.setFont(f);
    fmtCharShade.setBackground(Qt::lightGray);

    QTextCharFormat fmtCharHeader;
    fmtCharHeader.setFont(f);
    fmtCharHeader.setBackground(Qt::darkBlue);
    fmtCharHeader.setFontWeight(QFont::Bold);
    fmtCharHeader.setForeground(Qt::white);

    QTextBlockFormat fmtBlockStandard;
    fmtBlockStandard.setTopMargin(10);
    fmtBlockStandard.setBottomMargin(10);
    fmtBlockStandard.setAlignment(Qt::AlignJustify);

    QTextBlockFormat fmtBlockCenter;
    fmtBlockCenter.setAlignment(Qt::AlignCenter);

    QTextBlockFormat fmtBlockRight;
    fmtBlockRight.setAlignment(Qt::AlignRight);
    fmtBlockRight.setNonBreakableLines(true);

    QTextFrameFormat fmtFrameStandard;
    fmtFrameStandard.setTopMargin(ROOT_FRAME_MARGIN);
    fmtFrameStandard.setBottomMargin(ROOT_FRAME_MARGIN);
    fmtFrameStandard.setWidth(w - 2 * ROOT_FRAME_MARGIN);

    QTextFrameFormat fmtFrameRoot;
    fmtFrameRoot.setTopMargin(ROOT_FRAME_MARGIN);
    fmtFrameRoot.setBottomMargin(ROOT_FRAME_MARGIN);
    fmtFrameRoot.setLeftMargin(ROOT_FRAME_MARGIN);
    fmtFrameRoot.setRightMargin(ROOT_FRAME_MARGIN);

    QTextTableFormat fmtTableStandard;
    fmtTableStandard.setBorder(1);
    fmtTableStandard.setBorderStyle(QTextFrameFormat::BorderStyle_Groove);
    fmtTableStandard.setBorderBrush(QColor("#a0a0a0"));
    fmtTableStandard.setCellPadding(2);
    fmtTableStandard.setCellSpacing(0);
    fmtTableStandard.setHeaderRowCount(2);
    fmtTableStandard.setTopMargin(10);
    fmtTableStandard.setBottomMargin(20);
    fmtTableStandard.setWidth(w - 2 * ROOT_FRAME_MARGIN);

    QVector<QTextLength> constraints;
    constraints << QTextLength(QTextLength::FixedLength, 32);
    //    constraints << QTextLength(QTextLength::VariableLength, 50);
    //    constraints << QTextLength(QTextLength::VariableLength, 30);
    //    constraints << QTextLength(QTextLength::VariableLength, 30);
    //    constraints << QTextLength(QTextLength::VariableLength, 30);
    //    constraints << QTextLength(QTextLength::VariableLength, 100);
    fmtTableStandard.setColumnWidthConstraints(constraints);

    doc->rootFrame()->setFrameFormat(fmtFrameRoot);
    QTextCursor cursor = doc->rootFrame()->firstCursorPosition();

    // 2 rows header, 2 rows start/stop
    table = cursor.insertTable(wpts.count()+2+2, eMax, fmtTableStandard);

    for(int i = eSym; i < eMax; i++)
    {
        table->cellAt(0,i).setFormat(fmtCharHeader);
        table->cellAt(1,i).setFormat(fmtCharHeader);
    }

    // header -------------------------
    table->mergeCells(0, eEleWpt, 1, 2);
    table->cellAt(0,eEleWpt).firstCursorPosition().setBlockFormat(fmtBlockCenter);
    table->mergeCells(0, eToNextDist, 1, 4);
    table->cellAt(0,eToNextDist).firstCursorPosition().setBlockFormat(fmtBlockCenter);
    table->mergeCells(0, eTotalDist, 1, 4);
    table->cellAt(0,eTotalDist).firstCursorPosition().setBlockFormat(fmtBlockCenter);

    table->cellAt(0,eInfo).firstCursorPosition().insertText(tr("Name"));
    table->cellAt(0,eProx).firstCursorPosition().insertText(tr("Prox."));
    table->cellAt(0,ePic).firstCursorPosition().insertText(tr("Pic."));
    table->cellAt(0,eEleWpt).firstCursorPosition().insertText(tr("Elevation"));
    table->cellAt(0,eToNextDist).firstCursorPosition().insertText(tr("To Next"));
    table->cellAt(0,eTotalDist).firstCursorPosition().insertText(tr("Total"));
    table->cellAt(0,eComment).firstCursorPosition().insertText(tr("Comment"));

    table->cellAt(1,eEleWpt).firstCursorPosition().setBlockFormat(fmtBlockCenter);
    table->cellAt(1,eEleWpt).firstCursorPosition().insertText(tr("wpt"));

    table->cellAt(1,eEleTrk).firstCursorPosition().setBlockFormat(fmtBlockCenter);
    table->cellAt(1,eEleTrk).firstCursorPosition().insertText(tr("trk"));

    table->cellAt(1,eToNextDist).firstCursorPosition().setBlockFormat(fmtBlockCenter);
    table->cellAt(1,eToNextDist).firstCursorPosition().insertText(QChar(0x21A6));
    table->cellAt(1,eToNextTime).firstCursorPosition().setBlockFormat(fmtBlockCenter);
    table->cellAt(1,eToNextTime).firstCursorPosition().insertText(QChar(0x231a));
    table->cellAt(1,eToNextAsc).firstCursorPosition().setBlockFormat(fmtBlockCenter);
    table->cellAt(1,eToNextAsc).firstCursorPosition().insertText(QChar(0x2197));
    table->cellAt(1,eToNextDesc).firstCursorPosition().setBlockFormat(fmtBlockCenter);
    table->cellAt(1,eToNextDesc).firstCursorPosition().insertText(QChar(0x2198));

    table->cellAt(1,eTotalDist).firstCursorPosition().setBlockFormat(fmtBlockCenter);
    table->cellAt(1,eTotalDist).firstCursorPosition().insertText(QChar(0x21A6));
    table->cellAt(1,eTotalTime).firstCursorPosition().setBlockFormat(fmtBlockCenter);
    table->cellAt(1,eTotalTime).firstCursorPosition().insertText(QChar(0x231a));
    table->cellAt(1,eTotalAsc).firstCursorPosition().setBlockFormat(fmtBlockCenter);
    table->cellAt(1,eTotalAsc).firstCursorPosition().insertText(QChar(0x2197));
    table->cellAt(1,eTotalDesc).firstCursorPosition().setBlockFormat(fmtBlockCenter);
    table->cellAt(1,eTotalDesc).firstCursorPosition().insertText(QChar(0x2198));

    // first entry -------------------------
    stage_t stage;
    QString val, unit;
    if(track->getStartElevation() != WPT_NOFLOAT)
    {
        IUnit::self().meter2elevation(track->getStartElevation(),val,unit);
    }
    else
    {
        val     = "-";
        unit    = "";
    }

    table->cellAt(2,eSym).firstCursorPosition().insertImage(":/icons/face-plain.png");
    table->cellAt(2,eInfo).firstCursorPosition().insertText(tr("Start"), fmtCharStandard);
    table->cellAt(2,ePic).firstCursorPosition().insertText(tr(""), fmtCharStandard);
    table->cellAt(2,eProx).firstCursorPosition().setBlockFormat(fmtBlockRight);
    table->cellAt(2,eProx).firstCursorPosition().insertText(tr("-"), fmtCharStandard);
    table->cellAt(2,eEleWpt).firstCursorPosition().setBlockFormat(fmtBlockRight);
    table->cellAt(2,eEleWpt).firstCursorPosition().insertText(tr("-"), fmtCharStandard);
    table->cellAt(2,eEleTrk).firstCursorPosition().setBlockFormat(fmtBlockRight);
    table->cellAt(2,eEleTrk).firstCursorPosition().insertText(tr("%1 %2").arg(val).arg(unit), fmtCharStandard);

    table->cellAt(2,eTotalDist).firstCursorPosition().setBlockFormat(fmtBlockRight);
    table->cellAt(2,eTotalDist).firstCursorPosition().insertText(tr("-"), fmtCharStandard);
    table->cellAt(2,eTotalTime).firstCursorPosition().setBlockFormat(fmtBlockRight);
    table->cellAt(2,eTotalTime).firstCursorPosition().insertText(tr("-"), fmtCharStandard);
    table->cellAt(2,eTotalAsc).firstCursorPosition().setBlockFormat(fmtBlockRight);
    table->cellAt(2,eTotalAsc).firstCursorPosition().insertText(tr("-"), fmtCharStandard);
    table->cellAt(2,eTotalDesc).firstCursorPosition().setBlockFormat(fmtBlockRight);
    table->cellAt(2,eTotalDesc).firstCursorPosition().insertText(tr("-"), fmtCharStandard);

    table->cellAt(2,eComment).firstCursorPosition().insertText(tr("Start of track."), fmtCharStandard);

    int cnt = 3;

    float   distLast = 0;
    int     timeLast = track->getStartTimestamp().toTime_t();
    float   ascLast  = 0;
    float   dscLast  = 0;

    stages.clear();

    foreach(const CTrack::wpt_t& wpt, wpts)
    {

        // n...N-2  entry-------------------------
        if((cnt & 0x1))
        {
            for(int i = eSym; i < eMax; i++)
            {
                table->cellAt(cnt,i).setFormat(fmtCharShade);
            }
        }

        // prepare data -------------------------------
        QString comment = wpt.wpt->getComment();
        IItem::removeHtml(comment);

        QString proximity;
        if(wpt.wpt->prx != WPT_NOFLOAT)
        {
            QString val, unit;
            IUnit::self().meter2distance(wpt.wpt->prx, val, unit);
            proximity = tr("%1 %2").arg(val).arg(unit);
        }
        else
        {
            proximity = "-";
        }

        QString eleWpt, eleTrk;
        {
            QString valWpt, unitWpt, valTrk, unitTrk;
            if(wpt.wpt->ele != WPT_NOFLOAT)
            {
                IUnit::self().meter2elevation(wpt.wpt->ele, valWpt, unitWpt);
            }
            else
            {
                valWpt = "-";
            }
            if(wpt.trkpt.ele != WPT_NOFLOAT)
            {
                IUnit::self().meter2elevation(wpt.trkpt.ele, valTrk, unitTrk);
            }
            else
            {
                valTrk = "-";
            }

            eleWpt = tr("%1 %2").arg(valWpt).arg(unitWpt);
            eleTrk = tr("%1 %2").arg(valTrk).arg(unitTrk);
        }
        QString val,unit;

        IUnit::self().meter2distance(wpt.trkpt.distance - distLast, val, unit);
        QString strDistToNext = tr("%1 %2").arg(val).arg(unit);

        IUnit::self().meter2distance(wpt.trkpt.distance, val, unit);
        QString strDistTotal  = tr("%1 %2").arg(val).arg(unit);

        IUnit::self().meter2elevation(wpt.trkpt.ascend - ascLast, val, unit);
        QString strAscToNext  = tr("%1 %2 ").arg(val).arg(unit);
        IUnit::self().meter2elevation(wpt.trkpt.descend - dscLast, val, unit);
        QString strDescToNext = tr("%1 %2").arg(val).arg(unit);

        IUnit::self().meter2elevation(wpt.trkpt.ascend, val, unit);
        QString strAscTotal   = tr("%1 %2 ").arg(val).arg(unit);
        IUnit::self().meter2elevation(wpt.trkpt.descend, val, unit);
        QString strDescTotal  = tr("%1 %2").arg(val).arg(unit);

        QString strTimeToNext, strTimeTotal;
        quint32 timestamp = wpt.trkpt.timestamp;
        if(timeLast && timestamp)
        {
            quint32 t1s     = timestamp - timeLast;
            quint32 t1h     = qreal(t1s)/3600;
            quint32 t1m     = quint32(qreal(t1s - t1h * 3600)/60  + 0.5);
            strTimeToNext   = tr("%1:%2 h").arg(t1h).arg(t1m, 2, 10, QChar('0'));

            quint32 t2s     = timestamp - track->getStartTimestamp().toTime_t();
            quint32 t2h     = qreal(t2s)/3600;
            quint32 t2m     = quint32(qreal(t2s - t2h * 3600)/60  + 0.5);
            strTimeTotal    = tr("%1:%2 h").arg(t2h).arg(t2m, 2, 10, QChar('0'));

            timeLast = timestamp;
        }

        // fill in data -------------------------------
        table->cellAt(cnt ,eSym).firstCursorPosition().insertImage(wpt.wpt->getIcon().toImage().scaledToWidth(16, Qt::SmoothTransformation));
        table->cellAt(cnt ,eInfo).firstCursorPosition().insertText(wpt.wpt->getName(), fmtCharStandard);

        if(!wpt.wpt->images.isEmpty())
        {
            table->cellAt(cnt ,ePic).firstCursorPosition().setBlockFormat(fmtBlockCenter);
            table->cellAt(cnt, ePic).firstCursorPosition().insertImage(wpt.wpt->images.first().pixmap.scaledToWidth(32, Qt::SmoothTransformation).toImage());
        }

        table->cellAt(cnt ,eProx).firstCursorPosition().setBlockFormat(fmtBlockRight);
        table->cellAt(cnt ,eProx).firstCursorPosition().insertText(proximity, fmtCharStandard);
        table->cellAt(cnt ,eEleWpt).firstCursorPosition().setBlockFormat(fmtBlockRight);
        table->cellAt(cnt ,eEleWpt).firstCursorPosition().insertText(eleWpt, fmtCharStandard);
        table->cellAt(cnt ,eEleTrk).firstCursorPosition().setBlockFormat(fmtBlockRight);
        table->cellAt(cnt ,eEleTrk).firstCursorPosition().insertText(eleTrk, fmtCharStandard);

        table->cellAt(cnt - 1,eToNextDist).firstCursorPosition().setBlockFormat(fmtBlockRight);
        table->cellAt(cnt - 1,eToNextDist).firstCursorPosition().insertText(strDistToNext, fmtCharStandard);
        table->cellAt(cnt - 1,eToNextTime).firstCursorPosition().setBlockFormat(fmtBlockRight);
        table->cellAt(cnt - 1,eToNextTime).firstCursorPosition().insertText(strTimeToNext, fmtCharStandard);
        table->cellAt(cnt - 1,eToNextAsc).firstCursorPosition().setBlockFormat(fmtBlockRight);
        table->cellAt(cnt - 1,eToNextAsc).firstCursorPosition().insertText(strAscToNext, fmtCharStandard);
        table->cellAt(cnt - 1,eToNextDesc).firstCursorPosition().setBlockFormat(fmtBlockRight);
        table->cellAt(cnt - 1,eToNextDesc).firstCursorPosition().insertText(strDescToNext, fmtCharStandard);

        table->cellAt(cnt ,eTotalDist).firstCursorPosition().setBlockFormat(fmtBlockRight);
        table->cellAt(cnt ,eTotalDist).firstCursorPosition().insertText(strDistTotal, fmtCharStandard);
        table->cellAt(cnt ,eTotalTime).firstCursorPosition().setBlockFormat(fmtBlockRight);
        table->cellAt(cnt ,eTotalTime).firstCursorPosition().insertText(strTimeTotal, fmtCharStandard);
        table->cellAt(cnt ,eTotalAsc).firstCursorPosition().setBlockFormat(fmtBlockRight);
        table->cellAt(cnt ,eTotalAsc).firstCursorPosition().insertText(strAscTotal, fmtCharStandard);
        table->cellAt(cnt ,eTotalDesc).firstCursorPosition().setBlockFormat(fmtBlockRight);
        table->cellAt(cnt ,eTotalDesc).firstCursorPosition().insertText(strDescTotal, fmtCharStandard);

        table->cellAt(cnt ,eComment).firstCursorPosition().insertText(comment, fmtCharStandard);

        stage.x1 = distLast;
        stage.x2 = wpt.trkpt.distance;
        stages[QString("stage%1").arg(cnt - 3)] = stage;

        distLast    = wpt.trkpt.distance;
        ascLast     = wpt.trkpt.ascend;
        dscLast     = wpt.trkpt.descend;

        cnt++;
    }
    // n...N-2  entry-------------------------
    if((cnt & 0x1))
    {
        for(int i = eSym; i < eMax; i++)
        {
            table->cellAt(cnt,i).setFormat(fmtCharShade);
        }
    }

    // prepare data -------------------------------

    QString eleTrk;
    {
        QString valTrk, unitTrk;
        if(track->getEndElevation() != WPT_NOFLOAT)
        {
            IUnit::self().meter2elevation(track->getEndElevation(), valTrk, unitTrk);
        }
        else
        {
            valTrk = "-";
        }

        eleTrk = tr("%1 %2").arg(valTrk).arg(unitTrk);
    }

    IUnit::self().meter2distance(track->getTotalDistance() - distLast, val, unit);
    QString strDistToNext = tr("%1 %2").arg(val).arg(unit);

    IUnit::self().meter2distance(track->getTotalDistance(), val, unit);
    QString strDistTotal  = tr("%1 %2").arg(val).arg(unit);

    IUnit::self().meter2elevation(track->getAscend() - ascLast, val, unit);
    QString strAscToNext  = tr("%1 %2 ").arg(val).arg(unit);
    IUnit::self().meter2elevation(track->getDescend() - dscLast, val, unit);
    QString strDescToNext = tr("%1 %2").arg(val).arg(unit);

    IUnit::self().meter2elevation(track->getAscend(), val, unit);
    QString strAscTotal   = tr("%1 %2 ").arg(val).arg(unit);
    IUnit::self().meter2elevation(track->getDescend(), val, unit);
    QString strDescTotal  = tr("%1 %2").arg(val).arg(unit);

    QString strTimeToNext, strTimeTotal;
    quint32 timestamp = track->getEndTimestamp().toTime_t();
    if(timeLast && timestamp)
    {
        quint32 t1s     = timestamp - timeLast;
        quint32 t1h     = qreal(t1s)/3600;
        quint32 t1m     = quint32(qreal(t1s - t1h * 3600)/60  + 0.5);
        strTimeToNext   = tr("%1:%2 h").arg(t1h).arg(t1m, 2, 10, QChar('0'));

        quint32 t2s     = timestamp - track->getStartTimestamp().toTime_t();
        quint32 t2h     = qreal(t2s)/3600;
        quint32 t2m     = quint32(qreal(t2s - t2h * 3600)/60  + 0.5);
        strTimeTotal    = tr("%1:%2 h").arg(t2h).arg(t2m, 2, 10, QChar('0'));

        timeLast = timestamp;
    }

    // fill in data -------------------------------
    table->cellAt(cnt ,eSym).firstCursorPosition().insertImage(":/icons/face-laugh.png");
    table->cellAt(cnt ,eInfo).firstCursorPosition().insertText(tr("End"), fmtCharStandard);

    table->cellAt(cnt ,eProx).firstCursorPosition().setBlockFormat(fmtBlockRight);
    table->cellAt(cnt ,eProx).firstCursorPosition().insertText(tr("-"), fmtCharStandard);
    table->cellAt(cnt ,eEleWpt).firstCursorPosition().setBlockFormat(fmtBlockRight);
    table->cellAt(cnt ,eEleWpt).firstCursorPosition().insertText(tr("-"), fmtCharStandard);
    table->cellAt(cnt ,eEleTrk).firstCursorPosition().setBlockFormat(fmtBlockRight);
    table->cellAt(cnt ,eEleTrk).firstCursorPosition().insertText(eleTrk, fmtCharStandard);

    table->cellAt(cnt - 1,eToNextDist).firstCursorPosition().setBlockFormat(fmtBlockRight);
    table->cellAt(cnt - 1,eToNextDist).firstCursorPosition().insertText(strDistToNext, fmtCharStandard);
    table->cellAt(cnt - 1,eToNextTime).firstCursorPosition().setBlockFormat(fmtBlockRight);
    table->cellAt(cnt - 1,eToNextTime).firstCursorPosition().insertText(strTimeToNext, fmtCharStandard);
    table->cellAt(cnt - 1,eToNextAsc).firstCursorPosition().setBlockFormat(fmtBlockRight);
    table->cellAt(cnt - 1,eToNextAsc).firstCursorPosition().insertText(strAscToNext, fmtCharStandard);
    table->cellAt(cnt - 1,eToNextDesc).firstCursorPosition().setBlockFormat(fmtBlockRight);
    table->cellAt(cnt - 1,eToNextDesc).firstCursorPosition().insertText(strDescToNext, fmtCharStandard);

    table->cellAt(cnt ,eToNextDist).firstCursorPosition().setBlockFormat(fmtBlockRight);
    table->cellAt(cnt ,eToNextDist).firstCursorPosition().insertText(tr("-"), fmtCharStandard);
    table->cellAt(cnt ,eToNextTime).firstCursorPosition().setBlockFormat(fmtBlockRight);
    table->cellAt(cnt ,eToNextTime).firstCursorPosition().insertText(tr("-"), fmtCharStandard);
    table->cellAt(cnt ,eToNextAsc).firstCursorPosition().setBlockFormat(fmtBlockRight);
    table->cellAt(cnt ,eToNextAsc).firstCursorPosition().insertText(tr("-"), fmtCharStandard);
    table->cellAt(cnt ,eToNextDesc).firstCursorPosition().setBlockFormat(fmtBlockRight);
    table->cellAt(cnt ,eToNextDesc).firstCursorPosition().insertText(tr("-"), fmtCharStandard);

    table->cellAt(cnt ,eTotalDist).firstCursorPosition().setBlockFormat(fmtBlockRight);
    table->cellAt(cnt ,eTotalDist).firstCursorPosition().insertText(strDistTotal, fmtCharStandard);
    table->cellAt(cnt ,eTotalTime).firstCursorPosition().setBlockFormat(fmtBlockRight);
    table->cellAt(cnt ,eTotalTime).firstCursorPosition().insertText(strTimeTotal, fmtCharStandard);
    table->cellAt(cnt ,eTotalAsc).firstCursorPosition().setBlockFormat(fmtBlockRight);
    table->cellAt(cnt ,eTotalAsc).firstCursorPosition().insertText(strAscTotal, fmtCharStandard);
    table->cellAt(cnt ,eTotalDesc).firstCursorPosition().setBlockFormat(fmtBlockRight);
    table->cellAt(cnt ,eTotalDesc).firstCursorPosition().insertText(strDescTotal, fmtCharStandard);

    table->cellAt(cnt ,eComment).firstCursorPosition().insertText(tr("End of track."), fmtCharStandard);

    stage.x1 = distLast;
    stage.x2 = track->getTotalDistance();
    stages[QString("stage%1").arg(cnt - 3)] = stage;

    textStages->setDocument(doc);

    const int N = cnt;
    textStages->resetAreas();
    for(cnt = 2; cnt < N; cnt++)
    {
        QRect rect1 = textStages->cursorRect(table->cellAt(cnt, eToNextDist).firstCursorPosition());
        QRect rect2 = textStages->cursorRect(table->cellAt(cnt, eToNextDesc).lastCursorPosition());
        QRect rect(rect1.x(), rect1.y(), rect2.x() - rect1.x(), rect2.bottom() - rect1.top());
        rect.adjust(-5,-5,5,5);
        textStages->addArea(QString("stage%1").arg(cnt - 2), rect);
    }

    QApplication::restoreOverrideCursor();
}
