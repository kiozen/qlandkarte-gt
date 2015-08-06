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

#include "CTabWidget.h"

#include <QtGui>
#include <QTabBar>

CTabWidget::CTabWidget(QWidget * parent)
: QTabWidget(parent)
{
    tabBar()->hide();
}


CTabWidget::~CTabWidget()
{

}


void CTabWidget::addTab(QWidget * w, const QString& label)
{
    QTabWidget::addTab(w,label);
    setObjectName(label);
    setCurrentWidget(w);

    connect(w, SIGNAL(destroyed(QObject*)), this, SLOT(slotDestroyChild(QObject*)));

    count() > 1 ? tabBar()->show() : tabBar()->hide();
}


void CTabWidget::slotDestroyChild(QObject * child)
{
    count() > 2 ? tabBar()->show() : tabBar()->hide();
}
