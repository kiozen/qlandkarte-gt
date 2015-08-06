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

#include "CDiary.h"
#include "CDiaryDB.h"
#include "CDiaryEdit.h"
#include "CTabWidget.h"

#include "CWpt.h"
#include "CTrack.h"
#include "CRoute.h"

#include <QtCore>

struct diary_head_entry_t
{
    diary_head_entry_t() : type(CDiary::eEnd), offset(0) {}
    qint32      type;
    quint32     offset;
    QByteArray  data;
};

QDataStream& operator >>(QDataStream& s, CDiary& diary)
{
    QIODevice * dev = s.device();
    qint64      pos = dev->pos();

    char magic[9];
    s.readRawData(magic,9);

    if(strncmp(magic,"QLDry   ",9))
    {
        dev->seek(pos);
        return s;
    }

    QList<diary_head_entry_t> entries;

    while(1)
    {
        diary_head_entry_t entry;
        s >> entry.type >> entry.offset;
        entries << entry;
        if(entry.type == CDiary::eEnd) break;
    }

    QList<diary_head_entry_t>::iterator entry = entries.begin();
    while(entry != entries.end())
    {
        qint64 o = pos + entry->offset;
        dev->seek(o);
        s >> entry->data;
        switch(entry->type)
        {
            case CDiary::eBase:
            {
                QString comment, name,key;
                QDataStream s1(&entry->data, QIODevice::ReadOnly);
                s1.setVersion(QDataStream::Qt_4_5);

                s1 >> diary.timestamp;
                s1 >> comment;
                s1 >> name;
                s1 >> diary.keyProjectGeoDB;
                s1 >> key;

                diary.setComment(comment);
                diary.setName(name);
                diary.setKey(key);
                break;
            }
            case CDiary::eWpt:
            {
                int cnt;
                QDataStream s1(&entry->data, QIODevice::ReadOnly);
                s1.setVersion(QDataStream::Qt_4_5);

                s1 >> cnt;
                for(int i=0; i < cnt; i++)
                {
                    CWpt * wpt = new CWpt(&diary);
                    s1 >> *wpt;
                    diary.wpts << wpt;
                }
                break;

            }
            case CDiary::eTrk:
            {
                int cnt;
                QDataStream s1(&entry->data, QIODevice::ReadOnly);
                s1.setVersion(QDataStream::Qt_4_5);

                s1 >> cnt;
                for(int i=0; i < cnt; i++)
                {
                    CTrack * trk = new CTrack(&diary);
                    s1 >> *trk;
                    trk->rebuild(true);
                    diary.trks << trk;
                }
                break;

            }
            case CDiary::eRte:
            {
                int cnt;
                QDataStream s1(&entry->data, QIODevice::ReadOnly);
                s1.setVersion(QDataStream::Qt_4_5);

                s1 >> cnt;
                for(int i=0; i < cnt; i++)
                {
                    CRoute * rte = new CRoute(&diary);
                    s1 >> *rte;
                    diary.rtes << rte;
                }
                break;

            }
            default:;
        }

        ++entry;
    }

    return s;
}


