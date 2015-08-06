/**********************************************************************************************
    Copyright (C) 2007 Oliver Eichler oliver.eichler@gmx.de

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
#ifndef CDLGCONFIG_H
#define CDLGCONFIG_H

#include <QDialog>
#include "ui_IDlgConfig.h"

/// dialog to configure global parameters of QLandkarte
class CDlgConfig : public QDialog, private Ui::IDlgConfig
{
    Q_OBJECT
        public:
        CDlgConfig(QWidget * parent);
        virtual ~CDlgConfig();

    public slots:
#ifdef QK_QT5_PORT
        int exec();
#else
        void exec();
#endif
        void accept();

    private slots:
        void slotCurrentDeviceChanged(int index);
        void slotSelectFont();
        void slotSelectWptTextColor();
        void slotSetupGarminIcons();
        void slotSelectPathGeoDB();
        void slotSelectPathMapCache();
        void slotCheckBRouterLocal(int state);
        void slotSelectPathBRouterProfiles();
        void slotPushBRouterDefaultOnline();
        void slotPushBRouterDefaultLocal();

    private:
        void fillTypeCombo();
        void fillCharsetCombo();

};
#endif                           //CDLGCONFIG_H
