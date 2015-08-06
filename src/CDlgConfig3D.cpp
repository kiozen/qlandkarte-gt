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

#include "CDlgConfig3D.h"
#include "CMap3D.h"

#include <QtGui>

CDlgConfig3D::CDlgConfig3D(CMap3D& view3D)
: QDialog(&view3D)
, view3D(view3D)
{
    setupUi(this);
    comboQuality->addItem(tr("fine"),CMap3D::eFine);
    comboQuality->addItem(tr("medium"),CMap3D::eMedium);
    comboQuality->addItem(tr("coarse"),CMap3D::eCoarse);
}


CDlgConfig3D::~CDlgConfig3D()
{

}


#ifdef QK_QT5_PORT
int CDlgConfig3D::exec()
#else
void CDlgConfig3D::exec()
#endif
{
    comboQuality->setCurrentIndex(comboQuality->findData(view3D.quality3D));
    checkElePov->setChecked(view3D.coupleElePOV);
#ifdef QK_QT5_PORT
    return QDialog::exec();
#else
    QDialog::exec();
#endif
}


void CDlgConfig3D::accept()
{
    view3D.quality3D = (CMap3D::EQuality3D)comboQuality->itemData(comboQuality->currentIndex()).toInt();
    view3D.coupleElePOV = checkElePov->isChecked();

    QDialog::accept();
}
