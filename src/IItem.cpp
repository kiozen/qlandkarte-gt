/**********************************************************************************************
    Copyright (C) 2010 Oliver Eichler oliver.eichler@gmx.de

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

#include "IItem.h"

#include <QtGui>

quint32 IItem::keycnt = 0;

IItem::IItem(QObject * parent)
: QObject(parent)
, timestamp(QDateTime::currentDateTime().toUTC().toTime_t ())
{

}


IItem::~IItem()
{

}


QString IItem::getKey()
{
    if(key.isEmpty())
    {
        genKey();
    }

    return key;
}


void IItem::genKey()
{
    key = QString("%1%2%3").arg(timestamp).arg(name).arg(keycnt++);
}


void IItem::removeHtml(QString &str)
{
    QTextDocument html;
    html.setHtml(str);
    str = html.toPlainText();
}
