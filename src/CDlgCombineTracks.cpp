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

#include "CDlgCombineTracks.h"
#include "CTrackDB.h"
#include "CTrack.h"

#include <QtGui>

bool trackLessThan(CTrack * s1, CTrack * s2)
{
    return s1->getEndTimestamp() < s2->getStartTimestamp();
}


CDlgCombineTracks::CDlgCombineTracks(QWidget * parent)
: QDialog(parent)
{
    setupUi(this);

    toolAdd->setIcon(QPixmap(":/icons/iconRight16x16.png"));
    connect(toolAdd, SIGNAL(clicked()), this, SLOT(slotAdd()));
    toolDel->setIcon(QPixmap(":/icons/iconLeft16x16.png"));
    connect(toolDel, SIGNAL(clicked()), this, SLOT(slotDel()));

    toolUp->setIcon(QPixmap(":/icons/iconUpload16x16.png"));
    connect(toolUp, SIGNAL(clicked()), this, SLOT(slotUp()));
    toolDown->setIcon(QPixmap(":/icons/iconDownload16x16.png"));
    connect(toolDown, SIGNAL(clicked()), this, SLOT(slotDown()));

    connect(checkSortTimestamp, SIGNAL(toggled(bool)), this, SLOT(slotSortTimestamp(bool)));
    connect(listSelTracks, SIGNAL(itemSelectionChanged()), this, SLOT(slotItemSelectionChanged()));

    CTrack * track;
    QList<CTrack*> tracks =  CTrackDB::self().getTracks().values();

    foreach(track, tracks)
    {
        QListWidgetItem * item = new QListWidgetItem();
        item->setText(track->getName());
        item->setData(Qt::UserRole, track->getKey());
        listTracks->addItem(item);
    }

}


CDlgCombineTracks::~CDlgCombineTracks()
{

}


void CDlgCombineTracks::slotAdd()
{
    QListWidgetItem * item;
    QList<QListWidgetItem*> items = listTracks->selectedItems();

    foreach(item, items)
    {
        listSelTracks->addItem(listTracks->takeItem(listTracks->row(item)));
    }
}


void CDlgCombineTracks::slotDel()
{
    QListWidgetItem * item;
    QList<QListWidgetItem*> items = listSelTracks->selectedItems();

    foreach(item, items)
    {
        listTracks->addItem(listSelTracks->takeItem(listSelTracks->row(item)));
    }
}


void CDlgCombineTracks::accept()
{
    const QMap<QString,CTrack*>& dict = CTrackDB::self().getTracks();

    CTrack* track;
    QList<CTrack*> tracks;

    QListWidgetItem * item;
    QList<QListWidgetItem*> items = listSelTracks->findItems("*",Qt::MatchWildcard);

    foreach(item, items)
    {
        tracks << dict[item->data(Qt::UserRole).toString()];
    }

    if(tracks.isEmpty() || lineTrackName->text().isEmpty()) return;

    if(checkSortTimestamp->isChecked())
    {
        qSort(tracks.begin(), tracks.end(), trackLessThan);
    }

    CTrack * newtrack = new CTrack(&CTrackDB::self());
    newtrack->setName(lineTrackName->text());
    //the color is the same as first selected track
    newtrack->setColor(tracks[0]->getColorIdx());

    foreach(track, tracks)
    {
        *newtrack += *track;
    }

    CTrackDB::self().addTrack(newtrack, false);

    QDialog::accept();
}


void CDlgCombineTracks::slotSortTimestamp(bool yes)
{
    if(yes || (listSelTracks->currentItem() == 0))
    {
        toolUp->setEnabled(false);
        toolDown->setEnabled(false);
    }
    else
    {
        toolUp->setEnabled(true);
        toolDown->setEnabled(true);
    }
}


void CDlgCombineTracks::slotItemSelectionChanged ()
{
    slotSortTimestamp(checkSortTimestamp->isChecked());
}


void CDlgCombineTracks::slotUp()
{
    QListWidgetItem * item = listSelTracks->currentItem();
    if(item)
    {
        int row = listSelTracks->row(item);
        if(row == 0) return;
        listSelTracks->takeItem(row);
        row = row - 1;
        listSelTracks->insertItem(row,item);
        listSelTracks->setCurrentItem(item);
    }
}


void CDlgCombineTracks::slotDown()
{
    QListWidgetItem * item = listSelTracks->currentItem();
    if(item)
    {
        int row = listSelTracks->row(item);
        if(row == (listSelTracks->count() - 1)) return;
        listSelTracks->takeItem(row);
        row = row + 1;
        listSelTracks->insertItem(row,item);
        listSelTracks->setCurrentItem(item);
    }
}
