/**********************************************************************************************
    Copyright (C) 2013 Oliver Eichler oliver.eichler@gmx.de

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

#ifndef CMAPDEMSLOPESETUP_H
#define CMAPDEMSLOPESETUP_H

#include <QWidget>
#include <QPointer>
#include "ui_IMapDEMSlopeSetup.h"

class CMapDEM;

class CMapDEMSlopeSetup : public QWidget, private Ui::IMapDEMSlopeSetup
{
    Q_OBJECT;
    public:
        virtual ~CMapDEMSlopeSetup();

        static CMapDEMSlopeSetup* self(){return m_pSelf;}

        void registerDEMMap(CMapDEM * map);

    protected:
        void paintEvent(QPaintEvent * e);

    private slots:
        void slotValueChanged(int val);

    private:
        friend class CCanvas;
        CMapDEMSlopeSetup(QWidget * parent);

        static CMapDEMSlopeSetup *m_pSelf;

        QPointer<CMapDEM> dem;
};
#endif                           //CMAPDEMSLOPESETUP_H
