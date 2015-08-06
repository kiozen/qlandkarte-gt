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

#ifndef CPLOTDATA_H
#define CPLOTDATA_H

#include <QObject>
#include <QColor>
#include <QPolygonF>
#include <QPixmap>

class CPlotAxis;

class CPlotData : public QObject
{
    public:
        enum axis_type_e {eLinear, eTime};

        CPlotData(axis_type_e type, QObject * parent);
        virtual ~CPlotData();

        ///get a reference to the x axis
        CPlotAxis& x(){return *xaxis;}
        ///get a reference to the y axis
        CPlotAxis& y(){return *yaxis;}

        /// setup all internal data to fit the dynamic range of all data points
        void setLimits();

        struct line_t
        {
            QString label;
            QColor color;
            QPolygonF points;
        };

        /// text shown below the x axis
        QString xlabel;
        /// text shown left of the y axis
        QString ylabel;
        /// set true for grid
        bool grid;

        /// list of plot lines
        QList<line_t> lines;
        /// marks on line1
        line_t marks;

        struct point_t
        {
            QColor color;
            QPointF point;
            QPixmap icon;
            QString label;
        };

        QList<QPointF> focus;

        /// vector of plot tags such as waypoints
        QVector<point_t> tags;

        bool badData;

        axis_type_e axisType;

        double xmin;
        double xmax;
        double ymin;
        double ymax;

    protected:
        CPlotAxis * xaxis;
        CPlotAxis * yaxis;
};
#endif                           //CPLOTDATA_H
