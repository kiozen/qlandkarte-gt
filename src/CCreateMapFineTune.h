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
#ifndef CCREATEMAPFINETUNE_H
#define CCREATEMAPFINETUNE_H

#include <QWidget>
#include <QDir>
#include <ui_ICreateMapFineTune.h>
#include <gdal_priv.h>

class CCreateMapFineTune : public QWidget, private Ui::ICreateMapFineTune
{
    Q_OBJECT;
    public:
        CCreateMapFineTune(QWidget * parent);
        virtual ~CCreateMapFineTune();

    private slots:
        void slotOpenFile();
        void slotUp();
        void slotDown();
        void slotLeft();
        void slotRight();
        void slotSave();

    private:
        friend int CPL_STDCALL ProgressFunc(double dfComplete, const char *pszMessage, void *pProgressArg);
        QDir path;
};
#endif                           //CCREATEMAPFINETUNE_H
