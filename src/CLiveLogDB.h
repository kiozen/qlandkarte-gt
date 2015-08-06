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

#ifndef CLIVELOGDB_H
#define CLIVELOGDB_H

#include "IDB.h"
#include "CLiveLog.h"

#include <QPolygon>

class QPainter;
class QFile;

class CLiveLogDB : public IDB
{
    Q_OBJECT;
    public:
        virtual ~CLiveLogDB();

        struct simplelog_t
        {
            simplelog_t() : timestamp(0xFFFFFFFF), lon(WPT_NOFLOAT), lat(WPT_NOFLOAT), ele(WPT_NOFLOAT) {}
            quint32 timestamp;
            float lon;
            float lat;
            float ele;
        };

        static CLiveLogDB& self(){return *m_self;}

        void draw(QPainter& p, const QRect& rect, bool& needsRedraw);

        void loadGPX(CGpx& /*gpx*/){}
        void saveGPX(CGpx& /*gpx*/, const QStringList& ){}

        void loadQLB(CQlb& /*qlb*/, bool /*newKey*/){}
        void saveQLB(CQlb& /*qlb*/){}

        void upload(const QStringList&){}
        void download(){}

        void clear();

        void start(bool yes);
        bool logging();

        void setLockToCenter(bool on);
        bool lockToCenter(){return m_lockToCenter;}

        bool useSmallArrow(){return m_useSmallArrow;}
        void setUseSmallArrow(bool yes){m_useSmallArrow = yes;}

        void addWpt();

        bool contains(const QString& key){return false;}

    private slots:
        void slotLiveLog(const CLiveLog& log);
        void slotMapChanged();
        void slotMapDBChanged();
    private:
        friend class CMainWindow;
        CLiveLogDB(QTabWidget * tb, QObject * parent);
        void saveBackupLog();

        static CLiveLogDB * m_self;
        CLiveLog m_log;

        QVector<simplelog_t> track;
        QPolygon polyline;

        bool m_lockToCenter;

        QFile * backup;

        bool m_useSmallArrow;
};
#endif                           //CLIVELOGDB_H
