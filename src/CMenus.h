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

#ifndef CACTIONGROUPPROVIDER_H_
#define CACTIONGROUPPROVIDER_H_
#include <QList>
#include <QAction>
#include <QPointer>
#include <QHash>
#include <QSet>
#include <QFlags>

class CActions;
class QWidget;
class QLabel;

class CMenus: public QObject
{
    Q_OBJECT
        Q_ENUMS(ActionGroupName)
        public:
        CMenus(QObject * parent);
        virtual ~CMenus();
        enum ActionGroupName
        {
            NoMenu,
            Map3DMenu,
            WptMenu,
            LiveLogMenu,
            OverlayMenu,
            OverlayDistanceMenu,
            OverlayAreaMenu,
            MainMoreMenu,
            TrackMenu,
            RouteMenu,
            MapMenu,
            MainMenu,
            EditMenu
        };

        enum MenuContextName
        {
            LeftSideMenu= 0x1,
            ContextMenu = 0x2,
            MenuBarMenu = 0x4
        };

        Q_DECLARE_FLAGS(MenuContextNames, MenuContextName)

            void addAction(ActionGroupName group, QAction *action, bool force = false);
        void addAction(ActionGroupName group, const QString& actionName, bool force = false);

        void removeAction(ActionGroupName group, QAction *action);
        void removeAction(QAction *action);
        void switchToActionGroup(ActionGroupName group);
        CActions* getActions() {return actions;}
        QList<QAction *> *getActiveActions(ActionGroupName group = NoMenu);
        bool addActionsToMenu(QMenu *menu, MenuContextNames names = QFlags<CMenus::MenuContextName>(ContextMenu), ActionGroupName groupName = NoMenu);
        bool addActionsToWidget(QLabel *menu);
        QList<QAction *> getActiveActionsList(QObject *menu, MenuContextNames names , ActionGroupName groupName = NoMenu);
        signals:
        void stateChanged();
    private:
        QHash<QAction *, QList<QKeySequence> > actionsShortcuts;
        QSet<QAction *>  excludedActionForMenuBarMenu;
        QSet<QAction *>  controlledActions;
        ActionGroupName activeGroup;
        QHash<ActionGroupName, QList<QAction *> *> actionGroupHash;
        CActions* actions;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(CMenus::MenuContextNames)
#endif                           /* CMenus_H_ */
