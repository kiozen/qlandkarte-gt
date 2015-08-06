/**********************************************************************************************
    Copyright (C) 2006, 2007 Oliver Eichler oliver.eichler@gmx.de

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

#include "CPlot.h"
#include "CPlotAxis.h"
#include "CResources.h"
#include "CCanvas.h"

#include "config.h"
#include "CTrackDB.h"
#include "CTrack.h"
#include "CSettings.h"

#include <QtGui>
#include <QMenu>
#include <QMessageBox>
#include <QFileDialog>

CPlot::CPlot(CPlotData::axis_type_e type, mode_e mode, QWidget * parent)
: QWidget(parent)
, fontWidth(0)
, fontHeight(0)
, scaleWidthX1(0)
, scaleWidthY1(0)
, left(0)
, right(0)
, top(0)
, bottom(0)
, fm(QFont())
, initialYMax(0)
, initialYMin(0)
, mode(mode)
, showScale(true)
, thinLine(false)
, cursorFocus(false)
, needsRedraw(true)
, mouseMoveMode(false)
, checkClick(false)
, posMouse(-1,-1)
, posWpt(-1,-1)
, selTrkPt(0)
, idxHighlight1(0)
, idxHighlight2(0)
{
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);

    if(mode == eIcon)
    {
        showScale = false;
        thinLine = true;
    }

    m_pData = new CPlotData(type, this);
    createActions();

    connect(&CTrackDB::self(), SIGNAL(sigPointOfFocus(int)), this, SLOT(slotPointOfFocus(int)));
}


CPlot::~CPlot()
{

}


void CPlot::clear()
{
    m_pData->lines.clear();
    m_pData->marks.points.clear();
    m_pData->tags.clear();
    m_pData->badData = true;
    update();
}


double CPlot::getXValByPixel(int px)
{
    return m_pData->x().pt2val(px - left);
}


double CPlot::getYValByPixel(int px)
{
    if(m_pData->lines.isEmpty())
    {
        return 0;
    }

    double xx = getXValByPixel(px);

    const QPolygonF& line = m_pData->lines[0].points;
    foreach(const QPointF& pt, line)
    {
        if(xx <= pt.x())
        {
            return pt.y();
        }

    }

    return 0;
}


void CPlot::setYLabel(const QString& str)
{
    m_pData->ylabel = str;
    setSizes();
    update();
}


void CPlot::setXLabel(const QString& str)
{
    m_pData->xlabel = str;
    setSizes();
    update();
}


void CPlot::setLimits()
{
    m_pData->setLimits();
}


void CPlot::newLine(const QPolygonF& line, const QList<QPointF>& focus, const QString& label)
{
    m_pData->lines.clear();

    QRectF r = line.boundingRect();
    if((r.height() < 0) || (r.width() < 0))
    {
        m_pData->badData = true;
        return;
    }

    CPlotData::line_t l;
    l.points    = line;
    l.label     = label;

    m_pData->badData = false;
    m_pData->lines << l;
    setSizes();
    m_pData->x().setScale( rectGraphArea.width() );
    m_pData->y().setScale( rectGraphArea.height() );

    newFocus(focus);

}

void CPlot::newFocus(const QList<QPointF>& focus)
{
    m_pData->focus = focus;

    idxHighlight1 = -1;
    idxHighlight2 = -1;

    if(m_pData->focus.size() > 1)
    {
        int idx     = 0;
        double d    = WPT_NOFLOAT;
        double x    = m_pData->focus.first().x();
        QPolygonF& line = m_pData->lines[0].points;

        foreach(const QPointF& point, line)
        {
            if(fabs(x - point.x()) < d)
            {
                d = fabs(x - point.x());
                idxHighlight1 = idx;
            }
            idx++;
        }

        idx = 0;
        d   = WPT_NOFLOAT;
        x   = m_pData->focus.last().x();
        foreach(const QPointF& point, line)
        {
            if(fabs(x - point.x()) < d)
            {
                d = fabs(x - point.x());
                idxHighlight2 = idx;
            }
            idx++;
        }
    }

    needsRedraw = true;
    update();

}

void CPlot::addLine(const QPolygonF& line, const QString& label)
{
    QRectF r = line.boundingRect();
    if(!r.isValid())
    {
        m_pData->badData = true;
        return;
    }

    CPlotData::line_t l;
    l.points    = line;
    l.label     = label;

    m_pData->badData = false;
    m_pData->lines << l;
    setSizes();
    m_pData->x().setScale( rectGraphArea.width() );
    m_pData->y().setScale( rectGraphArea.height() );

    needsRedraw = true;
    update();
}


void CPlot::newMarks(const QPolygonF& line)
{
    m_pData->marks.points = line;
}


void CPlot::addTag(CPlotData::point_t& tag)
{
    m_pData->tags << tag;
}


void CPlot::paintEvent(QPaintEvent * )
{
    QPainter p(this);
    draw(p);
}


void CPlot::resizeEvent(QResizeEvent * e)
{
    setSizes();

    initialYMin = m_pData->y().min();
    initialYMax = m_pData->y().max();

    buffer = QImage(e->size(), QImage::Format_ARGB32);

    needsRedraw = true;
    update();
}


void CPlot::setSizes()
{
    fm = QFontMetrics(CResources::self().getMapFont());
    left = 0;

    scaleWidthX1    = showScale ? m_pData->x().getScaleWidth( fm ) : 0;
    scaleWidthY1    = showScale ? m_pData->y().getScaleWidth( fm ) : 0;

    scaleWidthY1    = scaleWidthX1 > scaleWidthY1 ? scaleWidthX1 : scaleWidthY1;

    fontWidth       = fm.maxWidth();
    fontHeight      = fm.height();
    deadAreaX       = fontWidth >> 1;
    deadAreaY       = ( fontHeight + 1 ) >> 1;

    setLRTB();
    setSizeIconArea();
    setSizeXLabel();
    setSizeYLabel();
    setSizeTrackInfo();
    setSizeDrawArea();

}


void CPlot::setLRTB()
{
    left = 0;

    left += m_pData->ylabel.isEmpty() ? 0 : fontHeight;
    left += scaleWidthY1;
    left += deadAreaX;

    right = size().width();
    right -= deadAreaX;
    right -= scaleWidthX1 / 2;

    top = 0;
    if(!m_pData->tags.isEmpty())
    {
        top += fontHeight;
        top += 16;
    }
    top += deadAreaY;

    bottom = size().height();
    bottom -= m_pData->xlabel.isEmpty() ? 0 : fontHeight;
    // tick marks
    if(scaleWidthX1)
    {
        bottom -= fontHeight;
    }
    bottom -= deadAreaY;

    if(!m_pData->xlabel.isEmpty())
    {
        bottom -= deadAreaY;
    }

    if(!m_pData->tags.isEmpty() && CResources::self().showTrackProfileEleInfo())
    {
        bottom -= fontHeight;
    }
}


void CPlot::setSizeIconArea()
{
    rectIconArea = QRect(left, deadAreaY, right - left, 16 + fontHeight + deadAreaY);
}


/*
  x = a <br>
  y = widget height - xlabel height <br>
  width = b-a <br>
  height = font height <br>
*/
void CPlot::setSizeXLabel()
{
    int y;
    if ( m_pData->xlabel.isEmpty() )
    {
        rectX1Label = QRect( 0, 0, 0, 0 );
    }
    else
    {
        rectX1Label.setWidth( right - left );
        rectX1Label.setHeight( fontHeight );
        y = ( size().height() - rectX1Label.height()) - deadAreaY;
        if(!m_pData->tags.isEmpty() && CResources::self().showTrackProfileEleInfo())
        {
            y -= fontHeight;
        }

        rectX1Label.moveTopLeft( QPoint( left, y ) );
    }
}