QDataStream& operator <<(QDataStream& s, CDiary& diary)
{
    QList<diary_head_entry_t> entries;

    if(!diary.editWidget.isNull())
    {
        diary.editWidget->collectData();
    }

    //---------------------------------------
    // prepare base data
    //---------------------------------------
    diary_head_entry_t entryBase;
    entryBase.type = CDiary::eBase;
    QDataStream s1(&entryBase.data, QIODevice::WriteOnly);
    s1.setVersion(QDataStream::Qt_4_5);

    s1 << diary.timestamp;
    s1 << diary.getComment();
    s1 << diary.getName();
    s1 << diary.keyProjectGeoDB;
    s1 << diary.getKey();
    entries << entryBase;

    //---------------------------------------
    // prepare waypoint data
    //---------------------------------------
    diary_head_entry_t entryWpt;
    entryWpt.type = CDiary::eWpt;
    QDataStream s2(&entryWpt.data, QIODevice::WriteOnly);
    s2.setVersion(QDataStream::Qt_4_5);

    s2 << diary.wpts.count();
    foreach(CWpt * wpt, diary.wpts)
    {
        s2 << *wpt;
    }

    entries << entryWpt;

    //---------------------------------------
    // prepare track data
    //---------------------------------------
    diary_head_entry_t entryTrk;
    entryTrk.type = CDiary::eTrk;
    QDataStream s3(&entryTrk.data, QIODevice::WriteOnly);
    s3.setVersion(QDataStream::Qt_4_5);

    s3 << diary.trks.count();
    foreach(CTrack * trk, diary.trks)
    {
        s3 << *trk;
    }

    entries << entryTrk;

    //---------------------------------------
    // prepare route data
    //---------------------------------------
    diary_head_entry_t entryRte;
    entryRte.type = CDiary::eRte;
    QDataStream s4(&entryRte.data, QIODevice::WriteOnly);
    s4.setVersion(QDataStream::Qt_4_5);

    s4 << diary.rtes.count();
    foreach(CRoute * rte, diary.rtes)
    {
        s4 << *rte;
    }

    entries << entryRte;

    //---------------------------------------
    // prepare terminator
    //---------------------------------------
    diary_head_entry_t entryEnd;
    entryEnd.type = CDiary::eEnd;
    entries << entryEnd;

    //---------------------------------------
    //---------------------------------------
    // now start to actually write data;
    //---------------------------------------
    //---------------------------------------
    // write magic key
    s.writeRawData("QLDry   ",9);

    // calculate offset table
    quint32 offset = entries.count() * 8 + 9;

    QList<diary_head_entry_t>::iterator entry = entries.begin();
    while(entry != entries.end())
    {
        entry->offset = offset;
        offset += entry->data.size() + sizeof(quint32);
        ++entry;
    }

    // write offset table
    entry = entries.begin();
    while(entry != entries.end())
    {
        s << entry->type << entry->offset;
        ++entry;
    }

    // write entry data
    entry = entries.begin();
    while(entry != entries.end())
    {
        s << entry->data;
        ++entry;
    }

    return s;
}


void operator >>(QFile& f, CDiary& diary)
{
    f.open(QIODevice::ReadOnly);
    QDataStream s(&f);
    s.setVersion(QDataStream::Qt_4_5);
    s >> diary;
    f.close();
}


void operator <<(QFile& f, CDiary& diary)
{
    f.open(QIODevice::WriteOnly);
    QDataStream s(&f);
    s.setVersion(QDataStream::Qt_4_5);
    s << diary;
    f.close();
}


CDiary::CDiary(QObject * parent)
: IItem(parent)
, keyProjectGeoDB(0)
, editWidget(0)
, modified(false)
{

}


CDiary::~CDiary()
{
    if(!editWidget.isNull()) delete editWidget;
}


void CDiary::close()
{
    if(!editWidget.isNull())
    {
        editWidget->close();
    }
}


void CDiary::clear()
{
    qDeleteAll(wpts);
    qDeleteAll(trks);
    qDeleteAll(rtes);

    wpts.clear();
    trks.clear();
    rtes.clear();
}


bool CDiary::isModified()
{
    return modified;
}


void CDiary::setModified()
{
    modified = true;
    if(!editWidget.isNull())
    {
        editWidget->setTabTitle();
    }
}


void CDiary::slotEditWidgetDied(QObject*)
{
    CDiaryDB::self().delDiary(getKey(), false);
}


QString CDiary::getInfo()
{
    QString str;

    str += "This is a diary";

    return str;
}


void CDiary::linkToProject(quint64 key)
{
    keyProjectGeoDB = key;
}


void CDiary::showEditWidget(CTabWidget * tab, bool fromDB)
{
    if(editWidget == 0)
    {
        editWidget = new CDiaryEdit(*this,tab);
        connect(editWidget.data(), SIGNAL(destroyed(QObject*)), this, SLOT(slotEditWidgetDied(QObject*)));
    }
    tab->addTab(editWidget, tr("Diary"));
    editWidget->slotReload(fromDB);
}
