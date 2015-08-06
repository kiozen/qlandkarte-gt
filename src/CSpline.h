/**********************************************************************************************
    Copyright (C) 2010 Oliver Eichler oliver.eichler@gmx.de

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

    This code is refactured from the QWT project. http://qwt.sourceforge.net/

**********************************************************************************************/
#ifndef CSPLINE_H
#define CSPLINE_H

#include <QVector>
#include <QPolygonF>

class CSpline
{
    public:
        CSpline();
        virtual ~CSpline();

        bool setPoints(const QPolygonF& points);

        double value(double x);

    private:
        void reset();
        bool buildNaturalSpline(const QPolygonF &points);
        bool buildPeriodicSpline(const QPolygonF &points);

        QVector<double> coefA;
        QVector<double> coefB;
        QVector<double> coefC;

        QPolygonF points;

};
#endif                           //CSPLINE_H