/*
  assume a -90 rotated coordinate grid

  x = widget height - d <br>
  y = 0 <br>
  width = d-c<br>
  height = font height <br>
*/
void CPlot::setSizeYLabel()
{
    if ( m_pData->ylabel.isEmpty() )
    {
        rectY1Label = QRect( 0, 0, 0, 0 );
    }
    else
    {
        rectY1Label.setWidth( bottom - top );
        rectY1Label.setHeight( fontHeight );
        rectY1Label.moveTopLeft( QPoint( size().height() - bottom, 0 ) );
    }
}


void CPlot::setSizeTrackInfo()
{
    if(m_pData->tags.isEmpty() || !CResources::self().showTrackProfileEleInfo())
    {
        rectTrackInfo = QRect();
        return;
    }

    rectTrackInfo.setWidth(right - left);
    rectTrackInfo.setHeight(fontHeight);
    rectTrackInfo.moveLeft(left);
    rectTrackInfo.moveTop(size().height() - fontHeight);
}


void CPlot::setSizeDrawArea()
{
    rectGraphArea.setWidth( right - left );
    rectGraphArea.setHeight( bottom - top );
    rectGraphArea.moveTopLeft( QPoint( left, top ) );

    m_pData->x().setScale( rectGraphArea.width() );
    m_pData->y().setScale( rectGraphArea.height() );
}


void CPlot::draw(QPainter& p, const QSize& s)
{
    resize(s);
    QResizeEvent e(s,s);
    resizeEvent(&e);

    draw(p);
}


