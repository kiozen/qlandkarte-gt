/** ********************************************************************************************
    Copyright (c) 2009 Marc Feld

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
********************************************************************************************* */

#include "CActions.h"
#include <QAction>
#include <QDebug>
#include <QInputDialog>
#include "CMainWindow.h"
#include "CMenus.h"
#include "CCanvas.h"
#include "CSearchDB.h"
#include "CWptDB.h"
#include "CMapDB.h"
#include "CTrackDB.h"
#include "CDiaryDB.h"
#include "CLiveLogDB.h"
#include "COverlayDB.h"
#include "CRouteDB.h"
#include "CTrackToolWidget.h"
#include "CCreateMapGeoTiff.h"
#include "CMainWindow.h"
#include "CResources.h"
#include "IDevice.h"
#include "CUndoStackModel.h"
#include "CUndoStackView.h"
#include "CTrackUndoCommandDeletePts.h"
#include "CTrackUndoCommandPurgePts.h"
#include "CMap3D.h"
#include "CDlgExport.h"
#include "CTabWidget.h"
#include "COverlayDistance.h"

#include <QtGui>

CActions::CActions(QObject *parent) :
QObject(parent), parent(parent)
{
    actionGroup = (CMenus*) parent;
    canvas = theMainWindow->getCanvas();

    createAction(tr("F1"), ":/icons/iconMap16x16.png", tr("&Map ..."), "aSwitchToMap", tr("Manage maps."));
    createAction(tr("F2"), ":/icons/iconWaypoint16x16.png", tr("&Waypoint ..."), "aSwitchToWpt", tr("Manage waypoints."));
    createAction(tr("F3"), ":/icons/iconTrack16x16.png", tr("&Track ..."), "aSwitchToTrack", tr("Manage tracks."));
    createAction(tr("F4"), ":/icons/iconRoute16x16.png", tr("&Route ..."), "aSwitchToRoute", tr(""));
    createAction(tr("F5"), ":/icons/iconLiveLog16x16.png", tr("Live &Log ..."), "aSwitchToLiveLog", tr("Toggle live log recording."));
    createAction(tr("F6"), ":/icons/iconOverlay16x16.png", tr("&Overlay ..."), "aSwitchToOverlay", tr("Manage overlays, such as textboxes"));
    createAction(tr("F7"), ":/icons/iconGlobe+16x16.png", tr("Mor&e ..."), "aSwitchToMainMore", tr("Extended functions."));
    createAction(tr("F8"), ":/icons/iconClear16x16.png", tr("&Clear all"), "aClearAll", tr("Remove all waypoints, tracks, ..."));
    createAction(tr("F9"), ":/icons/iconUpload16x16.png", tr("U&pload all"), "aUploadAll", tr("Upload all data to device."));
    createAction(tr("F10"), ":/icons/iconDownload16x16.png", tr("Down&load all"), "aDownloadAll", tr("Download all data from device."));

    createAction(tr("ESC"), ":/icons/iconBack16x16.png", tr("&Back"), "aSwitchToMain", tr("Go back to main menu."));
    createAction(tr("F1"), ":/icons/iconMoveMap16x16.png", tr("Mo&ve Map"), "aMoveArea", tr("Move the map. Press down the left mouse button and move the mouse."));
    createAction(tr("F2"), ":/icons/iconZoomArea16x16.png", tr("&Zoom Map"), "aZoomArea", tr("Select area for zoom."));
    createAction(tr("F3"), ":/icons/iconCenter16x16.png", tr("&Center Map"), "aCenterMap", tr("Find your map by jumping to it's center."));

    createAction(tr("F5"), ":/icons/iconSelect16x16.png", tr("Select &Sub Map"), "aSelectArea", tr("Select area of map to export. Select area by pressing down the left mouse button and move the mouse."));
    createAction(tr("F6"), ":/icons/iconEdit16x16.png", tr("&Edit / Create Map"), "aEditMap", tr(""));
    createAction(tr("F7"), ":/icons/iconFind16x16.png", tr("&Search Map"), "aSearchMap", tr("Find symbols on a map via image recognition."));
    createAction(tr("F8"),":/icons/icon3D16x16.png",tr("3&D Map..."), "aSwitchToMap3D", tr("Show 3D map"));
    createAction(tr("F9"), ":/icons/iconUpload16x16.png", tr("U&pload"), "aUploadMap", tr("Upload map selection to device."));


    createAction(tr("ESC"),":/icons/iconBack16x16",tr("&Close"),"aCloseMap3D",tr("Close 3D view."));
    createAction(tr("F1"),0,tr("3D / 2D"),"aMap3DMode",tr("Toggle between 3D and 2D map."));
    createAction(tr("F2"),0,tr("FPV / Rot."),"aMap3DFPVMode",tr("Toggle between first person view and rotation mode."));
    createAction(tr("F3"),":/icons/iconLight16x16",tr("Lighting On/Off"), "aMap3DLighting",tr("Turn on/off lighting."));
    createAction(tr("F4"),":/icons/iconTrack16x16",tr("Trackmode"), "aMap3DTrackMode",tr("Glue point of view to track."));

    //
    createAction(tr("F5"), ":/icons/iconAdd16x16.png", tr("&New Waypoint"), "aNewWpt", tr("Create a new user waypoint. The default position will be the current cursor position."));
    createAction(tr("F6"), ":/icons/iconSelect16x16.png", tr("&Radius Select"), "aSelWpt", tr("Select waypoints in a radius"));
#ifdef HAS_EXIF
    createAction(tr("F8"),":/icons/iconRaster16x16.png",tr("From &Images..."),"aImageWpt",tr("Create waypoints from geo-referenced images in a path."));
#endif
    createAction(tr("F9"), ":/icons/iconUpload16x16.png", tr("U&pload"), "aUploadWpt", tr("Upload waypoints to device."));
    createAction(tr("F10"), ":/icons/iconDownload16x16.png", tr("Down&load"), "aDownloadWpt", tr("Download waypoints from device."));
    //
    createAction(tr("F5"), ":/icons/iconAdd16x16.png", tr("Join &Tracks"), "aCombineTrack", tr("Join multiple selected tracks to one."));
    createAction(tr("F6"), ":/icons/iconEdit16x16.png", tr("&Edit Track"), "aEditTrack", tr("Toggle track edit dialog."));
    createAction(tr("F7"), ":/icons/iconEditCut16x16.png", tr("&Split Track"), "aCutTrack", tr("Split a track into pieces."));
    createAction(tr("F8"), ":/icons/iconSelect16x16.png", tr("&Select Points"), "aSelTrack", tr("Select track points by rectangle."));
    createAction(tr("F9"), ":/icons/iconUpload16x16.png", tr("U&pload"), "aUploadTrack", tr("Upload tracks to device."));
    createAction(tr("F10"), ":/icons/iconDownload16x16.png", tr("Down&load"), "aDownloadTrack", tr("Download tracks from device."));
    createAction(tr("ctrl+Del"), ":/icons/iconClear16x16.png", tr("Hide/Show Selection"), "aTrackPurgeSelection", tr("Toggle visibility of the selected track points."));
    //
    createAction(tr("F5"), ":/icons/iconPlayPause16x16.png", tr("&Start / Stop"), "aLiveLog", tr("Start / stop live log recording."));
    createAction(tr("F6"), ":/icons/iconLock16x16.png", tr("Move Map to &Pos."), "aLockMap", tr("Move the map to keep the positon cursor centered."));
    createAction(tr("F7"), ":/icons/iconAdd16x16.png", tr("Add &Waypoint"), "aAddWpt", tr("Add a waypoint at current position."));
    //

    createAction(tr("ESC"), ":/icons/iconBack16x16.png", tr("&Back"), "aBackToOverlay", tr("Go back to overlay menu."));
    createAction(tr("F5"), ":/icons/iconText16x16.png", tr("Add Static &Text Box"), "aText", tr("Add text on the map."));
    createAction(tr("F6"), ":/icons/iconTextBox16x16.png", tr("Add &Geo-Ref. Text Box"), "aTextBox", tr("Add a textbox on the map."));
    createAction(tr("F7"), ":/icons/iconDistance16x16.png", tr("Add Distance &Polyline"), "aDistance", tr("Add a polyline to measure distances."));
    createAction(tr("F7"), ":/icons/iconDistance16x16.png", tr("Distance &Polyline"), "aSwitchToOverlayDistance", tr("Add a polyline to measure distances."));
    createAction(tr("F5"), ":/icons/iconAdd16x16.png", tr("Join Distance PolyLines"), "aCombineDistOvl", tr("Join distance polylines to one."));
    createAction(tr("F8"), ":/icons/iconArea16x16.png", tr("Add Area Polygon"), "aArea", tr("Mark an area with a polygon."));
    createAction(tr("F8"), ":/icons/iconArea16x16.png", tr("Area Polygon"), "aSwitchToOverlayArea", tr("Mark an area with a polygon."));

    //
    createAction(tr("F6"), ":/icons/cache/Traditional-Cache.png", tr("&Export to OCM"), "aOcm", tr("Send current workspace to Open Cache Manager."));

    //
    createAction(tr("F9"), ":/icons/iconUpload16x16.png", tr("U&pload"), "aUploadRoute", tr("Upload routes to device."));
    createAction(tr("F10"), ":/icons/iconDownload16x16.png", tr("Down&load"), "aDownloadRoute", tr("Download routes from device."));
    //

    createAction(tr("+"), ":/icons/zoomin.png", tr("&Zoom in"), "aZoomIn", tr("Zoom's into the Map."));
    createAction(tr("-"), ":/icons/zoomout.png", tr("&Zoom out"), "aZoomOut", tr("Zoom's out of the Map."));
    createAction(QKeySequence (Qt::ALT + Qt::Key_Left).toString(), ":/icons/editcopy.png", tr("&Move left"), "aMoveLeft", tr("Move to the left side."));
    createAction(QKeySequence (Qt::ALT + Qt::Key_Right).toString(), ":/icons/editcopy.png", tr("&Move right"), "aMoveRight", tr("Move to the right side."));
    createAction(QKeySequence (Qt::ALT + Qt::Key_Up).toString(), ":/icons/editcopy.png", tr("&Move up"), "aMoveUp", tr("Move up."));
    createAction(QKeySequence (Qt::ALT + Qt::Key_Down).toString(), ":/icons/editcopy.png", tr("&Move down"), "aMoveDown", tr("Move down."));
    createAction("CTRL+c", ":/icons/editcopy.png", tr("&Copy"), "aCopyToClipboard", tr(""));
    createAction("CTRL+v", ":/icons/editpaste.png", tr("&Paste"), "aPasteFromClipboard", tr(""));
    createAction("CTRL+z", ":/icons/editundo.png", tr("&Undo"), "aUndo", tr("Undo a command."));
    createAction("SHIFT+CTRL+z", ":/icons/editredo.png", tr("&Redo"), "aRedo", tr("Redo a command."));

}


