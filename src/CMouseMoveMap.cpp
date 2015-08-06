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

#include "CMouseMoveMap.h"
#include "CCanvas.h"
#include "CMapDB.h"
#include "CWptDB.h"
#include "CSearchDB.h"
#include "CTrackDB.h"
#include "CDlgEditWpt.h"
#include "GeoMath.h"
#include "CTrackToolWidget.h"
#include "CGridDB.h"
#include "CDlgCropMap.h"

#include <QtGui>
#include <QMenu>
#include "CUndoStackView.h"
#include "CMapUndoCommandMove.h"

CMouseMoveMap::CMouseMoveMap(CCanvas * parent)
: IMouse(parent)
, moveMap(false)
, leftButtonPressed(false)
, altKeyPressed(false)
{
    cursor = QCursor(QPixmap(":/cursors/cursorMoveMap.png"),0,0);
}


CMouseMoveMap::~CMouseMoveMap()
{

}


void CMouseMoveMap::mouseMoveEvent(QMouseEvent * e)
{
    mousePos = e->pos();
    if(moveMap)
    {
        //CMapDB::self().getMap().move(oldPoint, e->pos());
        CUndoStackView::getInstance()->push(new CMapUndoCommandMove(&CMapDB::self().getMap(),oldPoint, e->pos()));
        canvas->update();
    }

    oldPoint = e->pos();

    mouseMoveEventWpt(e);
    mouseMoveEventTrack(e);
    mouseMoveEventRoute(e);
    mouseMoveEventOverlay(e);
    mouseMoveEventSearch(e);
    mouseMoveEventMapSel(e);
}


void CMouseMoveMap::mousePressEvent(QMouseEvent * e)
{
    if(e->button() == Qt::LeftButton)
    {

        oldPoint = e->pos();

        CTrack * track = CTrackDB::self().highlightedTrack();

        if(!selWpts.isEmpty())
        {
            if(selWpts.size() == 1)
            {
                CWpt * selWpt = selWpts.first().wpt;
                CWptDB::self().selWptByKey(selWpt->getKey(), false);
            }
            mousePressEventWpt(e);
        }
        else if(track && selTrkPt)
        {
            track->setPointOfFocus(selTrkPt->idx, CTrack::e3Way, false);
        }
        else if(!selSearch.isNull())
        {
            CSearchDB::self().selSearchByKey(selSearch->getKey());
            mousePressEventSearch(e);
        }
        else
        {
            leftButtonPressed = true;
            if (!moveMap)
            {
                CUndoStackView::getInstance()->beginMacro(tr("Move map"));
                cursor = QCursor(QPixmap(":/cursors/cursorMove.png"));
                QApplication::setOverrideCursor(cursor);
                moveMap     = true;
                CMapDB::self().getMap().fastDrawOn();
            }
        }
    }
    else if(e->button() == Qt::RightButton)
    {
        mousePos = e->pos();
        oldPoint = e->pos();
        canvas->raiseContextMenu(e->pos());
    }
}


void CMouseMoveMap::mouseReleaseEvent(QMouseEvent * e)
{
    if(e->button() == Qt::LeftButton)
    {
        leftButtonPressed = false;
        if(moveMap && (!altKeyPressed))
        {
            CUndoStackView::getInstance()->endMacro();
            moveMap = false;
            CMapDB::self().getMap().fastDrawOff();
            cursor = QCursor(QPixmap(":/cursors/cursorMoveMap.png"),0,0);
            QApplication::restoreOverrideCursor();
            canvas->update();
        }

    }
}


void CMouseMoveMap::keyPressEvent(QKeyEvent * e)
{
#ifdef ALTKEY_MOVES_MAP
    if (e->key() == Qt::Key_Alt)
    {
        altKeyPressed = true;
        if (!moveMap)
        {
            CUndoStackView::getInstance()->beginMacro(tr("Move map"));
            cursor = QCursor(QPixmap(":/cursors/cursorMove.png"));
            QApplication::setOverrideCursor(cursor);
            moveMap     = true;
            CMapDB::self().getMap().fastDrawOn();
        }
    }
#endif
}