void CPlot::draw(QPainter& p)
{

    if(needsRedraw)
    {
        draw();
        needsRedraw = false;
    }

    p.drawImage(0,0,buffer);

    // draw track information for point under mouse
    int x = posMouse.x();
    if(x != -1)
    {
        USE_ANTI_ALIASING(p, true);
        p.setPen(QPen(Qt::red,2));
        p.drawLine(x, rectGraphArea.top(), x, rectGraphArea.bottom());

        CTrack * track = CTrackDB::self().highlightedTrack();
        if(selTrkPt && track && (mode != eIcon))
        {
            QString str;
            double y = getYValByPixel(x);
            y = m_pData->y().val2pt(y);
            y = bottom - y;

            // highlight track point
            p.setPen(CCanvas::penBorderBlue);
            p.setBrush(CCanvas::brushBackWhite);
            p.drawEllipse(QRect(x - 5,  y - 5, 11, 11));

            str = track->getTrkPtInfo1(*selTrkPt);
            if(!str.isEmpty())
            {
                QFont           f = CResources::self().getMapFont();
                QFontMetrics    fm(f);
                QRect           r1 = fm.boundingRect(QRect(0,0,300,0), Qt::AlignLeft|Qt::AlignTop|Qt::TextWordWrap, str);

                if((r1.width() + 45 + x) > right)
                {
                    x = x - 45 - r1.width();
                }
                else
                {
                    x = x + 45;
                }

                if(r1.height() + y > bottom)
                {
                    y = y - r1.height();
                }

                r1.moveTopLeft(QPoint(x,y));

                QRect r2 = r1;
                r2.setWidth(r1.width() + 20);
                r2.moveLeft(r1.left() - 10);
                r2.setHeight(r1.height() + 20);
                r2.moveTop(r1.top() - 10);

                p.setPen(QPen(CCanvas::penBorderBlue));
                p.setBrush(CCanvas::brushBackWhite);
                PAINT_ROUNDED_RECT(p,r2);

                p.setFont(CResources::self().getMapFont());
                p.setPen(Qt::darkBlue);
                p.drawText(r1, Qt::AlignLeft|Qt::AlignTop|Qt::TextWordWrap,str);
            }

            x   = posMouse.x();
            str = track->getTrkPtInfo2(*selTrkPt);
            if(!str.isEmpty() && m_pData->focus.isEmpty())
            {
                QFont           f = CResources::self().getMapFont();
                QFontMetrics    fm(f);
                QRect           r1 = fm.boundingRect(QRect(0,0,1,1), Qt::AlignLeft|Qt::AlignTop, str);
                r1.moveCenter(QPoint(x, size().height() - fontHeight/2));
                if(r1.left() < left)
                {
                    r1.moveLeft(left);
                }
                if(r1.right() > right)
                {
                    r1.moveRight(right);
                }

                CCanvas::drawText(str, p, r1);
            }
        }
    }
}


void CPlot::draw()
{
    buffer.fill(Qt::transparent);
    QPainter p(&buffer);
    USE_ANTI_ALIASING(p, true);

    if(mode == eNormal)
    {
        p.fillRect(rect(),Qt::white);
    }
    else if(mode == eIcon)
    {
        QRect r = rect();
        r.adjust(2,2,-2,-2);
        if(cursorFocus || posMouse.x() != -1)
        {
            p.setPen(CCanvas::penBorderBlue);
            p.setBrush(QColor(255,255,255,255));
        }
        else
        {
            p.setPen(CCanvas::penBorderBlack);
            p.setBrush(QColor(255,255,255,150));
        }

        PAINT_ROUNDED_RECT(p,r);

    }

    if(m_pData->lines.isEmpty() || m_pData->badData)
    {
        p.drawText(rect(), Qt::AlignCenter, tr("No or bad data."));
        return;
    }

    p.setFont(CResources::self().getMapFont());
    drawTags(p);
    p.setClipping(true);
    p.setClipRect(rectGraphArea);
    drawData(p);
    p.setClipping(false);
    drawLabels(p);
    if(showScale)
    {
        drawXScale(p);
        drawYScale(p);
    }
    drawGridX(p);
    drawGridY(p);
    drawXTic(p);
    drawYTic(p);
    p.setPen(QPen(Qt::black,2));
    p.drawRect(rectGraphArea);

    drawLegend(p);
}


void CPlot::drawLabels( QPainter &p )
{
    p.setPen(Qt::darkBlue);

    if ( rectX1Label.isValid() )
    {
        p.drawText( rectX1Label, Qt::AlignCenter, m_pData->xlabel );
    }

    p.save();
    QMatrix m = p.matrix();
    m.translate( 0, size().height() );
    m.rotate( -90 );
    p.setMatrix( m );

    if ( rectY1Label.isValid() )
    {
        p.drawText( rectY1Label, Qt::AlignCenter, m_pData->ylabel );
    }
    p.restore();
}


