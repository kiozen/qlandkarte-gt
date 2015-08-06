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
#include "CPlotAxisTime.h"

#include <math.h>
#include <QtGui>

CPlotAxisTime::CPlotAxisTime(QObject * parent)
: CPlotAxis(parent)
{

}


CPlotAxisTime::~CPlotAxisTime()
{

}


void CPlotAxisTime::calc()
{

    int dSec    = used_max - used_min;
    tic_start   = used_min;

    strFormat = "hh:mm:ss";

    if(dSec < 20)
    {
        interval = 1;
        tic_start = used_min;
    }
    else if(dSec < 100)
    {
        interval = 5;
        tic_start = ceil(used_min / interval) * interval;
    }
    else if(dSec < 200)
    {
        interval = 10;
        tic_start = ceil(used_min / interval) * interval;
    }
    else if(dSec < 600)
    {
        interval = 30;
        tic_start = ceil(used_min / interval) * interval;
    }
    else if(dSec < 1200)
    {
        interval = 60;
        tic_start = ceil(used_min / interval) * interval;
    }
    else if(dSec < 6000)
    {
        interval = 600;
        tic_start = ceil(used_min / interval) * interval;
    }
    else if(dSec < 12000)
    {
        interval = 600;
        tic_start = ceil(used_min / interval) * interval;
    }
    else if(dSec < 36000)
    {
        interval = 1800;
        tic_start = ceil(used_min / interval) * interval;
    }
    else if(dSec < 72000)
    {
        interval = 3600;
        tic_start = ceil(used_min / interval) * interval;
    }
    else if(dSec < 216000)
    {
        interval = 10800;
        tic_start = ceil(used_min / interval) * interval;
    }
    else
    {
        qDebug() << "ouch";
    }

    if ( autoscale )
    {
        used_min = floor( used_min / interval ) * interval;
        used_max = ceil( used_max / interval ) * interval;
    }
    else
    {
        used_min = used_min;
        used_max = used_max;
    }

    int t1 = ( int )( used_min / interval + 0.5);
    tic_start = interval * t1;
    if ( tic_start < used_min )
    {
        tic_start += interval;
    }

}


const CPlotAxis::TTic* CPlotAxisTime::ticmark( const TTic * t )
{
    const TTic * _tic_ = CPlotAxis::ticmark(t);
    if(_tic_)
    {
        QDateTime time = QDateTime::fromTime_t(tic.val);
        time.setTimeSpec(Qt::LocalTime);
        tic.lbl = time.toString(strFormat);
    }

    return _tic_;
}
