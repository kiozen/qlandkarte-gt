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

#include "CMapSearchWidget.h"
#include "CMainWindow.h"
#include "CCanvas.h"
#include "GeoMath.h"
#include "CMapDB.h"
#include "CMapTDB.h"
#include "CSearchDB.h"
#include "CMapSearchCanvas.h"
#include "CTabWidget.h"
#include "CImage.h"
#include "CMapSearchThread.h"
#include "IMap.h"
#include "config.h"

#include <QtGui>
#include <QMessageBox>

#define SEARCHDELAY 1500

CMapSearchWidget::CMapSearchWidget(QWidget * parent)
: QWidget(parent)
, area(IMapSelection::eNo,0)
{
    setupUi(this);
    setObjectName("CMapSearchWidget");
    setAttribute(Qt::WA_DeleteOnClose,true);
    toolNewMask->setIcon(QIcon(":/icons/iconWizzard16x16.png"));
    toolSaveMask->setIcon(QIcon(":/icons/iconFileSave16x16.png"));
    toolDeleteMask->setIcon(QIcon(":/icons/iconDelete16x16.png"));
    toolSelectArea->setIcon(QIcon(":/icons/iconSelect16x16.png"));

    connect(toolSelectArea, SIGNAL(clicked()), this, SLOT(slotSelectArea()));
    connect(pushSearch, SIGNAL(clicked()), this, SLOT(slotSearch()));
    connect(pushCancel, SIGNAL(clicked()), this, SLOT(slotCancel()));
    connect(sliderThreshold, SIGNAL( valueChanged (int)), this, SLOT(slotThreshold(int)));
    connect(toolSaveMask, SIGNAL(clicked()), this, SLOT(slotSaveMask()));
    connect(toolNewMask, SIGNAL(clicked()), this, SLOT(slotSelectMask()));
    connect(comboSymbols, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(slotSelectMaskByName(const QString&)));
    connect(toolDeleteMask, SIGNAL(clicked()), this, SLOT(slotDeleteMask()));

    thread = new CMapSearchThread(this);
    connect(thread, SIGNAL(finished()), this, SLOT(slotSearchFinished()));
    connect(thread, SIGNAL(sigProgress(const QString&, const int)), this, SLOT(slotProgressSymbol(const QString&, const int)));

    mask = new CImage(this);
    loadMaskCollection();

    connect(&CMapDB::self(), SIGNAL(sigChanged()), this, SLOT(slotMapChanged()));
    slotMapChanged();

}


CMapSearchWidget::~CMapSearchWidget()
{
    if(!canvas.isNull())
    {
        delete canvas;
    }

    IMap * map      = &CMapDB::self().getMap();
    while(map && map->maptype != IMap::eGarmin) map = map->getOverlay();
    if(map == 0) return;

}


void CMapSearchWidget::slotSelectArea()
{
    if(canvas) canvas->deleteLater();
    theMainWindow->getCanvas()->setMouseMode(CCanvas::eMouseSelectArea);
}


void CMapSearchWidget::slotSelectMask()
{
    labelMask->setText(tr("No mask selected."));

    if(canvas.isNull())
    {
        canvas = new CMapSearchCanvas(this);
        connect(canvas, SIGNAL(sigSelection(const QPixmap&)), this, SLOT(slotMaskSelection(const QPixmap&)));
        theMainWindow->getCanvasTab()->addTab(canvas, tr("Symbols"));
    }

    binarizeViewport(-1);
}


void CMapSearchWidget::slotSearch()
{

    thread->start(sliderThreshold->value(), mask->rgb(), area);
    checkGui();
}


void CMapSearchWidget::slotProgressSymbol(const QString& status, const int progress)
{
    labelProgress->setText(status);
    progressBar->setValue(progress);
}


void CMapSearchWidget::slotCancel()
{
    thread->cancel();
}


void CMapSearchWidget::slotSearchFinished()
{
    QPointF symbol;
    const QList<QPointF>& symbols = thread->getLastResult();

    IMap& map = CMapDB::self().getMap();

    int cnt = 0;
    QString name = lineMaskName->text().isEmpty() ? "Sym." : lineMaskName->text();

    foreach(symbol, symbols)
    {
        double u = symbol.x();
        double v = symbol.y();

        if((u > M_PI/2) || (v > M_PI/4))
        {
            map.convertM2Rad(u,v);
        }

        CSearchDB::self().add(tr("%2 %1").arg(++cnt).arg(name), u, v);
    }

    checkGui();
}


void CMapSearchWidget::slotThreshold(int i)
{
    binarizeViewport(sliderThreshold->value());
}


