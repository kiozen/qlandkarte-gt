/**********************************************************************************************
    Copyright (C) 2013 Oliver Eichler oliver.eichler@gmx.de

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

#include "CMapDEMSlopeSetup.h"
#include "CCanvas.h"
#include "CMapDEM.h"
#include <QtGui>

CMapDEMSlopeSetup *CMapDEMSlopeSetup::m_pSelf = 0;

CMapDEMSlopeSetup::CMapDEMSlopeSetup(QWidget *parent)
: QWidget(parent)
{
    setupUi(this);

    m_pSelf = this;

    QPixmap pixmap(20,10);

    pixmap.fill(CMapDEM::slopeColorTable[5]);
    labelColor5->setPixmap(pixmap);
    pixmap.fill(CMapDEM::slopeColorTable[4]);
    labelColor4->setPixmap(pixmap);
    pixmap.fill(CMapDEM::slopeColorTable[3]);
    labelColor3->setPixmap(pixmap);
    pixmap.fill(CMapDEM::slopeColorTable[2]);
    labelColor2->setPixmap(pixmap);
    pixmap.fill(CMapDEM::slopeColorTable[1]);
    labelColor1->setPixmap(pixmap);

    connect(sliderGrade, SIGNAL(valueChanged(int)), this, SLOT(slotValueChanged(int)));

}


CMapDEMSlopeSetup::~CMapDEMSlopeSetup()
{
    m_pSelf = 0;
    qDebug() << "CMapDEMSlopeSetup";
}


void CMapDEMSlopeSetup::paintEvent(QPaintEvent * e)
{
    QPainter p(this);
    USE_ANTI_ALIASING(p,true);

    QRect r = rect();
    r.adjust(2,2,-2,-2);

    p.setPen(CCanvas::penBorderBlack);
    p.setBrush(QColor(255,255,255,150));
    PAINT_ROUNDED_RECT(p,r);

    QWidget::paintEvent(e);
}


void CMapDEMSlopeSetup::registerDEMMap(CMapDEM * map)
{
    dem = map;
    if(dem.isNull())
    {
        hide();
    }
    else
    {
        int i = map->getGrade();
        sliderGrade->setValue(i);
        slotValueChanged(i);
        show();
    }
}


void CMapDEMSlopeSetup::slotValueChanged(int val)
{
    if(dem.isNull())
    {
        return;
    }

    const double * g = CMapDEM::grade[val];
    labelValue1->setText(QString("> %1%2").arg(g[1]).arg(QChar(0260)));
    labelValue2->setText(QString("> %1%2").arg(g[2]).arg(QChar(0260)));
    labelValue3->setText(QString("> %1%2").arg(g[3]).arg(QChar(0260)));
    labelValue4->setText(QString("> %1%2").arg(g[4]).arg(QChar(0260)));
    labelValue5->setText(QString("> %1%2").arg(g[5]).arg(QChar(0260)));
    labelGrade->setText(tr("Grade %1").arg(val));
    dem->setGrade(val);
}