void CPlot::drawXScale( QPainter &p )
{
    QRect recText;

    if ( m_pData->x().getTicType() == CPlotAxis::notic )
        return ;

    p.setPen(Qt::darkBlue);
    recText.setHeight( fontHeight );
    recText.setWidth( scaleWidthX1 );

    int ix;
    int ix_ = -1;
    int iy;

    iy = bottom + deadAreaY;
    const CPlotAxis::TTic * t = m_pData->x().ticmark();
    while ( t )
    {
        ix = left + m_pData->x().val2pt( t->val ) - ( scaleWidthX1 + 1 ) / 2;
        if ( ( ( ix_ < 0 ) || ( ( ix - ix_ ) > scaleWidthX1 + 5 ) ) && !t->lbl.isEmpty() )
        {
            recText.moveTopLeft( QPoint( ix, iy ) );
            p.drawText( recText, Qt::AlignCenter, t->lbl );
            ix_ = ix;
        }
        t = m_pData->x().ticmark( t );
    }

    double limMin, limMax, useMin, useMax;
    m_pData->x().getLimits(limMin, limMax, useMin, useMax);

    if((limMax - limMin) <= (useMax - useMin)) return;

    double scale = (right - left) / (limMax - limMin);
    //     double val   = m_pData->x().pt2val(0);

    int x = left + (useMin - limMin) * scale;
    int y = bottom + 5;
    int w = (useMax - useMin) * scale;

    p.setPen(QPen(Qt::red,3));
    p.drawLine(x,y, x + w, y);

}


void CPlot::drawYScale( QPainter &p )
{
    QString format_single_prec;
    QRect recText;
    if ( m_pData->y().getTicType() == CPlotAxis::notic )
        return ;

    p.setPen(Qt::darkBlue);
    recText.setHeight( fontHeight );
    recText.setWidth( scaleWidthY1 );

    int ix;
    int iy;

    ix = left - scaleWidthY1 - deadAreaX;

    double limMin, limMax, useMin, useMax;
    m_pData->y().getLimits(limMin, limMax, useMin, useMax);

    // draw min/max lables 1st;
    QRect recTextMin;
    QRect recTextMax;

    format_single_prec = m_pData->y().fmtsgl(m_pData->ymin);
    if(m_pData->ymin >= useMin)
    {
        iy = bottom - m_pData->y().val2pt( m_pData->ymin ) - fontHeight / 2;
        recText.moveTopLeft( QPoint( ix, iy ) );
        p.drawText( recText, Qt::AlignRight, QString().sprintf( format_single_prec.toLatin1().data(), m_pData->ymin  ));
        recTextMin = recText;
    }
    format_single_prec = m_pData->y().fmtsgl(m_pData->ymax);
    if(m_pData->ymax <= useMax)
    {
        iy = bottom - m_pData->y().val2pt( m_pData->ymax ) - fontHeight / 2;
        recText.moveTopLeft( QPoint( ix, iy ) );
        p.drawText( recText, Qt::AlignRight, QString().sprintf( format_single_prec.toLatin1().data(), m_pData->ymax  ));
        recTextMax = recText;
    }

    // draw tic marks
    const CPlotAxis::TTic * t = m_pData->y().ticmark();
    while ( t )
    {
        iy = bottom - m_pData->y().val2pt( t->val ) - fontHeight / 2;

        recText.moveTopLeft( QPoint( ix, iy ) );

        if(!recTextMin.intersects(recText) && !recTextMax.intersects(recText))
        {
            p.drawText( recText, Qt::AlignRight, t->lbl );
        }

        t = m_pData->y().ticmark( t );
    }

    if((limMax - limMin) <= (useMax - useMin)) return;

    double scale = (top - bottom) / (limMax - limMin);
    //     double val   = m_pData->y().pt2val(0);

    int x = left - 5;
    int y = bottom + (useMin - limMin) * scale;
    int h = (useMax - useMin) * scale;

    p.setPen(QPen(Qt::red,3));
    p.drawLine(x,y, x, y + h);

}


void CPlot::drawXTic( QPainter & p )
{
    int ix;
    int iyb, iyt;
    const CPlotAxis::TTic * t = m_pData->x().ticmark();

    p.setPen(QPen(Qt::black,2));
    iyb = rectGraphArea.bottom();
    iyt = rectGraphArea.top();
    while ( t )
    {
        ix = left + m_pData->x().val2pt( t->val );
        p.drawLine( ix, iyb, ix, iyb - 5 );
        p.drawLine( ix, iyt, ix, iyt + 5 );
        t = m_pData->x().ticmark( t );
    }
}


