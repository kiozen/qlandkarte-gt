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
#ifndef CCREATEMAPQMAP_H
#define CCREATEMAPQMAP_H

#include <QWidget>
#include <QString>
#include <QTreeWidgetItem>
#include "ui_ICreateMapQMAP.h"

class CCreateMapQMAP : public QWidget, private Ui::ICreateMapQMAP
{
    Q_OBJECT;
    public:
        CCreateMapQMAP(QWidget * parent);
        virtual ~CCreateMapQMAP();

        void readqmap(const QString& filename);

    private slots:
        void slotOpenMap();
        void slotNewMap();
        void slotSaveMap();
        void slotLevelSelectionChanged();
        void slotAdd();
        void slotEdit();
        void slotDel();
        void slotUp();
        void slotDown();

    private:
        friend class CDlgEditMapLevel;

        enum text_e
        {
            eLevel
            ,eMinZoom
            ,eMaxZoom
            ,eFiles
        };

        enum data_e
        {
            eProjection    = Qt::UserRole + 0
            ,eZoom          = Qt::UserRole + 1
            ,eNorth         = Qt::UserRole + 2
            ,eWest          = Qt::UserRole + 3
            ,eSouth         = Qt::UserRole + 4
            ,eEast          = Qt::UserRole + 5
            ,eWidth         = Qt::UserRole + 6
            ,eHeight        = Qt::UserRole + 7
        };

        void resetdlg();
        void mapData2Item(QTreeWidgetItem *& item);
        void processLevelList();

        void writeqmap(const QString& filename);

        QString mapPath;

        QString topLeft;
        QString bottomRight;

        double width;
        double height;
};
#endif                           //CCREATEMAPQMAP_H
