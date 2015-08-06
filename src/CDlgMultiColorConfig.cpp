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

#include "CDlgMultiColorConfig.h"
#include "CCanvas.h"
#include <QtGui>

CDlgMultiColorConfig::CDlgMultiColorConfig(CTrack::multi_color_setup_t &setup)
: setup(setup)
{
    setupUi(this);

    labelName->setText(setup.name);
    if(setup.modeMinMax == CTrack::eMinMaxModeNoAuto)
    {
        checkAuto->setEnabled(false);
    }
    else
    {
        checkAuto->setChecked(setup.modeMinMax ==  CTrack::eMinMaxModeAuto);
    }

    lineMinValue->setText(QString("%1").arg(setup.minVal));
    lineMaxValue->setText(QString("%1").arg(setup.maxVal));

    sliderMinColor->setValue(setup.minHue);
    sliderMaxColor->setValue(setup.maxHue);

    spinMinColor->setValue(setup.minHue);
    spinMaxColor->setValue(setup.maxHue);

    connect(sliderMinColor, SIGNAL(sliderMoved(int)), this, SLOT(slotSliderChanged(int)));
    connect(sliderMaxColor, SIGNAL(sliderMoved(int)), this, SLOT(slotSliderChanged(int)));
    connect(spinMinColor, SIGNAL(valueChanged(int)), this, SLOT(slotSpinChanged(int)));
    connect(spinMaxColor, SIGNAL(valueChanged(int)), this, SLOT(slotSpinChanged(int)));
    connect(checkAuto, SIGNAL(toggled(bool)), this, SLOT(slotCheckAuto(bool)));

    slotCheckAuto(checkAuto->isChecked());
}


CDlgMultiColorConfig::~CDlgMultiColorConfig()
{

}


void CDlgMultiColorConfig::resizeEvent(QResizeEvent * e)
{
    QDialog::resizeEvent(e);

    drawColorBar();
}


void CDlgMultiColorConfig::accept()
{
    if(setup.modeMinMax != CTrack::eMinMaxModeNoAuto)
    {
        setup.modeMinMax = checkAuto->isChecked() ? CTrack::eMinMaxModeAuto : CTrack::eMinMaxModeFixed;
    }
    setup.minVal = lineMinValue->text().toFloat();
    setup.maxVal = lineMaxValue->text().toFloat();
    setup.minHue = spinMinColor->value();
    setup.maxHue = spinMaxColor->value();
    setup.buildColorTable();

    QDialog::accept();
}


void CDlgMultiColorConfig::slotCheckAuto(bool on)
{
    lineMinValue->setEnabled(!on);
    lineMaxValue->setEnabled(!on);
}


void CDlgMultiColorConfig::slotSliderChanged(int )
{
    spinMinColor->setValue(sliderMinColor->value());
    spinMaxColor->setValue(sliderMaxColor->value());
    drawColorBar();
}


void CDlgMultiColorConfig::slotSpinChanged(int )
{
    sliderMinColor->setValue(spinMinColor->value());
    sliderMaxColor->setValue(spinMaxColor->value());
    drawColorBar();
}


void CDlgMultiColorConfig::drawColorBar()
{
    QPixmap pixmap(QSize(sliderMinColor->width(),20));
    pixmap.fill();

    QPainter p(&pixmap);
    USE_ANTI_ALIASING(p,true);

    QColor color;
    float step = float(pixmap.width()) / 255;
    float off  = 0;

    for(int i = 0; i < 255; i++)
    {
        color.setHsv(i,255,255);
        p.setPen(color);
        p.setBrush(color);

        p.drawRect(off,0,step,20);

        off += step;
    }

    p.setPen(Qt::black);
    p.drawLine(sliderMinColor->value() * step,0,sliderMinColor->value() * step,20);
    p.drawLine(sliderMaxColor->value() * step,0,sliderMaxColor->value() * step,20);

    labelColor->setPixmap(pixmap);
}
