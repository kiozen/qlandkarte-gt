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

#include "CTrackToolWidget.h"
#include "CTrackEditWidget.h"
#include "CTrackDB.h"
#include "CTrack.h"
#include "CMapDB.h"
#include "CMainWindow.h"
#include "IUnit.h"
#include "COverlayDB.h"
#include "COverlayDistance.h"
#include "CMegaMenu.h"
#include "GeoMath.h"
#include "CSettings.h"

#include <QtGui>
#include <QMessageBox>
#include <QMenu>

#define N_LINES 7

CTrackToolWidget::sortmode_e CTrackToolWidget::sortmode = eSortByName;

CTrackToolWidget::CTrackToolWidget(QTabWidget * parent)
: QWidget(parent)
, originator(false)
{
    setupUi(this);
    setObjectName("Tracks");
    parent->addTab(this,QIcon(":/icons/iconTrack16x16.png"),"");
    parent->setTabToolTip(parent->indexOf(this), tr("Tracks"));

    connect(&CTrackDB::self(), SIGNAL(sigModified(const QString&)), this, SLOT(slotDBChanged()));
    connect(&CTrackDB::self(), SIGNAL(sigNeedUpdate(const QString&)), this, SLOT(slotDBChanged()));
    connect(&CTrackDB::self(), SIGNAL(sigChanged()), this, SLOT(slotDBChanged()));

    connect(listTracks,SIGNAL(itemClicked(QListWidgetItem*) ),this,SLOT(slotItemClicked(QListWidgetItem*)));
    connect(listTracks,SIGNAL(itemDoubleClicked(QListWidgetItem*) ),this,SLOT(slotItemDoubleClicked(QListWidgetItem*)));
    connect(listTracks,SIGNAL(itemSelectionChanged()),this,SLOT(slotSelectionChanged()));

    contextMenu     = new QMenu(this);
    actEdit         = contextMenu->addAction(QPixmap(":/icons/iconEdit16x16.png"),tr("Edit..."),this,SLOT(slotEdit()));
    actRevert       = contextMenu->addAction(QPixmap(":/icons/iconReload16x16.png"),tr("Revert"),this,SLOT(slotRevert()));
    contextMenu->addSeparator();
    actDistance     = contextMenu->addAction(QPixmap(":/icons/iconDistance16x16.png"),tr("Make Overlay"),this,SLOT(slotToOverlay()));
    contextMenu->addSeparator();
    actHide         = contextMenu->addAction(tr("Show"),this,SLOT(slotShow()));
    actShowBullets  = contextMenu->addAction(tr("Show Bullets"),this,SLOT(slotShowBullets()));
    actShowMinMax   = contextMenu->addAction(tr("Show Min/Max"),this,SLOT(slotShowMinMax()));
    contextMenu->addSeparator();
    actZoomToFit    = contextMenu->addAction(QPixmap(":/icons/iconZoomArea16x16.png"),tr("Zoom to fit"),this,SLOT(slotZoomToFit()));
    actDel          = contextMenu->addAction(QPixmap(":/icons/iconClear16x16.png"),tr("Delete"),this,SLOT(slotDelete()));

    actHide->setCheckable(true);
    actShowBullets->setCheckable(true);
    actShowBullets->setChecked(CTrackDB::self().getShowBullets());
    actShowMinMax->setCheckable(true);
    actShowMinMax->setChecked(CTrackDB::self().getShowMinMax());

    connect(listTracks,SIGNAL(customContextMenuRequested(const QPoint&)),this,SLOT(slotContextMenu(const QPoint&)));

    QFontMetrics fm(listTracks->font());
    listTracks->setIconSize(QSize(15,N_LINES*fm.height()));

    connect(toolSortAlpha, SIGNAL(clicked()), this, SLOT(slotDBChanged()));
    connect(toolSortTime, SIGNAL(clicked()), this, SLOT(slotDBChanged()));

    toolSortAlpha->setIcon(QPixmap(":/icons/iconDec16x16.png"));
    toolSortTime->setIcon(QPixmap(":/icons/iconTime16x16.png"));

    SETTINGS;
    toolSortAlpha->setChecked(cfg.value("track/sortAlpha", true).toBool());
    toolSortTime->setChecked(cfg.value("track/sortTime", true).toBool());

    listTracks->installEventFilter(this);
}


CTrackToolWidget::~CTrackToolWidget()
{
    SETTINGS;
    cfg.setValue("track/sortAlpha", toolSortAlpha->isChecked());
    cfg.setValue("track/sortTime", toolSortTime->isChecked());

}


