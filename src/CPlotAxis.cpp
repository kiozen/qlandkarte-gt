/**********************************************************************************************
    Copyright (C) 2006, 2007 Oliver Eichler oliver.eichler@gmx.de

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

#include "CPlotAxis.h"

#include <math.h>

#include <QtCore>

CPlotAxis::CPlotAxis( QObject * parent )
: QObject( parent )
, initialized( false )
, autoscale( false )
, scale( 1.0 )
, used_min( 0.0 )
, used_max( 0.0 )
, limit_min( 0.0 )
, limit_max( 0.0 )
, interval( 0.0 )
, tic_start( 0 )
, scaleWidth( 0 )
, tic_type( norm )
, firstTic( false )
, lastTic( false )
, points(0)
{}

CPlotAxis::~CPlotAxis()
{}

void CPlotAxis::setLimits(double min, double max)
{
    limit_min = min;
    limit_max = max;
}


void CPlotAxis::setMinMax( double given_min, double given_max )
{
    double tmp;

    if ( given_min == given_max )
    {
        if ( given_min != 0.0 )
        {
            given_min -= given_min / 10.0;
            given_max += given_max / 10.0;
        }
        else
        {
            given_min -= 0.1;
            given_max += 0.1;
        }
    }

    if ( given_min > given_max )
    {
        tmp = given_max;
        given_max = given_min;
        given_min = tmp;
    }

    used_min = given_min;
    used_max = given_max;

    calc();

    initialized = true;

}


void CPlotAxis::calc()
{
    double tmp_abs = ( used_max - used_min ) < 0 ? -( used_max - used_min ) : ( used_max - used_min );
    double tmp = log10( tmp_abs / 10.0 );

    double exponent = ( int ) tmp;
    double residue = tmp - exponent;

    if ( residue < 0 && residue <= log10( 0.1 ) )
        residue = log10( 0.1 );
    else if ( residue > log10( 0.1 ) && residue <= log10( 0.2 ) )
        residue = log10( 0.2 );
    else if ( residue > log10( 0.2 ) && residue <= log10( 0.5 ) )
        residue = log10( 0.5 );
    else if ( residue > log10( 0.5 ) && residue <= log10( 1.0 ) )
        residue = log10( 1.0 );
    else if ( residue > log10( 1.0 ) && residue <= log10( 2.0 ) )
        residue = log10( 2.0 );
    else if ( residue > log10( 2.0 ) && residue <= log10( 5.0 ) )
        residue = log10( 5.0 );
    else if ( residue > log10( 5.0 ) && residue <= log10( 10. ) )
        residue = log10( 10. );

    interval = exponent + residue;
    interval = pow( 10, interval );

    if ( autoscale )
    {
        used_min = floor( used_min / interval ) * interval;
        used_max = ceil( used_max / interval ) * interval;
    }
    else
    {
        used_min = used_min;
        used_max = used_max;
    }

    int t1 = ( int )( used_min / interval + 0.5);
    tic_start = interval * t1;
    if ( tic_start < used_min )
    {
        tic_start += interval;
    }
}


const QString CPlotAxis::fmtsgl( double val )
{
    static QString f;
    double tmp;
    double exponent;
    double residue;

    if ( val != 0 )
    {
        if ( val < 0 ) val = -val;
        tmp = log10( val );
        exponent = ( int ) tmp;
        residue = tmp - exponent;
    }
    else
    {
        exponent = 0;
        residue = 0;
    }

    if ( abs( ( int ) exponent ) > 5 )
    {
        f = "%1.2e";
    }
    else
    {
        if ( exponent >= 0 )
        {
            f = "%" + QString( "%1" ).arg( ( int ) ( exponent + 1 ) );
            if ( ( exponent == 0 ) && ( residue < 0 ) ) f += ".1f";
            else f += ".0f";
        }
        else
        {
            f = "%1." + QString( "%1" ).arg( ( int ) ( -exponent + 1 ) ) + "f";
        }
    }

    return f;
}


/**
  Generates a sprintf style format string for a given value.
  <pre>
  0.001   -> "%1.4f"
  0.01    -> "%1.3f"
  0.1     -> "%1.2f"
  1       -> "%1.1f"
  10      -> "%2.1f"
  >10000 scientific notation "%1.3e"
  </pre>

  @param val value to calculate the string on

  @return a zero terminated format string

*/
const QString CPlotAxis::fmtdbl( double val )
{
    static QString f;
    double tmp;
    double exponent;
    double residue;

    if ( val != 0 )
    {
        if ( val < 0 ) val = -val;
        tmp = log10( val );
        exponent = ( int ) tmp;
        residue = tmp - exponent;
    }
    else
    {
        exponent = 0;
        residue = 0;
    }

    if ( abs( ( int ) exponent ) > 5 )
    {
        f = "%1.3e";
    }
    else
    {
        if ( exponent >= 0 )
        {
            f = "%" + QString( "%1" ).arg( ( int ) ( exponent + 1 ) );
            if ( ( exponent == 0 ) && ( residue < 0 ) ) f += ".2f";
            else f += ".1f";
        }
        else
        {
            f = "%1." + QString( "%1" ).arg( ( int ) ( -exponent + 2 ) ) + "f";
        }
    }
    return f;
}


