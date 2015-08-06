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

#ifndef CDLGSCREENSHOT_H
#define CDLGSCREENSHOT_H

#include "ui_IDlgScreenshot.h"

#include <QDialog>

class CDlgScreenshot : public QDialog, private Ui::IDlgScreenshot
{
    Q_OBJECT;
    public:
        CDlgScreenshot(QWidget * parent);
        virtual ~CDlgScreenshot();

    private slots:
        void slotAcquire();
        void slotSave();

    private:
        QImage image;

};
#endif                           //CDLGSCREENSHOT_H
