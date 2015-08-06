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
#ifndef CUNITMETRIC_H
#define CUNITMETRIC_H

#include "IUnit.h"

class CUnitMetric : public IUnit
{
    Q_OBJECT;
    public:
        CUnitMetric(QObject * parent);
        virtual ~CUnitMetric();

        void meter2elevation(float meter, QString& val, QString& unit);
        void meter2distance(float meter, QString& val, QString& unit);
        void meter2speed(float meter, QString& val, QString& unit);
        float elevation2meter(const QString& val);

        float str2speed(QString& str);
        float str2distance(QString& str);

};
#endif                           //CUNITMETRIC_H
