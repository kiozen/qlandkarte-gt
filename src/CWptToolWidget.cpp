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

#include "CWptToolWidget.h"
#include "WptIcons.h"
#include "CWpt.h"
#include "CWptDB.h"
#include "CRoute.h"
#include "CRouteDB.h"
#include "CMainWindow.h"
#include "CCanvas.h"
#include "CDlgEditWpt.h"
#include "CDlgDelWpt.h"
#include "GeoMath.h"
#include "IUnit.h"
#include "CMapDB.h"
#include "CDlgWpt2Rte.h"
#include "CDlgWptIcon.h"
#include "CDlgParentWpt.h"
#include "CSettings.h"

#include <QtGui>
#include <QMenu>
#include <QInputDialog>

CWptToolWidget::sortmode_e CWptToolWidget::sortmode = CWptToolWidget::eSortByName;
QString CWptToolWidget::sortpos;

CWptToolWidget::CWptToolWidget(QTabWidget * parent)
: QWidget(parent)
{
    setupUi(this);
    setObjectName("Waypoints");
    parent->addTab(this,QIcon(":/icons/iconWaypoint16x16.png"),"");
    parent->setTabToolTip(parent->indexOf(this), tr("Waypoints"));

    connect(&CWptDB::self(), SIGNAL(sigChanged()), this, SLOT(slotDBChanged()));
    connect(&CWptDB::self(), SIGNAL(sigModified(QString)), this, SLOT(slotDBChanged()));
    connect(listWpts,SIGNAL(itemDoubleClicked(QListWidgetItem*)),this,SLOT(slotItemDoubleClicked(QListWidgetItem*)));
    connect(listWpts,SIGNAL(itemClicked(QListWidgetItem*)),this,SLOT(slotItemClicked(QListWidgetItem*)));

    listWpts->setSortingEnabled(false);

    contextMenu     = new QMenu(this);
    actEdit         = contextMenu->addAction(QPixmap(":/icons/iconEdit16x16.png"),tr("Edit..."),this,SLOT(slotEdit()));
    actCopyPos      = contextMenu->addAction(QPixmap(":/icons/iconClipboard16x16.png"),tr("Copy Position"),this,SLOT(slotCopyPosition()),Qt::CTRL + Qt::Key_C);
    contextMenu->addSeparator();
    actProximity    = contextMenu->addAction(QPixmap(":/icons/iconProximity16x16.png"),tr("Proximity ..."),this,SLOT(slotProximity()));
    actIcon         = contextMenu->addAction(QPixmap(":/icons/iconWaypoint16x16.png"),tr("Icon ..."),this,SLOT(slotIcon()));
    actParentWpt    = contextMenu->addAction(QPixmap(":/icons/iconWaypoint16x16.png"),tr("Parent Waypoint ..."),this,SLOT(slotParentWpt()));
    actMakeRte      = contextMenu->addAction(QPixmap(":/icons/iconRoute16x16.png"),tr("Make Route ..."),this,SLOT(slotMakeRoute()));
    actResetSel     = contextMenu->addAction(QPixmap(":/icons/iconClear16x16.png"),tr("Reset selection"),this,SLOT(slotResetSel()));
    contextMenu->addSeparator();
    actShowNames    = contextMenu->addAction(tr("Show Names"),this,SLOT(slotShowNames()));
    contextMenu->addSeparator();
    actZoomToFit    = contextMenu->addAction(QPixmap(":/icons/iconZoomArea16x16.png"),tr("Zoom to fit"),this,SLOT(slotZoomToFit()));
    actDelete       = contextMenu->addAction(QPixmap(":/icons/iconClear16x16.png"),tr("Delete"),this,SLOT(slotDelete()),Qt::CTRL + Qt::Key_Delete);
    actDeleteNonSel = contextMenu->addAction(QPixmap(":/icons/iconClear16x16.png"),tr("Delete non-selected"),this,SLOT(slotDeleteNonSel()));
    actDeleteBy     = contextMenu->addAction(QPixmap(":/icons/iconClear16x16.png"),tr("Delete by ..."),this,SLOT(slotDeleteBy()));

    actShowNames->setCheckable(true);
    actShowNames->setChecked(CWptDB::self().getShowNames());

    connect(listWpts,SIGNAL(customContextMenuRequested(const QPoint&)),this,SLOT(slotContextMenu(const QPoint&)));

    toolSortAlpha->setIcon(QPixmap(":/icons/iconDec16x16.png"));
    toolSortComment->setIcon(QPixmap(":/icons/iconText16x16.png"));
    toolSortIcon->setIcon(QPixmap(":/icons/iconWaypoint16x16.png"));
    toolSortPosition->setIcon(QPixmap(":/icons/iconLiveLog16x16.png"));
    toolSortTime->setIcon(QPixmap(":/icons/iconTime16x16.png"));

    connect(toolSortAlpha, SIGNAL(clicked()), SIGNAL(sigChanged()));
    connect(toolSortComment, SIGNAL(clicked()), SIGNAL(sigChanged()));
    connect(toolSortIcon, SIGNAL(clicked()),SIGNAL(sigChanged()));
    connect(toolSortPosition, SIGNAL(clicked()), SIGNAL(sigChanged()));
    connect(toolSortTime, SIGNAL(clicked()), SIGNAL(sigChanged()));

    connect(linePosition, SIGNAL(textChanged(const QString&)), this, SLOT(slotPosTextChanged(const QString&)));

    SETTINGS;
    toolSortAlpha->setChecked(cfg.value("waypoint/sortAlpha", true).toBool());
    toolSortTime->setChecked(cfg.value("waypoint/sortTime", true).toBool());
    toolSortComment->setChecked(cfg.value("waypoint/sortComment", true).toBool());
    toolSortIcon->setChecked(cfg.value("waypoint/sortIcon", false).toBool());
    toolSortPosition->setChecked(cfg.value("waypoint/sortPosition", false).toBool());
    linePosition->setText(cfg.value("waypoint/position",tr("enter valid position")).toString());

}


