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

#ifndef CACTIONS_H_
#define CACTIONS_H_
#include <QObject>
#include <QString>
#include <QPixmap>
#include <QPointer>
class QAction;
class CMenus;
class CCanvas;
class CActions : public QObject
{
    Q_OBJECT
        public:
        CActions(QObject *parent);
        virtual ~CActions();
        QAction *getAction(const QString& actionObjectName);
        const QString& getMenuTitle() { return menuTitle; };
        const QPixmap & getMenuPixmap( ) {return menuPixmap;};
    private:
        void createAction(const QString& shortCut,const char * icon,const QString& name, const QString& setObjectName, const QString& tooltip);
        QObject *parent;
        CMenus *actionGroup;
        QPointer<CCanvas>  canvas;
        QString menuTitle;
        QPixmap menuPixmap;
        void setMenuTitle(const QString &menuTitle);
        void setMenuPixmap(const QPixmap & menuPixmap);
    public slots:
        void funcSwitchToMain();
        void funcSwitchToMap();
        void funcSwitchToMap3D();

        void funcSwitchToWpt();
        void funcSwitchToTrack();
        void funcSwitchToRoute();

        void funcSwitchToLiveLog();
        void funcSwitchToOverlay();
        void funcSwitchToOverlayDistance();
        void funcSwitchToOverlayArea();
        void funcSwitchToMainMore();
        void funcClearAll();
        void funcUploadAll();
        void funcDownloadAll();

        void funcMoveArea();
        void funcZoomArea();
        void funcCenterMap();

        void funcSelectArea();
        void funcEditMap();
        void funcSearchMap();
        void funcUploadMap();

        void funcCloseMap3D();
        void funcMap3DMode();
        void funcMap3DFPVMode();
        void funcMap3DLighting();
        void funcMap3DTrackMode();

        void funcNewWpt();
        void funcSelWpt();
        void funcEditWpt();
        void funcMoveWpt();
        void funcImageWpt();
        void funcUploadWpt();
        void funcDownloadWpt();

        void funcCombineTrack();
        void funcEditTrack();
        void funcCutTrack();
        void funcSelTrack();
        void funcUploadTrack();
        void funcDownloadTrack();
        void funcTrackPurgeSelection();

        void funcDeleteTrackSelection();

        void funcUploadRoute();
        void funcDownloadRoute();

        void funcLiveLog();
        void funcLockMap();
        void funcAddWpt();

        void funcBackToOverlay();
        void funcText();
        void funcTextBox();
        void funcDistance();
        void funcArea();
        void funcCombineDistOvl();

        void funcOcm();
        void funcColorPicker();

        void funcZoomIn();
        void funcZoomOut();
        void funcMoveLeft();
        void funcMoveRight();
        void funcMoveUp();
        void funcMoveDown();
        void funcCopyToClipboard();
        void funcPasteFromClipboard();
        void funcRedo();
        void funcUndo();
};
#endif                           /* CACTIONS_H_ */
