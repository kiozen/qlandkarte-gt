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

#include "CDlgTrackFilter.h"
#include "CTrack.h"
#include "CTrackDB.h"
#include "GeoMath.h"
#include "IUnit.h"
#include "CSettings.h"

#include <QtGui>
#include <QProgressDialog>

// Change this when changing comboMeterFeet
enum meter_feet_index
{
    METER_INDEX,
    FEET_INDEX,
};

CDlgTrackFilter::CDlgTrackFilter(CTrack &track, QWidget * parent)
: QDialog(parent)
, track(track)
{
    setupUi(this);

    SETTINGS;

    checkReduceDataset->setChecked(false);
    checkModifyTimestamps->setChecked(false);

    QList<CTrack::pt_t>& trkpts = track.getTrackPoints();
    QList<CTrack::pt_t>::iterator trkpt   = trkpts.begin();

    if(IUnit::self().baseunit == "ft")
    {
        comboMeterFeet->setCurrentIndex((int)FEET_INDEX);
        spinDistance->setSuffix("ft");
    }
    else
    {
        comboMeterFeet->setCurrentIndex((int)METER_INDEX);
        spinDistance->setSuffix("m");
    }
    spinDistance->setValue(cfg.value("trackfilter/distance",10).toInt());
    spinAzimuthDelta->setValue(cfg.value("trackfilter/azimuthdelta",10).toInt());
    checkAzimuthDelta->setChecked(cfg.value("trackfilter/useazimuthdelta", false).toBool());

    if(trkpt->timestamp == 0x000000000 || trkpt->timestamp == 0xFFFFFFFF)
    {
        // no track time available
        tabTimestamp->setEnabled(false);
        radioTimedelta->setEnabled(false);
        spinTimedelta->setEnabled(false);
        //        qDebug() << "Track has no timestamps that could be modified.";
    }
    else
    {
        tabTimestamp->setEnabled(true);
        radioTimedelta->setEnabled(true);
        spinTimedelta->setEnabled(true);

        QDateTime t = QDateTime::fromTime_t(trkpt->timestamp).toLocalTime();
        datetimeStartTime->setDateTime(t);
        radioLocalTime->setChecked(true);
        radioUTC->setChecked(false);
    }

    spinTimedelta->setValue(cfg.value("trackfilter/timedelta",5).toInt());

    quint32 cnt = track.getMedianFilterCount();
    checkMedian->setText(tr("Smooth profile (Median filter, %1 tabs)").arg(5 + (cnt<<1)));

    // user-tunable elements on "Modify Timestamps" tab
    connect(buttonReset1stOfMonth, SIGNAL(clicked()), this, SLOT(slotReset1stOfMonth()));
    connect(buttonResetEpoch, SIGNAL(clicked()), this, SLOT(slotResetEpoch()));
    connect(datetimeStartTime, SIGNAL(dateTimeChanged(const QDateTime &)), this, SLOT(slotDateTimeChanged(const QDateTime &)));

    // user-tunable elements on "Reduce Dataset" tab
    connect(radioDistance, SIGNAL(clicked()), this, SLOT(slotRadioDistance()));
    connect(checkAzimuthDelta, SIGNAL(clicked()), this, SLOT(slotCheckAzimuthDelta()));
    connect(radioTimedelta, SIGNAL(clicked()), this, SLOT(slotRadioTimedelta()));
    connect(spinDistance, SIGNAL(valueChanged(int)), this, SLOT(slotSpinDistance(int)));
    connect(spinAzimuthDelta, SIGNAL(valueChanged(int)), this, SLOT(slotSpinAzimuthDelta(int)));
    connect(spinTimedelta, SIGNAL(valueChanged(int)), this, SLOT(slotSpinTimedelta(int)));
    connect(comboMeterFeet, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(slotComboMeterFeet(const QString &)));
    connect(checkMedian, SIGNAL(clicked()), this, SLOT(slotCheckMedian()));

    connect(radioSplitChunks, SIGNAL(clicked()), this, SLOT(slotRadioSplitChunks()));
    connect(radioSplitPoints, SIGNAL(clicked()), this, SLOT(slotRadioSplitPoints()));
    connect(spinSplitChunks, SIGNAL(valueChanged(int)), this, SLOT(slotSplitChunks(int)));
    connect(spinSplitPoints, SIGNAL(valueChanged(int)), this, SLOT(slotSplitPoints(int)));

}