CActions::~CActions()
{

}


void CActions::createAction(const QString& shortCut,
const char * icon,
const QString& name,
const QString& actionName,
const QString& toolTip)
{
    if (findChild<QAction *> (actionName))
    {
        qDebug() << tr("Action with the name '%1' already registered. Please choose another name.").arg(actionName);
        return;
    }

    QAction *tmpAction = new QAction(QIcon(icon), name, this);
    tmpAction->setObjectName(actionName);
    tmpAction->setShortcut(shortCut);
    tmpAction->setToolTip(toolTip);

    QString slotName;
    if (actionName.startsWith('a'))
    {
        slotName = "func" + actionName.mid(1);
    }
    else
    {
        slotName = actionName;
    }

    connect(tmpAction, SIGNAL(triggered()), this, QString("1" + slotName + "()").toLatin1().data());

}


QAction *CActions::getAction(const QString& actionObjectName)
{
    QAction *tmpAction = findChild<QAction *> (actionObjectName);
    if (tmpAction)
    {
        return tmpAction;
    }
    else
    {
        qDebug() << tr("Action with name '%1' not found. %2").arg(actionObjectName).arg(Q_FUNC_INFO);
        return new QAction(this);
    }
}


void CActions::setMenuTitle(const QString& title)
{
    menuTitle = title;
}