void CMouseMoveMap::mouseDoubleClickEvent(QMouseEvent * e)
{
    if(selWpts.size() == 1)
    {
        CWpt * selWpt = selWpts.first().wpt;
        CWptDB::self().selWptByKey(selWpt->getKey(), true);
        canvas->update();
    }
}


void CMouseMoveMap::keyReleaseEvent(QKeyEvent * e)
{
#ifdef ALTKEY_MOVES_MAP
    if (e->key() == Qt::Key_Alt)
    {
        altKeyPressed = false;
        if(moveMap && (!leftButtonPressed))
        {
            CUndoStackView::getInstance()->endMacro();
            moveMap = false;
            CMapDB::self().getMap().fastDrawOff();
            cursor = QCursor(QPixmap(":/cursors/cursorMoveMap.png"),0,0);
            QApplication::restoreOverrideCursor();
            canvas->update();
        }
    }
#endif
}


void CMouseMoveMap::draw(QPainter& p)
{
    drawPos1(p);
    drawSelWpt(p);
    drawSelTrkPt(p);
    drawSelRtePt(p);
    drawSelSearch(p);
}


void CMouseMoveMap::contextMenu(QMenu& menu)
{
    menu.addAction(QPixmap(":/icons/iconReload16x16.png"),tr("Reload Map"),this,SLOT(slotReloadMap()));
    menu.addSeparator();
    if(selWpts.size() == 1)
    {
        CWpt * selWpt = selWpts.first().wpt;

        menu.addSeparator();
        menu.addAction(QPixmap(":/icons/iconClipboard16x16.png"),tr("Copy Pos. Waypoint"),this,SLOT(slotCopyPositionWpt()));
        menu.addAction(QPixmap(":/icons/iconEdit16x16.png"),tr("Edit Waypoint ..."),this,SLOT(slotEditWpt()));

        if(selWpt->isMovable())
        {
            menu.addAction(QPixmap(":/icons/iconWptMove16x16.png"),tr("Move Waypoint"),this,SLOT(slotMoveWpt()));
        }

        if(!selWpt->sticky)
        {
            menu.addAction(QPixmap(":/icons/iconClear16x16.png"),tr("Delete Waypoint"),this,SLOT(slotDeleteWpt()));
        }
    }
    else
    {
        menu.addSeparator();
        menu.addAction(QPixmap(":/icons/iconAdd16x16.png"),tr("Add Waypoint ..."),this,SLOT(slotAddWpt()));
    }

    if(selTrkPt)
    {
        menu.addSeparator();
        menu.addAction(QPixmap(":/icons/iconGoogleMaps16x16.png"),tr("Open Pos. with Google Maps"),this,SLOT(slotOpenGoogleMaps()));
        menu.addAction(QPixmap(":/icons/iconClipboard16x16.png"),tr("Copy Pos. Trackpoint"),this,SLOT(slotCopyPositionTrack()));
        menu.addAction(QPixmap(":/icons/iconEdit16x16.png"),tr("Edit Track ..."),this,SLOT(slotEditTrack()));
        menu.addAction(QPixmap(":/icons/iconEditCut16x16.png"),tr("Split Track ..."),this,SLOT(slotSplitTrack()));
    }
    menu.addSeparator();

    bool isLonLat   = false;
    double u        = mousePos.x();
    double v        = mousePos.y();
    CGridDB::self().convertPt2Pos(u, v, isLonLat);
    if(isLonLat)
    {
        QString str;
        u *= RAD_TO_DEG;
        v *= RAD_TO_DEG;
        GPS_Math_Deg_To_Str(u, v, str);
        QString posMeter = tr("Grid: %1").arg(str);
        menu.addAction(QIcon(":/icons/iconClipboard16x16.png"), posMeter, this, SLOT(slotCopyPosGrid()));
    }
    else
    {
        QString posMeter = tr("Grid: N %1m E %2m").arg(v, 0,'f',0).arg(u,0,'f',0);
        menu.addAction(QIcon(":/icons/iconClipboard16x16.png"), posMeter, this, SLOT(slotCopyPosGrid()));
    }

    IMap& map = CMapDB::self().getMap();
    u = mousePos.x();
    v = mousePos.y();

    map.convertPt2M(u,v);
    if(!map.isLonLat())
    {
        QString posMeter = tr("Map: N %1m E %2m").arg(v, 0,'f',0).arg(u,0,'f',0);
        menu.addAction(QIcon(":/icons/iconClipboard16x16.png"), posMeter, this, SLOT(slotCopyPosMeter()));
    }

    map.convertM2Rad(u,v);
    u *= RAD_TO_DEG;
    v *= RAD_TO_DEG;
    QString posDeg;
    GPS_Math_Deg_To_Str(u, v, posDeg);
    menu.addAction(QIcon(":/icons/iconClipboard16x16.png"), posDeg, this, SLOT(slotCopyPosDegree()));

    u = mousePos.x();
    v = mousePos.y();
    map.convertPt2Pixel(u,v);

    if(u >= 0 && v >= 0)
    {
        QString fn = QFileInfo(map.getFilename(mousePos.x(), mousePos.y())).fileName();

        QString posPixel = tr("Pixel %1x%2 (%3)").arg(u, 0,'f',0).arg(v,0,'f',0).arg(fn);
        menu.addAction(QIcon(":/icons/iconClipboard16x16.png"), posPixel, this, SLOT(slotCopyPosPixel()));

        menu.addSeparator();
        menu.addAction(QIcon(":/icons/wpt/flag_pin_red15x15.png"), tr("Crop: set pos. 1"), this, SLOT(slotSetPos1()));

        if(pos1Pixel.x() >= 0 && pos1Pixel.y() >= 0)
        {
            double u1 = pos1Pixel.x();
            double v1 = pos1Pixel.y();

            QString posPixelSize = tr("Crop map: %1x%2 w:%3 h:%4").arg(u1, 0,'f',0).arg(v1,0,'f',0).arg(u - u1,0,'f',0).arg(v - v1, 0,'f',0);
            menu.addAction(QIcon(":/icons/iconClipboard16x16.png"), posPixelSize, this, SLOT(slotCopyPosPixelSize()));
        }

    }

}


