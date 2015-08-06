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

#include "CImageSelect.h"
#include "CWpt.h"

#include <QtGui>

#define WIDTH   100.0
#define HEIGHT  100.0

CImageSelect::CImageSelect(QWidget * parent)
: QWidget(parent)
, wpt(0)
{
    scrollBar = 0;
    setupUi(this);
    setMaximumHeight(HEIGHT + scrollBar->height());

    setTransparent(false);

    connect(scrollBar, SIGNAL(valueChanged(int)), this, SLOT(update()));
}


CImageSelect::~CImageSelect()
{

}


void CImageSelect::setTransparent(bool yes)
{
    images.clear();
    if(yes)
    {
        images << img_t(tr("leave right")       , "01t.png", ":/pics/roadbook/01t.png");
        images << img_t(tr("leave left")        , "02t.png", ":/pics/roadbook/02t.png");
        images << img_t(tr("straight on")       , "03t.png", ":/pics/roadbook/03t.png");
        images << img_t(tr("straight on")       , "04t.png", ":/pics/roadbook/04t.png");
        images << img_t(tr("turn right")        , "05t.png", ":/pics/roadbook/05t.png");
        images << img_t(tr("turn left")         , "06t.png", ":/pics/roadbook/06t.png");
        images << img_t(tr("straight on")       , "07t.png", ":/pics/roadbook/07t.png");
        images << img_t(tr("straight on")       , "08t.png", ":/pics/roadbook/08t.png");
        images << img_t(tr("hard right turn")   , "09t.png", ":/pics/roadbook/09t.png");
        images << img_t(tr("hard left turn")    , "10t.png", ":/pics/roadbook/10t.png");
        images << img_t(tr("straight on")       , "11t.png", ":/pics/roadbook/11t.png");
        images << img_t(tr("straight on")       , "12t.png", ":/pics/roadbook/12t.png");
        images << img_t(tr("go left")           , "13t.png", ":/pics/roadbook/13t.png");
        images << img_t(tr("go right")          , "14t.png", ":/pics/roadbook/14t.png");
        images << img_t(tr("take right")        , "15t.png", ":/pics/roadbook/15t.png");
        images << img_t(tr("take left")         , "16t.png", ":/pics/roadbook/16t.png");
        images << img_t(tr("hard right turn")   , "17t.png", ":/pics/roadbook/17t.png");
        images << img_t(tr("hard left turn")    , "18t.png", ":/pics/roadbook/18t.png");
        images << img_t(tr("go left")           , "19t.png", ":/pics/roadbook/19t.png");
        images << img_t(tr("go right")          , "20t.png", ":/pics/roadbook/20t.png");
        images << img_t(tr("turn right @x-ing") , "21t.png", ":/pics/roadbook/21t.png");
        images << img_t(tr("turn left @x-ing")  , "22t.png", ":/pics/roadbook/22t.png");
        images << img_t(tr("straight on")       , "23t.png", ":/pics/roadbook/23t.png");
        images << img_t(tr("u-turn right")      , "24t.png", ":/pics/roadbook/24t.png");
        images << img_t(tr("u-turn left")       , "25t.png", ":/pics/roadbook/25t.png");
        images << img_t(tr("river")             , "26t.png", ":/pics/roadbook/26t.png");
        images << img_t(tr("attention")         , "27t.jpg", ":/pics/roadbook/27t.png");

    }
    else
    {
        images << img_t(tr("leave right")       , "01.jpg", ":/pics/roadbook/01.png");
        images << img_t(tr("leave left")        , "02.jpg", ":/pics/roadbook/02.png");
        images << img_t(tr("straight on")       , "03.jpg", ":/pics/roadbook/03.png");
        images << img_t(tr("straight on")       , "04.jpg", ":/pics/roadbook/04.png");
        images << img_t(tr("turn right")        , "05.jpg", ":/pics/roadbook/05.png");
        images << img_t(tr("turn left")         , "06.jpg", ":/pics/roadbook/06.png");
        images << img_t(tr("straight on")       , "07.jpg", ":/pics/roadbook/07.png");
        images << img_t(tr("straight on")       , "08.jpg", ":/pics/roadbook/08.png");
        images << img_t(tr("hard right turn")   , "09.jpg", ":/pics/roadbook/09.png");
        images << img_t(tr("hard left turn")    , "10.jpg", ":/pics/roadbook/10.png");
        images << img_t(tr("straight on")       , "11.jpg", ":/pics/roadbook/11.png");
        images << img_t(tr("straight on")       , "12.jpg", ":/pics/roadbook/12.png");
        images << img_t(tr("go left")           , "13.jpg", ":/pics/roadbook/13.png");
        images << img_t(tr("go right")          , "14.jpg", ":/pics/roadbook/14.png");
        images << img_t(tr("take right")        , "15.jpg", ":/pics/roadbook/15.png");
        images << img_t(tr("take left")         , "16.jpg", ":/pics/roadbook/16.png");
        images << img_t(tr("hard right turn")   , "17.jpg", ":/pics/roadbook/17.png");
        images << img_t(tr("hard left turn")    , "18.jpg", ":/pics/roadbook/18.png");
        images << img_t(tr("go left")           , "19.jpg", ":/pics/roadbook/19.png");
        images << img_t(tr("go right")          , "20.jpg", ":/pics/roadbook/20.png");
        images << img_t(tr("turn right @x-ing") , "21.jpg", ":/pics/roadbook/21.png");
        images << img_t(tr("turn left @x-ing")  , "22.jpg", ":/pics/roadbook/22.png");
        images << img_t(tr("straight on")       , "23.jpg", ":/pics/roadbook/23.png");
        images << img_t(tr("u-turn right")      , "25.jpg", ":/pics/roadbook/25.png");
        images << img_t(tr("u-turn left")       , "26.jpg", ":/pics/roadbook/26.png");
        images << img_t(tr("river")             , "27.jpg", ":/pics/roadbook/27.png");
        images << img_t(tr("attention")         , "28.jpg", ":/pics/roadbook/28.png");

    }

    scrollBar->setMinimum(0);
    scrollBar->setMaximum(images.size() - 1);

    update();
}


