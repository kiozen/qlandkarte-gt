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

#include "CDlgCombineDistOvl.h"
#include "COverlayDB.h"
#include "IOverlay.h"

#include <QtGui>

CDlgCombineDistOvl::CDlgCombineDistOvl(QWidget * parent)
: QDialog(parent)
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

    connect(listSelOverlays, SIGNAL(itemSelectionChanged()), this, SLOT(slotItemSelectionChanged()));

    QMap<QString, IOverlay*>::const_iterator ovl = COverlayDB::self().begin();
    while(ovl != COverlayDB::self().end())
    {
        COverlayDistance * dist = qobject_cast<COverlayDistance*>(*ovl);
        if(dist)
        {

            QListWidgetItem * item = new QListWidgetItem(listOverlays);
            item->setText(dist->getName());
            item->setData(Qt::UserRole, dist->getKey());
        }
        ovl++;
    }

}


CDlgCombineDistOvl::~CDlgCombineDistOvl()
{

}


void CDlgCombineDistOvl::slotAdd()
{
    QListWidgetItem * item;
    QList<QListWidgetItem*> items = listOverlays->selectedItems();

    foreach(item, items)
    {
        listSelOverlays->addItem(listOverlays->takeItem(listOverlays->row(item)));
    }
}


void CDlgCombineDistOvl::slotDel()
{
    QListWidgetItem * item;
    QList<QListWidgetItem*> items = listSelOverlays->selectedItems();

    foreach(item, items)
    {
        listOverlays->addItem(listSelOverlays->takeItem(listSelOverlays->row(item)));
    }
}


void CDlgCombineDistOvl::accept()
{
    QListWidgetItem * item;
    QList<QListWidgetItem*> items = listSelOverlays->findItems("*",Qt::MatchWildcard);

    if(items.isEmpty() || lineOverlayName->text().isEmpty()) return;

    QList<COverlayDistance::pt_t> points;
    foreach(item, items)
    {
        IOverlay * ovl          = COverlayDB::self().getOverlayByKey(item->data(Qt::UserRole).toString());
        COverlayDistance * dist = qobject_cast<COverlayDistance*>(ovl);

        if(dist)
        {
            points += dist->getPoints();
        }

    }

    COverlayDB::self().addDistance(lineOverlayName->text(), "", 0.0, points);

    QDialog::accept();
}


void CDlgCombineDistOvl::slotItemSelectionChanged ()
{
    if(listSelOverlays->currentItem() == 0)
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


void CDlgCombineDistOvl::slotUp()
{
    QListWidgetItem * item = listSelOverlays->currentItem();
    if(item)
    {
        int row = listSelOverlays->row(item);
        if(row == 0) return;
        listSelOverlays->takeItem(row);
        row = row - 1;
        listSelOverlays->insertItem(row,item);
        listSelOverlays->setCurrentItem(item);
    }
}


void CDlgCombineDistOvl::slotDown()
{
    QListWidgetItem * item = listSelOverlays->currentItem();
    if(item)
    {
        int row = listSelOverlays->row(item);
        if(row == (listSelOverlays->count() - 1)) return;
        listSelOverlays->takeItem(row);
        row = row + 1;
        listSelOverlays->insertItem(row,item);
        listSelOverlays->setCurrentItem(item);
    }
}
