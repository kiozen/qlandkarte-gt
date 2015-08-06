/**********************************************************************************************
    Copyright (C) 2007 Oliver Eichler oliver.eichler@gmx.de

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
#ifndef CDLGWPTICON_H
#define CDLGWPTICON_H

#include <QDialog>
#include "ui_IDlgWptIcon.h"

class QToolButton;

/// dialog to select waypoint icon
class CDlgWptIcon : public QDialog, private Ui::IDlgWptIcon
{
    Q_OBJECT;
    public:
        CDlgWptIcon(QToolButton& but);
        virtual ~CDlgWptIcon();

    private slots:
        void slotItemClicked(QListWidgetItem * item);

    private:
        QToolButton& button;
};
#endif                           //CDLGWPTICON_H
