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
#ifndef CCREATEMAPGRIDTOOL_H
#define CCREATEMAPGRIDTOOL_H

#include <QWidget>
#include "ui_ICreateMapGridTool.h"

class CCreateMapGeoTiff;

class CCreateMapGridTool : public QWidget, private Ui::ICreateMapGridTool
{
    Q_OBJECT;
    public:
        CCreateMapGridTool(CCreateMapGeoTiff * geotifftool, QWidget * parent);
        virtual ~CCreateMapGridTool();

        static CCreateMapGridTool * self(){return m_self;}

    private slots:
        void slotOk();
        void slotCheck();
        void slotProjWizard();

    private:
        void place4GCPs();
        CCreateMapGeoTiff * geotifftool;

        static CCreateMapGridTool * m_self;

};
#endif                           //CCREATEMAPGRIDTOOL_H
