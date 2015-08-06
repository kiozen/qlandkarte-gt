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

#include "IOverlay.h"
#include "COverlayDB.h"
#include "COverlayDistance.h"
#include "GeoMath.h"

#include <QtGui>
struct ovl_head_entry_t
{
    ovl_head_entry_t() : type(IOverlay::eEnd), offset(0) {}
    qint32      type;
    quint32     offset;
    QByteArray  data;
};

QDataStream& operator >>(QDataStream& s, COverlayDB& db)
{
    QIODevice * dev = s.device();
    qint64      pos = dev->pos();

    char magic[9];
    s.readRawData(magic,9);

    if(strncmp(magic,"QLOvl   ",9))
    {
        dev->seek(pos);
        return s;
    }

    QList<ovl_head_entry_t> entries;

    while(1)
    {
        ovl_head_entry_t entry;
        s >> entry.type >> entry.offset;
        entries << entry;
        if(entry.type == IOverlay::eEnd) break;
    }

    QList<ovl_head_entry_t>::iterator entry = entries.begin();
    while(entry != entries.end())
    {
        qint64 o = pos + entry->offset;
        dev->seek(o);
        s >> entry->data;

        switch(entry->type)
        {
            case IOverlay::eBase:
            {

                QDataStream s1(&entry->data, QIODevice::ReadOnly);
                s1.setVersion(QDataStream::Qt_4_5);
                QString type, key;

                s1 >> type;
                if(type == "Text")
                {
                    QRect rect;
                    QString text;
                    s1 >> rect >> text >> key;
                    db.addText(text,rect, key);
                }
                else if(type == "TextBox")
                {
                    QRect rect;
                    QPoint pt;
                    QString text;
                    double lon, lat;
                    s1 >> lon >> lat >> pt >> rect >> text >> key;
                    db.addTextBox(text, lon, lat, pt, rect, key);
                }
                else if(type == "Distance")
                {
                    IOverlay * ovl;
                    float speed;
                    QString name;
                    QString comment;
                    QString parentWpt;
                    int size, idx = 0;
                    COverlayDistance::pt_t pt;
                    QList<COverlayDistance::pt_t> points;
                    s1 >> name >> comment >> size;
                    for(int i = 0; i < size; ++i)
                    {
                        s1 >> pt.u >> pt.v;
                        pt.idx = idx++;
                        points << pt;
                    }
                    s1 >> speed >> key >> parentWpt;
                    ovl = db.addDistance(name, comment, speed, points, key);
                    ovl->setParentWpt(parentWpt);
                }
                else if(type == "Area")
                {
                    COverlayArea * ovl;
                    QColor color;
                    QString name;
                    QString comment;
                    QString parentWpt;
                    qint32 style;
                    quint32 width;
                    quint8 opacity;
                    int size, idx = 0;
                    COverlayArea::pt_t pt;
                    QList<COverlayArea::pt_t> points;
                    s1 >> name >> comment >> size;
                    for(int i = 0; i < size; ++i)
                    {
                        s1 >> pt.u >> pt.v;
                        pt.idx = idx++;
                        points << pt;
                    }
                    s1 >> color >> key >> parentWpt >> style >> width >> opacity;
                    ovl = db.addArea(name, comment, color, (Qt::BrushStyle)style, points, key);
                    ovl->setParentWpt(parentWpt);
                    ovl->setWidth(width);
                    ovl->setOpacity(opacity);
                }
                break;
            }

            default:;
        }
        ++entry;
    }
    return s;
}


QDataStream& operator <<(QDataStream& s, IOverlay& ovl)
{
    QList<ovl_head_entry_t> entries;

    //---------------------------------------
    // prepare base data
    //---------------------------------------
    ovl_head_entry_t entryBase;
    entryBase.type = IOverlay::eBase;
    QDataStream s1(&entryBase.data, QIODevice::WriteOnly);
    s1.setVersion(QDataStream::Qt_4_5);
    s1 << ovl.type;
    ovl.save(s1);
    entries << entryBase;

    //---------------------------------------
    // prepare terminator
    //---------------------------------------
    ovl_head_entry_t entryEnd;
    entryEnd.type = IOverlay::eEnd;
    entries << entryEnd;

    //---------------------------------------
    //---------------------------------------
    // now start to actually write data;
    //---------------------------------------
    //---------------------------------------
    // write magic key
    s.writeRawData("QLOvl   ",9);

    // calculate offset table
    quint32 offset = entries.count() * 8 + 9;

    QList<ovl_head_entry_t>::iterator entry = entries.begin();
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


void operator >>(QFile& f, COverlayDB& db)
{
    f.open(QIODevice::ReadOnly);
    QDataStream s(&f);
    s.setVersion(QDataStream::Qt_4_5);
    s >> db;
    f.close();
}


void operator <<(QFile& f, IOverlay& ovl)
{
    f.open(QIODevice::WriteOnly);
    QDataStream s(&f);
    s.setVersion(QDataStream::Qt_4_5);
    s << ovl;
    f.close();
}


QPointer<IOverlay> IOverlay::selected = 0;

IOverlay::IOverlay(QObject * parent, const QString& type, const QString& iconString)
: IItem(parent)
, type(type)
, highlight(false)
, isVisible(true)
{
    setIcon(iconString);
}


IOverlay::~IOverlay()
{

}


void IOverlay::select(IOverlay * s)
{
    selected = s;
}


void IOverlay::setIcon(const QString& str)
{
    iconPixmap = QPixmap(str);
    iconString = str;
}
