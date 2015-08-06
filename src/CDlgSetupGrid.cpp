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

#include "CDlgSetupGrid.h"
#include "CDlgProjWizzard.h"
#include "CGridDB.h"
#include "CMapDB.h"

#include <QtGui>
#include <QColorDialog>

CDlgSetupGrid::CDlgSetupGrid(QWidget * parent)
: QDialog(parent)
{
    this->setWindowModality(Qt::WindowModal);
#if defined(Q_OS_MAC)
    this->setParent(qApp->focusWidget());
    this->setWindowFlags(Qt::Sheet);
#endif
    setupUi(this);

    toolProjWizzard->setIcon(QPixmap(":/icons/iconWizzard16x16.png"));
    connect(toolProjWizzard, SIGNAL(clicked()), this, SLOT(slotProjWizard()));
    connect(toolGridColor,SIGNAL(clicked()),this,SLOT(slotSelectGridColor()));

    lineProjection->setText(CGridDB::self().projstr);
    lineProjection->setCursorPosition(0);

    QPalette palette = labelGridColor->palette();
    palette.setColor(labelGridColor->foregroundRole(), CGridDB::self().color);
    labelGridColor->setPalette(palette);

    connect(toolRestoreDefault, SIGNAL(clicked()), this, SLOT(slotRestoreDefault()));
    connect(toolFromMap, SIGNAL(clicked()), this, SLOT(slotProjFromMap()));
}


CDlgSetupGrid::~CDlgSetupGrid()
{

}


void CDlgSetupGrid::accept()
{
    if (CDlgProjWizzard::validProjStr(lineProjection->text()))
    {
        QPalette palette = labelGridColor->palette();
        CGridDB::self().setProjAndColor(lineProjection->text(), palette.color(labelGridColor->foregroundRole()));

        QDialog::accept();
    }
}


void CDlgSetupGrid::slotProjWizard()
{
    CDlgProjWizzard dlg(*lineProjection, this);
    dlg.exec();
}


void CDlgSetupGrid::slotSelectGridColor()
{
    QPalette palette = labelGridColor->palette();
    QColor color = palette.color(labelGridColor->foregroundRole());

    color = QColorDialog::getColor(color);

    if(color.isValid())
    {
        palette.setColor(labelGridColor->foregroundRole(), color);
        labelGridColor->setPalette(palette);
    }

}


void CDlgSetupGrid::slotRestoreDefault()
{
    lineProjection->setText("+proj=longlat +datum=WGS84 +no_defs");
    lineProjection->setCursorPosition(0);
}


void CDlgSetupGrid::slotProjFromMap()
{
    IMap& map = CMapDB::self().getMap();
    lineProjection->setText(map.getProjection());
    lineProjection->setCursorPosition(0);
}
