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

#include "CTrackStatDistanceWidget.h"
#include "CPlot.h"
#include "CTrackDB.h"
#include "CTrack.h"
#include "IUnit.h"
#include "CWptDB.h"

#include <QtGui>

CTrackStatDistanceWidget::CTrackStatDistanceWidget(QWidget * parent)
: ITrackStat(eOverTime, parent)
, needResetZoom(true)
{
    plot->setXLabel(tr("time [h]"));
    plot->setYLabel(tr("distance [m]"));

    connect(&CWptDB::self(),SIGNAL(sigChanged()),this,SLOT(slotChanged()));
    connect(&CWptDB::self(),SIGNAL(sigModified(QString)),this,SLOT(slotChanged()));
    connect(&CTrackDB::self(),SIGNAL(sigModified(const QString&)),this,SLOT(slotChanged()));
    connect(&CTrackDB::self(),SIGNAL(sigNeedUpdate(const QString&)),this,SLOT(slotChanged()));
    connect(&CTrackDB::self(),SIGNAL(sigChanged()),this,SLOT(slotChanged()));
    connect(&CTrackDB::self(), SIGNAL(sigHighlightTrack(CTrack*)), this, SLOT(slotSetTrack(CTrack*)));

    slotChanged();
    plot->setLimits();
    plot->resetZoom();

}


CTrackStatDistanceWidget::~CTrackStatDistanceWidget()
{

}


void CTrackStatDistanceWidget::slotSetTrack(CTrack* track)
{
    needResetZoom = true;
    slotChanged();

}


void CTrackStatDistanceWidget::slotChanged()
{
    track = CTrackDB::self().highlightedTrack();
    if(track.isNull())
    {
        plot->clear();
        return;
    }

    QPolygonF lineDist;
    QPolygonF marksDist;
    QList<QPointF> focusDist;

    QList<CTrack::pt_t>& trkpts = track->getTrackPoints();
    QList<CTrack::pt_t>::const_iterator trkpt = trkpts.begin();
    while(trkpt != trkpts.end())
    {
        if(trkpt->flags & CTrack::pt_t::eDeleted)
        {
            ++trkpt; continue;
        }

        lineDist  << QPointF((double)trkpt->timestamp, trkpt->distance);

        if(trkpt->flags & CTrack::pt_t::eSelected)
        {
            marksDist << QPointF((double)trkpt->timestamp, trkpt->distance);
        }

        if(trkpt->flags & CTrack::pt_t::eFocus)
        {
            focusDist << QPointF((double)trkpt->timestamp, trkpt->distance);
        }

        ++trkpt;
    }

    plot->clear();

    const QList<CTrack::wpt_t>& wpts            = track->getStageWaypoints();
    QList<CTrack::wpt_t>::const_iterator wpt    = wpts.begin();
    while(wpt != wpts.end())
    {
        if(wpt->d < WPT_TO_TRACK_DIST)
        {
            CPlotData::point_t tag;
            tag.point = QPointF((double)wpt->trkpt.timestamp, wpt->trkpt.distance);
            tag.icon  = wpt->wpt->getIcon();
            tag.label = wpt->wpt->getName();
            plot->addTag(tag);

        }
        ++wpt;
    }

    plot->newLine(lineDist,focusDist, "dist.");
    plot->newMarks(marksDist);

    plot->setLimits();
    if (needResetZoom)
    {
        plot->resetZoom();
        needResetZoom = false;
    }

}
