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

#ifndef CTrackUndoCommandPurgePts_T_H_
#define CTrackUndoCommandPurgePts_T_H_
#include <QUndoCommand>
#include <QSet>
#include "CTrack.h"

class CTrackUndoCommandPurgePts : public QUndoCommand
{
    public:
        CTrackUndoCommandPurgePts(CTrack *track);
        virtual ~CTrackUndoCommandPurgePts();
        virtual void undo();
        virtual void redo();
    private:
        CTrack *track;
        QSet<int> purgedList;
};
#endif                           /* CTRACKUNDOCOMMANDDELETEPT_TS_H_ */
