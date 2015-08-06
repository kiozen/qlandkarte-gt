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
#ifndef CTRACKEDITWIDGET_H
#define CTRACKEDITWIDGET_H

#include <QWidget>
#include <QObject>
#include <QBoxLayout>
#include <QPointer>
#include <CGpxExtension.h>
#include <QVBoxLayout>
#include "ui_ITrackEditWidget.h"
#include "CTrack.h"

class CTrackStatProfileWidget;
class CTrackStatSpeedWidget;
class CTrackStatTraineeWidget;
class CTrackStatDistanceWidget;
class QMenu;

#ifdef GPX_EXTENSIONS
class CTrackStatExtensionWidget;
#endif

class CTrackTreeWidgetItem : public QTreeWidgetItem, public QObject
{

    public:
        CTrackTreeWidgetItem(QTreeWidget *tree)
            : QTreeWidgetItem(tree)
            , QObject(tree)
        {
        }

        CTrackTreeWidgetItem ( QTreeWidget * parent, const QStringList & strings)
            : QTreeWidgetItem (parent,strings)
            , QObject(parent)
        {
        }

        bool operator< ( const QTreeWidgetItem & other ) const;
};

class CTrackEditWidget : public QWidget, private Ui::ITrackEditWidget
{
    Q_OBJECT;
    public:
        CTrackEditWidget(QWidget * parent);
        virtual ~CTrackEditWidget();

        signals:
        void sigZoomToDistance(float d1, float d2);
        void sigZoomToTime(quint32 t1, quint32 t2);

    public slots:
        void slotSetTrack(CTrack * t);
        void slotPointSelectionChanged();
        void slotPointSelection(QTreeWidgetItem * item);
        void slotPurge();
        void slotUpdate();
        void slotToggleStatDistance();
        void slotToggleStatTime();
        void slotToggleTrainee();
        void slotShowProfile();
        void slotReset();
        void slotDelete();

#ifdef GPX_EXTENSIONS
        void slotToggleExtensionsGraph();
        void slotSetColumns(bool checked);
        void slotSetColumnsExt(bool checked);
#endif
        void slotGoogleMaps();
        void slotKillTab(int index);
        void slotResetAllZoom();

    protected slots:
        void slotContextMenu(const QPoint& pos);
        void slotSplit();
        void slotColorChanged(int idx);
        void slotNameChanged();
        void slotNameChanged(const QString& name);
        void slotCurrentChanged(int idx);
        void slotStagesChanged();
        void slotStagesChanged(int state);
        void slotPointOfFocus(const int idx);
        void slotHighlightArea(const QString& key);
        void slotToggleMultiColor(bool on);
        void slotMultiColorMode(int idx);
        void slotMultiColorConfig();

    protected:
        void keyPressEvent(QKeyEvent * e);
        void resizeEvent(QResizeEvent * e);

    private:
        void updateStages();
        enum columns_e
        {
            eNum       = 0
            ,eTime      = 1
            ,eAltitude  = 2
            ,eDelta     = 3
            ,eAzimuth   = 4
            ,eDistance  = 5
            ,eSpeed     = 6
            ,eAscend    = 7
            ,eDescend   = 8
            ,ePosition  = 9
            ,eMaxColumn = 10
        };

        enum eTblCol{eSym, eInfo, eProx, ePic, eEleWpt, eEleTrk, eToNextDist, eToNextTime, eToNextAsc, eToNextDesc, eTotalDist, eTotalTime, eTotalAsc, eTotalDesc, eComment, eMax};

        enum eTabs {eStages, ePoints, eFilter, eSetup};

        QPointer<CTrack> track;

        bool originator;

        QPointer<CTrackStatProfileWidget> trackStatProfileDist;
        QPointer<CTrackStatSpeedWidget> trackStatSpeedDist;
        QPointer<CTrackStatProfileWidget> trackStatProfileTime;
        QPointer<CTrackStatSpeedWidget> trackStatSpeedTime;
        QPointer<CTrackStatDistanceWidget> trackStatDistanceTime;
        QPointer<CTrackStatTraineeWidget> trackStatTrainee;

#ifdef GPX_EXTENSIONS
        QList<QCheckBox *> c_boxes;

        //QList with all extension tabs made for further handling
        QList<CTrackStatExtensionWidget *> trackStatExtensions;

        QSpacerItem *Vspace;

        int tabstat;
        int no_ext_info_stat;
#endif

        QMenu * contextMenu;
        QAction * actSplit;

        QPointer<QTextTable> table;

        QSize oldSize;

        struct stage_t
        {
            double x1;
            double x2;
        };

        QMap<QString, stage_t> stages;
        QString lastStageKey;
};
#endif                           //CTRACKEDITWIDGET_H
