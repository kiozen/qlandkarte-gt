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

#include "CMenus.h"
#include "CMainWindow.h"
#include "CActions.h"
#include "CCanvas.h"
#include "CMegaMenu.h"

#include <QtGui>
#include <QMenu>

// #define lqdebug(x) qDebug() << x
#define lqdebug(x)

CMenus::CMenus(QObject *parent) : QObject(parent)
{
    activeGroup = MainMenu;
    actions = new CActions(this);

    QStringList list;
    list << "aSwitchToMain" << "aMoveArea" << "aZoomArea" << "aCenterMap";

    foreach(QString s, list)
    {
        QAction *a = actions->getAction(s);
        if (a)
        {
            excludedActionForMenuBarMenu << a;
        }
    }

}


CMenus::~CMenus()
{

}


void CMenus::addAction(ActionGroupName groupName, const QString& actionName, bool force /*= false*/)
{
    QAction *action = actions->getAction(actionName);

    if (action)
    {
        addAction(groupName,action,force);
    }
}


void CMenus::addAction(ActionGroupName groupName, QAction *action, bool force /*= false*/)
{
    QList<QAction *> *actionGroup;
    if(!actionGroupHash.contains(groupName))
    {
        actionGroup = new QList<QAction *>();
        actionGroupHash.insert(groupName, actionGroup);
    }
    else
    {
        actionGroup = actionGroupHash.value(groupName);
    }

    actionGroup->append(action);
    controlledActions << action;
}


void CMenus::removeAction(ActionGroupName group, QAction *action)
{

}


void CMenus::removeAction(QAction *action)
{

}


void CMenus::switchToActionGroup(ActionGroupName group)
{

    if (!actionGroupHash.value(group))
    {
        qDebug() << tr("ActionGroup %1 not defined. Please fix.").arg(group);
        return;
    }

    activeGroup = group;

    foreach(QAction* a, ((QWidget *)theMainWindow)->actions())
    {
        if (controlledActions.contains(a))
        {
            lqdebug(QString("Action with '%1' as text is controlled -> removed").arg(a->text()));
            theMainWindow->removeAction(a);
            lqdebug(a->shortcuts());
            if (!actionsShortcuts.contains(a))
            {
                actionsShortcuts.insert(a, a->shortcuts());
            }
            a->setShortcuts(QList<QKeySequence> ());
            lqdebug(a->shortcuts());
        }
        else
        {
            lqdebug(QString("Action with '%1' as text is not controlled -> don't touch").arg(a->text()));
        }
    }

    foreach(QAction* a, *actionGroupHash.value(group))
    {
        lqdebug(QString("Controlled Action with '%1' added").arg(a->text()));
        theMainWindow->addAction(a);
        if (actionsShortcuts.contains(a))
        {
            a->setShortcuts(actionsShortcuts.value(a,QList<QKeySequence> ()));
        }
        lqdebug(a->shortcuts());
    }

    emit (stateChanged());
}


bool CMenus::addActionsToMenu(QMenu *menu, MenuContextNames contex, ActionGroupName groupName)
{
    menu->setTitle(actions->getMenuTitle());
    menu->addActions(getActiveActionsList(menu,contex,groupName));

    if(groupName == MapMenu)
    {
        menu->addAction(actions->getAction("aZoomIn"));
        menu->addAction(actions->getAction("aZoomOut"));
    }
    else if(groupName == EditMenu)
    {
        menu->addAction(actions->getAction("aUndo"));
        //        menu->addAction(actions->getAction("aRedo"));
        menu->addAction(actions->getAction("aCopyToClipboard"));
        menu->addAction(actions->getAction("aPasteFromClipboard"));
    }
    return true;
}


bool CMenus::addActionsToWidget(QLabel *menu)
{
    menu->setObjectName(actions->getMenuTitle());
    menu->addActions(getActiveActionsList(menu,LeftSideMenu,activeGroup));
    menu->setPixmap(actions->getMenuPixmap());
    return true;
}


QList<QAction *> CMenus::getActiveActionsList(QObject *menu, MenuContextNames names, ActionGroupName groupName)
{
    QList<QAction *> list;
    int i=0;

    foreach(QAction *a, *getActiveActions(groupName))
    {
        lqdebug(QString("enter menu: %1 ").arg(a->shortcut().toString()));
        if ( names.testFlag(LeftSideMenu) )
        {
            QRegExp re ("^F(\\d+)$");
            if (i==0)
            {
                if (a->shortcut().toString() != "Esc")
                {
                    lqdebug("add action Esc");
                    QAction *dummyAction = new QAction(menu);
                    dummyAction->setText(tr("-"));
                    dummyAction->setShortcut(tr("Esc"));
                    list << dummyAction;
                }
                i++;
            }
            if (re.exactMatch(a->shortcut().toString()) )
            {
                int nextNumber = re.cap(1).toInt();
                lqdebug(QString("match: i=%1, nextNumber=%2").arg(i).arg(nextNumber) << a->shortcut().toString());
                while(i < nextNumber )
                {
                    lqdebug("add action" << nextNumber << i);
                    QAction *dummyAction = new QAction(menu);
                    dummyAction->setText(tr("-"));
                    dummyAction->setShortcut(tr("F%1").arg(i));
                    list << dummyAction;
                    i++;
                }
                i++;
            }

            if (i> SIZE_OF_MEGAMENU)
            {
                return list;
            }
        }
        else
        {
            i++;
        }
        if (names.testFlag(MenuBarMenu) && (!excludedActionForMenuBarMenu.contains(a) || groupName == MapMenu))
        {
            if (!actionsShortcuts.contains(a))
            {
                actionsShortcuts.insert(a, a->shortcuts());
            }
            a->setShortcuts(QList<QKeySequence> ());
            list << a;
        }
        else if (names.testFlag(ContextMenu) || names.testFlag(LeftSideMenu))
        {
            list << a;
        }
    }

    return list;
}


QList<QAction *> *CMenus::getActiveActions(ActionGroupName group)
{
    ActionGroupName g = group;
    if (g == NoMenu)
    {
        g = activeGroup;
    }
    return actionGroupHash.value(g,new QList<QAction* >());
};
