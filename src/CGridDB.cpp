/**********************************************************************************************
    Copyright (C) 2011 Oliver Eichler oliver.eichler@gmx.de

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
#include "CGridDB.h"
#include "CMapDB.h"
#include "IMap.h"
#include "CMainWindow.h"
#include "CResources.h"
#include "CDlgSetupGrid.h"
#include "CSettings.h"

#include <QtGui>
#include <QCheckBox>
#include <QFileDialog>
#include <QStatusBar>

CGridDB * CGridDB::m_pSelf = 0;

CGridDB::CGridDB(QObject * parent)
: QObject(parent)
, pjWGS84(0)
, pjGrid(0)
, showGrid(false)
, projstr("+proj=longlat +datum=WGS84 +no_defs")
, color(Qt::magenta)

{
    m_pSelf = this;

    SETTINGS;
    color   = QColor(cfg.value("map/grid/color", color.name()).toString());
    projstr = cfg.value("map/grid/proj", projstr).toString();

    pjWGS84 = pj_init_plus("+proj=longlat +datum=WGS84 +no_defs");

    checkGrid = new QCheckBox(theMainWindow);
    checkGrid->setText(tr("Grid"));
    theMainWindow->statusBar()->addPermanentWidget(checkGrid);

    connect(checkGrid, SIGNAL(toggled(bool)), this, SLOT(slotShowGrid(bool)));
    connect(checkGrid, SIGNAL(clicked()), theMainWindow->getCanvas(), SLOT(update()));

    setupGrid = new QToolButton(theMainWindow);
    setupGrid->setIcon(QIcon(":/icons/iconConfig16x16.png"));
    theMainWindow->statusBar()->addPermanentWidget(setupGrid);

    connect(setupGrid,SIGNAL(clicked()),this,SLOT(slotSetupGrid()));

    showGrid = cfg.value("map/grid", showGrid).toBool();
    checkGrid->setChecked(showGrid);

    setProjAndColor(projstr, color);
}


CGridDB::~CGridDB()
{
    if(pjWGS84) pj_free(pjWGS84);
    if(pjGrid)  pj_free(pjGrid);

    SETTINGS;
    cfg.setValue("map/grid", showGrid);
    cfg.setValue("map/grid/proj", projstr);
    cfg.setValue("map/grid/color", color.name());
}


void CGridDB::slotSetupGrid()
{
    CDlgSetupGrid dlg(theMainWindow);
    dlg.exec();
}


void CGridDB::setProjAndColor(const QString& proj, const QColor& c)
{
    projstr = proj;
    color   = c;

    if(pjGrid) pj_free(pjGrid);
    pjGrid  = pj_init_plus(projstr.toLatin1());

    setupGrid->setToolTip(tr("Configure grid color and projection.\nCur. proj.: %1").arg(projstr));

    theMainWindow->getCanvas()->update();
}


void CGridDB::convertPt2Pos(double& x, double& y, bool& isLonLat)
{
    if(pjGrid == NULL)
    {
        x = 0; y = 0;
        return;
    }

    IMap& map = CMapDB::self().getMap();

    map.convertPt2Rad(x,y);
    pj_transform(pjWGS84, pjGrid, 1, 0, &x, &y, 0);
    isLonLat = pj_is_latlong(pjGrid);

}


void CGridDB::findGridSpace(double min, double max, double& xSpace, double& ySpace)
{
    double dX = fabs(min - max) / 10;
    if(dX < M_PI/180000)
    {
        xSpace = 5*M_PI/1800000;
        ySpace = 5*M_PI/1800000;
    }
    else if(dX < M_PI/18000)
    {
        xSpace = 5*M_PI/180000;
        ySpace = 5*M_PI/180000;
    }
    else if(dX < M_PI/1800)
    {
        xSpace = 5*M_PI/18000;
        ySpace = 5*M_PI/18000;
    }
    else if(dX < M_PI/180)
    {
        xSpace = 5*M_PI/1800;
        ySpace = 5*M_PI/1800;
    }
    else if(dX < M_PI/18)
    {
        xSpace = 5*M_PI/180;
        ySpace = 5*M_PI/180;
    }

    else if(dX < 3000)
    {
        xSpace = 1000;
        ySpace = 1000;
    }
    else if(dX < 7000)
    {
        xSpace = 5000;
        ySpace = 5000;
    }
    else if(dX < 30000)
    {
        xSpace = 10000;
        ySpace = 10000;
    }
    else if(dX < 70000)
    {
        xSpace = 50000;
        ySpace = 50000;
    }
    else if(dX < 300000)
    {
        xSpace = 100000;
        ySpace = 100000;
    }
    else if(dX < 700000)
    {
        xSpace = 500000;
        ySpace = 500000;
    }
    else if(dX < 3000000)
    {
        xSpace = 1000000;
        ySpace = 1000000;
    }
    else if(dX < 7000000)
    {
        xSpace = 5000000;
        ySpace = 5000000;
    }
    else if(dX < 30000000)
    {
        xSpace = 10000000;
        ySpace = 10000000;
    }
    else if(dX < 70000000)
    {
        xSpace = 50000000;
        ySpace = 50000000;
    }

}


bool CGridDB::calcIntersection(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4, double& x, double& y)
{
    double ua = ((x4 - x3)*(y1 - y3) - (y4 - y3)*(x1 - x3))/((y4 - y3)*(x2 - x1) - (x4 - x3)*(y2 - y1));

    x = x1 + ua * (x2 - x1);
    y = y1 + ua * (y2 - y1);

    double d12 = (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
    double d1x = (x1 - x)  * (x1 - x)  + (y1 - y)  * (y1 - y);
    double d2x = (x2 - x)  * (x2 - x)  + (y2 - y)  * (y2 - y);
    double d34 = (x4 - x3) * (x4 - x3) + (y4 - y3) * (y4 - y3);
    double d3x = (x3 - x)  * (x3 - x)  + (y3 - y)  * (y3 - y);
    double d4x = (x4 - x)  * (x4 - x)  + (y4 - y)  * (y4 - y);

    return (d12 >= d1x) && (d12 >= d2x) && (d34 >= d3x) && (d34 >= d4x);
}


struct val_t
{
    val_t(qint32 pos, double val) : pos(pos), val(val){}
    qint32 pos;
    double val;
};

void CGridDB::draw(QPainter& p, const QRect& rect, bool& needsRedraw)
{
    if(pjWGS84 == 0 || pjGrid == 0 || !showGrid) return;

    IMap& map       = CMapDB::self().getMap();

    projXY topLeft, topRight, btmLeft, btmRight;

    btmLeft.u   = topLeft.u     = rect.left();
    topRight.v  = topLeft.v     = rect.top();
    btmRight.u  = topRight.u    = rect.right();
    btmRight.v  = btmLeft.v     = rect.bottom();

    map.convertPt2Rad(topLeft.u,  topLeft.v);
    map.convertPt2Rad(topRight.u, topRight.v);
    map.convertPt2Rad(btmLeft.u,  btmLeft.v);
    map.convertPt2Rad(btmRight.u, btmRight.v);

    pj_transform(pjWGS84, pjGrid, 1, 0, &topLeft.u, &topLeft.v, 0);
    pj_transform(pjWGS84, pjGrid, 1, 0, &topRight.u, &topRight.v, 0);
    pj_transform(pjWGS84, pjGrid, 1, 0, &btmLeft.u, &btmLeft.v, 0);
    pj_transform(pjWGS84, pjGrid, 1, 0, &btmRight.u, &btmRight.v, 0);

    //    qDebug() << "---";
    //    qDebug() << "topLeft " << topLeft.u  << topLeft.v;
    //    qDebug() << "topRight" << topRight.u << topRight.v;
    //    qDebug() << "btmLeft " << btmLeft.u  << btmLeft.v;
    //    qDebug() << "btmRight" << btmRight.u << btmRight.v;

    //    qDebug() << topLeft.u - topRight.u;
    //    qDebug() << btmLeft.u - btmRight.u;

    //    qDebug() << topLeft.v  - btmLeft.v;
    //    qDebug() << topRight.v - btmRight.v;

    double topMax   = topLeft.v  > topRight.v   ? topLeft.v  : topRight.v;
    double btmMin   = btmLeft.v  < btmRight.v   ? btmLeft.v  : btmRight.v;
    double leftMin  = topLeft.u  < btmLeft.u    ? topLeft.u  : btmLeft.u;
    double rightMax = topRight.u > btmRight.u   ? topRight.u : btmRight.u;

    double xGridSpace = 1000;
    double yGridSpace = 1000;
    findGridSpace(leftMin, rightMax, xGridSpace, yGridSpace);

    double xStart = floor(leftMin / xGridSpace) * xGridSpace;
    double yStart = ceil(topMax / yGridSpace) * yGridSpace;

    double x = xStart - xGridSpace;
    double y = yStart + yGridSpace;

    //    qDebug() << xStart  << yStart ;
    //    qDebug() << xGridSpace  << yGridSpace ;

    QList< val_t > horzTopTicks;
    QList< val_t > horzBtmTicks;
    QList< val_t > vertLftTicks;
    QList< val_t > vertRgtTicks;

    p.save();
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(color,1));
    USE_ANTI_ALIASING(p,false); //669 DAV

    double h = rect.height();
    double w = rect.width();

    while(y > btmMin)
    {
        while(x < rightMax)
        {
            double x1 = x;
            double y1 = y;
            double x2 = x + xGridSpace;
            double y2 = y;
            double x3 = x + xGridSpace;
            double y3 = y - yGridSpace;
            double x4 = x;
            double y4 = y - yGridSpace;

            double xVal = x1;
            double yVal = y1;

            pj_transform(pjGrid, pjWGS84, 1, 0, &x1, &y1, 0);
            pj_transform(pjGrid, pjWGS84, 1, 0, &x2, &y2, 0);
            pj_transform(pjGrid, pjWGS84, 1, 0, &x3, &y3, 0);
            pj_transform(pjGrid, pjWGS84, 1, 0, &x4, &y4, 0);

            map.convertRad2Pt(x1, y1);
            map.convertRad2Pt(x2, y2);
            map.convertRad2Pt(x3, y3);
            map.convertRad2Pt(x4, y4);

            double xx,yy;
            if(calcIntersection(0,0,w,0, x1, y1, x4, y4, xx, yy))
            {
                horzTopTicks << val_t(xx, xVal);
            }
            if(calcIntersection(0,h,w,h, x1, y1, x4, y4, xx, yy))
            {
                horzBtmTicks << val_t(xx, xVal);
            }
            if(calcIntersection(0,0,0,h, x1, y1, x2, y2, xx, yy))
            {
                vertLftTicks << val_t(yy, yVal);
            }
            if(calcIntersection(w,0,w,h, x1, y1, x2, y2, xx, yy))
            {
                vertRgtTicks << val_t(yy, yVal);
            }

            p.drawLine(x1, y1, x2, y2);
            p.drawLine(x2, y2, x3, y3);
            p.drawLine(x3, y3, x4, y4);
            p.drawLine(x4, y4, x1, y1);

            x += xGridSpace;
        }
        x  = xStart;
        y -= yGridSpace;
    }
    USE_ANTI_ALIASING(p,true); //669 DAV
    p.restore();

    QColor textColor; //669 DAV
    textColor.setHsv(color.hslHue(), color.hsvSaturation(), (color.value()>128?color.value()-128:0));

    if(pj_is_latlong(pjGrid))
    {
        QFontMetrics fm(CResources::self().getMapFont());
        int yoff  = fm.height() + fm.ascent();
        int xoff  = fm.width("XX.XXXX")>>1;

        foreach(const val_t& val, horzTopTicks)
        {
            CCanvas::drawText(fabs(val.val)<1.e-5?"0":QString("%1%2").arg(val.val * RAD_TO_DEG).arg(QChar(0260)), p, QPoint(val.pos, yoff), textColor); //669 DAV
        }

        foreach(const val_t& val, horzBtmTicks)
        {
            CCanvas::drawText(fabs(val.val)<1.e-5?"0":QString("%1%2").arg(val.val * RAD_TO_DEG).arg(QChar(0260)), p, QPoint(val.pos, h), textColor); //669 DAV
        }

        foreach(const val_t& val, vertLftTicks)
        {
            CCanvas::drawText(fabs(val.val)<1.e-5?"0":QString("%1%2").arg(val.val * RAD_TO_DEG).arg(QChar(0260)), p, QPoint(xoff, val.pos), textColor); //669 DAV
        }

        foreach(const val_t& val, vertRgtTicks)
        {
            CCanvas::drawText(fabs(val.val)<1.e-5?"0":QString("%1%2").arg(val.val * RAD_TO_DEG).arg(QChar(0260)), p, QPoint(w - xoff, val.pos), textColor); //669 DAV
        }
    }
    else
    {
        QFontMetrics fm(CResources::self().getMapFont());
        int yoff  = fm.height() + fm.ascent();
        int xoff  = fm.width("XXXX")>>1;

        foreach(const val_t& val, horzTopTicks)
        {
            CCanvas::drawText(QString("%1").arg(qint32(val.val/1000)), p, QPoint(val.pos, yoff), textColor); //669 DAV
        }

        foreach(const val_t& val, horzBtmTicks)
        {
            CCanvas::drawText(QString("%1").arg(qint32(val.val/1000)), p, QPoint(val.pos, h), textColor); //669 DAV
        }

        foreach(const val_t& val, vertLftTicks)
        {
            CCanvas::drawText(QString("%1").arg(qint32(val.val/1000)), p, QPoint(xoff, val.pos), textColor); //669 DAV
        }

        foreach(const val_t& val, vertRgtTicks)
        {
            CCanvas::drawText(QString("%1").arg(qint32(val.val/1000)), p, QPoint(w - xoff, val.pos), textColor); //669 DAV
        }
    }
}
