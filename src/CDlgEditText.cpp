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

#include "CDlgEditText.h"
#include "CTextEditWidget.h"

#include <QtGui>
#include <QDialogButtonBox>

CDlgEditText::CDlgEditText(QString& content, QWidget * parent)
: QDialog(parent)
, content(content)
{
    setObjectName(QString::fromUtf8("IDlgEditText"));
    resize(600, 400);
    vboxLayout = new QVBoxLayout(this);
    vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
    buttonBox = new QDialogButtonBox(this);
    buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::NoButton|QDialogButtonBox::Ok);

    textedit = new CTextEditWidget(this);
    textedit->setHtml(content);

    vboxLayout->addWidget(textedit);
    vboxLayout->addWidget(buttonBox);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}


CDlgEditText::~CDlgEditText()
{

}


void CDlgEditText::accept()
{
    content = textedit->getHtml();
    QDialog::accept();
}
