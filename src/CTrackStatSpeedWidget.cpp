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
#include "CTrackStatSpeedWidget.h"
#include "CPlot.h"
#include "CTrackDB.h"
#include "CTrack.h"
#include "IUnit.h"
#include "GeoMath.h"

#include <QtGui>

CTrackStatSpeedWidget::CTrackStatSpeedWidget(type_e type, QWidget * parent)
: ITrackStat(type, parent)
, needResetZoom(true)
{
    if(type == eOverDistance)
    {
        plot->setXLabel(tr("distance [m]"));
    }
    else
    {
        plot->setXLabel(tr("time [h]"));
    }
    plot->setYLabel(tr("speed [km/h]"));

    connect(&CTrackDB::self(),SIGNAL(sigModified(const QString&)),this,SLOT(slotChanged()));
    connect(&CTrackDB::self(),SIGNAL(sigNeedUpdate(const QString&)),this,SLOT(slotChanged()));
    connect(&CTrackDB::self(),SIGNAL(sigChanged()),this,SLOT(slotChanged()));
    connect(&CTrackDB::self(), SIGNAL(sigHighlightTrack(CTrack*)), this, SLOT(slotSetTrack(CTrack*)));

    slotChanged();
    plot->setLimits();
    plot->resetZoom();
}


CTrackStatSpeedWidget::~CTrackStatSpeedWidget()
{

}


void CTrackStatSpeedWidget::slotSetTrack(CTrack* track)
{
    needResetZoom = true;
    slotChanged();
}


#define MEDIAN_FLT_LEN 15
void CTrackStatSpeedWidget::slotChanged()
{
    track = CTrackDB::self().highlightedTrack();
    if(track.isNull())
    {
        plot->clear();
        return;
    }

    if(type == eOverDistance)
    {
        plot->setXLabel(tr("distance [%1]").arg(IUnit::self().baseunit));
    }
    else
    {
        plot->setXLabel(tr("time [h]"));
    }

    plot->setYLabel(tr("speed [%1]").arg(IUnit::self().speedunit));

    QPolygonF lineSpeed;
    QPolygonF marksSpeed;
    QList<QPointF> focusSpeed;

    QPolygonF lineAvgSpeed;

    float speedfactor = IUnit::self().speedfactor;

    QList<CTrack::pt_t>& trkpts                 = track->getTrackPoints();
    QList<CTrack::pt_t>::const_iterator trkpt0  = trkpts.begin();

    QVector<float> speed;

    while(trkpt0 != trkpts.end())
    {

        if(trkpt0->flags & CTrack::pt_t::eDeleted)
        {
            ++trkpt0;
            continue;
        }

        speed << trkpt0->speed;

        lineSpeed       << QPointF(type == eOverDistance ? trkpt0->distance : (double)trkpt0->timestamp, trkpt0->speed * speedfactor);
        //        lineAvgSpeed    << QPointF(type == eOverDistance ? trkpt0->distance : (double)trkpt0->timestamp, trkpt0->avgspeed * speedfactor);

        if(trkpt0->flags & CTrack::pt_t::eSelected)
        {
            marksSpeed << QPointF(type == eOverDistance ? trkpt0->distance : (double)trkpt0->timestamp, trkpt0->speed * speedfactor);
        }

        if(trkpt0->flags & CTrack::pt_t::eFocus)
        {
            focusSpeed << QPointF(type == eOverDistance ? trkpt0->distance : (double)trkpt0->timestamp, trkpt0->speed * speedfactor);
        }

        ++trkpt0;
    }

    lineAvgSpeed = lineSpeed;
    if(speed.size() > MEDIAN_FLT_LEN)
    {
        QList<float> list;
        lineAvgSpeed = lineSpeed;

        for(int i = 0; i < MEDIAN_FLT_LEN; i++)
        {
            list << 0;
        }

        for(int i = (MEDIAN_FLT_LEN/2); i < speed.size() - (MEDIAN_FLT_LEN/2); i++)
        {
            for(int n=0; n < MEDIAN_FLT_LEN; n++)
            {
                list[n] = speed[i - (MEDIAN_FLT_LEN/2) + n];
            }
            qSort(list);

            lineAvgSpeed[i].setY(list[(MEDIAN_FLT_LEN/2)] * speedfactor);
        }
    }

    plot->newLine(lineSpeed,focusSpeed, tr("speed"));
    plot->addLine(lineAvgSpeed, tr("med. speed"));
    plot->newMarks(marksSpeed);

    plot->setLimits();
    if (needResetZoom)
    {
        plot->resetZoom();
        needResetZoom = false;
    }

}