CWptToolWidget::~CWptToolWidget()
{
    SETTINGS;
    cfg.setValue("waypoint/sortAlpha", toolSortAlpha->isChecked());
    cfg.setValue("waypoint/sortTime", toolSortTime->isChecked());
    cfg.setValue("waypoint/sortComment", toolSortComment->isChecked());
    cfg.setValue("waypoint/sortIcon", toolSortIcon->isChecked());
    cfg.setValue("waypoint/sortPosition", toolSortPosition->isChecked());
    cfg.setValue("waypoint/position", linePosition->text());
}


void CWptToolWidget::collectSelectedWaypoints(QList<CWpt*>& selWpts)
{
    CWptDB::keys_t key;
    QList<CWptDB::keys_t> keys = CWptDB::self().keys();

    foreach(key, keys)
    {
        CWpt * wpt = CWptDB::self().getWptByKey(key.key);

        if(wpt->selected)
        {
            selWpts << wpt;
        }
    }
}


void CWptToolWidget::keyPressEvent(QKeyEvent * e)
{
    if(e->key() == Qt::Key_Delete)
    {
        slotDelete();
        e->accept();
    }
    else if(e->key() == Qt::Key_C && e->modifiers() == Qt::ControlModifier)
    {
        slotCopyPosition();
    }
    else
    {
        QWidget::keyPressEvent(e);
    }
}