void CActions::setMenuPixmap(const QPixmap& pixmap)
{
    menuPixmap = pixmap;
}


void CActions::funcSwitchToMain()
{
    // qDebug() << Q_FUNC_INFO;
    setMenuTitle(tr("&Main"));
    setMenuPixmap(QPixmap(":/icons/backGlobe128x128.png"));
    actionGroup->switchToActionGroup(CMenus::MainMenu);
    funcMoveArea();
}


void CActions::funcSwitchToMap()
{
    // qDebug() << Q_FUNC_INFO;
    setMenuTitle(tr("&Maps"));
    setMenuPixmap(QPixmap(":/icons/backMap128x128.png"));

    if(theMainWindow->getCanvasTab()->currentWidget()->objectName() == "CMap3D")
    {
        actionGroup->switchToActionGroup(CMenus::Map3DMenu);
    }
    else
    {
        actionGroup->switchToActionGroup(CMenus::MapMenu);
    }

    CMapDB::self().gainFocus();
    funcMoveArea();
}


void CActions::funcSwitchToMap3D()
{
    setMenuTitle(tr("&Maps"));
    setMenuPixmap(QPixmap(":/icons/backMap128x128.png"));
    actionGroup->switchToActionGroup(CMenus::Map3DMenu);
    CMapDB::self().gainFocus();
    CMapDB::self().show3DMap(true);
}


