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

#include "CDlgEditRoute.h"
#include "CRoute.h"
#include "CDlgWptIcon.h"
#include "WptIcons.h"

#include <QtGui>

CDlgEditRoute::CDlgEditRoute(CRoute& rte, QWidget * parent)
: QDialog(parent)
, rte(rte)
{
    setupUi(this);

    connect(toolIcon,SIGNAL(pressed()),this,SLOT(slotSelectIcon()));

    toolIcon->setIcon(rte.getIcon());
    toolIcon->setToolTip(rte.getIconString());
    toolIcon->setObjectName(rte.getIconString());

    lineName->setText(rte.getName());
}


CDlgEditRoute::~CDlgEditRoute()
{

}


void CDlgEditRoute::accept()
{

    rte.setName(lineName->text());
    rte.setIcon(toolIcon->toolTip());

    QDialog::accept();
}


void CDlgEditRoute::slotSelectIcon()
{
    CDlgWptIcon dlg(*toolIcon);
    dlg.exec();
}