void CPlot::drawYTic( QPainter &p )
{
    int ixl, ixr;
    int iy;
    const CPlotAxis::TTic * t = m_pData->y().ticmark();

    p.setPen(QPen(Qt::black,2));
    ixl = rectGraphArea.left();
    ixr = rectGraphArea.right();
    while ( t )
    {
        iy = bottom - m_pData->y().val2pt( t->val );
        p.drawLine( ixl, iy, ixl + 5, iy );
        p.drawLine( ixr, iy, ixr - 5, iy );
        t = m_pData->y().ticmark( t );
    }
}


void CPlot::drawGridX( QPainter &p )
{
    int ix;
    int iy, dy;

    CPlotAxis::ETicType oldtic = m_pData->x().setTicType( CPlotAxis::norm );

    dy = rectGraphArea.height();
    const CPlotAxis::TTic * t = m_pData->x().ticmark();

    QPen oldpen = p.pen();
    p.setPen( QPen( QColor(0,150,0,128), 1, Qt::DotLine ) );

    iy = rectGraphArea.top();
    while ( t )
    {
        ix = left + m_pData->x().val2pt( t->val );
        p.drawLine( ix, iy, ix, iy + dy );
        t = m_pData->x().ticmark( t );
    }
    p.setPen( oldpen );
    m_pData->x().setTicType( oldtic );
}


void CPlot::drawGridY( QPainter &p )
{
    int ix, dx;
    int iy;

    CPlotAxis::ETicType oldtic = m_pData->y().setTicType( CPlotAxis::norm );
    dx = rectGraphArea.width();
    const CPlotAxis::TTic * t = m_pData->y().ticmark();

    QPen oldpen = p.pen();
    p.setPen( QPen( QColor(0,150,0,128), 1, Qt::DotLine ) );

    ix = rectGraphArea.left();
    while ( t )
    {
        iy = bottom - m_pData->y().val2pt( t->val );
        p.drawLine( ix, iy, ix + dx, iy );
        t = m_pData->y().ticmark( t );
    }

    // draw min/max lines
    double limMin, limMax, useMin, useMax;
    m_pData->y().getLimits(limMin, limMax, useMin, useMax);

    if(m_pData->ymin > useMin)
    {
        iy = bottom - m_pData->y().val2pt( m_pData->ymin );
        p.drawLine( ix, iy, ix + dx, iy );
    }
    if(m_pData->ymax < useMax)
    {
        iy = bottom - m_pData->y().val2pt( m_pData->ymax );
        p.drawLine( ix, iy, ix + dx, iy );
    }

    p.setPen( oldpen );
    m_pData->y().setTicType( oldtic );
}


QPen pens[] =
{
    QPen(Qt::blue,4)
    , QPen(Qt::red,2)
    , QPen(Qt::darkYellow,2)
    , QPen(Qt::darkGreen,2)

};

QPen pensThin[] =
{
    QPen(Qt::blue,2)
    , QPen(Qt::red,1)
    , QPen(Qt::darkYellow,1)
    , QPen(Qt::darkGreen,1)

};

QColor colors[] =
{
    QColor(0,0,255)
    , QColor(0,0,0,0)
    , QColor(0,0,0,0)
    , QColor(0,0,0,0)

};