void CActions::funcSwitchToWpt()
{
    setMenuTitle(tr("&Waypoints"));
    setMenuPixmap(QPixmap(":/icons/backWaypoint128x128.png"));
    actionGroup->switchToActionGroup(CMenus::WptMenu);
    CWptDB::self().gainFocus();
    funcMoveArea();
}


void CActions::funcSwitchToTrack()
{
    setMenuTitle(tr("&Tracks"));
    setMenuPixmap(QPixmap(":/icons/backTrack128x128.png"));
    actionGroup->switchToActionGroup(CMenus::TrackMenu);
    CTrackDB::self().gainFocus();
    funcMoveArea();
}


void CActions::funcSwitchToRoute()
{
    setMenuTitle(tr("&Routes"));
    setMenuPixmap(QPixmap(":/icons/backRoute128x128.png"));
    actionGroup->switchToActionGroup(CMenus::RouteMenu);
    CRouteDB::self().gainFocus();
    funcMoveArea();
}


void CActions::funcSwitchToLiveLog()
{
    setMenuTitle(tr("&Live Log"));
    setMenuPixmap(QPixmap(":/icons/backLiveLog128x128.png"));
    actionGroup->switchToActionGroup(CMenus::LiveLogMenu);
    CLiveLogDB::self().gainFocus();
    funcMoveArea();
}


void CActions::funcSwitchToOverlay()
{
    setMenuTitle(tr("&Overlay"));
    setMenuPixmap(QPixmap(":/icons/backOverlay128x128.png"));
    actionGroup->switchToActionGroup(CMenus::OverlayMenu);
    COverlayDB::self().gainFocus();
    funcMoveArea();
}


void CActions::funcSwitchToOverlayDistance()
{
    setMenuTitle(tr("&Overlay Distance"));
    setMenuPixmap(QPixmap(":/icons/backDistance128x128.png"));
    actionGroup->switchToActionGroup(CMenus::OverlayDistanceMenu);
    COverlayDB::self().gainFocus();

    funcDistance();
}


void CActions::funcSwitchToOverlayArea()
{
    setMenuTitle(tr("&Overlay Area"));
    setMenuPixmap(QPixmap(":/icons/backArea128x128.png"));
    actionGroup->switchToActionGroup(CMenus::OverlayAreaMenu);
    COverlayDB::self().gainFocus();

    funcArea();
}


void CActions::funcSwitchToMainMore()
{
    setMenuTitle(tr("&Main (More)"));
    setMenuPixmap(QPixmap(":/icons/backGlobe+128x128.png"));
    actionGroup->switchToActionGroup(CMenus::MainMoreMenu);
    funcMoveArea();
}


//void CActions::funcDiary()
//{
//    CDiaryDB::self().openEditWidget();
//}

void CActions::funcOcm()
{
    theMainWindow->exportToOcm();
}


void CActions::funcColorPicker()
{
    canvas->setMouseMode(CCanvas::eMouseColorPicker);
}


void CActions::funcClearAll()
{
    theMainWindow->clearAll();
}


