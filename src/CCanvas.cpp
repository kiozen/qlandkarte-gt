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
#include <QPrinter>
#include <QMenu>
#include "CResources.h"
#include "CCanvas.h"
#include "CMapNoMap.h"
#include "CMapQMAP.h"
#include "CMainWindow.h"
#include "CCreateMapGeoTiff.h"
#include "CMouseMoveMap.h"
#include "CMouseZoomMap.h"
#include "CMouseSelMap.h"
#include "CMouseAddWpt.h"
#include "CMouseMoveWpt.h"
#include "CMouseEditWpt.h"
#include "CMouseRefPoint.h"
#include "CMouseCutTrack.h"
#include "CMouseSelTrack.h"
#include "CMouseAddText.h"
#include "CMouseAddTextBox.h"
#include "CMouseAddDistance.h"
#include "CMouseAddArea.h"
#include "CMouseOverlay.h"
#include "CMouseColorPicker.h"
#include "CMouseSelWpt.h"
#include "CWpt.h"
#include "CTrack.h"
#include "CSearchDB.h"
#include "CWptDB.h"
#include "CMapDB.h"
#include "CTrackDB.h"
#include "CTrackToolWidget.h"
#include "CRouteDB.h"
#include "CLiveLogDB.h"
#include "COverlayDB.h"
#include "CMegaMenu.h"
#include "GeoMath.h"
#include "WptIcons.h"
#include "Platform.h"
#include "IUnit.h"
#include "CMenus.h"
#include "CUndoStackView.h"
#include "CCanvasUndoCommandZoom.h"
#include "CMapUndoCommandMove.h"
#include "CPlot.h"
#include "CGridDB.h"
#include "CMapDEMSlopeSetup.h"

#include <QtGui>

#include <stdio.h>
#ifndef QK_QT5_TZONE
#include <tzdata.h>
#endif

QPen CCanvas::penBorderBlue(QColor(10,10,150,220),2);
QPen CCanvas::penBorderBlack(QColor(0,0,0,200),2);
QBrush CCanvas::brushBackWhite(QColor(255,255,255,210));
QBrush CCanvas::brushBackYellow(QColor(0xff, 0xff, 0xcc, 0xE0));

#ifdef WIN32
#define isnan(x) _isnan(x)
#endif

CCanvas::CCanvas(QWidget * parent)
: QWidget(parent)
, mouse(0)
, info(0)
, profile(0)
, contextMenuActive(false)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    mouseMoveMap    = new CMouseMoveMap(this);
    mouseZoomMap    = new CMouseZoomMap(this);
    mouseSelMap     = new CMouseSelMap(this);
    mouseAddWpt     = new CMouseAddWpt(this);
    mouseMoveWpt    = new CMouseMoveWpt(this);
    mouseEditWpt    = new CMouseEditWpt(this);
    mouseRefPoint   = new CMouseRefPoint(this);
    mouseCutTrack   = new CMouseCutTrack(this);
    mouseSelTrack   = new CMouseSelTrack(this);
    mouseAddText    = new CMouseAddText(this);
    mouseAddTextBox = new CMouseAddTextBox(this);
    mouseAddDistance= new CMouseAddDistance(this);
    mouseAddArea    = new CMouseAddArea(this);
    mouseOverlay    = new CMouseOverlay(this);
    mouseColorPicker = new CMouseColorPicker(this);
    mouseSelWpt     = new CMouseSelWpt(this);

    timerFadingMessage = new QTimer(this);
    timerFadingMessage->setSingleShot(true);
    connect(timerFadingMessage, SIGNAL(timeout()), this, SLOT(slotFadingMessage()));

    timerClock = new QTimer(this);
    timerClock->setSingleShot(false);
    timerClock->start(1000);
    connect(timerClock, SIGNAL(timeout()), this, SLOT(slotTime()));

}


CCanvas::~CCanvas()
{
}


