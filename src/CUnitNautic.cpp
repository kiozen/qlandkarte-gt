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

#include "CUnitNautic.h"

// 1 nm = 1852 meters (exactly!)
// 1 ft = 0.3048 meter (exactly!)
CUnitNautic::CUnitNautic(QObject * parent)
: IUnit("nautic", "nm", 0.00053996f, "kt", 1.94384449f, parent)
{

}


CUnitNautic::~CUnitNautic()
{

}


void CUnitNautic::meter2elevation(float meter, QString& val, QString& unit)
{
    val.sprintf("%1.0f", meter / 0.3048);
    unit = "ft";
}


void CUnitNautic::meter2distance(float meter, QString& val, QString& unit)
{
    val.sprintf("%1.2f", meter * basefactor);
    unit = baseunit;
}


void CUnitNautic::meter2speed(float meter, QString& val, QString& unit)
{
    val.sprintf("%1.2f",meter * speedfactor);
    unit = speedunit;
}


float CUnitNautic::elevation2meter(const QString& val)
{
    return val.toDouble() * 0.3048;
}


float CUnitNautic::str2speed(QString& str)
{
    return (str.remove(" kt").toDouble() / speedfactor);
}


float CUnitNautic::str2distance(QString& str)
{
    if (str.contains(" nm"))
    {
        return (str.remove(" nm").toDouble() / basefactor);
    }
    else if (str.contains(" ft"))
    {
        return (str.remove(" ft").toDouble() * 0.3048);
    }
    return 0;
}
