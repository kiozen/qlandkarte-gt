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

#include "CDlgExport.h"
#include "IDevice.h"
#include "CWptDB.h"
#include "CWpt.h"
#include "CWptToolWidget.h"
#include "CTrackDB.h"
#include "CTrack.h"
#include "CRouteDB.h"
#include "CRoute.h"
#include "WptIcons.h"
#include "GeoMath.h"

#include <QtGui>
#include <proj_api.h>

CDlgExport::CDlgExport(QWidget * parent, QStringList * wpt, QStringList * trk, QStringList * rte)
: QDialog(parent)
, keysWpt(wpt)
, keysTrk(trk)
, keysRte(rte)
{
    setupUi(this);
    connect(checkAll, SIGNAL(toggled(bool)), this, SLOT(slotCheckAll(bool)));
}


CDlgExport::~CDlgExport()
{

}


int CDlgExport::exec()
{

    itemWpt = new QTreeWidgetItem(treeWidget, QStringList(tr("Waypoints")));
    itemWpt->setIcon(0,QIcon(":/icons/iconWaypoint16x16.png"));
    itemWpt->setExpanded(true);

    itemTrk = new QTreeWidgetItem(treeWidget, QStringList(tr("Tracks")));
    itemTrk->setIcon(0,QIcon(":/icons/iconTrack16x16.png"));
    itemTrk->setExpanded(true);

    itemRte = new QTreeWidgetItem(treeWidget, QStringList(tr("Routes")));
    itemRte->setIcon(0,QIcon(":/icons/iconRoute16x16.png"));
    itemRte->setExpanded(true);

    if(keysWpt)
    {

        CWptDB::keys_t key;
        QList<CWptDB::keys_t> keys = CWptDB::self().keys();

        foreach(key, keys)
        {
            QStringList str;
            str << key.name << key.comment.left(32);
            QTreeWidgetItem * item = new QTreeWidgetItem(itemWpt, str);

            CWpt *wpt = CWptDB::self().getWptByKey(key.key);
            if (wpt->sticky)
            {
                item->setCheckState(0, Qt::Unchecked);
                item->setFlags(0);
            }
            else
            {
                item->setCheckState(0, Qt::Checked);
            }

            item->setIcon(0,getWptIconByName(key.icon));
            item->setData(0, Qt::UserRole, key.key);
        }

    }
    else
    {
        itemWpt->setDisabled(true);
    }

    if(keysTrk)
    {
        CTrackDB::keys_t key;
        QList<CTrackDB::keys_t> keys = CTrackDB::self().keys();

        foreach(key, keys)
        {
            QStringList str;
            str << key.name << key.comment.left(32);
            QTreeWidgetItem * item = new QTreeWidgetItem(itemTrk, str);
            item->setCheckState(0, Qt::Checked);
            item->setIcon(0,key.icon);
            item->setData(0, Qt::UserRole, key.key);
        }
    }
    else
    {
        itemTrk->setDisabled(true);
    }

    if(keysRte)
    {
        CRouteDB::keys_t key;
        QList<CRouteDB::keys_t> keys = CRouteDB::self().keys();

        qSort(keys.begin(), keys.end(), CRouteDB::keyLessThanAlpha);

        foreach(key, keys)
        {
            QStringList str;
            str << key.name;
            QTreeWidgetItem * item = new QTreeWidgetItem(itemRte, str);
            item->setCheckState(0, Qt::Checked);
            item->setIcon(0,key.icon);
            item->setData(0, Qt::UserRole, key.key);
        }
    }
    else
    {
        itemRte->setDisabled(true);
    }

    return QDialog::exec();
}


void CDlgExport::accept()
{

    QTreeWidgetItem * item;
    QList<QTreeWidgetItem*> items;

    if(keysWpt)
    {
        items = itemWpt->takeChildren();
        foreach(item, items)
        {
            if(item->checkState(0) == Qt::Checked)
            {
                *keysWpt << item->data(0, Qt::UserRole).toString();
            }

            delete item;
        }

        if(keysWpt->isEmpty())
        {
            *keysWpt << "dummy";
        }
    }

    if(keysTrk)
    {
        items = itemTrk->takeChildren();
        foreach(item, items)
        {
            if(item->checkState(0) == Qt::Checked)
            {
                *keysTrk << item->data(0, Qt::UserRole).toString();
            }

            delete item;
        }
        if(keysTrk->isEmpty())
        {
            *keysTrk << "dummy";
        }
    }

    if(keysRte)
    {
        items = itemRte->takeChildren();
        foreach(item, items)
        {
            if(item->checkState(0) == Qt::Checked)
            {
                *keysRte << item->data(0, Qt::UserRole).toString();
            }

            delete item;
        }
        if(keysRte->isEmpty())
        {
            *keysRte << "dummy";
        }
    }

    QDialog::accept();
}


void CDlgExport::slotCheckAll(bool checked)
{
    int i;
    int max;

    max = itemWpt->childCount();
    for(i = 0; i < max; i++)
    {
        if ((itemWpt->child(i)->flags() & Qt::ItemIsEnabled) == Qt::ItemIsEnabled)
            itemWpt->child(i)->setCheckState(0, checked ? Qt::Checked : Qt::Unchecked);
    }

    max = itemTrk->childCount();
    for(i = 0; i < max; i++)
    {
        itemTrk->child(i)->setCheckState(0, checked ? Qt::Checked : Qt::Unchecked);
    }

    max = itemRte->childCount();
    for(i = 0; i < max; i++)
    {
        itemRte->child(i)->setCheckState(0, checked ? Qt::Checked : Qt::Unchecked);
    }
}