void CCanvas::setupDelayed()
{
    profile = new CPlot(CPlotData::eLinear, CPlot::eIcon, this);
    profile->resize(300,120);
    profile->hide();

    slopeSetup = new CMapDEMSlopeSetup(this);
    slopeSetup->adjustSize();
    slopeSetup->hide();

    connect(&CTrackDB::self(), SIGNAL(sigPointOfFocus(int)), this, SLOT(slotPointOfFocus(int)));
}


QColor CCanvas::getSelectedColor()
{
    return mouseColorPicker->getSelectedColor();
}


void CCanvas::slotMapChanged()
{
    if(mouse)
    {
        mouse->slotMapChanged();
    }
    update();
}


void CCanvas::slotCopyPosition()
{
    IMap& map = CMapDB::self().getMap();

    double u = posMouse.x();
    double v = posMouse.y();
    map.convertPt2Rad(u,v);
    u = u * RAD_TO_DEG;
    v = v * RAD_TO_DEG;

    QString position;
    GPS_Math_Deg_To_Str(u, v, position);

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(position);
}


void CCanvas::setMouseMode(mouse_mode_e mode)
{
    leaveEvent(0);

    if(mouse) mouse->looseFocus();
    COverlayDB::self().looseFocus();

    switch(mode)
    {

        case eMouseMoveArea:
            mouse = mouseMoveMap;
            break;

        case eMouseZoomArea:
            mouse = mouseZoomMap;
            break;

        case eMouseAddWpt:
            mouse = mouseAddWpt;
            break;

        case eMouseEditWpt:
            mouse = mouseEditWpt;
            break;

        case eMouseMoveWpt:
            mouse = mouseMoveWpt;
            break;

        case eMouseMoveRefPoint:
            mouse = mouseRefPoint;
            break;

        case eMouseSelectArea:
            mouse = mouseSelMap;
            break;

        case eMouseCutTrack:
            mouse = mouseCutTrack;
            break;

        case eMouseSelTrack:
            mouse = mouseSelTrack;
            break;

        case eMouseAddText:
            mouse = mouseAddText;
            break;

        case eMouseAddTextBox:
            mouse = mouseAddTextBox;
            break;

        case eMouseAddDistance:
            mouse = mouseAddDistance;
            break;

        case eMouseAddArea:
            mouse = mouseAddArea;
            break;

        case eMouseOverlay:
            mouse = mouseOverlay;
            break;

        case eMouseColorPicker:
            mouse = mouseColorPicker;
            break;

        case eMouseSelWpt:
            mouse = mouseSelWpt;
            break;

        default:;

    }
    if(underMouse())
    {
        enterEvent(0);
    }
    mouseMode = mode;
    update();
}


void CCanvas::resizeEvent(QResizeEvent * e)
{
    QWidget::resizeEvent(e);

    QSize s = e->size();

    if(s.height() < 700)
    {
        profile->resize(200,80);
    }
    else
    {
        profile->resize(300,120);
    }

    profile->move(20, s.height() - profile->height() - 20);

    slopeSetup->move(s.width() - slopeSetup->width() - 10, s.height() - slopeSetup->height() - 200);

    emit sigResize(e->size());
}


void CCanvas::paintEvent(QPaintEvent * e)
{
    QWidget::paintEvent(e);

    QPainter p;
    p.begin(this);
    p.fillRect(rect(),Qt::white);
    p.setFont(CResources::self().getMapFont());
    draw(p);

    p.end();
}


void CCanvas::mouseMoveEvent(QMouseEvent * e)
{
    // this check shouldn't be necessary in an ideal world.  However, at least
    // on OS X with Qt 4.7.4 I've seen sporadic mouse move events messing up
    // posMouse while the context menu is shown
    if (!contextMenuActive)
    {
        posMouse = e->pos();
        mouseMoveEventCoord(e);
        mouse->mouseMoveEvent(e);
    }
}


void CCanvas::mousePressEvent(QMouseEvent * e)
{
    if (!contextMenuActive)
    {
        posMouse = e->pos();
        mouse->mousePressEvent(e);
    }
}