CDlgTrackFilter::~CDlgTrackFilter()
{

}


void CDlgTrackFilter::accept()
{
    if(checkSplitTrack->isChecked())
    {
        splitTrack(&track);
    }
    else
    {
        modifyTimestamp(&track);
        reduceDataset(&track);
    }

    SETTINGS;
    cfg.setValue("trackfilter/distance",spinDistance->value());
    cfg.setValue("trackfilter/azimuthdelta",spinAzimuthDelta->value());
    cfg.setValue("trackfilter/timedelta",spinTimedelta->value());
    cfg.setValue("trackfilter/useazimuthdelta",checkAzimuthDelta->isChecked());

    QDialog::accept();
}


void CDlgTrackFilter::splitTrack(CTrack * trk)
{

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    QList<CTrack::pt_t>& trkpts         = trk->getTrackPoints();
    QList<CTrack::pt_t>::iterator trkpt = trkpts.begin();
    int npts = trkpts.count();

    QProgressDialog progress(tr("Filter track..."), tr("Abort filter"), 0, npts, this);
    progress.setWindowTitle("Filter Progress");
    progress.setWindowModality(Qt::WindowModal);

    int chunk;
    if(radioSplitChunks->isChecked())
    {
        int n = spinSplitChunks->value();
        chunk = (trkpts.size() + n) / n;
    }
    else
    {
        chunk = spinSplitPoints->value();
    }

    int totalCnt    = 0;
    int trkptCnt    = 0;
    int trkCnt      = 1;

    CTrack * track1 = new CTrack(&CTrackDB::self());
    track1->setName(trk->getName() + QString("_%1").arg(trkCnt++));
    track1->setColor(trk->getColorIdx());

    while(trkpt != trkpts.end())
    {

        progress.setValue(totalCnt++);
        qApp->processEvents();

        *track1 << *trkpt;
        if(++trkptCnt >= chunk)
        {
            modifyTimestamp(track1);
            reduceDataset(track1);

            CTrackDB::self().addTrack(track1, true);

            trkptCnt = 0;
            track1 = new CTrack(&CTrackDB::self());
            track1->setName(trk->getName() + QString("_%1").arg(trkCnt++));
            track1->setColor(trk->getColorIdx());
        }

        trkpt++;
    }

    if(trkptCnt)
    {
        modifyTimestamp(track1);
        reduceDataset(track1);

        CTrackDB::self().addTrack(track1, false);
    }
    else
    {
        delete track1;
    }

    QApplication::restoreOverrideCursor();

}


void CDlgTrackFilter::modifyTimestamp(CTrack * trk)
{
    if(tabTimestamp->isEnabled() && checkModifyTimestamps->isChecked())
    {
        QList<CTrack::pt_t>& trkpts = trk->getTrackPoints();
        int npts = trkpts.count();

        QProgressDialog progress(tr("Filter track..."), tr("Abort filter"), 0, npts, this);
        progress.setWindowTitle("Filter Progress");
        progress.setWindowModality(Qt::WindowModal);

        QList<CTrack::pt_t>::iterator trkpt   = trkpts.begin();

        if(trkpt->timestamp != 0x000000000 && trkpt->timestamp != 0xFFFFFFFF)
        {
            QDateTime t;
            if(radioLocalTime->isChecked())
            {
                t = QDateTime::fromTime_t(trkpt->timestamp).toLocalTime();
            }
            else
            {
                t = QDateTime::fromTime_t(trkpt->timestamp).toUTC();
            }
            QDateTime tn = datetimeStartTime->dateTime();
            int offset = (int)tn.toTime_t() - (int)t.toTime_t();
            int i = 0;

            while(trkpt != trkpts.end())
            {
                if(radioDelta1s->isChecked())
                {
                    trkpt->timestamp = tn.toTime_t() + i;
                }
                else
                {
                    trkpt->timestamp += offset;
                }
                ++trkpt;
                progress.setValue(i);
                qApp->processEvents();
                ++i;
                if (progress.wasCanceled())
                {
                    break;
                }
            }
        }
        progress.setValue(npts);

        track.setTimestamp(trkpts.begin()->timestamp);
        track.rebuild(false);
    }
}


