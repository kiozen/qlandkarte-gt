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

#ifndef COVERLAYDB_H
#define COVERLAYDB_H

#include "IDB.h"
#include "COverlayDistance.h"
#include "COverlayArea.h"

#include <QMap>
#include <proj_api.h>
#ifdef __MINGW32__
#undef LP
#endif

class QPoint;
class QRect;
class IOverlay;
class QPainter;
class QString;
class COverlayText;
class COverlayTextBox;
class QMenu;

class COverlayDB : public IDB
{
    Q_OBJECT;
    public:

        virtual ~COverlayDB();

        static COverlayDB& self(){return *m_self;}

        void draw(QPainter& p, const QRect& r, bool& needsRedraw);

        void loadGPX(CGpx& gpx);
        void saveGPX(CGpx& gpx, const QStringList& keys);
        void loadQLB(CQlb& qlb, bool newKey);
        void saveQLB(CQlb& qlb);
        void upload(const QStringList&){}
        void download(){}
        void clear();
        int count(){return overlays.size();}

        /// get iterator access to track point list
        QMap<QString,IOverlay*>::iterator begin(){return overlays.begin();}
        /// get iterator access to track point list
        QMap<QString,IOverlay*>::iterator end(){return overlays.end();}

        /// delete several overlays by their keys
        void delOverlays(const QStringList& keys);

        void delOverlay(const QString& key, bool silent);

        COverlayText * addText(const QString& text, const QRect& rect, const QString& key = QString(), bool silent = false);
        COverlayTextBox * addTextBox(const QString& text, double lon, double lat, const QPoint& anchor, const QRect& rect, const QString& key = QString(), bool silent = false);
        COverlayDistance * addDistance(const QString& name, const QString& comment, double speed, const QList<COverlayDistance::pt_t>& pts, const QString& key = QString(), bool silent = false);
        COverlayArea * addArea(const QString& name, const QString& comment, const QColor &color, Qt::BrushStyle style, const QList<COverlayArea::pt_t>& pts, const QString& key = QString(), bool silent = false);

        void customMenu(const QString& key, QMenu& menu);

        void looseFocus();

        IOverlay * getOverlayByKey(const QString& key);

        void highlightOverlay(const QString& key);

        /// get highlighted Overlay
        /**
            <b>WARNING</b> The object referenced by the returned
            pointer might be subject to destruction at any time.
            Thus you must use it temporarily or store it by a
            QPointer object.

            @return A pointer to the current highlighted track or 0.
        */
        IOverlay * highlightedOverlay();

        void copyToClipboard(bool deleteSelection = false);
        void pasteFromClipboard();
        void combineDistOvl();

        struct keys_t{QString key; QString name; QString comment; QPixmap icon; quint32 time;};
        /// get all keys in the database
        QList<keys_t> keys();

        void makeVisible(const QStringList& keys);

        bool contains(const QString& key){return overlays.contains(key);}

    private slots:
        void slotModified();

    private:
        friend class CMainWindow;
        friend class COverlayToolWidget;
        COverlayDB(QTabWidget * tb, QObject * parent);
        static COverlayDB * m_self;

        QMap<QString,IOverlay*> overlays;

        bool addOverlaysAsDuplicate;
};
#endif                           //COVERLAYDB_H