void CPlot::drawData(QPainter& p)
{
    int penIdx = 0;
    int ptx, pty, oldPtx;
    QList<CPlotData::line_t> lines                  = m_pData->lines;
    QList<CPlotData::line_t>::const_iterator line   = lines.begin();

    CPlotAxis& xaxis = m_pData->x();
    CPlotAxis& yaxis = m_pData->y();

    while(line != lines.end())
    {
        QPolygonF background;
        QPolygonF foreground;

        const QPolygonF& polyline       = line->points;
        QPolygonF::const_iterator point = polyline.begin();

        ptx = left   + xaxis.val2pt( point->x() );
        pty = bottom - yaxis.val2pt( point->y() );
        oldPtx = ptx;

        background << QPointF(left,bottom);
        background << QPointF(left,pty);
        background << QPointF(ptx,pty);

        while(point != polyline.end())
        {
            ptx = left   + xaxis.val2pt( point->x() );
            pty = bottom - yaxis.val2pt( point->y() );

            if(oldPtx == ptx)
            {
                ++point;
                continue;
            }
            oldPtx = ptx;

            if(ptx >= left && ptx <= right)
            {
                background << QPointF(ptx,pty);
                foreground << QPointF(ptx,pty);
            }
            ++point;
        }

        background << QPointF(right,pty);
        background << QPointF(right,bottom);

        QLinearGradient gradient(0, bottom - yaxis.val2pt(initialYMin), 0, bottom - yaxis.val2pt(initialYMax));
        gradient.setColorAt(0, colors[penIdx]);
        gradient.setColorAt(1, QColor(0,0,0,0));
        p.setPen(Qt::NoPen);
        p.setBrush(gradient);
        p.drawPolygon(background);

        p.setPen(thinLine ? pensThin[penIdx++] : pens[penIdx++]);
        p.setBrush(Qt::NoBrush);
        p.drawPolyline(foreground);

        ++line;
    }

    if((idxHighlight1 >= 0) && (idxHighlight2 >= 0) && mode != eIcon)
    {
        QPolygonF background;
        QPolygonF line              = lines[0].points.mid(idxHighlight1, idxHighlight2 - idxHighlight1 + 1);
        QPolygonF::iterator point   = line.begin();

        ptx = left   + xaxis.val2pt( point->x() );
        pty = bottom - yaxis.val2pt( point->y() );

        background << QPointF(ptx,bottom);

        while(point != line.end())
        {
            ptx = left   + xaxis.val2pt( point->x() );
            pty = bottom - yaxis.val2pt( point->y() );

            if(ptx >= left && ptx <= right)
            {
                background << QPointF(ptx,pty);
            }
            ++point;
        }

        background << QPointF(ptx,bottom);

        p.save();
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255,0,0,150));
        p.drawPolygon(background);

        p.setPen(QPen(Qt::darkRed,5));
        p.drawPolyline(background.mid(1,line.size()));
        p.restore();

    }

    if(m_pData->focus.size() < 2)
    {
        QPolygonF& marks                = m_pData->marks.points;
        QPolygonF::const_iterator point = marks.begin();
        p.setPen(QPen(Qt::darkRed,2));

        while(point != marks.end())
        {
            ptx = left   + xaxis.val2pt( point->x() );
            pty = bottom - yaxis.val2pt( point->y() );

            p.drawLine(ptx-2,pty,ptx+2,pty);
            p.drawLine(ptx,pty-2,ptx,pty+2);

            ++point;
        }
    }
    else if(m_pData->focus.size() > 1 && mode != eIcon)
    {
        CTrack * trk = CTrackDB::self().highlightedTrack();
        if(trk != 0)
        {
            QString str = trk->getFocusInfo();
            if (str != "")
            {
                QPointF&        focus = m_pData->focus.last();
                QFont           f = CResources::self().getMapFont();
                QFontMetrics    fm(f);
                QRect           r1 = fm.boundingRect(QRect(0,0,300,0), Qt::AlignLeft|Qt::AlignTop, str);

                ptx = left   + xaxis.val2pt( focus.x() );
                pty = bottom - yaxis.val2pt( focus.y() );

                if((r1.width() + 15 + ptx) > right)
                {
                    ptx = ptx - 15 - r1.width();
                }
                else
                {
                    ptx = ptx + 15;
                }

                if(r1.height() + pty > bottom)
                {
                    pty = pty - r1.height();
                }

                r1.moveTopLeft(QPoint(ptx, pty));

                QRect r2 = r1;
                r2.setWidth(r1.width() + 20);
                r2.moveLeft(r1.left() - 10);
                r2.setHeight(r1.height() + 10);
                r2.moveTop(r1.top() - 5);

                p.save();
                p.setPen(QPen(CCanvas::penBorderBlue));
                p.setBrush(CCanvas::brushBackWhite);
                PAINT_ROUNDED_RECT(p,r2);

                p.setFont(CResources::self().getMapFont());
                p.setPen(Qt::darkBlue);
                p.drawText(r1, Qt::AlignLeft|Qt::AlignTop,str);
                p.restore();
            }
        }
    }

    foreach(const QPointF& point, m_pData->focus)
    {
        p.setPen(QPen(Qt::red,2));
        ptx = left   + xaxis.val2pt( point.x() );
        p.drawLine(ptx,rectGraphArea.top(),ptx,rectGraphArea.bottom());
    }
}


void CPlot::drawLegend(QPainter& p)
{
    if(m_pData->lines.size() < 2) return;

    int penIdx = 0;
    QFontMetrics fm(p.font());
    int h = fm.height();

    int x = rectGraphArea.left() + 10;
    int y = rectGraphArea.top()  + 2 + h;

    QList<CPlotData::line_t> lines                  = m_pData->lines;
    QList<CPlotData::line_t>::const_iterator line   = lines.begin();

    while(line != lines.end())
    {
        p.setPen(Qt::black);
        p.drawText(x + 30 ,y,line->label);
        p.setPen(pens[penIdx++]);
        p.drawLine(x, y, x + 20, y);

        y += fm.height();
        ++line;
    }

}


