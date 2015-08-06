/* -*-mode:c++; c-basic-offset:4; -*- */
/**********************************************************************************************
    Copyright (C) 2010 Albrecht Dre <albrecht.dress@arcor.de>

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

#include "CHelpButton.h"

CHelpDialog::CHelpDialog(QWidget * parent)
: QDialog(parent)
{
    setupUi(this);
    helpIcn->setPixmap(QPixmap(":/icons/iconHelp48x48.png"));

    setWindowFlags(windowFlags()|Qt::WindowStaysOnTopHint);
}


static CHelpDialog *helpPopup = 0;

CHelpButton::CHelpButton(QWidget * parent)
: QToolButton(parent)
{
    this->setIcon(QPixmap(":/icons/iconHelp16x16.png"));
    connect(this, SIGNAL(clicked()), this, SLOT(slotClicked()));
}


void CHelpButton::setHelp(const QString & title, const QString & contents)
{
    m_title = title;
    m_contents = contents;
}


void CHelpButton::slotClicked()
{
    if (helpPopup == 0)
        helpPopup = new CHelpDialog();

    helpPopup->setWindowTitle(m_title);
    helpPopup->setContents(m_contents);
    helpPopup->show();
    helpPopup->raise();
}
