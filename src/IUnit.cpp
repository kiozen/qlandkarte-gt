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
#include "IUnit.h"

IUnit * IUnit::m_self = 0;

IUnit::IUnit(const QString& type, const QString& baseunit, const float basefactor, const QString& speedunit, const float speedfactor, QObject * parent)
: QObject(parent)
, type(type)
, baseunit(baseunit)
, basefactor(basefactor)
, speedunit(speedunit)
, speedfactor(speedfactor)
{
    //there can be only one...
    if(m_self) delete m_self;
    m_self = this;
}


IUnit::~IUnit()
{

}


void IUnit::meter2speed(float meter, QString& val, QString& unit)
{
    val.sprintf("%1.2f",meter * speedfactor);
    unit = speedunit;
}
