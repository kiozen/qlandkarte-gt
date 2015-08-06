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
#include "CUndoStackView.h"

CUndoStackView::CUndoStackView(QObject * parent): QUndoStack(parent)
{

}


CUndoStackView::~CUndoStackView()
{

}


CUndoStackView *CUndoStackView::getInstance( QObject * parent)
{
    static CUndoStackView *instance = 0;

    if (!instance)
        instance = new CUndoStackView(parent);

    return instance;

}
