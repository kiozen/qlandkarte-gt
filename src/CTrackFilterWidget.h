/**********************************************************************************************
    Copyright (C) 2012 Oliver Eichler oliver.eichler@gmx.de

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

#ifndef CTRACKFILTERWIDGET_H
#define CTRACKFILTERWIDGET_H

#include <QWidget>
#include <QPointer>

#include "ui_ITrackFilterWidget.h"

class CTrackEditWidget;
class CTrack;
class QMenu;

class CTrackFilterWidget : public QWidget, private Ui::ITrackFilterWidget
{
    Q_OBJECT;
    public:
        CTrackFilterWidget(QWidget * parent);
        virtual ~CTrackFilterWidget();

        void setTrackEditWidget(CTrackEditWidget * w);

    private slots:
        void slotSaveFilter();
        void slotApplyFilter();
        void slotHighlightTrack(CTrack * trk);
        void slotComboMeterFeet(const QString &text);
        void slotResetFilterList();
        void slotAddFilterHidePoints1();
        void slotAddFilterHidePoints2();
        void slotAddFilterSmoothProfile1();
        void slotAddFilterSplit1();
        void slotAddFilterSplit2();
        void slotAddFilterSplit3();
        void slotAddFilterSplit4();
        void slotAddFilterSplit5();
        void slotAddFilterReset();
        void slotAddFilterDelete();
        void slotAddFilterReplaceElevation();
        void slotAddFilterOffsetElevation();
        void slotAddFilterTime1();
        void slotAddFilterTime2();
        void slotAddFilterTime3();

        void slotDoubleClickStoredFilter(QListWidgetItem * item);
        void slotContextMenuStoredFilter( const QPoint & pos);
        void slotStoredFilterEdit();
        void slotStoredFilterDelete();

        void slotResetNow();
        void slotHidePoints1Now();
        void slotHidePoints2Now();
        void slotDeleteNow();
        void slotSmoothProfile1Now();
        void slotReplaceEleNow();
        void slotOffsetEleNow();
        void slotSplit1Now();
        void slotSplit2Now();
        void slotSplit3Now();
        void slotSplit4Now();
        void slotSplit5Now();
        void slotTime1Now();
        void slotTime2Now();
        void slotTime3Now();

        void slotUpdate();

    private:
        void saveFilterList(const QString& filename);
        void loadFilterList(const QString& filename);
        void addFilter(const QString& name, const QString& icon, QByteArray& args);
        bool filterHidePoints1(QDataStream &args, QList<CTrack *> &tracks);
        bool filterHidePoints2(QDataStream &args, QList<CTrack *> &tracks);
        bool filterSmoothProfile1(QDataStream &args, QList<CTrack *> &tracks);
        bool filterSplit1Tracks(QDataStream &args, QList<CTrack *> &tracks);
        bool filterSplit1Stages(QDataStream &args, QList<CTrack *> &tracks);
        bool filterSplit2Tracks(QDataStream &args, QList<CTrack *> &tracks);
        bool filterSplit2Stages(QDataStream &args, QList<CTrack *> &tracks);
        bool filterSplit3Tracks(QDataStream &args, QList<CTrack *> &tracks);
        bool filterSplit3Stages(QDataStream &args, QList<CTrack *> &tracks);
        bool filterSplit4Tracks(QDataStream &args, QList<CTrack *> &tracks);
        bool filterSplit4Stages(QDataStream &args, QList<CTrack *> &tracks);
        bool filterSplit5Tracks(QDataStream &args, QList<CTrack *> &tracks);
        bool filterReset(QDataStream &args, QList<CTrack *> &tracks);
        bool filterDelete(QDataStream &args, QList<CTrack *> &tracks);
        bool filterReplaceElevation(QDataStream &args, QList<CTrack *> &tracks);
        bool filterOffsetElevation(QDataStream &args, QList<CTrack *> &tracks);
        bool filterTime1(QDataStream &args, QList<CTrack *> &tracks);
        bool filterTime2(QDataStream &args, QList<CTrack *> &tracks);
        bool filterTime3(QDataStream &args, QList<CTrack *> &tracks);

        void readGuiReset(QByteArray& args);
        void readGuiHidePoints1(QByteArray& args, double &d, double &a);
        void readGuiHidePoints2(QByteArray& args, double &d);
        void readGuiDelete(QByteArray& args);
        void readGuiSmoothProfile1(QByteArray& args, quint32& tabs);
        void readGuiReplaceEle(QByteArray& args, quint32& type);
        void readGuiOffsetEle(QByteArray& args, double& val, QString& unit);
        void readGuiSplit1(QByteArray& args, double &val);
        void readGuiSplit2(QByteArray& args, double &val);
        void readGuiSplit3(QByteArray& args, double &val);
        void readGuiSplit4(QByteArray& args, double &val);
        void readGuiSplit5(QByteArray& args);
        void readGuiTime1(QByteArray& args, QDateTime &time);
        void readGuiTime2(QByteArray& args, double& speed);
        void readGuiTime3(QByteArray& args, quint32& delta);

        void postProcessTrack();
        void showFilterPartMessage(bool show);

        enum filterType_e
        {
            eHidePoints1
            , eSmoothProfile1
            , eSplit1
            , eSplit2
            , eSplit3
            , eSplit4
            , eReset
            , eDelete
            , eReplaceElevation
            , eHidePoints2
            , eOffsetElevation
            , eSplit5
            , eTime1
            , eTime2
            , eTime3
        };

        enum replaceEleType_e {eLocal, eRemote};

        QPointer<CTrackEditWidget> trackEditWidget;
        QPointer<CTrack> track;

        QMenu * contextMenuStoredFilter;
};
#endif                           //CTRACKFILTERWIDGET_H
