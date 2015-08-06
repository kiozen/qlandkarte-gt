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

#include "CMegaMenu.h"
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
#include "CMenus.h"
#include "CActions.h"
#include "CMap3D.h"

#include <QtGui>
#include <QMenu>
#include <QMenuBar>

CMegaMenu * CMegaMenu::m_self = 0;

/// Left hand side multi-function menu
CMegaMenu::CMegaMenu(CCanvas * canvas)
: QLabel(canvas)
, canvas(canvas)
, currentItemIndex(-1)
, mouseDown(false)
, yoff(1)
{
    m_self = this;
    setScaledContents(true);
    setMouseTracking(true);

    actionGroup = theMainWindow->getActionGroupProvider();
    actions     = actionGroup->getActions();

}


CMegaMenu::~CMegaMenu()
{

}


void CMegaMenu::switchState()
{
    QList<QAction *> acts = QWidget::actions();
    QAction * act;
    foreach(act,acts)
    {
        removeAction(act);
    }
    actionGroup->addActionsToWidget(this);

    title = tr("%1 ...").arg(objectName().remove('&'));

}


void CMegaMenu::switchByKeyWord(const QString& key)
{
    if (!isEnabled())
        return;

    if (key == "Main")
    {
        actions->funcSwitchToMain();
    }
    else if (key == "Waypoints")
    {
        actions->funcSwitchToWpt();
    }
    else if (key == "Search")
    {
        actions->funcSwitchToMain();
    }
    else if (key == "Maps")
    {
        actions->funcSwitchToMap();
    }
    else if (key == "Tracks")
    {
        actions->funcSwitchToTrack();
    }
    else if (key == "LiveLog")
    {
        actions->funcSwitchToLiveLog();
    }
    else if (key == "Overlay")
    {
        actions->funcSwitchToOverlay();
    }
    else if (key == "OverlayDistance")
    {
        actions->funcSwitchToOverlayDistance();
    }
    else if (key == "Routes")
    {
        actions->funcSwitchToRoute();
    }
    else if (key == "GeoDB")
    {
        actions->funcSwitchToMain();
    }

}


/*!
    Initialize \a option with the values from this menu and information from \a action. This method
    is useful for subclasses when they need a QStyleOptionMenuItem, but don't want
    to fill in all the information themselves.

    \sa QStyleOption::initFrom() QMenuBar::initStyleOption()
*/
void CMegaMenu::initStyleOption(QStyleOptionMenuItem *option, const QAction *action, bool isCurrent) const
{
    if (!option || !action)
    {
        return;
    }

    option->initFrom(this);
    option->palette = palette();
    option->palette.setCurrentColorGroup(QPalette::Normal);

    option->palette.setBrush(QPalette::Normal, QPalette::Button, Qt::NoBrush);
    option->palette.setBrush(QPalette::Inactive, QPalette::Button, Qt::NoBrush);

    option->state = QStyle::State_None;

    if (window()->isActiveWindow())
    {
        option->state |= QStyle::State_Active;
    }
    if(!isCurrent)
    {
        option->palette.setBrush(QPalette::Normal, QPalette::Window, Qt::transparent);
        option->palette.setBrush(QPalette::Normal, QPalette::Button, Qt::transparent);
    }

    if (isEnabled() && action->isEnabled() && (!action->menu() || action->menu()->isEnabled()))
    {
        option->state |= QStyle::State_Enabled;
    }
    else
    {
        option->palette.setCurrentColorGroup(QPalette::Disabled);
    }

    option->font = action->font();

    if (isCurrent && !action->isSeparator())
    {
        option->state |= QStyle::State_Selected | (mouseDown ? QStyle::State_Sunken : QStyle::State_None);
#ifdef Q_OS_MAC
        option->palette.setColor(QPalette::Normal, QPalette::HighlightedText, Qt::white);
#endif
    }

    //     option->menuHasCheckableItems = d->hasCheckableItems;
    if (!action->isCheckable())
    {
        option->checkType = QStyleOptionMenuItem::NotCheckable;
    }
    else
    {
        option->checkType = (action->actionGroup() && action->actionGroup()->isExclusive())
            ? QStyleOptionMenuItem::Exclusive : QStyleOptionMenuItem::NonExclusive;
        option->checked = action->isChecked();
    }
    if (action->menu())
    {
        option->menuItemType = QStyleOptionMenuItem::SubMenu;
    }
    else if (action->isSeparator())
    {
        option->menuItemType = QStyleOptionMenuItem::Separator;
    }
    else
    {
        option->menuItemType = QStyleOptionMenuItem::Normal;
    }

    option->icon = action->icon();

    QString textAndAccel = action->text();

    if (textAndAccel.indexOf(QLatin1Char('\t')) == -1)
    {
        QKeySequence seq = action->shortcut();
        if (!seq.isEmpty())
        {
#ifdef QK_QT5_PORT
            textAndAccel += QLatin1Char('\t') + QString(seq.toString());
#else
            textAndAccel += QLatin1Char('\t') + QString(seq);
#endif
        }
    }

    option->text            = textAndAccel;

    QFontMetrics fm(option->font);
    option->tabWidth        = fm.width("MMMM");
    option->maxIconWidth    = 16;
    option->menuRect        = rect();

}