void CCanvas::mouseReleaseEvent(QMouseEvent * e)
{
    if (!contextMenuActive)
    {
        posMouse = e->pos();
        mouse->mouseReleaseEvent(e);
    }
}


void CCanvas::mouseDoubleClickEvent(QMouseEvent * e)
{
    posMouse = e->pos();
    mouse->mouseDoubleClickEvent(e);
}


void CCanvas::keyPressEvent(QKeyEvent * e)
{
    mouse->keyPressEvent(e);
}


void CCanvas::keyReleaseEvent(QKeyEvent * e)
{
    mouse->keyReleaseEvent(e);
}


void CCanvas::enterEvent(QEvent * )
{
    QApplication::setOverrideCursor(*mouse);
    setMouseTracking(true);
}


void CCanvas::leaveEvent(QEvent * )
{
    QApplication::restoreOverrideCursor();
    setMouseTracking(false);
    if(mouse && !contextMenuActive)
    {
        mouse->setSelTrackPt(0);
    }
}


#define BORDER  0



void CCanvas::print(QPrinter& printer)
{
    QPainter p;

    p.begin(&printer);
    print(p, printer.pageRect().size());
    p.end();

}

void CCanvas::print(QPainter& p, const QSize& pagesize)
{
    QSize _size_ = pagesize;
    qreal s = 0.0;

    p.save();

    if(pagesize.height() > pagesize.width())
    {
        _size_.setWidth(pagesize.height());
        _size_.setHeight(pagesize.width());
        p.rotate(90.0);
        p.translate(0,-pagesize.width());
    }

    qreal s1 = (qreal)(_size_.width() - 2 * BORDER) / (qreal)size().width();
    qreal s2 = (qreal)(_size_.height() - 2 * BORDER) / (qreal)size().height();

    s = (s1 > s2) ? s2 : s1;

    p.translate(BORDER,BORDER);
    p.scale(s,s);
    p.setClipRegion(rect());
    p.setFont(CResources::self().getMapFont());
    draw(p);
    p.restore();
}





void CCanvas::print(QImage& img)
{
    QPainter p;

    p.begin(&img);
    p.fillRect(rect(), QBrush(Qt::white));
    draw(p);
    p.end();

}


void CCanvas::print(QImage& img, const QSize& pagesize)
{

    bool rotate = false;
    QSize s = pagesize ;
    if(pagesize.width() < pagesize.height())
    {
        s = QSize(s.height(), s.width());
        rotate = true;
    }

    img = QImage(s,  QImage::Format_ARGB32);
    img.fill(Qt::white);

    {
        QPainter p(&img);
        print(p, s);
    }

    if(rotate)
    {
        QMatrix matrix;
        matrix.rotate(90);
        img = img.transformed(matrix, Qt::SmoothTransformation);
    }
}


#undef DEBUG_DRAW
#ifdef DEBUG_DRAW

#define DEBUG_TIME(label)\
qDebug() << label << et.restart() << "ms";
#else
#define DEBUG_TIME(label)
#endif

void CCanvas::draw(QPainter& p)
{
    IMap& map = CMapDB::self().getMap();
    bool needsRedraw = map.getNeedsRedraw();

    USE_ANTI_ALIASING(p,!map.getFastDrawFlag() && CResources::self().useAntiAliasing());
#ifdef DEBUG_DRAW
    QElapsedTimer et;
    et.restart();
#endif

    CMapDB::self().draw(p,rect(), needsRedraw);
    DEBUG_TIME("CMapDB:     ");
    CRouteDB::self().draw(p, rect(), needsRedraw);
    DEBUG_TIME("CRouteDB:   ");
    CTrackDB::self().draw(p, rect(), needsRedraw);
    DEBUG_TIME("CTrackDB:   ");
    COverlayDB::self().draw(p, rect(), needsRedraw);
    DEBUG_TIME("COverlayDB: ");
    CLiveLogDB::self().draw(p, rect(), needsRedraw);
    DEBUG_TIME("CLiveLogDB: ");
    CWptDB::self().draw(p, rect(), needsRedraw);
    DEBUG_TIME("CWptDB:     ");
    CSearchDB::self().draw(p, rect(), needsRedraw);
    DEBUG_TIME("CSearchDB:  ");
    CGridDB::self().draw(p, rect(), needsRedraw);
    DEBUG_TIME("CGridDB:    ");

    drawRefPoints(p);
    drawScale(p);
    drawCompass(p);
    drawClock(p);
    drawFadingMessage(p);
    drawTrackLegend(p);
    DEBUG_TIME("Other:      ");

    mouse->draw(p);
    DEBUG_TIME("Mouse:      ");

#ifdef DEBUG_DRAW
    qDebug() << "-----------------------";
#endif
}


