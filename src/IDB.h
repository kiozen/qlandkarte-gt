/**********************************************************************************************
    Copyright (C) 2007 Oliver Eichler oliver.eichler@gmx.de
    Copyright (C) 2010 Joerg Wunsch <j@uriah.heep.sax.de>

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
#ifndef IDB_H
#define IDB_H

#include <QObject>
#include <QDateTime>

class QTabWidget;
class QWidget;
class QPainter;
class QRect;
class CGpx;
class CQlb;
class QString;

/// base class for all database objects
class IDB : public QObject
{
    Q_OBJECT;
    public:
        enum dbType_e
        {
            eTypeWpt = 0x00000001
            ,eTypeTrk = 0x00000002
            ,eTypeRte = 0x00000004
            ,eTypeSrc = 0x00000008
            ,eTypeGeo = 0x00000010
            ,eTypeOvl = 0x00000020
            ,eTypeMap = 0x00000040
            ,eTypeDry = 0x00000080
            ,eTypeLog = 0x00000100

        };

        IDB(dbType_e type, QTabWidget * tb, QObject * parent);
        virtual ~IDB();

        /// move database views into focus
        /**
            If this is called the database should try to make all it's
            toolviews visible to the user.
        */
        virtual void gainFocus();

        /// parse a GPX timestamp, including timezone calculations
        virtual bool parseTimestamp(const QString &time, quint32 &tstamp);
        virtual bool parseTimestamp(const QString &time, quint32 &tstamp,
            quint32 &tstamp_msec);

        /// load database data from gpx
        virtual void loadGPX(CGpx& gpx) = 0;
        /// save database data to gpx
        virtual void saveGPX(CGpx& gpx, const QStringList& keys) = 0;

        /// load database data from QLandkarte binary
        virtual void loadQLB(CQlb& qlb, bool newKey) = 0;
        /// save database data to QLandkarte binary
        virtual void saveQLB(CQlb& qlb) = 0;

        virtual void upload(const QStringList& keys) = 0;
        virtual void download() = 0;

        virtual void clear() = 0;

        virtual int count(){return -1;}

        virtual void draw(QPainter& p, const QRect& rect, bool& needsRedraw){}

        virtual void makeVisible(const QStringList& keys){}

        virtual bool contains(const QString& key) = 0;

        static QDateTime parseTimestamp(const QString &timetext, int& tzoffset);

        static void signalsOff();
        static void signalsOn();

        virtual void emitSigChanged(){emit sigChanged(); signalFlags |= type;}
        virtual void emitSigModified(const QString& key){emit sigModified(key);}
        virtual void emitSigNeedUpdate(const QString& key){emit sigNeedUpdate(key);}
        signals:
        void sigChanged();
        void sigModified(const QString&);
        void sigNeedUpdate(const QString&);

    protected:
        dbType_e type;

        QTabWidget *  tabbar;
        QWidget *  toolview;

        static quint32 signalFlags;
};
#endif                           //IDB_H