int CPlotAxis::getScaleWidth( const QFontMetrics& m )
{

    if ( scaleWidth > 0 ) return scaleWidth * m.width( " " );

    int width = 0;
    int tmp;
    QString format_single_prec = fmtsgl( interval );

    const TTic * t = ticmark();
    while ( t )
    {
        tmp = m.width( QString().sprintf( format_single_prec.toLatin1().data(), t->val ) );
        if ( tmp > width ) width = tmp;
        t = ticmark( t );
    }
    return width;
}


void CPlotAxis::getLimits(double& limMin, double& limMax, double& useMin, double& useMax)
{
    limMin = limit_min;
    limMax = limit_max;
    useMin = used_min;
    useMax = used_max;
}


const CPlotAxis::TTic* CPlotAxis::ticmark( const TTic * t )
{
    QString format_single_prec = fmtsgl( interval );

    switch ( tic_type )
    {
        case notic:
            return 0;
            break;

        case minmax:
            if ( t == NULL )
            {
                tic.val = used_min;
                firstTic = true;
            }
            else if ( firstTic == true )
            {
                tic.val = used_max;
                firstTic = false;
            }
            else
            {
                return 0;
            }
            break;

        case norm:
            if ( interval == 0 )
            {
                //qWarning() << "CPlotAxis::ticmark() mode 'norm': interval == 0";
                return 0;
            }
            if ( t == NULL )
            {
                tic.val = tic_start;
            }
            else
            {
                tic.val += interval;
                if ( ( tic.val - used_max ) > interval / 20 )
                {
                    return 0;
                }
            }
            break;

        case full:
            if ( t == NULL )
            {
                tic.val = used_min;
                firstTic = true;
            }
            else if ( firstTic == true )
            {
                tic.val = tic_start;
                firstTic = false;
            }
            else if ( lastTic == true )
            {
                lastTic = false;
                return 0;
            }
            else
            {
                tic.val += interval;
                if ( ( tic.val - used_max ) > interval / 20 )
                {
                    tic.val = used_max;
                    lastTic = true;
                }
            }
            break;
    }

    tic.lbl.sprintf( format_single_prec.toLatin1(), tic.val );

    return &tic;
}


void CPlotAxis::setScale( const unsigned int pts )
{
    //if ( !initialized )
    //qWarning( "you try to set the scale before defining the min & max value. not very sensible." );
    points = pts;
    scale = pts / ( used_max - used_min );
}


void CPlotAxis::resetZoom()
{
    setMinMax(limit_min, limit_max);
}


void CPlotAxis::zoom(bool in, int point)
{
    double min, p, d, factor;
    if (in)
        factor = 1/1.1;
    else
        factor = 1.1;

    p = pt2val(point);
    min = (p - used_min) * (1 - factor) + used_min;
    d = min - used_min * factor;

    setMinMax(min, used_max * factor + d);
    move(0);
}


void CPlotAxis::move(int delta_pt)
{
    double delta_val = pt2val(delta_pt) - pt2val(0);
    bool f = ! (used_max - used_min < limit_max - limit_min);
    if (f ^ (used_min + delta_val < limit_min))
    {
        delta_val = (limit_min - used_min);
    }
    if (f ^ (used_max + delta_val > limit_max))
    {
        delta_val = (limit_max - used_max);
    }
    setMinMax(used_min + delta_val, used_max + delta_val);
}