void CCanvas::drawRefPoints(QPainter& p)
{
    CCreateMapGeoTiff * dlg = CCreateMapGeoTiff::self();
    if(dlg == 0) return;

    IMap& map = CMapDB::self().getMap();

    QMap<quint32,CCreateMapGeoTiff::refpt_t>& refpts         = dlg->getRefPoints();
    QMap<quint32,CCreateMapGeoTiff::refpt_t>::iterator refpt = refpts.begin();
    while(refpt != refpts.end())
    {
        double x = refpt->x;
        double y = refpt->y;
        map.convertM2Pt(x,y);

        if(rect().contains(x,y))
        {
            p.drawPixmap(x - 15,y - 31,QPixmap(":/icons/iconRefPoint31x31.png"));
            drawText(refpt->item->text(CCreateMapGeoTiff::eLabel),p,QPoint(x, y - 35));
        }

        ++refpt;
    }

    QRect r1 = dlg->getSelArea();

    double x1 = r1.left();
    double y1 = r1.top();
    double x2 = r1.right();
    double y2 = r1.bottom();

    map.convertM2Pt(x1,y1);
    map.convertM2Pt(x2,y2);

    r1 = QRect(QPoint(x1,y1), QPoint(x2,y2));

    if(rect().intersects(r1))
    {
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(QColor("#FF8000"),3));
        p.drawRect(r1);
    }
}


void CCanvas::drawScale(QPainter& p)
{
    if(!CResources::self().showScale())
    {
        return;
    }

    IMap& map = CMapDB::self().getMap();

    double a,b,d;
    int yshift = 0;

    QPoint px1(rect().bottomRight() - QPoint(50,30 + yshift));

    // step I: get the approximate distance for 200px in the bottom right corner
    double u1 = px1.x();
    double v1 = px1.y();

    double u2 = px1.x() - 200;
    double v2 = v1;

    map.convertPt2M(u1,v1);
    map.convertPt2M(u2,v2);

    if(map.isLonLat())
    {
        projXY p1,p2;
        double a1,a2;
        p1.u = u1;
        p1.v = v1;
        p2.u = u2;
        p2.v = v2;
        d = distance(p1, p2, a1, a2);

    }
    else
    {
        d = u1 - u2;
        //     qDebug() << log10(d) << d << a << b;
    }

    // step II: derive the actual scale length in [m]
    a = (int)log10(d);
    b = log10(d) - a;

    if(0 <= b && b < log10(3.0f))
    {
        d = 1 * pow(10,a);
    }
    else if(log10(3.0f) < b && b < log10(5.0f))
    {
        d = 3 * pow(10,a);
    }
    else
    {
        d = 5 * pow(10,a);
    }

    //     qDebug() << "----" << d;

    // step III: convert the scale length from [m] into [px]
    projXY pt1, pt2;
    pt1.u = px1.x();
    pt1.v = px1.y();
    map.convertPt2Rad(pt1.u,pt1.v);

    if(pt1.u == px1.x() && pt1.v == px1.y())
    {
        return;
    }

    pt2 = GPS_Math_Wpt_Projection(pt1, d, -90 * DEG_TO_RAD);
    map.convertRad2Pt(pt2.u, pt2.v);

    if(isnan(pt2.u) || isnan(pt2.v) || abs(pt2.u) > 5000) return;

    // step IV: draw the scale
    QPoint px2(px1 - QPoint(px1.x() - pt2.u,0));

    p.setPen(QPen(Qt::white, 9));
    p.drawLine(px1, px2 + QPoint(9,0));
    p.setPen(QPen(Qt::black, 7));
    p.drawLine(px1, px2 + QPoint(9,0));
    p.setPen(QPen(Qt::white, 5));
    p.drawLine(px1, px2 + QPoint(9,0));

    QVector<qreal> pattern;
    pattern << 2 << 4;
    QPen pen(Qt::black, 5, Qt::CustomDashLine);
    pen.setDashPattern(pattern);
    p.setPen(pen);
    p.drawLine(px1, px2 + QPoint(9,0));

    QPoint px3(px2.x() + (px1.x() - px2.x())/2, px2.y());

    QString val, unit;
    IUnit::self().meter2distance(d,val,unit);
    drawText(QString("%1 %2").arg(val).arg(unit), p, px3, Qt::black);
}