void CMapSearchWidget::slotMaskSelection(const QPixmap& pixmap)
{
    mask->setPixmap(pixmap.toImage());
    labelMask->setPixmap(QPixmap::fromImage(mask->mask()));

    checkGui();
}


void CMapSearchWidget::setArea(CMapSelectionRaster& ms)
{
    if(canvas) canvas->deleteLater();

    area = ms;
    labelArea->setText(area.getDescription());
    checkGui();
}


void CMapSearchWidget::binarizeViewport(int t)
{
    if(canvas.isNull())
    {
        return;
    }

    CImage xxx(CMapDB::self().getMap().getBuffer());
    canvas->setBuffer(QPixmap::fromImage(xxx.binarize(t)));

    if(t < 0)sliderThreshold->setValue(xxx.getThreshold());
}


void CMapSearchWidget::slotSaveMask()
{
    if(lineMaskName->text().isEmpty())
    {
        QMessageBox::warning(this, tr("Missing name..."), tr("Please provide a symbol name to save the symbol."), QMessageBox::Abort, QMessageBox::Abort);
        return;
    }

    QFile file(QDir::home().filePath(QString(CONFIGDIR "%1.msk").arg(lineMaskName->text())));
    file.open(QIODevice::WriteOnly);

    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_4_5);
    out << sliderThreshold->value();
    out << (int)1;
    out << mask->rgb();
    out << (int)0;

    file.close();

    loadMaskCollection();
}


void CMapSearchWidget::slotSelectMaskByName(const QString& name)
{
    QImage pixmap;
    int     intval;

    QDir path(QDir::home().filePath(CONFIGDIR));
    QFile file(path.filePath(name + ".msk"));
    if(file.open(QIODevice::ReadOnly))
    {
        QDataStream in(&file);
        in.setVersion(QDataStream::Qt_4_5);

        in >> intval;
        sliderThreshold->setValue(intval);
        in >> intval;
        in >> pixmap;

        lineMaskName->setText(name);

        mask->setPixmap(pixmap);
        labelMask->setPixmap(QPixmap::fromImage(mask->mask()));

        pushSearch->setEnabled(true);
        toolSaveMask->setEnabled(true);

        file.close();
    }
    else
    {
        labelMask->setText(tr("No mask selected."));
    }

    checkGui();
}


void CMapSearchWidget::slotDeleteMask()
{
    QString name    = lineMaskName->text();
    int index       = comboSymbols->findText(name);

    if(index != -1)
    {
        comboSymbols->removeItem(index);
        QDir path(QDir::home().filePath(CONFIGDIR));
        QFile file(path.filePath(name + ".msk"));
        file.remove();

        labelMask->setText(tr("No mask selected."));
    }

    checkGui();
}


void CMapSearchWidget::loadMaskCollection()
{
    QString name = lineMaskName->text();

    comboSymbols->clear();
    comboSymbols->addItem(tr("no mask"));

    QDir path(QDir::home().filePath(CONFIGDIR));
    QStringList filter;
    filter << "*.msk";
    QStringList maskfiles = path.entryList(filter, QDir::Files, QDir::Name);
    QString     maskfile;

    foreach(maskfile, maskfiles)
    {
        QImage pixmap;
        int     intval;

        QFile file(path.filePath(maskfile));
        file.open(QIODevice::ReadOnly);
        QDataStream in(&file);
        in.setVersion(QDataStream::Qt_4_5);
        in >> intval;
        in >> intval;
        in >> pixmap;

        file.close();

        CImage imgMask(pixmap);
        pixmap = imgMask.mask();

        comboSymbols->addItem(QPixmap::fromImage(pixmap), QFileInfo(maskfile).baseName());
    }

    int idx = comboSymbols->findText(name);
    if(idx != -1)
    {
        comboSymbols->setCurrentIndex(comboSymbols->findText(name));
    }
    checkGui();
}


void CMapSearchWidget::checkGui()
{
    if(!labelMask->text().isEmpty())
    {
        lineMaskName->clear();
        toolSaveMask->setEnabled(false);
        toolDeleteMask->setEnabled(false);
    }
    else
    {
        toolSaveMask->setEnabled(true);
        toolDeleteMask->setEnabled(true);
    }

    if(!labelMask->text().isEmpty() || labelArea->text() == tr("No area selected.") || thread->isRunning())
    {
        pushSearch->setEnabled(false);
    }
    else
    {
        pushSearch->setEnabled(true);
    }

    pushCancel->setEnabled(thread->isRunning());
    toolSelectArea->setEnabled(!thread->isRunning());
}


void CMapSearchWidget::slotMapChanged()
{
    IMap& map  = CMapDB::self().getMap();
    tabWidget->widget(0)->setEnabled(map.maptype == IMap::eRaster);
}
