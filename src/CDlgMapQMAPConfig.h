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

#ifndef CDLGMAPQMAPCONFIG_H
#define CDLGMAPQMAPCONFIG_H

#include <QDialog>
#include "ui_IDlgMapQMAPConfig.h"

class CMapQMAP;

class CDlgMapQMAPConfig : public QDialog, private Ui::IDlgMapQMAPConfig
{
    Q_OBJECT;
    public:
        CDlgMapQMAPConfig(CMapQMAP * map);
        virtual ~CDlgMapQMAPConfig();

    public slots:
        void accept();

    private:
        CMapQMAP * map;
        static const QString text;
};
#endif                           //CDLGMAPQMAPCONFIG_H