void CCanvas::drawCompass(QPainter& p)
{
    if(!CResources::self().showNorthIndicator())
    {
        return;
    }
    QPolygon arrow;

    arrow << QPoint(0, -COMPASS_H/2) << QPoint(-COMPASS_W/2, COMPASS_H/2) << QPoint(0, COMPASS_H/3) << QPoint(COMPASS_W/2, COMPASS_H/2);

    p.save();

    p.translate(size().width() - COMPASS_OFFSET_X - COMPASS_W/2, size().height() - COMPASS_OFFSET_Y);
    p.rotate(-CMapDB::self().getMap().getAngleNorth());

    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(Qt::white,3));
    p.drawPolygon(arrow);

    p.setPen(QPen(Qt::black,1));
    p.setBrush(QColor(150,150,255,100));
    p.drawPolygon(arrow);

    drawText("N", p, QPoint(0, -COMPASS_H/2));
    drawText("S", p, QPoint(0, +COMPASS_H/2 + 15));

    p.restore();
}


void CCanvas::drawClock(QPainter& p)
{
    if(!CResources::self().showClock())
    {
        return;
    }
#ifdef QK_QT5_TZONE
    QDateTime tstamp = QDateTime::currentDateTimeUtc().toTimeZone(QTimeZone(timezone.toLatin1()));
    QString strDateTime = tstamp.toString() + " (" + timezone + ")";;
#else
    TimeStamp tstamp = TimeStamp::now().toUTC();
    QString strDateTime = tstamp.toZone(timezone).toDateTime().toString() + " (" + timezone + ")";
#endif
#ifdef WIN32
    strDateTime = " "+strDateTime+" ";
#endif

    QFontMetrics fm(CResources::self().getMapFont());

    QRect r = fm.boundingRect(strDateTime);

    r.moveBottomRight(QPoint(size().width() - 10, size().height() - 10));

    drawText(strDateTime,p,r);

}


void CCanvas::drawText(const QString& str, QPainter& p, const QPoint& center, const QColor& color)
{
    CCanvas::drawText(str,p,center, color, CResources::self().getMapFont());
}


void CCanvas::drawText(const QString& str, QPainter& p, const QPoint& center, const QColor& color, const QFont& font)
{

    QFontMetrics    fm(font);
    QRect           r = fm.boundingRect(str);

    r.moveCenter(center);

    p.setPen(Qt::white);
    p.setFont(font);

    p.drawText(r.topLeft() - QPoint(-1,-1), str);
    p.drawText(r.topLeft() - QPoint( 0,-1), str);
    p.drawText(r.topLeft() - QPoint(+1,-1), str);

    p.drawText(r.topLeft() - QPoint(-1, 0), str);
    p.drawText(r.topLeft() - QPoint(+1, 0), str);

    p.drawText(r.topLeft() - QPoint(-1,+1), str);
    p.drawText(r.topLeft() - QPoint( 0,+1), str);
    p.drawText(r.topLeft() - QPoint(+1,+1), str);

    p.setPen(color);
    p.drawText(r.topLeft(),str);

}


