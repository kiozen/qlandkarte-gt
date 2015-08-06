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

#include "CUnitImperial.h"

CUnitImperial::CUnitImperial(QObject * parent)
: IUnit("imperial", "ft", 3.28084f, "ml/h", 2.23693164f, parent)
{

}


CUnitImperial::~CUnitImperial()
{

}


void CUnitImperial::meter2elevation(float meter, QString& val, QString& unit)
{
    val.sprintf("%1.0f", meter * 3.28084);
    unit = "ft";
}


void CUnitImperial::meter2distance(float meter, QString& val, QString& unit)
{

    if(meter < 10)
    {
        val.sprintf("%1.1f", meter * 3.28084);
        unit = "ft";
    }
    else if(meter < 1600)
    {
        val.sprintf("%1.0f", meter * 3.28084);
        unit = "ft";
    }
    else if(meter < 16000)
    {
        val.sprintf("%1.2f", meter * 0.6213699E-3);
        unit = "ml";
    }
    else if(meter < 32000)
    {
        val.sprintf("%1.1f", meter * 0.6213699E-3);
        unit = "ml";
    }
    else
    {
        val.sprintf("%1.0f", meter * 0.6213699E-3);
        unit = "ml";
    }
}


float CUnitImperial::elevation2meter(const QString& val)
{
    return val.toDouble() / 3.28084;
}


float CUnitImperial::str2speed(QString& str)
{
    return (str.remove(" ml/h").toDouble() / 0.6213699);
}


float CUnitImperial::str2distance(QString& str)
{
    if(str.contains(" ml"))
    {
        return (str.remove(" ml").toDouble() / 0.6213699E-3);
    }
    else if (str.contains(" ft"))
    {
        return (str.remove(" ft").toDouble() * 0.305);
    }
    return 0;
}
