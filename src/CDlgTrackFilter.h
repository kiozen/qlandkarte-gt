/**********************************************************************************************
    Copyright (C) 2009 Joerg Wunsch <j@uriah.heep.sax.de>

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
#ifndef CDLGTRACKFILTER_H
#define CDLGTRACKFILTER_H

#include <QDialog>
#include <QDateTime>

#include "ui_IDlgTrackFilter.h"

class CTrack;

/// dialog to edit waypoint properties
class CDlgTrackFilter : public QDialog, private Ui::IDlgTrackFilter
{
    Q_OBJECT;
    public:
        CDlgTrackFilter(CTrack &track, QWidget * parent);
        virtual ~CDlgTrackFilter();

    public slots:
        void accept();

    private slots:
        // "Modify Timestamp" tab slots
        void slotReset1stOfMonth();
        void slotResetEpoch();
        void slotDateTimeChanged(const QDateTime & datetime);

        // "Reduce Dataset" tab slots
        void slotRadioDistance();
        void slotCheckAzimuthDelta();
        void slotRadioTimedelta();
        void slotSpinDistance(int i);
        void slotSpinAzimuthDelta(int i);
        void slotSpinTimedelta(int i);
        void slotComboMeterFeet(const QString &text);
        void slotCheckMedian();

        void slotRadioSplitChunks();
        void slotRadioSplitPoints();
        void slotSplitChunks(int);
        void slotSplitPoints(int);

    private:
        void modifyTimestamp(CTrack * trk);
        void reduceDataset(CTrack * trk);
        void splitTrack(CTrack * trk);
        CTrack &track;
};
#endif                           //CDLGTRACKFILTER_H
