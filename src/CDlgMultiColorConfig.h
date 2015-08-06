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

#ifndef CDLGMULTICOLORCONFIG_H
#define CDLGMULTICOLORCONFIG_H

#include <QDialog>
#include "ui_IDlgMultiColorConfig.h"
#include "CTrack.h"

class CDlgMultiColorConfig : public QDialog, private Ui::IDlgMultiColorConfig
{
    Q_OBJECT;
    public:
        CDlgMultiColorConfig(CTrack::multi_color_setup_t& setup);
        virtual ~CDlgMultiColorConfig();

    public slots:
        void accept();

    protected:
        void resizeEvent(QResizeEvent * e);

    private slots:
        void slotSliderChanged(int i);
        void slotSpinChanged(int i);
        void slotCheckAuto(bool on);

    private:
        void drawColorBar();
        CTrack::multi_color_setup_t& setup;

};
#endif                           //CDLGMULTICOLORCONFIG_H
