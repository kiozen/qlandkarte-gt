/**********************************************************************************************
    Copyright (C) 2008 Oliver Eichler oliver.eichler@gmx.de
    Copyright (C) 2010 Christian Treffs ctreffs@gmail.com

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

#include "CTrackStatExtensionWidget.h"
#include "CPlot.h"
#include "CTrackDB.h"
#include "CTrack.h"
#include "IUnit.h"

#include <QtGui>

CTrackStatExtensionWidget::CTrackStatExtensionWidget(type_e type, QWidget * parent,  QString name)
: ITrackStat(type, parent)
, needResetZoom(true)
{

    myName = name;
    setObjectName(name);
    setToolTip(name);

    connect(&CTrackDB::self(),SIGNAL(sigModified(const QString&)),this,SLOT(slotChanged()));
    connect(&CTrackDB::self(),SIGNAL(sigNeedUpdate(const QString&)),this,SLOT(slotChanged()));
    connect(&CTrackDB::self(),SIGNAL(sigChanged()),this,SLOT(slotChanged()));
    connect(&CTrackDB::self(), SIGNAL(sigHighlightTrack(CTrack*)), this, SLOT(slotSetTrack(CTrack*)));

    slotChanged();
    plot->setLimits();
    plot->resetZoom();

}


CTrackStatExtensionWidget::~CTrackStatExtensionWidget()
{
}


//This method plots the diagram
void CTrackStatExtensionWidget::slotChanged()
{
    track = CTrackDB::self().highlightedTrack();
    if(track.isNull())
    {
        plot->clear();
        return;
    }

    QPolygonF lineExt;
    QPolygonF marksExt;
    QList<QPointF> focusExt;
    double val = 0;

    QList<CTrack::pt_t>& trkpts = track->getTrackPoints();
    QList<CTrack::pt_t>::iterator trkpt = trkpts.begin();

    while(trkpt != trkpts.end())
    {
                                 //Wert einfgen
        QString val1 = trkpt->gpx_exts.getValue(myName);
        if (val1 == "")
        {
            val = 0;
        }
        else
        {
            val = val1.toDouble();
        }

        if(trkpt->flags & CTrack::pt_t::eDeleted)
        {
            ++trkpt;
            continue;
        }

        lineExt << QPointF((double)trkpt->timestamp, val);

        if(trkpt->flags & CTrack::pt_t::eSelected)
        {
            marksExt << QPointF((double)trkpt->timestamp, val);
        }

        if(trkpt->flags & CTrack::pt_t::eFocus)
        {
            focusExt << QPointF((double)trkpt->timestamp, val);
        }

        ++trkpt;
    }

    plot->setXLabel(tr("time [h]"));
    plot->setYLabel(myName);
    plot->clear();
    plot->newLine(lineExt,focusExt, myName);
    plot->newMarks(marksExt);

    plot->setLimits();
    if (needResetZoom)
    {
        plot->resetZoom();
        needResetZoom = false;
    }

}


void CTrackStatExtensionWidget::slotSetTrack(CTrack* track)
{
    needResetZoom = true;
    slotChanged();
}
