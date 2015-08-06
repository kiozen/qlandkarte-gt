/**********************************************************************************************
    Copyright (C) 2007 Oliver Eichler oliver.eichler@gmx.de

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

#include "CSearchToolWidget.h"
#include "CSearchDB.h"
#include "CMainWindow.h"
#include "CCanvas.h"
#include "CMegaMenu.h"
#include "GeoMath.h"
#include "CMapDB.h"
#include "CWptDB.h"
#include "CSettings.h"

#include <QtGui>
#include <QMenu>

CSearchToolWidget::CSearchToolWidget(QTabWidget * parent)
: QWidget(parent)
{
    setupUi(this);
    setObjectName("Search");

    connect(lineInput, SIGNAL(returnPressed()), this, SLOT(slotReturnPressed()));
    connect(&CSearchDB::self(), SIGNAL(sigStatus(const QString&)), labelStatus, SLOT(setText(const QString&)));
    connect(&CSearchDB::self(), SIGNAL(sigFinished()), this, SLOT(slotQueryFinished()));
    connect(&CSearchDB::self(), SIGNAL(sigChanged()), this, SLOT(slotDBChanged()));

    connect(listResults,SIGNAL(itemClicked(QListWidgetItem*)),this,SLOT(slotItemClicked(QListWidgetItem*)));

    parent->insertTab(0,this,QIcon(":/icons/iconSearch16x16.png"),"");
    parent->setTabToolTip(parent->indexOf(this), tr("Search"));

    contextMenu = new QMenu(this);
    contextMenu->addAction(QPixmap(":/icons/iconClipboard16x16.png"),tr("Copy Position"),this,SLOT(slotCopyPosition()),Qt::CTRL + Qt::Key_C);
    contextMenu->addAction(QPixmap(":/icons/iconAdd16x16.png"),tr("Add Waypoint ..."),this,SLOT(slotAdd()));
    contextMenu->addAction(QPixmap(":/icons/iconClear16x16.png"),tr("Delete"),this,SLOT(slotDelete()),Qt::Key_Delete);

    connect(listResults,SIGNAL(customContextMenuRequested(const QPoint&)),this,SLOT(slotContextMenu(const QPoint&)));

    comboHost->addItem(tr("OpenRouteService"), CSearchDB::eOpenRouteService);
    //    comboHost->addItem(tr("MapQuest"), CSearchDB::eMapQuest);
    comboHost->addItem(tr("Google"), CSearchDB::eGoogle);

    SETTINGS;
    cfg.beginGroup("search");
    int idx = comboHost->findData(cfg.value("host", CSearchDB::eOpenRouteService));
    if(idx != -1)
    {
        comboHost->setCurrentIndex(idx);
    }

    cfg.endGroup();

    connect(comboHost, SIGNAL(currentIndexChanged(int)), this, SLOT(slotHostChanged(int)));
}


CSearchToolWidget::~CSearchToolWidget()
{

}


void CSearchToolWidget::selSearchByKey(const QString& key)
{
    for(int i=0; i<listResults->count(); ++i)
    {
        QListWidgetItem * item = listResults->item(i);
        if(item && item->data(Qt::UserRole) == key)
        {
            listResults->setCurrentItem(item);
        }
    }

}


void CSearchToolWidget::slotReturnPressed()
{
    QString line = lineInput->text().trimmed();
    if(!line.isEmpty())
    {
        lineInput->setEnabled(false);
        CSearchDB::self().search(line, (CSearchDB::hosts_t)comboHost->itemData(comboHost->currentIndex()).toInt());

    }
}


void CSearchToolWidget::slotQueryFinished()
{
    lineInput->setEnabled(true);
}


void CSearchToolWidget::slotDBChanged()
{
    listResults->clear();

    QMap<QString,CSearch*>::const_iterator result = CSearchDB::self().begin();
    while(result != CSearchDB::self().end())
    {
        QListWidgetItem * item = new QListWidgetItem(listResults);
        item->setText((*result)->getInfo());
        item->setData(Qt::UserRole, (*result)->getKey());
        item->setIcon((*result)->getIcon());
        ++result;
    }

    listResults->sortItems();

}


void CSearchToolWidget::slotContextMenu(const QPoint& pos)
{
    if(listResults->currentItem())
    {
        QPoint p = listResults->mapToGlobal(pos);
        contextMenu->exec(p);
    }
}


void CSearchToolWidget::slotItemClicked(QListWidgetItem* item)
{
    CSearch * result = CSearchDB::self().getResultByKey(item->data(Qt::UserRole).toString());
    if(result)
    {
        theMainWindow->getCanvas()->move(result->lon, result->lat);
    }
}


void CSearchToolWidget::slotDelete()
{
    QStringList keys;
    QListWidgetItem * item;
    const QList<QListWidgetItem*>& items = listResults->selectedItems();
    foreach(item,items)
    {
        keys << item->data(Qt::UserRole).toString();
    }
    CSearchDB::self().delResults(keys);
}


void CSearchToolWidget::slotCopyPosition()
{
    QListWidgetItem * item = listResults->currentItem();
    if(item == 0) return;
    CSearch * result = CSearchDB::self().getResultByKey(item->data(Qt::UserRole).toString());
    if(result == 0) return;

    QString position;
    GPS_Math_Deg_To_Str(result->lon, result->lat, position);

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(position);

}


void CSearchToolWidget::slotAdd()
{
    QListWidgetItem * item = listResults->currentItem();
    if(item == 0) return;
    CSearch * result = CSearchDB::self().getResultByKey(item->data(Qt::UserRole).toString());
    if(result == 0) return;

    float ele = CMapDB::self().getDEM().getElevation(result->lon * DEG_TO_RAD, result->lat * DEG_TO_RAD);
    CWptDB::self().newWpt(result->lon * DEG_TO_RAD, result->lat * DEG_TO_RAD, ele, result->getName());
}


void CSearchToolWidget::keyPressEvent(QKeyEvent * e)
{
    if(e->key() == Qt::Key_Delete)
    {
        slotDelete();
        e->accept();
    }
    else if(e->key() == Qt::Key_C && e->modifiers() == Qt::ControlModifier)
    {
        slotCopyPosition();
    }
    else
    {
        QWidget::keyPressEvent(e);
    }
}


void CSearchToolWidget::slotHostChanged(int idx)
{
    SETTINGS;
    cfg.beginGroup("search");
    cfg.setValue("host", comboHost->itemData(idx).toInt());
    cfg.endGroup();
}
