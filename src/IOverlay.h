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
#ifndef IOVERLAY_H
#define IOVERLAY_H

#include "IItem.h"

#include <QKeyEvent>
#include <QObject>
#include <QPixmap>
#include <QPointer>

class QPainter;
class QMouseEvent;
class QFile;
class COverlayDB;
class QMenu;

/// base class for any kind of overlays other than waypoints, tracks or routes
class IOverlay : public IItem
{
    Q_OBJECT;
    public:
        /**
            @param parent pointer to parent object
            @param type a type string to identify the subclass
            @param icon an icon to display in a list widget
        */
        IOverlay(QObject * parent, const QString& type, const QString& iconString);
        virtual ~IOverlay();

        enum type_e {eEnd,eBase};

        /// draw what ever you want
        virtual void draw(QPainter& p, const QRect& viewport) = 0;
        /// return a short string to be displayed in a list widget
        virtual QString getInfo(){return tr("No info set");}

        /// return true if coursor is close to the overlay to redirect mouse events into the overlay
        virtual bool isCloseEnough(const QPoint& pt) = 0;
        /// return true if some mouse action is in progress
        /**
            Some mouse actions will leave the rectangle returend by getRect(). However the
            cursor state machine must not deselect the overlay in this case. Simply return
            true, to keep it from deselecting the overlay.
        */
        virtual bool mouseActionInProgress(){return false;}

        /// get key press event when selected
        virtual void keyPressEvent(QKeyEvent * e){}
        /// get key release event when selected
        virtual void keyRelaseEvent(QKeyEvent * e){}

        /// get mouse move event when selected
        virtual void mouseMoveEvent(QMouseEvent * e){}
        /// get mouse press event when selected
        virtual void mousePressEvent(QMouseEvent * e){}
        /// get mouse release event when selected
        virtual void mouseReleaseEvent(QMouseEvent * e){}

        /// save overlay data to datastream
        virtual void save(QDataStream& s){}
        /// load overlay data from datastream
        virtual void load(QDataStream& s){}
        /// set the static selected pointer
        static void select(IOverlay * s);

        /// move map to make overlay visible
        virtual void makeVisible(){}

        virtual void customMenu(QMenu& menu){}

        virtual void looseFocus(){}

        virtual QRectF getBoundingRectF(){return QRectF();}

        /// the overlay type as string
        const QString type;

        void setHighlight(bool on){highlight = on;}
        bool isHighlighted(){return highlight;}

        void setIcon(const QString& str);

        bool visible(){return isVisible;}

    protected:
        friend class COverlayDB;

        static QPointer<IOverlay> selected;

        /// set true to draw overlay highlighted
        bool highlight;

        bool isVisible;

};

QDataStream& operator >>(QDataStream& s, COverlayDB& db);
QDataStream& operator <<(QDataStream& s, IOverlay& ovl);

void operator >>(QFile& f, COverlayDB& db);
void operator <<(QFile& f, IOverlay& ovl);
#endif                           //IOVERLAY_H
