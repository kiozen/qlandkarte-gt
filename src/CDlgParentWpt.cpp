/**********************************************************************************************
    Copyright (C) 2012 Oliver Eichler oliver.eichler@gmx.de

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

#include "CDlgParentWpt.h"
#include "CWptDB.h"
#include "CWpt.h"

#include <QtGui>

CDlgParentWpt::CDlgParentWpt(QString &name, QWidget *parent)
: QDialog(parent)
, name(name)
{
    setupUi(this);

    const QMap<QString,CWpt*>& wpts = CWptDB::self().getWpts();
    foreach(const CWpt* wpt, wpts)
    {
        if(!wpt->isGeoCache()) continue;

        QListWidgetItem * item = new QListWidgetItem(listParentWpt);
        item->setText(wpt->getName());
        item->setIcon(wpt->getIcon());

    }

    connect(listParentWpt, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(slotItemClicked(QListWidgetItem*)));
}


CDlgParentWpt::~CDlgParentWpt()
{

}


void CDlgParentWpt::slotItemClicked(QListWidgetItem * item)
{
    if(item)
    {
        name = item->text();
        accept();
    }
}