void CCanvas::drawText(const QString& str, QPainter& p, const QRect& r, const QColor& color)
{

    p.setPen(Qt::white);
    p.setFont(CResources::self().getMapFont());

    p.drawText(r.adjusted(-1,-1,-1,-1),Qt::AlignCenter,str);
    p.drawText(r.adjusted( 0,-1, 0,-1),Qt::AlignCenter,str);
    p.drawText(r.adjusted(+1,-1,+1,-1),Qt::AlignCenter,str);

    p.drawText(r.adjusted(-1, 0,-1, 0),Qt::AlignCenter,str);
    p.drawText(r.adjusted(+1, 0,+1, 0),Qt::AlignCenter,str);

    p.drawText(r.adjusted(-1,+1,-1,+1),Qt::AlignCenter,str);
    p.drawText(r.adjusted( 0,+1, 0,+1),Qt::AlignCenter,str);
    p.drawText(r.adjusted(+1,+1,+1,+1),Qt::AlignCenter,str);

    p.setPen(color);
    p.drawText(r,Qt::AlignCenter,str);

}


void CCanvas::wheelEvent(QWheelEvent * e)
{
    zoom(CResources::self().flipMouseWheel() ? (e->delta() > 0) : (e->delta() < 0), e->pos());
}


void CCanvas::zoom(bool in, const QPoint& p)
{
    CUndoStackView::getInstance()->push(new CCanvasUndoCommandZoom(in, p));
    update();
}


void CCanvas::move(double lon, double lat)
{
    IMap& map = CMapDB::self().getMap();
    double u = lon * DEG_TO_RAD;
    double v = lat * DEG_TO_RAD;
    map.convertRad2Pt(u,v);

    CUndoStackView::getInstance()->push(new CMapUndoCommandMove(&map,QPoint(u,v), rect().center()));
    update();
}


void CCanvas::move(move_direction_e dir)
{
    IMap& map = CMapDB::self().getMap();
    QPoint p1 = geometry().center();
    QPoint p2 = p1;

    switch(dir)
    {

        case eMoveLeft:
            p2.rx() += width() / 4;
            break;

        case eMoveRight:
            p2.rx() -= width() / 4;
            break;

        case eMoveUp:
            p2.ry() += height() / 4;
            break;

        case eMoveDown:
            p2.ry() -= height() / 4;
            break;

        case eMoveLeftSmall:
            p2.rx() += width() / 50;
            break;

        case eMoveRightSmall:
            p2.rx() -= width() / 50;
            break;

        case eMoveUpSmall:
            p2.ry() += height() / 50;
            break;

        case eMoveDownSmall:
            p2.ry() -= height() / 50;
            break;

        case eMoveCenter:
        {
            double lon1 = 0, lat1 = 0, lon2 = 0, lat2 = 0;

            map.dimensions(lon1, lat1, lon2, lat2);

            lon1 += (lon2 - lon1)/2;
            lat2 += (lat1 - lat2)/2;
            map.convertRad2Pt(lon1,lat2);

            p1.rx() = lon1;
            p1.ry() = lat2;

            p2 = geometry().center();

        }
        break;
    }
    CUndoStackView::getInstance()->push(new CMapUndoCommandMove(&map,p1, p2));

    update();
}