void CActions::funcUploadAll()
{
    IDevice * dev = CResources::self().device();
    if (dev == 0)
        return;

    dev->uploadAll();
}


void CActions::funcDownloadAll()
{
    IDevice * dev = CResources::self().device();
    if (dev == 0)
        return;

    dev->downloadAll();
}


void CActions::funcMoveArea()
{
    canvas->setMouseMode(CCanvas::eMouseMoveArea);
}


void CActions::funcZoomArea()
{
    canvas->setMouseMode(CCanvas::eMouseZoomArea);
}


void CActions::funcCenterMap()
{
    canvas->move(CCanvas::eMoveCenter);
}


void CActions::funcSelectArea()
{
    canvas->setMouseMode(CCanvas::eMouseSelectArea);
}


void CActions::funcEditMap()
{

    CMapDB::self().editMap();
    if (CCreateMapGeoTiff::self())
    {
        // setEnabled(false); // not finished
        connect(CCreateMapGeoTiff::self(), SIGNAL(destroyed(QObject*)), this, SLOT(slotEnable()));
    }
}


void CActions::funcSearchMap()
{
    CMapDB::self().searchMap();
}


void CActions::funcUploadMap()
{
    QStringList keys;
    CMapDB::self().upload(keys);
}


void CActions::funcNewWpt()
{
    canvas->setMouseMode(CCanvas::eMouseAddWpt);
}


void CActions::funcSelWpt()
{
    canvas->setMouseMode(CCanvas::eMouseSelWpt);
}


void CActions::funcCloseMap3D()
{
    CMapDB::self().show3DMap(false);
    setMenuTitle(tr("Maps ..."));
    setMenuPixmap(QPixmap(":/icons/backMap128x128.png"));
    actionGroup->switchToActionGroup(CMenus::MapMenu);
    CMapDB::self().gainFocus();
    funcMoveArea();
}


void CActions::funcMap3DLighting()
{
    CMap3D * map = CMapDB::self().getMap3D();
    map->lightTurn();
}


void CActions::funcMap3DMode()
{
    CMap3D * map = CMapDB::self().getMap3D();
    if(map)
    {
        map->slotChange3DMode();
    }
}


void CActions::funcMap3DFPVMode()
{
    CMap3D * map = CMapDB::self().getMap3D();
    if(map)
    {
        map->slotChange3DFPVMode();
    }
}


void CActions::funcMap3DTrackMode()
{
    CMap3D * map = CMapDB::self().getMap3D();
    if(map)
    {
        map->slotChange3DTrackMode();
    }
}


void CActions::funcEditWpt()
{
    canvas->setMouseMode(CCanvas::eMouseEditWpt);
}


void CActions::funcMoveWpt()
{
    canvas->setMouseMode(CCanvas::eMouseMoveWpt);
}


void CActions::funcImageWpt()
{
#ifdef HAS_EXIF
    CWptDB::self().createWaypointsFromImages();
#endif
}


void CActions::funcUploadWpt()
{
    QStringList keys;
    CDlgExport dlg(0,&keys,0,0);
    if( dlg.exec() == QDialog::Rejected)
    {
        return;
    }
    CWptDB::self().upload(keys);
}


void CActions::funcDownloadWpt()
{
    CWptDB::self().download();
}


void CActions::funcEditTrack()
{
    CTrackToolWidget * toolview = CTrackDB::self().getToolWidget();
    if (toolview)
        toolview->slotEdit();
}


void CActions::funcCombineTrack()
{
    CTrackDB::self().CombineTracks();
}


void CActions::funcCutTrack()
{
    canvas->setMouseMode(CCanvas::eMouseCutTrack);
}


void CActions::funcSelTrack()
{
    canvas->setMouseMode(CCanvas::eMouseSelTrack);
}


void CActions::funcUploadTrack()
{
    QStringList keys;
    CDlgExport dlg(0,0,&keys,0);
    if( dlg.exec() == QDialog::Rejected)
    {
        return;
    }
    CTrackDB::self().upload(keys);
}


void CActions::funcDownloadTrack()
{
    CTrackDB::self().download();
}


void CActions::funcTrackPurgeSelection()
{
    CTrack *track = CTrackDB::self().highlightedTrack();
    if (track)
    {
        CUndoStackModel::getInstance()->push(new CTrackUndoCommandPurgePts(track));
    }
}