void CWptToolWidget::slotDBChanged()
{
    listWpts->clear();

    CWptDB::keys_t key;

    sortpos  = linePosition->text();

    if(toolSortAlpha->isChecked())
    {
        sortmode = eSortByName;
    }
    else if(toolSortTime->isChecked())
    {
        sortmode = eSortByTime;
    }
    else if(toolSortComment->isChecked())
    {
        sortmode = eSortByComment;
    }
    else if(toolSortIcon->isChecked())
    {
        sortmode = eSortByIcon;
    }
    else if(toolSortPosition->isChecked())
    {
        sortmode = eSortByDistance;
    }
    else
    {
        sortmode = eSortByName;
    }

    QList<CWptDB::keys_t> keys = CWptDB::self().keys();

    foreach(key, keys)
    {
        QString name;
        CWpt * wpt = CWptDB::self().getWptByKey(key.key);

        QListWidgetItem * item = new QListWidgetItem(listWpts);
        item->setIcon(wpt->getIcon().scaled(16,16,Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        item->setData(Qt::UserRole, wpt->getKey());

        if(toolSortComment->isChecked())
        {
            name = key.comment;

            if(name.isEmpty())
            {
                name = key.name;
            }
        }
        else
        {
            name = key.name;
        }

        if(wpt->sticky)
        {
            name += tr(" (sticky)");
        }

        if(toolSortPosition->isChecked())
        {
            QString val, unit;
            IUnit::self().meter2distance(key.d, val, unit);
            item->setText(QString("%1 (%2 %3)").arg(name, val, unit));
        }
        else
        {
            item->setText(name);
        }

        item->setToolTip(wpt->getInfo());
        item->setCheckState(wpt->selected ? Qt::Checked : Qt::Unchecked);
    }
}


void CWptToolWidget::slotItemDoubleClicked(QListWidgetItem* item)
{
    CWpt * wpt = CWptDB::self().getWptByKey(item->data(Qt::UserRole).toString());
    if(wpt)
    {
        theMainWindow->getCanvas()->move(wpt->lon, wpt->lat);
    }
}


void CWptToolWidget::slotItemClicked(QListWidgetItem* item)
{
    CWpt * wpt = CWptDB::self().getWptByKey(item->data(Qt::UserRole).toString());
    if(wpt)
    {
        wpt->selected = item->checkState() == Qt::Checked;
    }
}


void CWptToolWidget::slotContextMenu(const QPoint& pos)
{
    QList<CWpt*> selWpts;
    collectSelectedWaypoints(selWpts);
    actDeleteNonSel->setEnabled(!selWpts.isEmpty());

    int cnt = listWpts->selectedItems().count();
    if(cnt > 0)
    {
        if(cnt > 1)
        {
            actCopyPos->setEnabled(false);
            actEdit->setEnabled(false);
        }
        else
        {
            actCopyPos->setEnabled(true);
            actEdit->setEnabled(true);
        }

        QPoint p = listWpts->mapToGlobal(pos);
        contextMenu->exec(p);
    }
}


void CWptToolWidget::slotEdit()
{
    CWpt * wpt = CWptDB::self().getWptByKey(listWpts->currentItem()->data(Qt::UserRole).toString());
    if(wpt)
    {
        CDlgEditWpt dlg(*wpt,this);
        dlg.exec();
    }
}


void CWptToolWidget::slotDelete()
{
    QStringList keys;
    QList<CWpt*> selWpts;
    collectSelectedWaypoints(selWpts);

    if(selWpts.isEmpty())
    {

        QListWidgetItem * item;
        const QList<QListWidgetItem*>& items = listWpts->selectedItems();
        foreach(item,items)
        {
            keys << item->data(Qt::UserRole).toString();
        }
    }
    else
    {
        foreach(CWpt* wpt, selWpts)
        {
            keys << wpt->getKey();
        }
    }
    CWptDB::self().delWpt(keys, false);
}


void CWptToolWidget::slotDeleteNonSel()
{
    QStringList nonSelWpts;
    CWptDB::keys_t key;
    QList<CWptDB::keys_t> keys = CWptDB::self().keys();

    foreach(key, keys)
    {
        CWpt * wpt = CWptDB::self().getWptByKey(key.key);

        if(!wpt->selected)
        {
            nonSelWpts << wpt->getKey();
        }
    }

    CWptDB::self().delWpt(nonSelWpts);
}


void CWptToolWidget::slotDeleteBy()
{
    CDlgDelWpt dlg(this);
    dlg.exec();
}


void CWptToolWidget::slotCopyPosition()
{
    QListWidgetItem * item = listWpts->currentItem();
    if(item == 0) return;
    CWpt * wpt = CWptDB::self().getWptByKey(item->data(Qt::UserRole).toString());
    if(wpt == 0) return;

    QString position;
    GPS_Math_Deg_To_Str(wpt->lon, wpt->lat, position);

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(position);

}


void CWptToolWidget::slotZoomToFit()
{

    QStringList keys;
    QList<QListWidgetItem*> items = listWpts->selectedItems();
    QListWidgetItem* item;
    foreach(item, items)
    {
        keys << item->data(Qt::UserRole).toString();
    }
    CWptDB::self().makeVisible(keys);
}


void CWptToolWidget::selWptByKey(const QStringList& keys)
{

    qDebug() << keys;

    QListWidgetItem * item = 0;
    listWpts->setUpdatesEnabled(false);
    for(int i=0; i<listWpts->count(); ++i)
    {
        QListWidgetItem * item1 = listWpts->item(i);
        if(!item1)
        {
            continue;
        }

        QString key = item1->data(Qt::UserRole).toString();

        if(keys.contains(key))
        {
            CWpt * wpt = CWptDB::self().getWptByKey(key);
            if(wpt)
            {
                item1->setCheckState(wpt->selected ? Qt::Checked : Qt::Unchecked);
                item = item1;
            }

        }
    }
    listWpts->setUpdatesEnabled(true);
    if(item)
    {
        listWpts->setCurrentItem(item);
    }

    listWpts->update();
}


void CWptToolWidget::slotProximity()
{
    bool ok         = false;
    QString str    = tr("Distance [%1]").arg(IUnit::self().baseunit);
    const QList<QListWidgetItem*>& items = listWpts->selectedItems();
    QListWidgetItem * item;
    double prx = 0.0;
    // If no item is selected, or multiple items, don't try to provide
    // an initial value to the dialog.
    if (items.count() == 1)
    {
        item = items.first();
        QString key = item->data(Qt::UserRole).toString();
        CWpt *pt    = CWptDB::self().getWptByKey(key);

        if (pt->prx != WPT_NOFLOAT)
        {
            prx  = pt->prx * IUnit::self().basefactor;
        }
    }
    double dist     = QInputDialog::getDouble(0,tr("Proximity distance ..."), str, prx, 0, 2147483647, 0,&ok);
    if(ok)
    {
        str = QString("%1 %2").arg(dist).arg(IUnit::self().baseunit);
        dist = IUnit::self().str2distance(str);

        QStringList keys;
        QList<CWpt*> selWpts;
        collectSelectedWaypoints(selWpts);

        if(selWpts.count())
        {
            foreach(CWpt* wpt, selWpts)
            {
                keys << wpt->getKey();
            }
        }
        else
        {
            foreach(item,items)
            {
                keys << item->data(Qt::UserRole).toString();
            }
        }
        CWptDB::self().setProxyDistance(keys,(dist == 0 ? WPT_NOFLOAT : dist));
    }
}


void CWptToolWidget::slotIcon()
{
    QToolButton button(this);
    button.hide();

    CDlgWptIcon dlg(button);
    dlg.exec();

    if(!button.objectName().isEmpty())
    {
        QStringList keys;
        QList<CWpt*> selWpts;
        collectSelectedWaypoints(selWpts);

        if(selWpts.count())
        {
            foreach(CWpt* wpt, selWpts)
            {
                keys << wpt->getKey();
            }
        }
        else
        {
            QListWidgetItem * item;
            const QList<QListWidgetItem*>& items = listWpts->selectedItems();

            foreach(item,items)
            {
                keys << item->data(Qt::UserRole).toString();
            }
        }
        CWptDB::self().setIcon(keys,button.objectName());
    }
}


void CWptToolWidget::slotMakeRoute()
{
    QList<CWpt*> selWpts;
    collectSelectedWaypoints(selWpts);

    CDlgWpt2Rte dlg(selWpts);
    dlg.exec();
}


void CWptToolWidget::slotPosTextChanged(const QString& text)
{
    float lon   = 0, lat = 0;
    bool ok     = GPS_Math_Str_To_Deg(text, lon, lat, true);
    toolSortPosition->setEnabled(ok);
    if(ok)
    {
        slotDBChanged();
    }
}


void CWptToolWidget::slotShowNames()
{
    CWptDB::self().setShowNames(!CWptDB::self().getShowNames());
}


void CWptToolWidget::slotResetSel()
{
    CWptDB::keys_t key;
    QList<CWptDB::keys_t> keys = CWptDB::self().keys();

    foreach(key, keys)
    {
        CWpt * wpt = CWptDB::self().getWptByKey(key.key);
        wpt->selected = false;
    }

    slotDBChanged();
    theMainWindow->getCanvas()->update();
}


void CWptToolWidget::slotParentWpt()
{
    QString parentWpt;

    CDlgParentWpt dlg(parentWpt, this);
    dlg.exec();

    if(!parentWpt.isEmpty())
    {
        QStringList keys;
        QList<CWpt*> selWpts;
        collectSelectedWaypoints(selWpts);

        if(selWpts.count())
        {
            foreach(CWpt* wpt, selWpts)
            {
                keys << wpt->getKey();
            }
        }
        else
        {
            QListWidgetItem * item;
            const QList<QListWidgetItem*>& items = listWpts->selectedItems();

            foreach(item,items)
            {
                keys << item->data(Qt::UserRole).toString();
            }
        }

        CWptDB::self().setParentWpt(keys, parentWpt);
    }

}