void CTrackToolWidget::slotDBChanged()
{
    if(originator) return;

    //    qDebug() << "void CTrackToolWidget::slotDBChanged()";

    if(toolSortAlpha->isChecked())
    {
        sortmode = eSortByName;
    }
    else if(toolSortTime->isChecked())
    {
        sortmode = eSortByTime;
    }

    QFontMetrics fm(listTracks->font());
    QPixmap icon(15,N_LINES*fm.height());
    listTracks->clear();
    listTracks->setIconSize(icon.size());

    QListWidgetItem * highlighted = 0;

    CTrackDB::keys_t key;
    QList<CTrackDB::keys_t> keys = CTrackDB::self().keys();

    foreach(key, keys)
    {

        CTrack * track = CTrackDB::self().getTrackByKey(key.key);

        QListWidgetItem * item = new QListWidgetItem(listTracks);
        icon.fill(track->getColor());

        QPainter p;
        p.begin(&icon);

        if(track->isHidden())
        {
            p.drawPixmap(0,0,QPixmap(":icons/iconClear16x16"));
        }
        else
        {
            p.drawPixmap(0,0,QPixmap(":icons/iconOk16x16"));
        }
        p.end();

        item->setText(track->getInfo());
        item->setData(Qt::UserRole, track->getKey());
        item->setIcon(icon);

        if(track->isHighlighted())
        {
            highlighted = item;
        }

    }

    if(highlighted)
    {
        listTracks->setCurrentItem(highlighted);
    }
}


void CTrackToolWidget::slotItemDoubleClicked(QListWidgetItem * item)
{
    QString key = item->data(Qt::UserRole).toString();

    QRectF r = CTrackDB::self().getBoundingRectF(key);
    if (!r.isNull ())
    {
        CMapDB::self().getMap().zoom(r.left() * DEG_TO_RAD, r.top() * DEG_TO_RAD, r.right() * DEG_TO_RAD, r.bottom() * DEG_TO_RAD);
    }
}


void CTrackToolWidget::slotItemClicked(QListWidgetItem * item)
{
    originator = true;
    CTrackDB::self().highlightTrack(item->data(Qt::UserRole).toString());
    originator = false;
}


void CTrackToolWidget::slotSelectionChanged()
{
    if(originator)
    {
        return;
    }
    //    qDebug() << "void CTrackToolWidget::slotSelectionChanged()" << listTracks->selectedItems().isEmpty() << listTracks->hasFocus();
    if(listTracks->hasFocus() && listTracks->selectedItems().isEmpty())
    {
        CTrackDB::self().highlightTrack("");
    }
}


void CTrackToolWidget::keyPressEvent(QKeyEvent * e)
{
    if(e->key() == Qt::Key_Delete)
    {
        slotDelete();
        e->accept();
    }
    else
    {
        QWidget::keyPressEvent(e);
    }
}


void CTrackToolWidget::slotContextMenu(const QPoint& pos)
{
    int cnt = listTracks->selectedItems().count();
    if(cnt > 0)
    {
        if(listTracks->currentItem())
        {
            originator = true;
            CTrackDB::self().highlightTrack(listTracks->currentItem()->data(Qt::UserRole).toString());
            originator = false;
        }

        if(cnt > 1)
        {
            actEdit->setEnabled(false);
            actRevert->setEnabled(false);
        }
        else
        {
            actEdit->setEnabled(true);
            actRevert->setEnabled(true);
        }

        actHide->setChecked(!CTrackDB::self().highlightedTrack()->isHidden());

        QPoint p = listTracks->mapToGlobal(pos);
        contextMenu->exec(p);
    }

}


void CTrackToolWidget::slotEdit()
{
    const QListWidgetItem* item = listTracks->currentItem();

    if(item == 0)
    {
        QMessageBox::information(0,tr("Edit track ..."), tr("You have to select a track first."),QMessageBox::Ok,QMessageBox::Ok);
        return;
    };

    if(trackedit.isNull())
    {
        trackedit = new CTrackEditWidget(theMainWindow->getCanvas());
        connect(&CTrackDB::self(), SIGNAL(sigHighlightTrack(CTrack*)), trackedit, SLOT(slotSetTrack(CTrack*)));
        theMainWindow->setTempWidget(trackedit, tr("Track"));
        trackedit->slotSetTrack(CTrackDB::self().highlightedTrack());
    }
    else
    {
        delete trackedit;
    }
}