void CActions::funcDeleteTrackSelection()
{
    CTrack *track = CTrackDB::self().highlightedTrack();
    if (track)
    {
        CUndoStackModel::getInstance()->push(new CTrackUndoCommandPurgePts(track));
    }
}


void CActions::funcUploadRoute()
{
    QStringList keys;
    CDlgExport dlg(0,0,0,&keys);
    if( dlg.exec() == QDialog::Rejected)
    {
        return;
    }
    CRouteDB::self().upload(keys);
}


void CActions::funcDownloadRoute()
{
    CRouteDB::self().download();
}


void CActions::funcLiveLog()
{
    CLiveLogDB::self().start(!CLiveLogDB::self().logging());
}


void CActions::funcLockMap()
{
    CLiveLogDB::self().setLockToCenter(!CLiveLogDB::self().lockToCenter());
}


void CActions::funcAddWpt()
{
    CLiveLogDB::self().addWpt();
}


void CActions::funcBackToOverlay()
{
    funcSwitchToOverlay();
}


void CActions::funcText()
{
    canvas->setMouseMode(CCanvas::eMouseAddText);
}


void CActions::funcTextBox()
{
    canvas->setMouseMode(CCanvas::eMouseAddTextBox);
}


void CActions::funcDistance()
{
    canvas->setMouseMode(CCanvas::eMouseAddDistance);
}


void CActions::funcArea()
{
    canvas->setMouseMode(CCanvas::eMouseAddArea);
}


void CActions::funcCombineDistOvl()
{
    COverlayDB::self().combineDistOvl();
}


//    else if(e->key() == Qt::Key_Plus) {
void CActions::funcZoomIn()
{
    canvas->zoom(true, canvas->geometry().center());
}


//    else if(e->key() == Qt::Key_Minus) {
void CActions::funcZoomOut()
{
    canvas->zoom(false, canvas->geometry().center());
}


//    else if(e->key() == Qt::Key_Left) {
void CActions::funcMoveLeft()
{
    canvas->move(CCanvas::eMoveLeft);
}


//    else if(e->key() == Qt::Key_Right) {
void CActions::funcMoveRight()
{
    canvas->move(CCanvas::eMoveRight);
}


//    else if(e->key() == Qt::Key_Up) {
void CActions::funcMoveUp()
{
    canvas->move(CCanvas::eMoveUp);
}


//    else if(e->key() == Qt::Key_Down) {
void CActions::funcMoveDown()
{
    canvas->move(CCanvas::eMoveDown);
}


//    else if (e->key() == Qt::Key_C && e->modifiers() == Qt::ControlModifier)
void CActions::funcCopyToClipboard()
{
    IOverlay * ovl = 0;
    CTrack *   trk = 0;

    ovl = COverlayDB::self().highlightedOverlay();
    trk = CTrackDB::self().highlightedTrack();

    if(trk && ovl)
    {
        bool ok;
        QStringList items;
        items << tr("Track") << tr("Overlay");
        QString res = QInputDialog::getItem(0, tr("What to do?"), tr("I do not know what to copy. Please select:"), items, 0, false, &ok);

        if(res == tr("Track"))
        {
            CTrackDB::self().copyToClipboard();
        }
        else if(res == tr("Overlay"))
        {
            COverlayDB::self().copyToClipboard();
        }
    }
    else if(trk)
    {
        CTrackDB::self().copyToClipboard();
    }
    else if(ovl)
    {
        COverlayDB::self().copyToClipboard();
    }
}


//    else if (e->key() == Qt::Key_V && e->modifiers() == Qt::ControlModifier)
void CActions::funcPasteFromClipboard()
{
    CTrackDB::self().pasteFromClipboard();
    COverlayDB::self().pasteFromClipboard();
}


void CActions::funcRedo()
{
    CUndoStackModel::getInstance()->redo();
    //    emit CTrackDB::m_self.sigChanged();
    //    emit CTrackDB::m_self->sigModified();
}


void CActions::funcUndo()
{
    CUndoStackModel::getInstance()->undo();
    //    emit CTrackDB::m_self()->sigChanged();
    //    emit CTrackDB::m_self()->sigModified();
}