void CDlgTrackFilter::reduceDataset(CTrack * trk)
{
    if(checkReduceDataset->isChecked())
    {
        QList<CTrack::pt_t>& trkpts = trk->getTrackPoints();
        int npts = trkpts.count();

        QProgressDialog progress(tr("Filter track..."), tr("Abort filter"), 0, npts, this);
        progress.setWindowTitle("Filter Progress");
        progress.setWindowModality(Qt::WindowModal);

        QList<CTrack::pt_t>::iterator trkpt   = trkpts.begin();

        if(radioTimedelta->isEnabled() && radioTimedelta->isChecked())
        {

            quint32 timedelta = spinTimedelta->value();
            quint32 lasttime = trkpt->timestamp;
            ++trkpt;

            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

            int i = 1;
            while(trkpt != trkpts.end())
            {
                if(trkpt->timestamp < lasttime + timedelta)
                {
                    trkpt->flags |= CTrack::pt_t::eDeleted;
                }
                else
                {
                    lasttime = trkpt->timestamp;
                }
                ++trkpt;
                ++i;
                progress.setValue(i);
                qApp->processEvents();
                if (progress.wasCanceled())
                    break;
            }

            QApplication::restoreOverrideCursor();
        }
        else if(radioDistance->isChecked())
        {
            float min_distance = spinDistance->value();
            float minAzimuthDelta = spinAzimuthDelta->value();
            float lastAzimuth = trkpt->azimuth;
            float AzimuthDelta = 0;

            if(spinDistance->suffix() == "ft")
            {
                min_distance *= 0.3048f;
            }
            projXY p1, p2;
            p1.u = DEG_TO_RAD * trkpt->lon;
            p1.v = DEG_TO_RAD * trkpt->lat;
            ++trkpt;

            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

            int i = 1;

            double lastEle = trkpt->ele;

            bool checkAzimuth = checkAzimuthDelta->isEnabled() && checkAzimuthDelta->isChecked();

            while(trkpt != trkpts.end())
            {

                //qDebug() << "LastAzimuth: " << lastAzimuth << "   Azimuth: "<< trkpt->azimuth << " ABS(Azimuth-LastAzimuth): " << abs(trkpt->azimuth - lastAzimuth);

                p2.u = DEG_TO_RAD * trkpt->lon;
                p2.v = DEG_TO_RAD * trkpt->lat;
                double a1, a2;

                double delta = distance(p1,p2,a1,a2);

                if(checkAzimuth)
                {
                    if (abs(trkpt->azimuth) <= 180)
                    {
                        if(abs(trkpt->azimuth - lastAzimuth) > 180)
                        {
                            AzimuthDelta = 360 - abs(trkpt->azimuth - lastAzimuth);
                        }
                        else
                        {
                            AzimuthDelta = abs(trkpt->azimuth - lastAzimuth);
                        }
                    }
                    else
                    {
                        AzimuthDelta = 0;
                    }
                }

                double deltaEle = abs(lastEle - trkpt->ele);

                if (delta < min_distance || (checkAzimuth && AzimuthDelta < minAzimuthDelta))
                {
                    if(deltaEle < 3)
                    {
                        trkpt->flags |= CTrack::pt_t::eDeleted;
                    }

                }
                else
                {
                    p1 = p2;
                    progress.setValue(i);
                    qApp->processEvents();
                    if(AzimuthDelta >= minAzimuthDelta)
                    {
                        lastAzimuth = trkpt->azimuth;
                    }

                    lastEle = trkpt->ele;
                }
                ++trkpt;
                ++i;
                if (progress.wasCanceled())
                {
                    break;
                }
            }

            QApplication::restoreOverrideCursor();
        }

        // can be done in parallel to other reductions
        if(checkMedian->isChecked())
        {
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            trk->medianFilter(5, progress);
            QApplication::restoreOverrideCursor();
        }
        progress.setValue(npts);
        track.rebuild(false);
    }
}


