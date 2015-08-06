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

#ifndef CTRACKUNDOCOMMANDDELETE_H_
#define CTRACKUNDOCOMMANDDELETE_H_
#include <QUndoCommand>
#include <QString>
#include <QMap>
class CTrack;
class CTrackDB;

class CTrackUndoCommandDelete :  public QUndoCommand
{
    public:
        CTrackUndoCommandDelete(CTrackDB *trackdb, const QString & key, bool silent);
        virtual ~CTrackUndoCommandDelete();
        virtual void undo();
        virtual void redo();
    private:
        CTrackDB *trackDB;
        CTrack *track;
        QString key;
        bool silent;
};
#endif                           /* CUNDOCOMMANDTRACKDELETE_H_ */
