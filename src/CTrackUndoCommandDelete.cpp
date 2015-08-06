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

#include "CTrackUndoCommandDelete.h"
#include <QMap>
#include <QString>
#include <QDebug>
#include "CTrack.h"
#include "CTrackDB.h"
#include "CTrackToolWidget.h"

CTrackUndoCommandDelete::CTrackUndoCommandDelete(CTrackDB *trackDB, const QString &key, bool silent)
: trackDB(trackDB) , key(key), silent(silent)
{
    setText(QObject::tr("delete track"));
    track = 0;
}


CTrackUndoCommandDelete::~CTrackUndoCommandDelete()
{
    qDebug() << Q_FUNC_INFO << track << track->ref;
    if (track->ref < 1)
    {
        delete track;
    }
}


void CTrackUndoCommandDelete::redo()
{
    qDebug() << Q_FUNC_INFO;
    track = trackDB->take(key, silent);
    track->ref++;
}


void CTrackUndoCommandDelete::undo()
{
    qDebug() << Q_FUNC_INFO << track;
    trackDB->insert(key, track, silent);
    track->ref--;
}
