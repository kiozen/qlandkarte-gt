/**********************************************************************************************
    Copyright (C) 2012 Oliver Eichler oliver.eichler@gmx.de

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

#include "CTextBrowser.h"
#include "CCanvas.h"

#include <QtGui>
#include <QScrollBar>

CTextBrowser::CTextBrowser(QWidget *parent)
: QTextBrowser(parent)
{
    connect(this, SIGNAL(sigHighlightArea(QString)), this, SLOT(slotHighlightArea(QString)));
}


CTextBrowser::~CTextBrowser()
{

}


void CTextBrowser::resetAreas()
{
    areas.clear();
}


void CTextBrowser::addArea(const QString& key, const QRect& rect)
{
    int offset = verticalScrollBar()->value();

    QRect r = rect;
    r.moveTop(r.top() + offset);
    areas[key] = r;
}


void CTextBrowser::slotHighlightArea(const QString& key)
{
    if(areaKey == key)
    {
        return;
    }

    if(!key.isEmpty() && areas.contains(key))
    {
        areaKey     = key;

        QRect r     = areas[key];
        int top     = verticalScrollBar()->value();
        int bottom  = verticalScrollBar()->value() + verticalScrollBar()->pageStep();

        if(r.top() < (top + r.height()))
        {
            int val = r.top() - r.height();
            if(val < 0) val = 0;
            verticalScrollBar()->setValue(val);
        }

        if(r.bottom() > (bottom - r.height()))
        {
            int val = r.bottom() + r.height() - verticalScrollBar()->pageStep();
            if(val < 0) val = 0;
            verticalScrollBar()->setValue(val);
        }
    }
    else
    {
        verticalScrollBar()->setValue(0);
        areaKey.clear();
    }
    viewport()->update();
}


void CTextBrowser::paintEvent(QPaintEvent * e)
{
    QTextBrowser::paintEvent(e);
    QPainter p(viewport());

    int offset = verticalScrollBar()->value();

    USE_ANTI_ALIASING(p, true);
    p.setPen(QPen(CCanvas::penBorderBlue));

    QRect r = areas[areaKey];
    r.moveTop(r.top() - offset);

    PAINT_ROUNDED_RECT(p, r);
}


void CTextBrowser::mouseMoveEvent(QMouseEvent * e)
{
    QTextBrowser::mouseMoveEvent(e);

    QPoint p = e->pos();
    p.setX(p.x() - verticalScrollBar()->value());

    foreach(const QString& key, areas.keys())
    {
        if(areas[key].contains(p))
        {
            emit sigHighlightArea(key);
            return;
        }
    }

    emit sigHighlightArea("");
}
