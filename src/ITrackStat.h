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
#ifndef ITRACKSTAT_H
#define ITRACKSTAT_H

#include <QWidget>
#include <QPointer>
#include "ui_ITrackStatWidget.h"

#include "CTrack.h"

class CPlot;
class CWpt;

class ITrackStat : public QWidget, private Ui::ITrackStatWidget
{
    Q_OBJECT;
    public:
        enum type_e {eOverDistance, eOverTime};

        ITrackStat(type_e type, QWidget * paren);
        virtual ~ITrackStat();

        CPlot * getPlot(){return plot;}

    protected:

        type_e type;
        CPlot * plot;
        QPointer<CTrack> track;
    protected slots:
        void slotActivePoint(double x);
        void slotSetWaypoint(double dist);
};
#endif                           //ITRACKSTAT_H
