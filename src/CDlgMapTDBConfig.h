/**********************************************************************************************
    Copyright (C) 2009 Oliver Eichler oliver.eichler@gmx.de

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

#ifndef CDLGMAPTDBCONFIG_H
#define CDLGMAPTDBCONFIG_H

#include <QDialog>
#include "ui_IDlgMapTDBConfig.h"

class CMapTDB;

class CDlgMapTDBConfig : public QDialog, private Ui::IDlgMapTDBConfig
{
    Q_OBJECT;
    public:
        CDlgMapTDBConfig(CMapTDB * map);
        virtual ~CDlgMapTDBConfig();

    public slots:
        void accept();

    private:
        CMapTDB * map;
        static const QString text;
};
#endif                           //CDLGMAPTDBCONFIG_H
