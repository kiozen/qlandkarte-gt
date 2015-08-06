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
#include "CSpline.h"
#include <QtGui>

static int lookup(double x, const QPolygonF &values)

{
    int i1;
    const int size = (int)values.size();

    if (x <= values[0].x())
    {
        i1 = 0;
    }
    else if (x >= values[size - 2].x())
    {
        i1 = size - 2;
    }
    else
    {
        i1 = 0;
        int i2 = size - 2;
        int i3 = 0;

        while ( i2 - i1 > 1 )
        {
            i3 = i1 + ((i2 - i1) >> 1);

            if (values[i3].x() > x)
            {
                i2 = i3;
            }
            else
            {
                i1 = i3;
            }
        }
    }
    return i1;
}


CSpline::CSpline()
{

}


CSpline::~CSpline()
{

}


void CSpline::reset()
{
    coefA.resize(0);
    coefB.resize(0);
    coefC.resize(0);
    points.resize(0);
}


bool CSpline::setPoints(const QPolygonF& p)
{
    const int size = p.size();
    if (size <= 2)
    {
        reset();
        return false;
    }

    points = p;

    coefA.resize(size-1);
    coefB.resize(size-1);
    coefC.resize(size-1);

    bool ok;
    //    if ( d_data->splineType == Periodic )
    //    {
    //        ok = buildPeriodicSpline(points);
    //    }
    //    else
    //    {
    //        ok = buildNaturalSpline(points);
    //    }
    ok = buildNaturalSpline(points);

    if (!ok)
    {
        reset();
    }

    return ok;
}


double CSpline::value(double x)
{
    if (coefA.size() == 0)
    {
        return 0.0;
    }

    const int i = lookup(x, points);

    const double delta = x - points[i].x();
    return( ( ( ( coefA[i] * delta) + coefB[i] ) * delta + coefC[i] ) * delta + points[i].y() );
}


bool CSpline::buildNaturalSpline(const QPolygonF &points)
{
    int i;

    const QPointF *p = points.data();
    const int size = points.size();

    double *a = coefA.data();
    double *b = coefB.data();
    double *c = coefC.data();

    //  set up tridiagonal equation system; use coefficient
    //  vectors as temporary buffers
    QVector<double> h(size-1);
    for (i = 0; i < size - 1; i++)
    {
        h[i] = p[i+1].x() - p[i].x();
        if (h[i] <= 0)
        {
            return false;
        }
    }

    QVector<double> d(size-1);
    double dy1 = (p[1].y() - p[0].y()) / h[0];
    for (i = 1; i < size - 1; i++)
    {
        b[i] = c[i] = h[i];
        a[i] = 2.0 * (h[i-1] + h[i]);

        const double dy2 = (p[i+1].y() - p[i].y()) / h[i];
        d[i] = 6.0 * ( dy1 - dy2);
        dy1 = dy2;
    }

    //
    // solve it
    //

    // L-U Factorization
    for(i = 1; i < size - 2;i++)
    {
        c[i] /= a[i];
        a[i+1] -= b[i] * c[i];
    }

    // forward elimination
    QVector<double> s(size);
    s[1] = d[1];
    for ( i = 2; i < size - 1; i++)
    {
        s[i] = d[i] - c[i-1] * s[i-1];
    }

    // backward elimination
    s[size - 2] = - s[size - 2] / a[size - 2];
    for (i = size -3; i > 0; i--)
    {
        s[i] = - (s[i] + b[i] * s[i+1]) / a[i];
    }
    s[size - 1] = s[0] = 0.0;

    //
    // Finally, determine the spline coefficients
    //
    for (i = 0; i < size - 1; i++)
    {
        a[i] = ( s[i+1] - s[i] ) / ( 6.0 * h[i]);
        b[i] = 0.5 * s[i];
        c[i] = ( p[i+1].y() - p[i].y() ) / h[i] - (s[i+1] + 2.0 * s[i] ) * h[i] / 6.0;
    }

    return true;
}


bool CSpline::buildPeriodicSpline(const QPolygonF &points)
{
    int i;

    const QPointF *p = points.data();
    const int size = points.size();

    double *a = coefA.data();
    double *b = coefB.data();
    double *c = coefC.data();

    QVector<double> d(size-1);
    QVector<double> h(size-1);
    QVector<double> s(size);

    //
    //  setup equation system; use coefficient
    //  vectors as temporary buffers
    //
    for (i = 0; i < size - 1; i++)
    {
        h[i] = p[i+1].x() - p[i].x();
        if (h[i] <= 0.0)
        {
            return false;
        }
    }

    const int imax = size - 2;
    double htmp = h[imax];
    double dy1 = (p[0].y() - p[imax].y()) / htmp;
    for (i = 0; i <= imax; i++)
    {
        b[i] = c[i] = h[i];
        a[i] = 2.0 * (htmp + h[i]);
        const double dy2 = (p[i+1].y() - p[i].y()) / h[i];
        d[i] = 6.0 * ( dy1 - dy2);
        dy1 = dy2;
        htmp = h[i];
    }

    //
    // solve it
    //

    // L-U Factorization
    a[0] = sqrt(a[0]);
    c[0] = h[imax] / a[0];
    double sum = 0;

    for( i = 0; i < imax - 1; i++)
    {
        b[i] /= a[i];
        if (i > 0)
        {
            c[i] = - c[i-1] * b[i-1] / a[i];
        }
        a[i+1] = sqrt( a[i+1] - b[i] * b[i]);
        sum += c[i] * c[i];
    }
    b[imax-1] = (b[imax-1] - c[imax-2] * b[imax-2]) / a[imax-1];
    a[imax] = sqrt(a[imax] - b[imax-1] * b[imax-1] - sum);

    // forward elimination
    s[0] = d[0] / a[0];
    sum = 0;
    for( i = 1; i < imax; i++)
    {
        s[i] = (d[i] - b[i-1] * s[i-1]) / a[i];
        sum += c[i-1] * s[i-1];
    }
    s[imax] = (d[imax] - b[imax-1] * s[imax-1] - sum) / a[imax];

    // backward elimination
    s[imax] = - s[imax] / a[imax];
    s[imax-1] = -(s[imax-1] + b[imax-1] * s[imax]) / a[imax-1];
    for (i= imax - 2; i >= 0; i--)
    {
        s[i] = - (s[i] + b[i] * s[i+1] + c[i] * s[imax]) / a[i];
    }

    //
    // Finally, determine the spline coefficients
    //
    s[size-1] = s[0];
    for ( i=0; i < size-1; i++)
    {
        a[i] = ( s[i+1] - s[i] ) / ( 6.0 * h[i]);
        b[i] = 0.5 * s[i];
        c[i] = ( p[i+1].y() - p[i].y() ) / h[i] - (s[i+1] + 2.0 * s[i] ) * h[i] / 6.0;
    }

    return true;
}
