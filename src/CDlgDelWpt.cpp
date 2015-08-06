/**********************************************************************************************
    Copyright (C) 2009 Oliver Eichler oliver.eichler@gmx.de

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

#include "CDlgDelWpt.h"
#include "CWpt.h"
#include "CWptDB.h"
#include "WptIcons.h"

#include <QtGui>

CDlgDelWpt::CDlgDelWpt(QWidget * parent)
: QDialog(parent)
{
    setupUi(this);

    const QMap<QString,CWpt*>& wpts         = CWptDB::self().getWpts();
    QMap<QString,CWpt*>::const_iterator wpt = wpts.begin();
    QSet<QString> types;

    while(wpt != wpts.end())
    {
        types << (*wpt)->getIconString();
        ++wpt;
    }

    QIcon icon;
    QString type;
    foreach(type, types)
    {
        icon = getWptIconByName(type);
        new QListWidgetItem(icon, type, listWptByType);
    }

}


CDlgDelWpt::~CDlgDelWpt()
{

}


void CDlgDelWpt::accept()
{
    QStringList             keys;
    QStringList             types;
    QListWidgetItem *       item;
    QList<QListWidgetItem*> selTypes = listWptByType->selectedItems();
    foreach(item,selTypes)
    {
        types << item->text();
    }

    const QMap<QString,CWpt*>& wpts         = CWptDB::self().getWpts();
    QMap<QString,CWpt*>::const_iterator wpt = wpts.begin();

    while(wpt != wpts.end())
    {
        if(types.contains((*wpt)->getIconString()))
        {
            keys << (*wpt)->getKey();
        }
        ++wpt;
    }

    CWptDB::self().delWpt(keys, true);

    QDialog::accept();
}
