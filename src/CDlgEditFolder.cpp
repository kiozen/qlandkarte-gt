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

#include "CDlgEditFolder.h"
#include "CGeoDB.h"

#include <QtGui>

CDlgEditFolder::CDlgEditFolder(QString& name, QString& comment, int& type)
: name(name)
, comment(comment)
, type(type)
{
    setupUi(this);

    lineName->setText(name);
    lineComment->setText(comment);

    comboType->addItem(QIcon(":/icons/iconFolderBlue16x16"), tr("Group"), CGeoDB::eFolder1);
    comboType->addItem(QIcon(":/icons/iconFolderGreen16x16"), tr("Project"), CGeoDB::eFolder2);
    comboType->addItem(QIcon(":/icons/iconFolderOrange16x16"), tr("Other data"), CGeoDB::eFolderN);

    comboType->setCurrentIndex(comboType->findData(type));
}


CDlgEditFolder::~CDlgEditFolder()
{

}


void CDlgEditFolder::accept()
{
    name    = lineName->text();
    comment = lineComment->text();
    type    = comboType->itemData(comboType->currentIndex()).toInt();

    QDialog::accept();
}