void CMouseMoveMap::slotCopyPosDegree()
{
    IMap& map = CMapDB::self().getMap();
    double u = mousePos.x();
    double v = mousePos.y();

    map.convertPt2Rad(u,v);
    u *= RAD_TO_DEG;
    v *= RAD_TO_DEG;
    QString position;
    GPS_Math_Deg_To_Str(u, v, position);

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(position);

}


void CMouseMoveMap::slotCopyPosGrid()
{
    QString position;
    bool isLonLat   = false;
    double u        = mousePos.x();
    double v        = mousePos.y();
    CGridDB::self().convertPt2Pos(u, v, isLonLat);
    if(isLonLat)
    {
        u *= RAD_TO_DEG;
        v *= RAD_TO_DEG;
        GPS_Math_Deg_To_Str(u, v, position);
    }
    else
    {
        position = tr("N %1m E %2m").arg(u, 0,'f',0).arg(v,0,'f',0);
    }

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(position);
}


void CMouseMoveMap::slotCopyPosMeter()
{
    IMap& map = CMapDB::self().getMap();
    double u = mousePos.x();
    double v = mousePos.y();

    map.convertPt2M(u,v);
    QString position = tr("N %1m E %2m").arg(u, 0,'f',0).arg(v,0,'f',0);

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(position);
}