void CCanvas::mouseMoveEventCoord(QMouseEvent * e)
{
    IMap& map = CMapDB::self().getMap();
    QString info;                // = QString("%1 %2, ").arg(e->x()).arg(e->y());

    bool isLonLat = false;

    double x = e->x();
    double y = e->y();

    double x_m = e->x();
    double y_m = e->y();

    map.convertPt2Rad(x,y);
    CGridDB::self().convertPt2Pos(x_m, y_m, isLonLat);

    //    qDebug() << x * RAD_TO_DEG << y * RAD_TO_DEG << ">>>" << x_m << y_m;

    if((x == e->x()) && (y == e->y()))
    {
        map.convertPt2M(x,y);
        info += QString(" (%1 %2)").arg(x,0,'f',0).arg(y,0,'f',0);
        //        qDebug() << "--" << info;
    }
    else
    {
        timezone = GPS_Timezone(x*RAD_TO_DEG, y*RAD_TO_DEG);

        float ele = CMapDB::self().getDEM().getElevation(x,y);
        if(ele != WPT_NOFLOAT)
        {
            QString val, unit;
            IUnit::self().meter2elevation(ele, val, unit);
            info += QString(" (ele: %1 %2) ").arg(val).arg(unit);
        }

        if(isLonLat)
        {
            QString lat,lng;
            x_m *= RAD_TO_DEG;
            y_m *= RAD_TO_DEG;
            lat = y_m < 0 ? "S" : "N";
            lng = x_m < 0 ? "W" : "E";
            info += tr("[Grid: %1%2%5 %3%4%5] ").arg(lat).arg(qAbs(y_m), 0, 'f', 6).arg(lng).arg(qAbs(x_m), 0, 'f', 6).arg(QChar(0260));

        }
        else
        {
            info += tr("[Grid: N %1m, E %2m] ").arg(y_m,0,'f',0).arg(x_m,0,'f',0);
        }

        x *= RAD_TO_DEG;
        y *= RAD_TO_DEG;

        QString str;
        GPS_Math_Deg_To_Str(x,y, str);
        info += str + " ";
                                 //add plain degrees
        //info += QString(", %1\260 %2\260 ").arg(y,0,'f',7).arg(x,0,'f',7);
    }
    //     qDebug() << "------" << info;
    theMainWindow->setPositionInfo(info);

}


void CCanvas::raiseContextMenu(const QPoint& pos)
{
    QMenu menu(this);

    if(!CMegaMenu::self().isEnabled())
    {
        foreach(QAction *a, *theMainWindow->getActionGroupProvider()->getActiveActions())
        {
            menu.addAction(a);
        }
        menu.addSeparator();
    }
    //    menu.addAction(QIcon(":/icons/iconClipboard16x16.png"),tr("Copy Position"),this,SLOT(slotCopyPosition()));
    mouse->contextMenu(menu);

    contextMenuActive = true;
    QPoint p = mapToGlobal(pos);
    setMouseTracking(false);
    menu.exec(p);
    setMouseTracking(true);
    contextMenuActive = false;
}


void CCanvas::showEvent ( QShowEvent * event )
{
    IMap& map = CMapDB::self().getMap();
    map.resize(size());
}


void CCanvas::slotTrackChanged()
{
    CTrack * trk = CTrackDB::self().highlightedTrack();
    slotHighlightTrack(trk);
}


void CCanvas::slotPointOfFocus(const int idx)
{
    CTrack * track = CTrackDB::self().highlightedTrack();
    if(track == 0)
    {
        return;
    }

    QList<CTrack::pt_t>& trkpts = track->getTrackPoints();
    if(idx < trkpts.size())
    {
        if(idx < 0)
        {
            mouseMoveMap->setSelTrackPt(0);
            return;
        }
        mouseMoveMap->setSelTrackPt(&trkpts[idx]);

    }
}


void CCanvas::slotProfileChanged()
{
    CTrack * track = CTrackDB::self().highlightedTrack();
    if(track == 0)
    {
        return;
    }

    QList<QPointF> focusElev;
    float basefactor = IUnit::self().basefactor;

    foreach(const CTrack::pt_t trkpt, track->getTrackPoints())
    {
        if(trkpt.flags & CTrack::pt_t::eFocus)
        {
            focusElev << QPointF(trkpt.distance, trkpt.ele * basefactor);
            break;
        }
    }
    profile->newFocus(focusElev);
}


