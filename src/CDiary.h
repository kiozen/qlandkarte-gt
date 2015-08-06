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

#ifndef CDIARY_H
#define CDIARY_H

#include "IItem.h"
#include <QDataStream>
#include <QFile>
#include <QPointer>

class CDiaryEdit;
class CTabWidget;

class CWpt;
class CTrack;
class CRoute;
class QTextFrame;
class QTextTable;

class CDiary : public IItem
{
    Q_OBJECT;
    public:
        CDiary(QObject * parent);
        virtual ~CDiary();

        enum type_e {eEnd,eBase, eWpt, eTrk, eRte};

        QString getInfo();

        void setIcon(const QString&){}

        void linkToProject(quint64 key);

        void showEditWidget(CTabWidget * tab, bool fromDB);

        void clear();

        QList<CWpt*>& getWpts(){return wpts;}
        QList<CTrack*>& getTrks(){return trks;}
        QList<CRoute*>& getRtes(){return rtes;}

        bool isModified();

        void setModified();

        void close();

    private slots:
        void slotEditWidgetDied(QObject*);

    private:
        friend QDataStream& operator >>(QDataStream& s, CDiary& diary);
        friend QDataStream& operator <<(QDataStream& s, CDiary& diary);
        friend class CDiaryEdit;
        friend class CDiaryDB;

        quint64 keyProjectGeoDB;

        QPointer<CDiaryEdit> editWidget;

        QList<CWpt*> wpts;
        QList<CRoute*> rtes;
        QList<CTrack*> trks;

        QPointer<QTextTable> tblWpt;
        QPointer<QTextTable> tblTrk;
        QPointer<QTextTable> tblRte;

        QPointer<QTextFrame> diaryFrame;

        bool modified;
};

QDataStream& operator >>(QDataStream& s, CDiary& diary);
QDataStream& operator <<(QDataStream& s, CDiary& diary);

void operator >>(QFile& f, CDiary& diary);
void operator <<(QFile& f, CDiary& diary);
#endif                           //CDIARY_H
