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

#include "CDlgWpt2Rte.h"
#include "CWpt.h"
#include "CWptDB.h"
#include "CRoute.h"
#include "CRouteDB.h"
#include "CMegaMenu.h"
#include <QtGui>

CDlgWpt2Rte::CDlgWpt2Rte(QList<CWpt*>& selWpt)
: selWpt(selWpt)
{
    setupUi(this);

    toolAdd->setIcon(QPixmap(":/icons/iconRight16x16.png"));
    connect(toolAdd, SIGNAL(clicked()), this, SLOT(slotAdd()));
    toolDel->setIcon(QPixmap(":/icons/iconLeft16x16.png"));
    connect(toolDel, SIGNAL(clicked()), this, SLOT(slotDel()));

    toolUp->setIcon(QPixmap(":/icons/iconUpload16x16.png"));
    connect(toolUp, SIGNAL(clicked()), this, SLOT(slotUp()));
    toolDown->setIcon(QPixmap(":/icons/iconDownload16x16.png"));
    connect(toolDown, SIGNAL(clicked()), this, SLOT(slotDown()));

    connect(listSelWaypoints, SIGNAL(itemSelectionChanged()), this, SLOT(slotItemSelectionChanged()));

    if(selWpt.count() < 2)
    {
        CWptDB::keys_t key;
        QList<CWptDB::keys_t> keys = CWptDB::self().keys();

        foreach(key, keys)
        {
            QString name;
            CWpt * wpt = CWptDB::self().getWptByKey(key.key);

            QListWidgetItem * item = new QListWidgetItem(listWaypoints);

            if(wpt->isGeoCache())
            {
                item->setText(wpt->getGeocacheData().name + " (" + wpt->getName() + ")");
            }
            else
            {
                item->setText(wpt->getName());
            }

            item->setData(Qt::UserRole, wpt->getKey());
        }
    }
    else
    {
        foreach(CWpt * wpt, selWpt)
        {
            QListWidgetItem * item = new QListWidgetItem(listSelWaypoints);
            if(wpt->isGeoCache())
            {
                item->setText(wpt->getGeocacheData().name + " (" + wpt->getName() + ")");
            }
            else
            {
                item->setText(wpt->getName());
            }

            item->setData(Qt::UserRole, wpt->getKey());
        }
    }
}


CDlgWpt2Rte::~CDlgWpt2Rte()
{

}


void CDlgWpt2Rte::accept()
{
    QList<QListWidgetItem*> items = listSelWaypoints->findItems("*",Qt::MatchWildcard);
    if(items.count() < 2 || lineRouteName->text().isEmpty()) return;

    CRoute * route = new CRoute(&CRouteDB::self());

    QListWidgetItem * item;
    foreach(item,items)
    {
        CWpt * wpt = CWptDB::self().getWptByKey(item->data(Qt::UserRole).toString());
        if(wpt)
        {
            QString description = wpt->getDescription();
            QString comment     = wpt->getComment();
            QString action;

            IItem::removeHtml(description);
            IItem::removeHtml(comment);

            if(!description.isEmpty())
            {
                action = description;
            }
            else if(!comment.isEmpty())
            {
                action = comment;
            }
            else if(!wpt->getName().isEmpty())
            {
                action = wpt->getName();
            }

            route->addPosition(wpt->lon, wpt->lat, action);
        }
    }

    route->setName(lineRouteName->text());

    CRouteDB::self().addRoute(route, false);
    CMegaMenu::self().switchByKeyWord("Routes");

    QDialog::accept();
}


void CDlgWpt2Rte::slotAdd()
{
    QListWidgetItem * item;
    QList<QListWidgetItem*> items = listWaypoints->selectedItems();

    foreach(item, items)
    {
        listSelWaypoints->addItem(listWaypoints->takeItem(listWaypoints->row(item)));
    }
}


void CDlgWpt2Rte::slotDel()
{
    QListWidgetItem * item;
    QList<QListWidgetItem*> items = listSelWaypoints->selectedItems();

    foreach(item, items)
    {
        listWaypoints->addItem(listSelWaypoints->takeItem(listSelWaypoints->row(item)));
    }
}


void CDlgWpt2Rte::slotUp()
{
    QListWidgetItem * item = listSelWaypoints->currentItem();
    if(item)
    {
        int row = listSelWaypoints->row(item);
        if(row == 0) return;
        listSelWaypoints->takeItem(row);
        row = row - 1;
        listSelWaypoints->insertItem(row,item);
        listSelWaypoints->setCurrentItem(item);
    }
}


void CDlgWpt2Rte::slotDown()
{
    QListWidgetItem * item = listSelWaypoints->currentItem();
    if(item)
    {
        int row = listSelWaypoints->row(item);
        if(row == (listSelWaypoints->count() - 1)) return;
        listSelWaypoints->takeItem(row);
        row = row + 1;
        listSelWaypoints->insertItem(row,item);
        listSelWaypoints->setCurrentItem(item);
    }
}


void CDlgWpt2Rte::slotItemSelectionChanged()
{
    if(listSelWaypoints->currentItem() == 0)
    {
        toolUp->setEnabled(false);
        toolDown->setEnabled(false);
    }
    else
    {
        toolUp->setEnabled(true);
        toolDown->setEnabled(true);
    }
}
