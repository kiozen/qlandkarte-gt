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
#include "CDlgNoMapConfig.h"
#include "CMapNoMap.h"
#include "CDlgProjWizzard.h"

#include <QtGui>

CDlgNoMapConfig::CDlgNoMapConfig(CMapNoMap &map)
: map(map)
{
    setupUi(this);

    connect(toolProjWizzard, SIGNAL(clicked()), this, SLOT(slotProjWizard()));
    connect(toolRestoreDefault, SIGNAL(clicked()), this, SLOT(slotRestoreDefault()));

    lineProjection->setText(map.getProjection());
    lineProjection->setCursorPosition(0);

    lineXScale->setText(QString::number(  map.xscale, 'f'));
    lineYScale->setText(QString::number(- map.yscale, 'f'));
}


CDlgNoMapConfig::~CDlgNoMapConfig()
{

}


void CDlgNoMapConfig::accept()
{
    if (CDlgProjWizzard::validProjStr(lineProjection->text()))
    {
        map.setup(lineProjection->text(), lineXScale->text().toDouble(), -lineYScale->text().toDouble());

        QDialog::accept();
    }
}


void CDlgNoMapConfig::slotRestoreDefault()
{
    lineProjection->setText("+proj=merc +a=6378137.0000 +b=6356752.3142 +towgs84=0,0,0,0,0,0,0,0 +units=m  +no_defs");
    lineProjection->setCursorPosition(0);
}


void CDlgNoMapConfig::slotProjWizard()
{
    CDlgProjWizzard dlg(*lineProjection, this);
    dlg.exec();
}
