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
#ifndef IUNIT_H
#define IUNIT_H
#include <QObject>

class IUnit : public QObject
{
    Q_OBJECT;
    public:
        virtual ~IUnit();

        static IUnit& self(){return *m_self;}
        /// convert meter of elevation into a value and unit string
        virtual void meter2elevation(float meter, QString& val, QString& unit) = 0;
        /// convert meter of distance into a value and unit string
        virtual void meter2distance(float meter, QString& val, QString& unit) = 0;
        /// convert meter per second to a speed value string and unit label
        virtual void meter2speed(float meter, QString& val, QString& unit);

        virtual float elevation2meter(const QString& val) = 0;

        virtual float str2speed(QString& str) = 0;
        virtual float str2distance(QString& str) = 0;

        const QString type;
        const QString baseunit;
        const float   basefactor;
        const QString speedunit;
        const float   speedfactor;

    protected:
        friend class CResources;
        IUnit(const QString& type, const QString& baseunit, const float basefactor, const QString& speedunit, const float speedfactor, QObject * parent);

    private:
        static IUnit * m_self;
};
#endif                           //IUNIT_H
