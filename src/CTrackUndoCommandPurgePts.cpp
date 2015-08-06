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

#include "CTrackUndoCommandPurgePts.h"
#include <QObject>
#include "CTrackDB.h"
#include <QDebug>
CTrackUndoCommandPurgePts::CTrackUndoCommandPurgePts(CTrack *track)
: track(track)
{
    setText(QObject::tr("Purge Selection"));
}


CTrackUndoCommandPurgePts::~CTrackUndoCommandPurgePts()
{

}


void CTrackUndoCommandPurgePts::redo()
{
    //qDebug() << Q_FUNC_INFO;
    purgedList.clear();
    QList<CTrack::pt_t>& trkpts           = track->getTrackPoints();
    QList<CTrack::pt_t>::iterator trkpt   = trkpts.begin();
    while(trkpt != trkpts.end())
    {
        if(trkpt->flags & CTrack::pt_t::eSelected)
        {
            //            trkpt->flags |= CTrack::pt_t::eDeleted;
            trkpt->flags &= ~CTrack::pt_t::eSelected;
            if(trkpt->flags & CTrack::pt_t::eDeleted)
            {
                trkpt->flags &= ~CTrack::pt_t::eDeleted;
            }
            else
            {
                trkpt->flags |= CTrack::pt_t::eDeleted;
            }

            purgedList << trkpt->idx;
        }
        ++trkpt;
    }
    track->rebuild(false);
    emit CTrackDB::self().emitSigChanged();
}


void CTrackUndoCommandPurgePts::undo()
{
    //qDebug() << Q_FUNC_INFO;
    QList<CTrack::pt_t>& trkpts           = track->getTrackPoints();
    QList<CTrack::pt_t>::iterator trkpt   = trkpts.begin();

    while(trkpt != trkpts.end())
    {
        if(purgedList.contains(trkpt->idx))
        {
            if(trkpt->flags & CTrack::pt_t::eDeleted)
            {
                trkpt->flags &= ~CTrack::pt_t::eDeleted;
            }
            else
            {
                trkpt->flags |= CTrack::pt_t::eDeleted;
            }
            //            trkpt->flags |= CTrack::pt_t::eSelected;
        }
        ++trkpt;
    }
    track->rebuild(false);
    emit CTrackDB::self().emitSigChanged();
}
