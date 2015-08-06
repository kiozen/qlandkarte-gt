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

#include "CTrackUndoCommandSelect.h"
#include <QObject>
#include <QDebug>
#include "CTrackDB.h"

CTrackUndoCommandSelect::CTrackUndoCommandSelect(CTrack *track, const QRect& rect, bool select)
: track(track), select(select) , rect(rect)
{
    if(select)
        setText(QObject::tr("Select Trackpoints"));
    else
        setText(QObject::tr("Unselect Trackpoints"));

}


CTrackUndoCommandSelect::~CTrackUndoCommandSelect()
{

}


void CTrackUndoCommandSelect::redo()
{
    //qDebug() << Q_FUNC_INFO;

    selectionList.clear();
    QList<CTrack::pt_t>& trkpts = track->getTrackPoints();
    // trkpt = trkpts.begin();
    for (trkpt = trkpts.begin(); trkpt != trkpts.end(); ++trkpt)
    {
        //     qDebug() << &*trkpt ;//<< *trkpt ;
        if(rect.contains(trkpt->px) && !(trkpt->flags & CTrack::pt_t::eDeleted))
        {
            if (select)
            {
                trkpt->flags |= CTrack::pt_t::eSelected;
                selectionList.insert(trkpt->idx);
            }
            else
            {
                if (trkpt->flags & CTrack::pt_t::eSelected)
                {
                    selectionList.insert(trkpt->idx);
                    trkpt->flags &= ~CTrack::pt_t::eSelected;
                }
            }

        }
    }

    emit CTrackDB::self().emitSigChanged();
}


void CTrackUndoCommandSelect::undo()
{
    //qDebug() << Q_FUNC_INFO;

    QList<CTrack::pt_t>& trkpts = track->getTrackPoints();
    for (trkpt = trkpts.begin(); trkpt != trkpts.end(); ++trkpt)
    {
        if (selectionList.contains(trkpt->idx))
        {
            if (!select)
                trkpt->flags |= CTrack::pt_t::eSelected;
            else
                trkpt->flags &= ~CTrack::pt_t::eSelected;
        }
    }

    emit CTrackDB::self().emitSigChanged();
}
