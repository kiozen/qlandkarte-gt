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
#ifndef CTRACKDB_H
#define CTRACKDB_H

#include "IDB.h"
#include <QRectF>
#include <QMap>
#include <QString>
#include <QPixmap>

class QToolBox;
class CTrack;
class CTrackToolWidget;
class QUndoStack;

class CTrackDB : public IDB
{
    Q_OBJECT;
    public:
        virtual ~CTrackDB();

        static CTrackDB& self(){return *m_self;}

        void loadGPX(CGpx& gpx);
        void saveGPX(CGpx& gpx, const QStringList& keys);
        void loadQLB(CQlb& qlb, bool asDuplicat);
        void saveQLB(CQlb& qlb);

        void upload(const QStringList& keys);
        void download();

        void clear();

        void addTrack(CTrack* track, bool silent);
        void delTrack(const QString& key, bool silent);
        void delTracks(const QStringList& keys);
        void CombineTracks();
        void splitTrack(int idx);
        void revertTrack(const QString& key);

        void highlightTrack(const QString& key);
        /// get highlighted track
        /**
            <b>WARNING</b> The object referenced by the returned
            pointer might be subject to destruction at any time.
            Thus you must use it temporarily or store it by a
            QPointer object.

            @return A pointer to the current highlighted track or 0.
        */
        CTrack* highlightedTrack();

        void hideTrack(const QStringList& keys, bool hide);

        /// get access to track dictionary
        const QMap<QString,CTrack*>& getTracks(){return tracks;}

        CTrackToolWidget * getToolWidget();

        int count(){return tracks.count();}

        QRectF getBoundingRectF(const QString key);
        QRectF getBoundingRectF();

        void draw(QPainter& p, const QRect& rect, bool& needsRedraw);

        void select(const QRect& rect, bool select = true);

        void copyToClipboard(bool deleteSelection = false);
        void pasteFromClipboard();
        CTrack *take(const QString& key, bool silent);
        void insert(const QString& key, CTrack *track, bool silent);
        void setShowBullets(bool on){showBullets = on; emitSigChanged();}
        bool getShowBullets(){return showBullets;}
        void setShowMinMax(bool on){showMinMax = on; emitSigChanged();}
        bool getShowMinMax(){return showMinMax;}

        struct keys_t{QString key; QString name; QString comment; QPixmap icon; quint32 time;};
        /// get all keys in the database
        QList<keys_t> keys();

        CTrack * getTrackByKey(const QString& key);

        static bool keyLessThanAlpha(keys_t&  s1, keys_t&  s2);
        static bool keyLessThanTime(keys_t&  s1, keys_t&  s2);

        void makeVisible(const QStringList& keys);

        bool contains(const QString& key){return tracks.contains(key);}

        // set the point of focus to the point closest to distance
        void setPointOfFocusByDist(double distance);
        // set the point of focus to the point closest to timestamp
        void setPointOfFocusByTime(quint32 timestamp);
        // set the point of focus to the point passed
        void setPointOfFocusByIdx(qint32 idx);

        // find the point with the lowest delta to the given timestamp
        bool getClosestPoint2Timestamp(quint32 timestamp, quint32 maxDelta, double& lon, double& lat);
        // find the point with the lowest distance delta to the given position and return it's position and timestamp
        bool getClosestPoint2Position(double& lon, double& lat, quint32& timestamp, double maxDelta);

        signals:
        void sigHighlightTrack(CTrack * track);
        /// the index into the current track
        void sigPointOfFocus(const int idx);

    private slots:
        void slotMapChanged();
        void slotModified();
        void slotNeedUpdate();

    private:

        friend class CMainWindow;
        friend class CTrackEditWidget;
        friend class CTrackFilterWidget;

        CTrackDB(QTabWidget * tb, QObject * parent);
        void splitLineToViewport(const QPolygon& line, const QRect& extViewport, QList<QPolygon>& lines);
        void drawLine(const QPolygon& line, const QRect& extViewport, QPainter& p);
        void drawLine(const QPolygon& line, const QVector<QColor> colors, const QRect& extViewport, QPainter& p);
        void drawArrows(const QPolygon& line, const QRect& viewport, QPainter& p);

        quint32 cnt;

        static CTrackDB * m_self;

        QMap<QString,CTrack*> tracks;
        QUndoStack *undoStack;

        bool showBullets;
        bool showMinMax;
};
#endif                           //CTRACKDB_H
