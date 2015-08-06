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
#ifndef CMAPTOOLWIDGET_H
#define CMAPTOOLWIDGET_H

#include <QWidget>
#include <QDir>
#include "ui_IMapToolWidget.h"

class QToolBox;
class QPoint;
class QListWidgetItem;
class QAction;

class CMapToolWidget : public QWidget, private Ui::IMapToolWidget
{
    Q_OBJECT;
    public:
        CMapToolWidget(QTabWidget * parent);
        virtual ~CMapToolWidget();

        signals:
        void sigChanged();

    private slots:
        void slotDBChanged();
        void slotKnownMapDoubleClicked(QTreeWidgetItem* item, int);
        void slotKnownMapClicked(QTreeWidgetItem* item, int);
        void slotSelectedMapClicked(QListWidgetItem* item);
        void slotSelectMap(QListWidgetItem* item);
        void slotContextMenuKnownMaps(const QPoint& pos);
        void slotContextMenuSelectedMaps(const QPoint& pos);
        void slotDeleteKnownMap();
        void slotDeleteSelectedMap();
        void slotExportMap();
        void slotAddDEM();
        void slotDelDEM();
        void slotCfgMap();
        void slotAddTmsMap();
        void slotReload();

    private:
        enum tabs_t
        {
            eTabStream
            ,eTabRaster
            ,eTabVector
        };

        enum columns_e
        {
            eMode = 0
            ,eType = 1
            ,eName = 2
            ,eMaxColumn = 3
        };

        enum mode_e
        {
            eNoMode
            ,eSelected
            ,eOverlay
            ,eOverlayActive
        };

        void updateExportButton();
        QMenu * contextMenuKnownMaps;
        QMenu * contextMenuSelectedMaps;
        QDir path;

        QAction * actAddDEM;
        QAction * actDelDEM;
        QAction * actDelMap;
        QAction * actCfgMap;
        QAction * actAddTMS;
        QAction * actReload;

        QTreeWidget * lastTreeWidget;

};
#endif                           //CMAPTOOLWIDGET_H
