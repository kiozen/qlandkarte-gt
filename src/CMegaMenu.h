/**********************************************************************************************
    Copyright (C) 2007 Oliver Eichler oliver.eichler@gmx.de

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

#ifndef CMEGAMENU_H
#define CMEGAMENU_H

#include <QLabel>
#include <QPointer>
#include <QVector>
#include "CMenus.h"
class CCanvas;
class QLabel;
class QGridLayout;
class CActions;
class QVBoxLayout;
class QScrollArea;
class QStyleOptionMenuItem;

#define SIZE_OF_MEGAMENU 11

/// the left hand context sensitive menu
class CMegaMenu : public QLabel
{
    Q_OBJECT;
    public:
        virtual ~CMegaMenu();

        static CMegaMenu& self(){return *m_self;}

        void switchByKeyWord(const QString& key);
    public slots:
        void slotSplitterMoved(int pos, int index);

    protected slots:
        void slotEnable(){setEnabled(true);}

    protected:
        void mousePressEvent(QMouseEvent * e);
        void mouseReleaseEvent(QMouseEvent * e);
        void mouseMoveEvent(QMouseEvent * e);
        void paintEvent(QPaintEvent *e);
        void resizeEvent(QResizeEvent * e);
        void leaveEvent ( QEvent * event );
        void initStyleOption(QStyleOptionMenuItem *option, const QAction *action, bool isCurrent) const;
    private:
        friend class CMainWindow;
        CMegaMenu(CCanvas * canvas);
        CMenus *actionGroup;
        CActions *actions;

    private slots:
        void switchState();

    private:
        static CMegaMenu * m_self;
        QPointer<CCanvas>  canvas;

        QString title;

        QRect rectTitle;
        QRect rectF[11];

        int currentItemIndex;
        bool mouseDown;

        int yoff;
};
#endif                           //CMEGAMENU_H
