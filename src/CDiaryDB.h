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

#ifndef CDIARYDB_H
#define CDIARYDB_H

#include "IDB.h"
#include "CDiary.h"

#include <QMap>

class CDiaryEditWidget;

class CDiaryDB : public IDB
{
    Q_OBJECT
        public:
        virtual ~CDiaryDB();

        static CDiaryDB& self(){return *m_self;}

        void addDiary(CDiary * diary, bool silent, bool fromDB);
        void delDiary(const QString& key, bool silent);
        void delDiarys(const QStringList& keys);

        CDiary * getDiaryByKey(const QString& key);

        void loadGPX(CGpx& gpx);
        void saveGPX(CGpx& gpx, const QStringList& keys);

        void loadQLB(CQlb& qlb, bool newKey);
        void saveQLB(CQlb& qlb);

        void upload(const QStringList& ){}
        void download(){}

        void clear();

        int count();

        bool contains(const QString& key){return diarys.contains(key);}

        QMap<QString,CDiary*> ::iterator begin(){return diarys.begin();}
        QMap<QString,CDiary*> ::iterator end(){return diarys.end();}

        void setModified(const QStringList& keys);

    private:
        friend class CMainWindow;
        friend class CDiaryEditWidget;

        CDiaryDB(QTabWidget * tb, QObject * parent);

        static CDiaryDB * m_self;

        QMap<QString,CDiary*> diarys;
};
#endif                           //CDIARYDB_H
