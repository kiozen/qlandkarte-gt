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
#ifndef CTRACKTOOLWIDGET_H
#define CTRACKTOOLWIDGET_H

#include <QWidget>
#include <QPointer>
#include "ui_ITrackToolWidget.h"

class QToolBox;
class QListWidgetItem;
class QMenu;
class QAction;
class CTrackEditWidget;

class CTrackToolWidget : public QWidget, private Ui::ITrackToolWidget
{
    Q_OBJECT;
    public:
        CTrackToolWidget(QTabWidget * parent);
        virtual ~CTrackToolWidget();

        enum sortmode_e
        {
            eSortByName
            ,eSortByTime
        };

        static sortmode_e  getSortMode(){return sortmode;}

    public slots:
        void slotEdit();
        void slotShowProfile();
        void slotDBChanged();

    protected:
        void keyPressEvent(QKeyEvent * e);
        bool eventFilter(QObject *obj, QEvent *event);

    private slots:
        void slotItemDoubleClicked(QListWidgetItem * item);
        void slotItemClicked(QListWidgetItem * item);
        void slotSelectionChanged();
        void slotContextMenu(const QPoint& pos);
        void slotDelete();
        void slotToOverlay();
        void slotShow();
        void slotZoomToFit();
        void slotRevert();
        void slotShowBullets();
        void slotShowMinMax();

    private:
        bool originator;

        QMenu * contextMenu;

        QPointer<CTrackEditWidget> trackedit;

        QAction * actEdit;
        QAction * actDistance;
        QAction * actHide;
        QAction * actZoomToFit;
        QAction * actDel;
        QAction * actRevert;
        QAction * actShowBullets;
        QAction * actShowMinMax;

        static sortmode_e sortmode;

};
#endif                           //CTRACKTOOLWIDGET_H