void CDlgTrackFilter::slotReset1stOfMonth()
{
    QDateTime t = datetimeStartTime->dateTime();

    int day = t.date().day();
    int hour = t.time().hour();
    int offset = (day - 1) * 86400 + hour * 3600;
    QDateTime tn = t.addSecs(-offset);

    //    qDebug() << "Resetting starttime:" << t.toString("yyyy-MM-dd'T'hh:mm:ss'Z'")
    //        << "to:" << tn.toString("yyyy-MM-dd'T'hh:mm:ss'Z'");

    datetimeStartTime->setDateTime(tn);

    checkModifyTimestamps->setChecked(true);
}


void CDlgTrackFilter::slotResetEpoch()
{
    QDateTime t = datetimeStartTime->dateTime();

    //    qDebug() << "Resetting starttime:" << t.toString("yyyy-MM-dd'T'hh:mm:ss'Z'")
    //        << "to epoch";

    QDateTime tn;
    radioUTC->setChecked(true);
    radioLocalTime->setChecked(false);
    tn.setTimeSpec(Qt::UTC);
    tn.setDate(QDate(1970, 1, 1));
    tn.setTime(QTime(0, 0, 0));

    datetimeStartTime->setDateTime(tn);

    checkModifyTimestamps->setChecked(true);
}


void CDlgTrackFilter::slotDateTimeChanged(const QDateTime &tn)
{
    checkModifyTimestamps->setChecked(true);

    //    qDebug() << "Resetting starttime to:" << tn.toString("yyyy-MM-dd'T'hh:mm:ss'Z'");
}


void CDlgTrackFilter::slotRadioDistance()
{
    checkAzimuthDelta->setEnabled(true);
    checkReduceDataset->setChecked(true);
    if(checkAzimuthDelta->isChecked() && checkAzimuthDelta->isEnabled())
    {
        spinAzimuthDelta->setEnabled(true);
    }
}


void CDlgTrackFilter::slotCheckMedian()
{
    checkReduceDataset->setChecked(true);
}


void CDlgTrackFilter::slotSpinDistance(int i)
{
    radioDistance->setChecked(true);
    checkAzimuthDelta->setEnabled(true);
    radioTimedelta->setChecked(false);
    checkReduceDataset->setChecked(true);
    if(checkAzimuthDelta->isChecked() && checkAzimuthDelta->isEnabled())
    {
        spinAzimuthDelta->setEnabled(true);
    }
}


void CDlgTrackFilter::slotCheckAzimuthDelta()
{
    if(checkAzimuthDelta->isChecked())
    {
        spinAzimuthDelta->setEnabled(true);
    }
    else
    {
        spinAzimuthDelta->setEnabled(false);
    }
}


void CDlgTrackFilter::slotSpinAzimuthDelta(int i)
{
    checkAzimuthDelta->setChecked(true);
    spinAzimuthDelta->setEnabled(true);
}


void CDlgTrackFilter::slotRadioTimedelta()
{
    checkAzimuthDelta->setEnabled(false);
    spinAzimuthDelta->setEnabled(false);
    checkReduceDataset->setChecked(true);
}


void CDlgTrackFilter::slotSpinTimedelta(int i)
{
    radioTimedelta->setChecked(true);
    radioDistance->setChecked(false);
    checkAzimuthDelta->setEnabled(false);
    checkReduceDataset->setChecked(true);
}


void CDlgTrackFilter::slotComboMeterFeet(const QString &text)
{
    spinDistance->setSuffix(text);
}


void CDlgTrackFilter::slotRadioSplitChunks()
{

    checkSplitTrack->setChecked(true);
}


void CDlgTrackFilter::slotRadioSplitPoints()
{

    checkSplitTrack->setChecked(true);
}


void CDlgTrackFilter::slotSplitChunks(int)
{
    radioSplitChunks->setChecked(true);
    radioSplitPoints->setChecked(false);

    checkSplitTrack->setChecked(true);
}


void CDlgTrackFilter:: slotSplitPoints(int)
{
    radioSplitChunks->setChecked(false);
    radioSplitPoints->setChecked(true);

    checkSplitTrack->setChecked(true);
}