void CCanvas::slotHighlightTrack(CTrack * track)
{
    if(track && CResources::self().showTrackProfilePreview())
    {
        QPolygonF lineElev;
        QList<QPointF> focusElev;
        float basefactor = IUnit::self().basefactor;

        profile->clear();

        QList<CTrack::pt_t>& trkpts = track->getTrackPoints();
        QList<CTrack::pt_t>::const_iterator trkpt = trkpts.begin();
        while(trkpt != trkpts.end())
        {
            if(trkpt->flags & CTrack::pt_t::eDeleted)
            {
                ++trkpt; continue;
            }

            if(trkpt->ele != WPT_NOFLOAT)
            {
                lineElev << QPointF(trkpt->distance, trkpt->ele * basefactor);
            }

            if(trkpt->flags & CTrack::pt_t::eFocus)
            {
                focusElev << QPointF(trkpt->distance, trkpt->ele * basefactor);
            }

            trkpt++;
        }

        profile->newLine(lineElev,focusElev, "GPS");
        profile->setXLabel(track->getName());
        profile->setLimits();
        profile->resetZoom();
        profile->show();
        disconnect(profile, SIGNAL(sigClicked()), CTrackDB::self().getToolWidget(), SLOT(slotShowProfile()));
        connect(profile, SIGNAL(sigClicked()), CTrackDB::self().getToolWidget(), SLOT(slotShowProfile()));
        disconnect(&CTrackDB::self(),SIGNAL(sigNeedUpdate(const QString&)),this,SLOT(slotProfileChanged()));
        connect(&CTrackDB::self(),SIGNAL(sigNeedUpdate(const QString&)),this,SLOT(slotProfileChanged()));
    }
    else
    {
        profile->clear();
        profile->hide();
        disconnect(profile, SIGNAL(sigClicked()), CTrackDB::self().getToolWidget(), SLOT(slotShowProfile()));
        disconnect(&CTrackDB::self(),SIGNAL(sigNeedUpdate(const QString&)),this,SLOT(slotProfileChanged()));
    }
    update();
}


void CCanvas::slotTime()
{
    if(CResources::self().showClock())
    {
        update();
    }
}


void CCanvas::setFadingMessage(const QString& msg)
{
    fadingMessage = msg;
    update();
    timerFadingMessage->start(1000);
}


void CCanvas::slotFadingMessage()
{
    fadingMessage.clear();
    update();

}


void CCanvas::drawFadingMessage(QPainter& p)
{
    if(fadingMessage.isEmpty())
    {
        timerFadingMessage->stop();
        return;
    }

    QFont           f = CResources::self().getMapFont();
    f.setPixelSize(f.pointSize() + 10);
    QFontMetrics    fm(f);
    QRect           r1 = fm.boundingRect(QRect(0,0,300,0), Qt::AlignLeft|Qt::AlignTop|Qt::TextWordWrap, fadingMessage);
    r1.moveCenter(rect().center());

    QRect           r2 = r1;
    r2.setWidth(r1.width() + 20);
    r2.moveLeft(r1.left() - 10);
    r2.setHeight(r1.height() + 20);
    r2.moveTop(r1.top() - 10);

    p.setPen(penBorderBlue);
    p.setBrush(brushBackWhite);
    PAINT_ROUNDED_RECT(p,r2);

    p.setFont(f);
    p.setPen(Qt::darkBlue);
    p.drawText(r1, Qt::AlignLeft|Qt::AlignTop|Qt::TextWordWrap,fadingMessage);
}


void CCanvas::drawTrackLegend(QPainter& p)
{
    CTrack * track = CTrackDB::self().highlightedTrack();
    if(track == 0)
    {
        return;
    }

    QPoint pt = profile->pos();

    p.save();
    p.translate(pt.x(), pt.y() - 30 - 200);
    track->drawMultiColorLegend(p);
    p.restore();

}