void CPlot::drawTags(QPainter& p)
{
    if(m_pData->tags.isEmpty() || !CResources::self().showTrackProfileEleInfo()) return;

    QRect rect;
    int ptx, pty;
    CPlotAxis& xaxis = m_pData->x();
    CPlotAxis& yaxis = m_pData->y();

    QFontMetrics fm(p.font());
    QColor textColor = CResources::self().wptTextColor();

    QVector<CPlotData::point_t>::const_iterator tag = m_pData->tags.begin();
    while(tag != m_pData->tags.end())
    {
        ptx = left   + xaxis.val2pt( tag->point.x() );
        pty = bottom - yaxis.val2pt( tag->point.y() );

        if (left < ptx &&  ptx < right)
        {
            rect = fm.boundingRect(tag->label);
            rect.moveCenter(QPoint(ptx, fontHeight / 2));
            rect.adjust(-1,-1,1,1);

            p.setPen(textColor);
            p.drawText(rect, Qt::AlignCenter, tag->label);

            QPixmap icon = tag->icon.scaled(15,15, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            p.drawPixmap(ptx - icon.width() / 2, fontHeight, icon);

            p.setPen(QPen(Qt::white, 3));
            if (fontHeight + 16 < pty)
            {
                if (pty > bottom)
                {
                    pty = bottom;
                }

                p.drawLine(ptx, fontHeight + 16, ptx, pty);
                p.setPen(QPen(Qt::black, 1));
                p.drawLine(ptx, fontHeight + 16, ptx, pty);
            }
        }
        ++tag;
    }
}


void CPlot::contextMenuEvent(QContextMenuEvent *event)
{
    CTrack * track = CTrackDB::self().highlightedTrack();
    if(mode != eNormal || !track)
    {
        return ;
    }

    QMenu menu(this);
    menu.addAction(hZoomAct);
    menu.addAction(vZoomAct);
    menu.addAction(resetZoomAct);
    menu.addAction(save);

    posWpt = posMouse;
    if(posMouse.x() != -1 )
    {
        menu.addAction(addWpt);
    }

    menu.exec(event->globalPos());

    posWpt = QPoint(-1,-1);
}


void CPlot::createActions()
{
    hZoomAct = new QAction("Horizontal zoom", this);
    hZoomAct->setCheckable(true);
    hZoomAct->setChecked(true);

    vZoomAct = new QAction(tr("Vertical zoom"), this);
    vZoomAct->setCheckable(true);
    vZoomAct->setChecked(true);

    resetZoomAct = new QAction(tr("Reset zoom"), this);
    connect(resetZoomAct, SIGNAL(triggered()), this, SLOT(resetZoom()));

    save = new QAction(tr("Save..."), this);
    connect(save, SIGNAL(triggered()), this, SLOT(slotSave()));

    addWpt = new QAction(tr("Add Waypoint..."), this);
    connect(addWpt, SIGNAL(triggered()), this, SLOT(slotAddWpt()));

}


void CPlot::resetZoom()
{
    m_pData->x().resetZoom();
    m_pData->y().resetZoom();
    setSizes();

    initialYMin = m_pData->y().min();
    initialYMax = m_pData->y().max();

    needsRedraw = true;
    update();
}


void CPlot::slotSave()
{

    SETTINGS;
    QString pathData = cfg.value("path/data","./").toString();
    QString filter   = cfg.value("trackstat/imagetype","Bitmap (*.png)").toString();

    QString filename = QFileDialog::getSaveFileName( 0, tr("Select output file")
        ,pathData
        ,"Bitmap (*.png)"
        ,&filter
        , FILE_DIALOG_FLAGS
        );

    if(filename.isEmpty()) return;

    QFileInfo fi(filename);
    if(fi.suffix().toLower() != "png")
    {
        filename += ".png";
    }

    QImage img(size(), QImage::Format_ARGB32);
    QPainter p;
    p.begin(&img);
    p.fillRect(rect(), QBrush(Qt::white));
    draw(p);
    p.end();

    img.save(filename);

    pathData = fi.absolutePath();
    cfg.setValue("path/data", pathData);
    cfg.setValue("trackstat/imagetype", filter);

}


void CPlot::slotAddWpt()
{
    double x = getXValByPixel(posWpt.x());
    emit sigSetWaypoint(x);
}


void CPlot::zoom(CPlotAxis &axis, bool in, int curInt)
{
    axis.zoom(in, curInt);
    setSizes();
    m_pData->x().setScale( rectGraphArea.width() );
    m_pData->y().setScale( rectGraphArea.height() );

    needsRedraw = true;
    update();
}


void CPlot::wheelEvent ( QWheelEvent * e )
{
    bool in = CResources::self().flipMouseWheel() ? (e->delta() > 0) : (e->delta() < 0);

    needsRedraw = true;

    if (hZoomAct->isChecked())
    {
        zoom(m_pData->x(), in, e->pos().x() - left);
    }
    if (vZoomAct->isChecked())
    {
        zoom(m_pData->y(), in, - e->pos().y() + bottom);
    }
}


void CPlot::mouseMoveEvent(QMouseEvent * e)
{
    posMouse = QPoint(-1,-1);

    if(checkClick)
    {
        checkClick      = false;
        mouseMoveMode   = true;
    }

    if(mouseMoveMode)
    {
        needsRedraw = true;

        CPlotAxis &xaxis = m_pData->x();
        xaxis.move(startMovePos.x() - e->pos().x());

        CPlotAxis &yaxis = m_pData->y();
        yaxis.move(e->pos().y() - startMovePos.y());

        startMovePos = e->pos();
    }
    else
    {
        CTrack * trk = CTrackDB::self().highlightedTrack();
        if(trk == 0)
        {
            return;
        }

        QPoint pos = e->pos();

        double min,max;
        if(m_pData->axisType == CPlotData::eLinear)
        {
            min = 0;
            max = trk->getTotalDistance();
        }
        else
        {
            min = trk->getStartTimestamp().toTime_t();
            max = trk->getEndTimestamp().toTime_t();
        }

        double x = getXValByPixel(pos.x());

        if(x < min || x > max)
        {
            return;
        }

        posMouse = e->pos();

        selTrkPt = 0;
        if(m_pData->axisType == CPlotData::eLinear)
        {
            CTrackDB::self().setPointOfFocusByDist(x);
        }
        else
        {
            CTrackDB::self().setPointOfFocusByTime((quint32)x);
        }
    }
    update();
}


void CPlot::mouseReleaseEvent(QMouseEvent * e)
{
    needsRedraw = true;

    if(mode == eNormal)
    {
        if (e->button() == Qt::LeftButton)
        {
            QApplication::restoreOverrideCursor();
        }
        if (checkClick && e->button() == Qt::LeftButton && !mouseMoveMode)
        {
            checkClick = false;

            QPoint pos = e->pos();
            double dist = getXValByPixel(pos.x());
            emit sigActivePoint(dist);

            update();
        }
    }
    else
    {
        CTrackDB::self().setPointOfFocusByIdx(-1);
        emit sigClicked();
    }

    mouseMoveMode = false;
}


void CPlot::mousePressEvent(QMouseEvent * e)
{
    if(mode == eNormal)
    {
        if (e->button() == Qt::LeftButton)
        {
            QApplication::setOverrideCursor(QCursor(QPixmap(":/cursors/cursorMove.png")));
            startMovePos = e->pos();
            checkClick = true;
        }
    }

}


void CPlot::leaveEvent(QEvent * event)
{
    cursorFocus = false;
    needsRedraw = true;
    posMouse    = QPoint(-1, -1);

    CTrackDB::self().setPointOfFocusByIdx(-1);
    QApplication::restoreOverrideCursor();

    update();
}


void CPlot::enterEvent(QEvent * event)
{
    cursorFocus = true;
    needsRedraw = true;
    QApplication::setOverrideCursor(Qt::PointingHandCursor);
    update();
}


void CPlot::slotTrkPt(CTrack::pt_t * pt)
{
    if(pt == 0)
    {
        posMouse = QPoint(-1,-1);
    }
    else
    {
        if(m_pData->axisType == CPlotData::eLinear)
        {
            int x       = m_pData->x().val2pt(pt->distance);
            int y       = m_pData->y().val2pt(pt->altitude);
            posMouse    = QPoint(x + left, y);
        }
        else
        {
            int x       = m_pData->x().val2pt(pt->timestamp);
            int y       = m_pData->y().val2pt(pt->distance);
            posMouse    = QPoint(x + left, y);
        }
    }
    needsRedraw = true;
    update();
}


void CPlot::slotPointOfFocus(const int idx)
{
    selTrkPt = 0;

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
            slotTrkPt(0);
        }
        else
        {
            selTrkPt = &trkpts[idx];
            slotTrkPt(selTrkPt);
        }

    }
}


void CPlot::slotHighlightSection(double x1, double x2)
{
    int idx = 0;
    // no stages if focus is active
    if(!m_pData || !m_pData->focus.isEmpty())
    {
        return;
    }

    idxHighlight1 = -1;
    idxHighlight2 = -1;

    if(m_pData->lines.isEmpty())
    {
        return;
    }

    if(x1 == WPT_NOFLOAT || x2 == WPT_NOFLOAT)
    {
        needsRedraw = true;
        update();
        return;
    }

    QPolygonF& line = m_pData->lines[0].points;
    foreach(const QPointF& point, line)
    {
        if(idxHighlight1 < 0 && x1 <= point.x())
        {
            idxHighlight1 = idx;
        }

        if(idxHighlight2 < 0 && x2 <= point.x())
        {
            idxHighlight2 = idx;
        }

        idx++;
    }

    needsRedraw = true;
    update();
}
