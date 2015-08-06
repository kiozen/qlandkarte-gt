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

#ifndef COVERLAYTOOLWIDGET_H
#define COVERLAYTOOLWIDGET_H

#include <QWidget>
#include "ui_IOverlayToolWidget.h"

class QTabWidget;

class COverlayToolWidget : public QWidget, private Ui::IOverlayToolWidget
{
    Q_OBJECT;
    public:
        COverlayToolWidget(QTabWidget * parent);
        virtual ~COverlayToolWidget();

    protected:
        void keyPressEvent(QKeyEvent * e);

    private slots:
        void slotDBChanged();
        void slotItemDoubleClicked(QListWidgetItem * item);
        void slotItemClicked(QListWidgetItem * item);
        void slotSelectionChanged();
        void slotContextMenu(const QPoint& pos);
        void slotDelete();
        void slotZoomToFit();

    private:
        bool originator;

};
#endif                           //COVERLAYTOOLWIDGET_H
