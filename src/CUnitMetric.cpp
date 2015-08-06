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
#include "CUnitMetric.h"

CUnitMetric::CUnitMetric(QObject * parent)
: IUnit("metric", "m", 1.0f, "km/h", 3.6f, parent)
{

}


CUnitMetric::~CUnitMetric()
{

}


void CUnitMetric::meter2elevation(float meter, QString& val, QString& unit)
{
    val.sprintf("%1.0f", meter);
    unit = "m";
}


void CUnitMetric::meter2distance(float meter, QString& val, QString& unit)
{
    if(meter < 10)
    {
        val.sprintf("%1.1f", meter);
        unit = "m";
    }
    else if(meter < 1000)
    {
        val.sprintf("%1.0f", meter);
        unit = "m";
    }
    else if(meter < 10000)
    {
        val.sprintf("%1.2f", meter / 1000);
        unit = "km";
    }
    else if(meter < 20000)
    {
        val.sprintf("%1.1f", meter / 1000);
        unit = "km";
    }
    else
    {
        val.sprintf("%1.0f", meter / 1000);
        unit = "km";
    }
}


void CUnitMetric::meter2speed(float meter, QString& val, QString& unit)
{
    if (meter < 10.0)
    {
        val.sprintf("%1.2f",meter * speedfactor);
    }
    else
    {
        val.sprintf("%1.0f",meter * speedfactor);
    }
    unit = speedunit;
}


float CUnitMetric::elevation2meter(const QString& val)
{
    return val.toDouble();
}


float CUnitMetric::str2speed(QString& str)
{
    return str.remove(" km/h").toDouble();
}


float CUnitMetric::str2distance(QString& str)
{
    if(str.contains(" km"))
    {
        return (1000 * str.remove(" km").toDouble());
    }
    else if (str.contains(" m"))
    {
        return str.remove(" m").toDouble();
    }
    return 0;
}
