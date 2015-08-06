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
#ifndef CDLGEXPORT_H
#define CDLGEXPORT_H

#include <QDialog>
#include "ui_IDlgExport.h"

class QTreeWidgetItem;
class QStringList;

class CDlgExport : public QDialog, private Ui::IDlgExport
{
    Q_OBJECT;
    public:
        CDlgExport(QWidget * parent, QStringList * wpt, QStringList * trk, QStringList * rte);
        virtual ~CDlgExport();

    public slots:
        int exec();
        void accept();

    private slots:
        void slotCheckAll(bool checked);

    private:
        QTreeWidgetItem * itemWpt;
        QTreeWidgetItem * itemTrk;
        QTreeWidgetItem * itemRte;

        QStringList * keysWpt;
        QStringList * keysTrk;
        QStringList * keysRte;

};
#endif                           //CDLGEXPORT_H
