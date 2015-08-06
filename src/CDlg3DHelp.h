/**********************************************************************************************
    Copyright (C) 2010 Oliver Eichler oliver.eichler@gmx.de

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

#ifndef CDLG3DHELP_H
#define CDLG3DHELP_H

#include <QDialog>
#include "ui_IDlg3DHelp.h"

class CDlg3DHelp : public QDialog, private Ui::IDlg3DHelp
{
    Q_OBJECT;
    public:
        CDlg3DHelp();
        virtual ~CDlg3DHelp();
};
#endif                           //CDLG3DHELP_H
