/**********************************************************************************************
    Copyright (C) 2014 Oliver Eichler oliver.eichler@gmx.de

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

#ifndef COVERLAYAREAEDITWIDGET_H
#define COVERLAYAREAEDITWIDGET_H

#include <QWidget>
#include <QPointer>

#include "ui_IOverlayAreaEditWidget.h"

class COverlayArea;

class COverlayAreaEditWidget : public QWidget, private Ui::IOverlayAreaEditWidget
{
    Q_OBJECT
        public:
        COverlayAreaEditWidget(QWidget *parent, COverlayArea * ovl);
        virtual ~COverlayAreaEditWidget();

        bool isAboutToClose();
    private slots:
        void slotApply();
        void slotChanged();
        void slotSelectionChanged();
        void slotItemSelectionChanged();
        void slotContextMenu(const QPoint& pos);
        void slotDelete();
        void slotChangeColor();
        void slotChangeColor(const QColor& c);

    private:
        friend class COverlayArea;
        enum columns_e {eNo, ePos};

        QPointer<COverlayArea> ovl;
        QMenu * contextMenu;

        QString color;

};
#endif                           // COVERLAYAREAEDITWIDGET_H
