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
#ifndef CTABWIDGET_H
#define CTABWIDGET_H

#include <QTabWidget>

class CTabWidget : public QTabWidget
{
    Q_OBJECT;
    public:
        CTabWidget(QWidget * parent);
        virtual ~CTabWidget();

        void addTab(QWidget * w, const QString& label);

    private slots:
        void slotDestroyChild(QObject * child);
};
#endif                           //CTABWIDGET_H
