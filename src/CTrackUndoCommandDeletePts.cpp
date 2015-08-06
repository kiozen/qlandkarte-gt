/** ********************************************************************************************
    Copyright (c) 2009 Marc Feld

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
********************************************************************************************* */

#include "CTrackUndoCommandDeletePts.h"
#include <QObject>
#include "CTrackDB.h"
#include <QDebug>
CTrackUndoCommandDeletePts::CTrackUndoCommandDeletePts(CTrack *track)
: track(track)
{
    setText(QObject::tr("Delete Selection"));
}


CTrackUndoCommandDeletePts::~CTrackUndoCommandDeletePts()
{

}


void CTrackUndoCommandDeletePts::redo()
{
    //qDebug() << Q_FUNC_INFO;
    oldList.clear();
    QList<CTrack::pt_t>& trkpts           = track->getTrackPoints();
    QList<CTrack::pt_t>::iterator trkpt   = trkpts.begin();
    while(trkpt != trkpts.end())
    {
        oldList.append(*trkpt);
        if(trkpt->flags & CTrack::pt_t::eDeleted)
        {
            trkpt = trkpts.erase(trkpt);
        }
        else
        {
            ++trkpt;
        }
    }
    emit CTrackDB::self().emitSigChanged();

}


void CTrackUndoCommandDeletePts::undo()
{
    //qDebug() << Q_FUNC_INFO;
    QList<CTrack::pt_t>& trkpts           = track->getTrackPoints();
    trkpts.clear();

    foreach(CTrack::pt_t tp, oldList)
    {
        trkpts.append(tp);
    }
    track->rebuild(true);
    emit CTrackDB::self().emitSigChanged();

}
