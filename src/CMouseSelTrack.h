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

#ifndef CMOUSESELTRACK_H
#define CMOUSESELTRACK_H

#include "IMouse.h"

class CMouseSelTrack : public IMouse
{
    Q_OBJECT;
    public:
        CMouseSelTrack(CCanvas * canvas);
        virtual ~CMouseSelTrack();

        void mouseMoveEvent(QMouseEvent * e);
        void mousePressEvent(QMouseEvent * e);
        void mouseReleaseEvent(QMouseEvent * e);

        void draw(QPainter& p);
    private:
        bool selTrack;
        bool unselectTrack;

};
#endif                           //CMOUSESELTRACK_H
