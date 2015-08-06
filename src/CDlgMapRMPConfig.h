/**********************************************************************************************
    Copyright (C) 2012 Oliver Eichler oliver.eichler@gmx.de

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
#ifndef CDLGMAPRMPCONFIG_H
#define CDLGMAPRMPCONFIG_H

#include <QDialog>
#include "ui_IDlgMapRMPConfig.h"

class CMapRmp;

class CDlgMapRMPConfig : public QDialog, private Ui::IDlgMapRMPConfig
{
    public:
        CDlgMapRMPConfig(CMapRmp * map);
        virtual ~CDlgMapRMPConfig();

    private:
        static const QString text;
};
#endif                           //CDLGMAPRMPCONFIG_H
