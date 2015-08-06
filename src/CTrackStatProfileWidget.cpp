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

#include "CTrackStatProfileWidget.h"
#include "CPlot.h"
#include "CTrackDB.h"
#include "CTrack.h"
#include "IUnit.h"
#include "CWptDB.h"
#include "WptIcons.h"

#include <QtGui>

CTrackStatProfileWidget::CTrackStatProfileWidget(type_e type, QWidget * parent)
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

    plot->setYLabel(tr("alt. [m]"));

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


CTrackStatProfileWidget::~CTrackStatProfileWidget()
{

}


void CTrackStatProfileWidget::slotSetTrack(CTrack* track)
{
    needResetZoom = true;
    slotChanged();
}


void CTrackStatProfileWidget::slotChanged()
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
    plot->setYLabel(tr("alt. [%1]").arg(IUnit::self().baseunit));

    QPolygonF lineDEM;

    QPolygonF lineElev;
    QPolygonF marksElev;
    QList<QPointF> focusElev;

    float basefactor = IUnit::self().basefactor;

    QList<CTrack::pt_t>& trkpts = track->getTrackPoints();
    QList<CTrack::pt_t>::const_iterator trkpt = trkpts.begin();
    while(trkpt != trkpts.end())
    {
        if(trkpt->flags & CTrack::pt_t::eDeleted)
        {
            ++trkpt; continue;
        }

        if(trkpt->dem != WPT_NOFLOAT)
        {
            lineDEM << QPointF(type == eOverDistance ? trkpt->distance : (double)trkpt->timestamp, trkpt->dem * basefactor);
        }

        if(trkpt->ele != WPT_NOFLOAT)
        {
            lineElev << QPointF(type == eOverDistance ? trkpt->distance : (double)trkpt->timestamp, trkpt->ele * basefactor);
        }
        else
        {
            //            lineElev.clear();
            //            lineDEM.clear();
            //            marksElev.clear();
            //            break;
        }

        if(trkpt->flags & CTrack::pt_t::eSelected)
        {
            marksElev  << QPointF(type == eOverDistance ? trkpt->distance : (double)trkpt->timestamp, trkpt->ele * basefactor);
        }

        if(trkpt->flags & CTrack::pt_t::eFocus)
        {
            focusElev  << QPointF(type == eOverDistance ? trkpt->distance : (double)trkpt->timestamp, trkpt->ele * basefactor);
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
            tag.point = QPointF(type == eOverDistance ? wpt->trkpt.distance :  (double)wpt->trkpt.timestamp, wpt->trkpt.ele);
            tag.icon  = wpt->wpt->getIcon();
            tag.label = wpt->wpt->getName();
            plot->addTag(tag);

        }
        ++wpt;
    }

    plot->newLine(lineElev,focusElev, "GPS");
    plot->newMarks(marksElev);
    if(!lineDEM.isEmpty())
    {
        plot->addLine(lineDEM, "DEM");
    }
    plot->setLimits();
    if (needResetZoom)
    {
        plot->resetZoom();
        needResetZoom = false;
    }

}