void CImageSelect::resizeEvent(QResizeEvent * e)
{
    QSize s = e->size();
    scrollBar->setPageStep(s.width()/WIDTH);
    scrollBar->setMaximum(images.size() - scrollBar->pageStep());
}


void CImageSelect::mousePressEvent(QMouseEvent * e)
{
    QPoint p = e->pos();
    int idx = scrollBar->value() + p.x() / WIDTH;

    if(idx < images.size())
    {
        emit sigSelectImage(images[idx]);
    }
}


void CImageSelect::wheelEvent(QWheelEvent * e)
{
    if(!scrollBar->rect().contains(e->pos()))
    {
        int value = scrollBar->value() + ((e->delta() > 0) ? -1 : 1);
        scrollBar->setValue(value);
    }
}


void CImageSelect::paintEvent(QPaintEvent * e)
{
    QPainter p(this);
    int start   = scrollBar->value();

    p.fillRect(rect(), Qt::white);

    int h       = size().height();
    int xoff    = 0;
    for(int i = start; i < images.size(); i++)
    {
        img_t& img = images[i];

        QPixmap pixmap(150,150);
        pixmap.fill(Qt::white);
        QPainter p1;
        p1.begin(&pixmap);
        p1.drawPixmap((150 - img.img.width())>>1, (150 - img.img.height())>>1, img.img);
        p1.end();

        int wImg = pixmap.width();
        int hImg = pixmap.height();

        float f = WIDTH/wImg;

        if((hImg * f) > h)
        {
            p.drawPixmap(xoff, 0, pixmap.scaledToHeight(h, Qt::SmoothTransformation));
        }
        else
        {
            p.drawPixmap(xoff, 0, pixmap.scaledToWidth(WIDTH, Qt::SmoothTransformation));
        }

        xoff += WIDTH;

        if(xoff > width())
        {
            break;
        }
    }
}
