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

#ifndef CPLOTAXIS_H
#define CPLOTAXIS_H

#include <QObject>
#include <QFontMetrics>

class CPlotAxis : public QObject
{
    Q_OBJECT
        public:
        CPlotAxis(QObject * parent);
        virtual ~CPlotAxis();

        /// tic mark information structure
        struct TTic
        {
            TTic(){val=0;lbl="";}
            double val;
            QString lbl;
        };

        ///tic type
        enum ETicType
        {
            notic,               /**< no tics are produced*/
            minmax,              /**< only min max tics are produced*/
            norm,                /**< tics by interval*/
            full                 /**< minmax && norm*/
        };

        ///zoom in/out with a given point as static
        virtual void zoom(bool in, int point);
        ///set the desired minimum and maximum value equal to limit values
        virtual void resetZoom();
        ///add delta_pt to min and max values
        virtual void move(int delta_pt);
        ///set the desired minimum and maximum value
        virtual void setMinMax(double min, double max);
        ///set the limit minimum and maximum value
        virtual void setLimits(double min, double max);
        ///set the scale factor for a given size in points
        virtual void setScale(const unsigned int pts);
        ///calculate format for the given value
        virtual const QString fmtsgl(double val);
        ///calculate format for the given value
        virtual const QString fmtdbl(double val);
        ///get the maximum width of a scale with provided fontmetrics
        virtual int getScaleWidth(const QFontMetrics& m);
        ///get a new ticmark object
        virtual const TTic* ticmark(const TTic * t = NULL);
        /// get the total limits and the used ones
        virtual void getLimits(double& limMin, double& limMax, double& useMin, double& useMax);

        inline int val2pt( double val )
        {
            if ( scale == 0 )
            {
                return 0;
            }
            return ( int ) ( ( val - used_min ) * scale + 0.5 );
        }

        inline double pt2val( int pt )
        {
            if ( scale == 0 )
            {
                return 0;
            }
            return ( double ) ( ( (double)pt - 0.5 ) / scale + used_min );
        }

        void setAutoscale(bool on){autoscale = on;}

        inline ETicType getTicType(){return tic_type;}
        inline ETicType setTicType(ETicType t)
        {
            ETicType old = tic_type;
            tic_type = t;
            return old;
        }

        double min(){return used_min;}
        double max(){return used_max;}

    protected:
        virtual void calc();

        ///true if axis has been initialized
        bool initialized;
        ///true if autoscaling
        bool autoscale;

        ///scalefactor
        double scale;

        ///the actual applied min value
        double used_min;
        ///the actual applied max value
        double used_max;

        double limit_min;
        double limit_max;

        ///the intervall of the ticmarks
        double interval;

        ///start value of the tic marks
        double tic_start;

        /// this is set to -1 by default
        /**
            a value > 0 will override the dynamic value in getScaleWidth();
        */
        qint32 scaleWidth;

        ///the ticmark generation type
        ETicType tic_type;
        ///local copy of the last ticmark object
        TTic tic;

        /// used by ticmark()
        bool firstTic;
        /// used by ticmark()
        bool lastTic;

        ///points of dimension
        quint32 points;

};
#endif                           //CPLOTAXIS_H