void CTrackToolWidget::slotShowProfile()
{
    const QListWidgetItem* item = listTracks->currentItem();

    if(item == 0)
    {
        QMessageBox::information(0,tr("Edit track ..."), tr("You have to select a track first."),QMessageBox::Ok,QMessageBox::Ok);
        return;
    };

    if(trackedit.isNull())
    {
        trackedit = new CTrackEditWidget(theMainWindow->getCanvas());
        connect(&CTrackDB::self(), SIGNAL(sigHighlightTrack(CTrack*)), trackedit, SLOT(slotSetTrack(CTrack*)));
        theMainWindow->setTempWidget(trackedit, tr("Track"));
        trackedit->slotSetTrack(CTrackDB::self().highlightedTrack());
    }
    trackedit->slotShowProfile();

    // remove falsely triggered point of focus
    CTrackDB::self().setPointOfFocusByIdx(-1);
}


void CTrackToolWidget::slotDelete()
{
    QStringList keys;
    QListWidgetItem * item;
    QList<QListWidgetItem*> items = listTracks->selectedItems();
    foreach(item,items)
    {
        keys << item->data(Qt::UserRole).toString();
    }

    CTrackDB::self().delTracks(keys);

    if(!trackedit.isNull())
    {
        trackedit->deleteLater();
    }
}


void CTrackToolWidget::slotShow()
{
    QStringList keys;
    QListWidgetItem * item;
    const QList<QListWidgetItem*>& items = listTracks->selectedItems();
    foreach(item,items)
    {
        keys << item->data(Qt::UserRole).toString();
    }
    CTrackDB::self().hideTrack(keys, !actHide->isChecked());;
}


void CTrackToolWidget::slotToOverlay()
{
    CTrack * track;
    const QMap<QString,CTrack*>& tracks = CTrackDB::self().getTracks();

    QListWidgetItem * item;
    const QList<QListWidgetItem*>& items = listTracks->selectedItems();
    foreach(item,items)
    {
        track = tracks[item->data(Qt::UserRole).toString()];

        QList<COverlayDistance::pt_t> pts;

        int idx = 0;
        CTrack::pt_t trkpt;
        QList<CTrack::pt_t>& trkpts = track->getTrackPoints();
        foreach(trkpt, trkpts)
        {
            if(trkpt.flags & CTrack::pt_t::eDeleted) continue;

            COverlayDistance::pt_t pt;
            pt.u = trkpt.lon * DEG_TO_RAD;
            pt.v = trkpt.lat * DEG_TO_RAD;
            pt.idx = idx++;

            pts << pt;
        }

        COverlayDB::self().addDistance(track->getName(), tr("created from track"), 0.0, pts);
    }

    CMegaMenu::self().switchByKeyWord("Overlay");
}


void CTrackToolWidget::slotZoomToFit()
{
    QRectF r;

    const QList<QListWidgetItem*>& items = listTracks->selectedItems();
    QList<QListWidgetItem*>::const_iterator item = items.begin();

    r = CTrackDB::self().getBoundingRectF((*item)->data(Qt::UserRole).toString());

    while(item != items.end())
    {
        r |= CTrackDB::self().getBoundingRectF((*item)->data(Qt::UserRole).toString());
        ++item;
    }

    if (!r.isNull ())
    {
        CMapDB::self().getMap().zoom(r.left() * DEG_TO_RAD, r.top() * DEG_TO_RAD, r.right() * DEG_TO_RAD, r.bottom() * DEG_TO_RAD);
    }
}


void CTrackToolWidget::slotRevert()
{
    const QListWidgetItem* item = listTracks->currentItem();

    if(item == 0)
    {
        QMessageBox::information(0,tr("Filter"), tr("You have to select a track first."), QMessageBox::Ok,QMessageBox::Ok);
        return;
    };

    CTrackDB::self().revertTrack(item->data(Qt::UserRole).toString());
}


void CTrackToolWidget::slotShowBullets()
{
    CTrackDB::self().setShowBullets(!CTrackDB::self().getShowBullets());
}


void CTrackToolWidget::slotShowMinMax()
{
    CTrackDB::self().setShowMinMax(!CTrackDB::self().getShowMinMax());
}


bool CTrackToolWidget::eventFilter(QObject *obj, QEvent *event)
{

    if(event->type() == QEvent::Leave)
    {
        listTracks->clearFocus();
    }

    return QObject::eventFilter(obj, event);
}