void CMegaMenu::paintEvent(QPaintEvent *e)
{
    QLabel::paintEvent(e);

    QPainter p(this);

    QPalette palette = theMainWindow->menuBar()->palette();
    p.fillRect(rect(), palette.brush(QPalette::Normal, QPalette::Window));
    QPixmap pix = *pixmap();
    p.drawPixmap(0,0, pix.scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));

    QFont f = font();
    f.setBold(true);
    p.setFont(f);

    p.setClipRegion(rectTitle);
    p.setPen(palette.brush(QPalette::Normal, QPalette::WindowText).color());
    p.drawText(rectTitle, Qt::AlignCenter, title);

    p.setFont(font());

    int idx = 0;
    QList<QAction *> acts = QWidget::actions();
    QAction * act;
    foreach(act,acts)
    {
        p.setClipRegion(rectF[idx]);

        QStyleOptionMenuItem opt;
        initStyleOption(&opt, act, currentItemIndex == idx);
        opt.rect = rectF[idx];

        style()->drawControl(QStyle::CE_MenuItem, &opt, &p, this);

        ++idx;
        if (idx >= SIZE_OF_MEGAMENU)
        {
            break;
        }
    }
}


void CMegaMenu::resizeEvent(QResizeEvent * e)
{
    QFont f = font();
    f.setBold(true);
    QFontMetrics fm(f);

    int w   = e->size().width();
    int h   = fm.height()+fm.descent()*2;
    yoff    = 0;

    if(h < 16+2)
    {
        h = 16+2;
    }

    rectTitle = QRect(0,yoff, w, h);
    for(int i=0; i < SIZE_OF_MEGAMENU; ++i)
    {
        yoff += h;
        rectF[i] = QRect(0,yoff, w, h);
    }

    yoff += h;
    setMinimumHeight(yoff);
}


void CMegaMenu::leaveEvent ( QEvent * event )
{
    currentItemIndex = -1;
    mouseDown = false;
    update();
}


void CMegaMenu::mousePressEvent(QMouseEvent * e)
{
    mouseDown = true;
    update();
}


void CMegaMenu::mouseReleaseEvent(QMouseEvent * e)
{
    QList<QAction*> acts = QWidget::actions();

    QPoint pos = e->pos();
    currentItemIndex = -1;
    for(int i = 0; i < SIZE_OF_MEGAMENU; ++i)
    {
        if(rectF[i].contains(pos))
        {
            if (acts.size() > i)
            {
                acts[i]->trigger();
                break;
            }
        }
    }

    mouseDown = false;
    update();
}


void CMegaMenu::mouseMoveEvent(QMouseEvent * e)
{
    QPoint pos = e->pos();
    currentItemIndex = -1;
    for(int i = 0; i < SIZE_OF_MEGAMENU; ++i)
    {
        if(rectF[i].contains(pos))
        {
            currentItemIndex = i;
            update();
            return;
        }
    }
}


void CMegaMenu::slotSplitterMoved(int pos, int index)
{
    if(index == 1)
    {
        setEnabled(pos);
    }
}
