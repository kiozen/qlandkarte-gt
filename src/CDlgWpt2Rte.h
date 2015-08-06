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

#ifndef CDLGWPT2RTE_H
#define CDLGWPT2RTE_H

#include <QDialog>
#include <QList>
#include "ui_IDlgWpt2Rte.h"

class CWpt;

class CDlgWpt2Rte  : public QDialog, private Ui::IDlgWpt2Rte
{
    Q_OBJECT;
    public:
        CDlgWpt2Rte(QList<CWpt*>& selWpt);
        virtual ~CDlgWpt2Rte();

    public slots:
        void accept();

    private slots:
        void slotAdd();
        void slotDel();
        void slotUp();
        void slotDown();
        void slotItemSelectionChanged();

    private:
        QList<CWpt*>& selWpt;
};
#endif                           //CDLGWPT2RTE_H
