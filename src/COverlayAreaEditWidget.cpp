/**********************************************************************************************
    Copyright (C) 2014 Oliver Eichler oliver.eichler@gmx.de

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

#include "COverlayAreaEditWidget.h"
#include "COverlayArea.h"
#include "CMainWindow.h"
#include "CMenus.h"
#include "CActions.h"
#include "GeoMath.h"
#include "CCanvas.h"

#include <QtGui>
#include <QMenu>
#include <QColorDialog>

#define N_STYLES        8
#define N_WIDTHS        4

Qt::BrushStyle styles[N_STYLES] =
{
    Qt::NoBrush
    , Qt::HorPattern
    , Qt::VerPattern
    , Qt::CrossPattern
    , Qt::BDiagPattern
    , Qt::FDiagPattern
    , Qt::DiagCrossPattern
    , Qt::SolidPattern
};

struct width_t
{
    int width;
    QString string;
};

width_t widths[N_WIDTHS] =
{
    {3, QObject::tr("thin")}
    ,{5, QObject::tr("normal")}
    ,{9, QObject::tr("wide")}
    ,{13, QObject::tr("strong")}
};

COverlayAreaEditWidget::COverlayAreaEditWidget(QWidget *parent, COverlayArea *ovl)
: QWidget(parent)
, ovl(ovl)
{
    ovl->isEdit = true;

    setupUi(this);

    lineName->setText(ovl->name);
    textComment->setText(ovl->comment);
    color = ovl->color.name();

    QPixmap icon(64,24);
    icon.fill(color);
    labelColor->setPixmap(icon);

    for(int i = 0; i < N_STYLES; i++)
    {
        QPixmap icon(64,24);
        icon.fill(Qt::white);
        QPainter p(&icon);
        p.setPen(Qt::black);
        p.setBrush(styles[i]);
        p.drawRect(icon.rect());

        comboStyle->addItem(icon,"",(int)styles[i]);
    }
    comboStyle->setCurrentIndex(comboStyle->findData((int)ovl->style));

    for(int i = 0; i < N_WIDTHS; i++)
    {
        comboWidth->addItem(widths[i].string, widths[i].width);
    }
    comboWidth->setCurrentIndex(comboWidth->findData(ovl->width));

    checkOpacity->setChecked(ovl->opacity != 255);

    connect(toolColor, SIGNAL(clicked()), this, SLOT(slotChangeColor()));
    connect(pushApply, SIGNAL(clicked()), this, SLOT(slotApply()));
    connect(treeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(slotItemSelectionChanged()));

    connect(ovl, SIGNAL(sigChanged()), this, SLOT(slotChanged()));
    connect(ovl, SIGNAL(destroyed()), this, SLOT(deleteLater()));
    connect(ovl, SIGNAL(sigSelectionChanged()), this, SLOT(slotSelectionChanged()));
    connect(treeWidget, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(slotContextMenu(const QPoint&)));

    CActions * actions = theMainWindow->getActionGroupProvider()->getActions();

    contextMenu = new QMenu(this);
    contextMenu->addAction(actions->getAction("aCopyToClipboard"));
    contextMenu->addAction(QIcon(":/icons/iconDelete16x16.png"), tr("Delete"), this, SLOT(slotDelete()));

    slotChanged();
    slotSelectionChanged();
}


COverlayAreaEditWidget::~COverlayAreaEditWidget()
{
    if(ovl)
    {
        ovl->isEdit = false;
    }
}


bool COverlayAreaEditWidget::isAboutToClose()
{
    return !ovl->isEdit;
}


void COverlayAreaEditWidget::slotApply()
{
    ovl->name       = lineName->text();
    ovl->comment    = textComment->toPlainText();
    ovl->color.setNamedColor(color);
    ovl->style      = (Qt::BrushStyle)comboStyle->itemData(comboStyle->currentIndex()).toInt();
    ovl->width      = comboWidth->itemData(comboWidth->currentIndex()).toInt();
    ovl->opacity    = checkOpacity->isChecked() ? 100 : 255;

    emit ovl->sigChanged();
}


void COverlayAreaEditWidget::slotChanged()
{
    QString pos;

    int i;
    const int size = ovl->points.size();

    treeWidget->clear();

    QList<QTreeWidgetItem*> items;
    for(i = 0; i < size; i++)
    {
        COverlayArea::pt_t pt = ovl->points[i];
        GPS_Math_Deg_To_Str(pt.u * RAD_TO_DEG, pt.v * RAD_TO_DEG, pos);

        QTreeWidgetItem * item = new QTreeWidgetItem();
        item->setText(eNo, QString::number(pt.idx));
        item->setData(eNo,Qt::UserRole, pt.idx);
        item->setText(ePos, pos);

        items << item;
    }

    treeWidget->addTopLevelItems(items);
#ifdef QK_QT5_PORT
    treeWidget->header()->setSectionResizeMode(eNo,QHeaderView::ResizeToContents);
#else
    treeWidget->header()->setResizeMode(eNo,QHeaderView::ResizeToContents);
#endif
}


void COverlayAreaEditWidget::slotSelectionChanged()
{
    QTreeWidgetItem * item = 0;
    int idx;
    const int size = ovl->points.size();
    for(idx = 0; idx < size; idx++)
    {
        if(ovl->selectedPoints.contains(idx))
        {
            if(!item) item = treeWidget->topLevelItem(idx);
            treeWidget->topLevelItem(idx)->setSelected(true);
        }
        else
        {
            treeWidget->topLevelItem(idx)->setSelected(false);
        }
    }

    if(item)
    {
        treeWidget->scrollToItem(item);
    }
}


void COverlayAreaEditWidget::slotItemSelectionChanged()
{
    ovl->selectedPoints.clear();
    const QList<QTreeWidgetItem *>& items = treeWidget->selectedItems();
    QTreeWidgetItem * item;

    foreach(item, items)
    {
        ovl->selectedPoints << item->data(eNo, Qt::UserRole).toInt();
    }

    theMainWindow->getCanvas()->update();

}


void COverlayAreaEditWidget::slotContextMenu(const QPoint& pos)
{
    int cnt = treeWidget->selectedItems().count();
    if(cnt > 0)
    {

        //        actSplit->setEnabled(cnt == 1);
        QPoint p = treeWidget->mapToGlobal(pos);
        contextMenu->exec(p);
    }
}


void COverlayAreaEditWidget::slotDelete()
{
    QList<int> idx;
    QTreeWidgetItem * item;
    const QList<QTreeWidgetItem*>& items = treeWidget->selectedItems();

    foreach(item, items)
    {
        idx << item->data(eNo, Qt::UserRole).toInt();
    }

    ovl->delPointsByIdx(idx);
}


void COverlayAreaEditWidget::slotChangeColor()
{
    QColorDialog dlg(color);
    dlg.open(this, SLOT(slotChangeColor(QColor)));
    dlg.exec();
}


void COverlayAreaEditWidget::slotChangeColor(const QColor& c)
{
    color = c.name();

    QPixmap icon(64,32);
    icon.fill(color);
    labelColor->setPixmap(icon);
}