void CMouseMoveMap::slotCopyPosPixel()
{
    IMap& map = CMapDB::self().getMap();

    double u = mousePos.x();
    double v = mousePos.y();

    map.convertPt2Pixel(u,v);
    QString position = QString("%1 %2").arg(u, 0,'f',0).arg(v,0,'f',0);

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(position);

}


void CMouseMoveMap::slotCopyPosPixelSize()
{
    IMap& map = CMapDB::self().getMap();

    double u1 = pos1Pixel.x();
    double v1 = pos1Pixel.y();
    double u2 = mousePos.x();
    double v2 = mousePos.y();

    QString filename  = map.getFilename(mousePos.x(), mousePos.y());

    map.convertPt2Pixel(u2,v2);

    CDlgCropMap dlg(filename, u1, v1, u2 - u1, v2 - v1);
    dlg.exec();

}


void CMouseMoveMap::slotEditWpt()
{
    if(selWpts.isEmpty()) return;

    CWpt * selWpt = selWpts.first().wpt;
    CDlgEditWpt dlg(*selWpt,canvas);
    dlg.exec();
}


void CMouseMoveMap::slotCopyPositionWpt()
{
    if(selWpts.isEmpty()) return;

    CWpt * selWpt = selWpts.first().wpt;

    QString position;
    GPS_Math_Deg_To_Str(selWpt->lon, selWpt->lat, position);

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(position);
}


void CMouseMoveMap::slotDeleteWpt()
{
    if(selWpts.isEmpty()) return;

    CWpt * selWpt = selWpts.first().wpt;
    QString key = selWpt->getKey();

    selWpts.clear();

    CWptDB::self().delWpt(key);
}


void CMouseMoveMap::slotMoveWpt()
{
    if(selWpts.isEmpty()) return;
    canvas->setMouseMode(CCanvas::eMouseMoveWpt);

    CWpt * selWpt = selWpts.first().wpt;

    double u = selWpt->lon * DEG_TO_RAD;
    double v = selWpt->lat * DEG_TO_RAD;
    CMapDB::self().getMap().convertRad2Pt(u,v);

    QMouseEvent event1(QEvent::MouseMove, QPoint(u,v), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(canvas,&event1);

    QMouseEvent event2(QEvent::MouseButtonPress, QPoint(u,v), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(canvas,&event2);
}


void CMouseMoveMap::slotAddWpt()
{
    IMap& map = CMapDB::self().getMap();
    IMap& dem = CMapDB::self().getDEM();

    double u = mousePos.x();
    double v = mousePos.y();
    map.convertPt2Rad(u,v);
    float ele = dem.getElevation(u,v);

    CWptDB::self().newWpt(u, v, ele,CWptDB::self().getNewWptName());

}


void CMouseMoveMap::slotCopyPositionTrack()
{
    if(!selTrkPt) return;

    QString position;
    GPS_Math_Deg_To_Str(selTrkPt->lon, selTrkPt->lat, position);

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(position);

}


void CMouseMoveMap::slotEditTrack()
{
    if(!selTrkPt) return;

    CTrackToolWidget * toolview = CTrackDB::self().getToolWidget();
    if(toolview) toolview->slotEdit();
}


void CMouseMoveMap::slotSplitTrack()
{
    if(selTrkPt)
    {
        CTrackDB::self().splitTrack(selTrkPt->idx);
    }
}


void CMouseMoveMap::slotOpenGoogleMaps()
{
    QString position;
    GPS_Math_Deg_To_Str(selTrkPt->lon, selTrkPt->lat, position);

    QDateTime utime = QDateTime::fromTime_t(selTrkPt->timestamp);
    utime.setTimeSpec(Qt::LocalTime);
    QString time = utime.toString();

    QDesktopServices::openUrl(QUrl("http://maps.google.com/maps?t=h&z=18&om=1&q="+position+"("+time+")", QUrl::TolerantMode));
}


void CMouseMoveMap::slotReloadMap()
{
    CMapDB::self().reloadMap();
}
