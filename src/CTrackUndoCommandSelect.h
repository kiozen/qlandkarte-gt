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

#ifndef CTRACKUNDOCOMMANDSELECT_H_
#define CTRACKUNDOCOMMANDSELECT_H_
#include <QUndoCommand>
#include <QSet>
#include <QList>
#include "CTrack.h"

class CTrackUndoCommandSelect : public QUndoCommand
{
    public:
        CTrackUndoCommandSelect(CTrack *track, const QRect& rect, bool select);
        virtual ~CTrackUndoCommandSelect();
        virtual void undo();
        virtual void redo();
    private:
        CTrack *track;
        QSet<int> selectionList;
        QList<CTrack::pt_t>::iterator trkpt;
        bool select;
        QRect rect;
};
#endif                           /* CTrackUndoCommandSelect_H_ */
