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
#ifndef CDLGPARENTWPT_H
#define CDLGPARENTWPT_H

#include <QDialog>

#include "ui_IDlgParentWpt.h"

class CDlgParentWpt : public QDialog, private Ui::IDlgParentWpt
{
    Q_OBJECT;
    public:
        CDlgParentWpt(QString& name, QWidget * parent);
        virtual ~CDlgParentWpt();

    private slots:
        void slotItemClicked(QListWidgetItem * item);

    private:
        QString& name;
};
#endif                           //CDLGPARENTWPT_H
