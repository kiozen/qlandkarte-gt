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
#ifndef CSEARCHTOOLWIDGET_H
#define CSEARCHTOOLWIDGET_H

#include <QWidget>
#include "ui_ISearchToolWidget.h"

class QToolBox;

/// search tool view
class CSearchToolWidget : public QWidget, private Ui::ISearchToolWidget
{
    Q_OBJECT;
    public:
        CSearchToolWidget(QTabWidget * parent);
        virtual ~CSearchToolWidget();
        void selSearchByKey(const QString& key);

    protected:
        void keyPressEvent(QKeyEvent * e);

    private slots:
        void slotReturnPressed();
        void slotQueryFinished();
        void slotDBChanged();
        void slotItemClicked(QListWidgetItem* item);
        void slotContextMenu(const QPoint& pos);
        void slotDelete();
        void slotCopyPosition();
        void slotAdd();

        void slotHostChanged(int idx);

    private:

        QMenu * contextMenu;

};
#endif                           //CSEARCHTOOLWIDGET_H
