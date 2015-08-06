/**********************************************************************************************
    Copyright (C) 2009 Oliver Eichler oliver.eichler@gmx.de

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

#include "CRoute.h"
#include "CRouteDB.h"
#include "CRouteToolWidget.h"
#include "CMapDB.h"
#include "IMap.h"
#include "CQlb.h"
#include "CGpx.h"
#include "CResources.h"
#include "IDevice.h"
#include "GeoMath.h"

#include <QtGui>

CRouteDB * CRouteDB::m_self = 0;

bool CRouteDB::keyLessThanAlpha(keys_t&  s1, keys_t&  s2)
{
    return s1.name.toLower() < s2.name.toLower();
}


bool CRouteDB::keyLessThanTime(keys_t&  s1, keys_t&  s2)
{
    return s1.time < s2.time;
}


CRouteDB::CRouteDB(QTabWidget * tb, QObject * parent)
: IDB(IDB::eTypeRte, tb, parent)
, cnt(0)
{
    m_self      = this;
    toolview    = new CRouteToolWidget(tb);
}


CRouteDB::~CRouteDB()
{

}


void CRouteDB::addRoute(CRoute * route, bool silent)
{
    if(route->getName().isEmpty())
    {
        route->setName(tr("Route%1").arg(cnt++));
    }

    delRoute(route->getKey(), silent);
    routes[route->getKey()] = route;

    connect(route,SIGNAL(sigChanged()),this, SLOT(slotModified()));
    if(!silent)
    {
        emitSigChanged();
    }

}


void CRouteDB::delRoute(const QString& key, bool silent)
{
    if(!routes.contains(key)) return;
    CRoute * rte = routes.take(key);
    rte->deleteLater();
    if(!silent)
    {
        emitSigChanged();
    }
}


void CRouteDB::delRoutes(const QStringList& keys)
{
    QString key;
    foreach(key,keys)
    {
        delRoute(key, true);
    }
    if(!keys.isEmpty())
    {
        emitSigChanged();
    }
}


CRoute * CRouteDB::getRouteByKey(const QString& key)
{
    if(routes.contains(key))
    {
        return routes[key];
    }
    else
    {
        return 0;
    }
}


void CRouteDB::highlightRoute(const QString& key)
{
    QMap<QString,CRoute*>::iterator route = routes.begin();
    while(route != routes.end())
    {
        (*route)->setHighlight(false);
        ++route;
    }

    if(routes.contains(key))
    {
        routes[key]->setHighlight(true);
        emit sigHighlightRoute(routes[key]);
        emitSigModified(key);

    }
    else
    {
        emit sigHighlightRoute(0);
    }

}


CRoute* CRouteDB::highlightedRoute()
{

    QMap<QString,CRoute*>::iterator route = routes.begin();
    while(route != routes.end())
    {
        if((*route)->isHighlighted()) return *route;
        ++route;
    }
    return 0;

}


QRectF CRouteDB::getBoundingRectF(const QString key)
{
    if(!routes.contains(key))
    {
        return QRectF();
    }
    return routes.value(key)->getBoundingRectF();
}


/// load database data from gpx
void CRouteDB::loadGPX(CGpx& gpx)
{
    bool hasItems = false;
    const QDomNodeList& rtes = gpx.elementsByTagName("rte");
    uint N = rtes.count();
    for(uint n = 0; n < N; ++n)
    {
        hasItems = true;
        QDomElement tmpelem;
        const QDomNode& rte = rtes.item(n);

        CRoute * r = 0;
        /* name is not a required element. */
        if(rte.namedItem("name").isElement())
        {
            r = new CRoute(this);
            r->setName(rte.namedItem("name").toElement().text());
        }
        else
        {
            /* Use desc if name is unavailable, else give it no name. */
            if (rte.namedItem("desc").isElement())
            {
                r = new CRoute(this);
                r->setName(rte.namedItem("desc").toElement().text());
            }
            else
            {
                r = new CRoute(this);
                r->setName(tr("Unnamed"));
            }
        }

        tmpelem = rte.namedItem("extensions").toElement();
        if(!tmpelem.isNull())
        {
            QMap<QString,QDomElement> extensionsmap = CGpx::mapChildElements(tmpelem);

            tmpelem = extensionsmap.value(CGpx::ql_ns + ":" + "key");
            if(!tmpelem.isNull())
            {
                r->setKey(tmpelem.text());
            }

        }

        if(rte.namedItem("parent").isElement())
        {
            r->setParentWpt(rte.namedItem("parent").toElement().text());
        }

        QDomElement rtept = rte.firstChildElement("rtept");

        while (!rtept.isNull())
        {
            QString name;
            projXY pt;
            QDomNamedNodeMap attr = rtept.attributes();

            if(rtept.namedItem("name").isElement())
            {
                name = rtept.namedItem("name").toElement().text();
            }

            pt.u = attr.namedItem("lon").nodeValue().toDouble();
            pt.v = attr.namedItem("lat").nodeValue().toDouble();

            r->addPosition(pt.u,pt.v, name);

            if(rtept.namedItem("sym").isElement())
            {
                QString symname = rtept.namedItem("sym").toElement().text();
                r->setIcon(symname);
            }

            rtept = rtept.nextSiblingElement("rtept");
        }

        if(routes.contains(r->getKey()))
        {
            delete routes.take(r->getKey());
        }

        r->calcDistance();
        routes[r->getKey()] = r;

        connect(r,SIGNAL(sigChanged()),SIGNAL(sigChanged()));

    }

    if(hasItems)
    {
        emitSigChanged();
    }
}


/// save database data to gpx
void CRouteDB::saveGPX(CGpx& gpx, const QStringList& keys)
{
    QDomElement root = gpx.documentElement();
    QMap<QString,CRoute*>::iterator route = routes.begin();
    while(route != routes.end())
    {
        if(!keys.isEmpty() && !keys.contains((*route)->getKey()))
        {
            ++route;
            continue;
        }

        QDomElement rte = gpx.createElement("rte");
        root.appendChild(rte);

        QDomElement name = gpx.createElement("name");
        rte.appendChild(name);
        QDomText _name_ = gpx.createTextNode((*route)->getName());
        name.appendChild(_name_);

        QDomElement extensions = gpx.createElement("extensions");
        rte.appendChild(extensions);

        QDomElement qlkey = gpx.createElement("ql:key");
        extensions.appendChild(qlkey);
        QDomText _qlkey_ = gpx.createTextNode((*route)->getKey());
        qlkey.appendChild(_qlkey_);

        if(!(*route)->getParentWpt().isEmpty())
        {
            QDomElement parent = gpx.createElement("parent");
            parent.setAttribute("xmlns", "http://opencachemanage.sourceforge.net/schema1");
            rte.appendChild(parent);
            QDomText _parent_ = gpx.createTextNode((*route)->getParentWpt());
            parent.appendChild(_parent_);
        }

        unsigned cnt = 0;
        QVector<CRoute::pt_t> rtepts;
        if(gpx.getExportMode() == CGpx::eQlgtExport)
        {
            rtepts = (*route)->getPriRtePoints();
        }
        else
        {
            rtepts = (*route)->getSecRtePoints().isEmpty() ? (*route)->getPriRtePoints() : (*route)->getSecRtePoints();
        }
        QVector<CRoute::pt_t>::const_iterator rtept = rtepts.begin();
        while(rtept != rtepts.end())
        {
            if((gpx.getExportMode() != CGpx::eQlgtExport) && rtept->action.isEmpty())
            {
                rtept++;
                continue;
            }

            QDomElement gpxRtept = gpx.createElement("rtept");
            rte.appendChild(gpxRtept);

            gpxRtept.setAttribute("lat",QString::number(rtept->lat,'f',6));
            gpxRtept.setAttribute("lon",QString::number(rtept->lon,'f',6));

            QString str     = QString("%1").arg(++cnt,3,10,QChar('0'));

            QDomElement name = gpx.createElement("name");
            gpxRtept.appendChild(name);
            QDomText _name_ = gpx.createTextNode(str);
            name.appendChild(_name_);

            QString action = rtept->action;
            IItem::removeHtml(action);

            QDomElement cmt = gpx.createElement("cmt");
            gpxRtept.appendChild(cmt);
            QDomText _cmt_ = gpx.createTextNode(action);
            cmt.appendChild(_cmt_);

            QDomElement desc = gpx.createElement("desc");
            gpxRtept.appendChild(desc);
            QDomText _desc_ = gpx.createTextNode(action);
            desc.appendChild(_desc_);

            QDomElement sym = gpx.createElement("sym");
            gpxRtept.appendChild(sym);
            QDomText _sym_ = gpx.createTextNode((*route)->getIconString());
            sym.appendChild(_sym_);

            ++rtept;
        }

        ++route;
    }
}


/// load database data from QLandkarte binary
void CRouteDB::loadQLB(CQlb& qlb, bool newKey)
{
    QDataStream stream(&qlb.routes(),QIODevice::ReadOnly);
    stream.setVersion(QDataStream::Qt_4_5);

    while(!stream.atEnd())
    {
        CRoute * route = new CRoute(this);
        stream >> *route;
        if(newKey)
        {
            route->setKey(route->getKey() + QString("%1").arg(QDateTime::currentDateTime().toTime_t()));
        }
        route->calcDistance();
        addRoute(route, true);
    }

    if(qlb.routes().size())
    {
        emitSigChanged();
    }
}


/// save database data to QLandkarte binary
void CRouteDB::saveQLB(CQlb& qlb)
{
    QMap<QString, CRoute*>::const_iterator route = routes.begin();
    while(route != routes.end())
    {
        qlb << *(*route);
        ++route;
    }
}


void CRouteDB::upload(const QStringList& keys)
{
    if(routes.isEmpty()) return;

    IDevice * dev = CResources::self().device();
    if(dev)
    {
        QList<CRoute*> tmprtes;

        if(keys.isEmpty())
        {
            tmprtes = routes.values();
        }
        else
        {
            QString key;
            foreach(key, keys)
            {
                tmprtes << routes[key];
            }
        }
        dev->uploadRoutes(tmprtes);
    }
}


void CRouteDB::download()
{
    IDevice * dev = CResources::self().device();
    if(dev)
    {
        QList<CRoute*> tmprtes;
        dev->downloadRoutes(tmprtes);

        if(tmprtes.isEmpty()) return;

        CRoute * rte;
        foreach(rte,tmprtes)
        {
            addRoute(rte, true);
        }
    }

    emitSigChanged();
}


void CRouteDB::clear()
{
    if(routes.isEmpty()) return;
    cnt = 0;
    delRoutes(routes.keys());
    emitSigChanged();

}


void CRouteDB::draw(QPainter& p, const QRect& rect, bool& needsRedraw)
{

    IMap& map = CMapDB::self().getMap();

    // extended vieport rectangle to cut line segments properly
    QRect extRect = rect.adjusted(-10, -10, 10, 10);

    QMap<QString,CRoute*>::iterator route       = routes.begin();
    QMap<QString,CRoute*>::iterator highlighted = routes.end();

    while(route != routes.end())
    {
        QPolygon& line      = (*route)->getPolyline();
        QPolygon& points    = (*route)->getPoints();
        bool firstTime      = (*route)->firstTime;

        if ( needsRedraw || firstTime)
        {
            bool isPriRoute = (*route)->secRoute.isEmpty();

            line.clear();
            points.clear();
            QVector<CRoute::pt_t>& rtepts = isPriRoute ? (*route)->getPriRtePoints() : (*route)->getSecRtePoints();
            QVector<CRoute::pt_t>::iterator rtept = rtepts.begin();
            while(rtept != rtepts.end())
            {
                double u = rtept->lon * DEG_TO_RAD;
                double v = rtept->lat * DEG_TO_RAD;

                map.convertRad2Pt(u,v);
                line << QPoint(u,v);
                if(!rtept->action.isEmpty() || isPriRoute)
                {
                    points << QPoint(u,v);
                }

                ++rtept;
            }

        }

        if(!rect.intersects(line.boundingRect()))
        {
            ++route; continue;
        }

        if((*route)->isHighlighted())
        {
            // store highlighted route to draw it later
            // it must be drawn above all other routes
            highlighted = route;
        }
        else
        {

            // draw normal route
            QPen pen(QColor(192,0,192,128),5);
            pen.setCapStyle(Qt::RoundCap);
            pen.setJoinStyle(Qt::RoundJoin);
            p.setPen(pen);
            drawLine(line, extRect, p);
            p.setPen(Qt::white);
            drawLine(line, extRect, p);

            drawArrows(line, rect, p);

        }

        (*route)->firstTime = false;
        ++route;
    }

    // if there is a highlighted route, draw it
    if(highlighted != routes.end())
    {
        route = highlighted;

        QPolygon& line = (*route)->getPolyline();
        QPolygon& points = (*route)->getPoints();

        // draw skunk line
        QPen pen(QColor(255,0,255,128),19);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        p.setPen(pen);
        drawLine(line, extRect, p);
        p.setPen(Qt::white);
        drawLine(line, extRect, p);

        // draw bubbles
        QPoint pt;
        QPixmap bullet = (*route)->getIcon();
        foreach(pt,points)
        {
            p.drawPixmap(pt.x() - 8 ,pt.y() - 8, bullet);
        }

        drawArrows(line, rect, p);
    }

}


void CRouteDB::drawArrows(const QPolygon& line, const QRect& viewport, QPainter& p)
{
    QPointF arrow[4] =
    {
        QPointF( 20.0, 7.0),     //front
        QPointF( 0.0, 0.0),      //upper tail
        QPointF( 5.0, 7.0),      //mid tail
        QPointF( 0.0, 15.0)      //lower tail
    };

    QPoint  pt, pt1, ptt;

    // draw direction arrows
    bool    start = true;
    double  heading;

    //generate arrow pic
    QImage arrow_pic(21,16, QImage::Format_ARGB32);
    arrow_pic.fill( qRgba(0,0,0,0));
    QPainter t_paint(&arrow_pic);
    USE_ANTI_ALIASING(t_paint, true);
    t_paint.setPen(QPen(Qt::white, 2));
    t_paint.setBrush(QColor(192,0,192,255));
    t_paint.drawPolygon(arrow, 4);
    t_paint.end();

    foreach(pt,line)
    {
        if(start)                // no arrow on  the first loop
        {
            start = false;
        }
        else
        {
            if(!viewport.contains(pt))
            {
                pt1 = pt;
                continue;
            }
            if((abs(pt.x() - pt1.x()) + abs(pt.y() - pt1.y())) < 7)
            {
                pt1 = pt;
                continue;
            }
            // keep distance
            if((abs(pt.x() - ptt.x()) + abs(pt.y() - ptt.y())) > 100)
            {
                if(0 != pt.x() - pt1.x() && (pt.y() - pt1.y()))
                {
                    heading = ( atan2((double)(pt.y() - pt1.y()), (double)(pt.x() - pt1.x())) * 180.) / M_PI;

                    p.save();
                    // draw arrow between bullets
                    p.translate((pt.x() + pt1.x())/2,(pt.y() + pt1.y())/2);
                    p.rotate(heading);
                    p.drawImage(-11, -7, arrow_pic);
                    p.restore();
                    //remember last point
                    ptt = pt;
                }
            }
        }
        pt1 = pt;
    }

}


void CRouteDB::drawLine(const QPolygon& line, const QRect& extViewport, QPainter& p)
{
    QPolygon subline;
    QList<QPolygon> lines;

    int i;
    QPoint pt, ptt, pt1;
    const int size = line.size();

    pt = line[0];
    subline << pt;

    for(i = 1; i < size; i++)
    {
        pt1 = line[i];

        if(!GPS_Math_LineCrossesRect(pt, pt1, extViewport))
        {
            pt = pt1;
            if(subline.size() > 1)
            {
                lines << subline;
            }
            subline.clear();
            subline << pt;
            continue;
        }

        ptt = pt1 - pt;
        if(ptt.manhattanLength() < 5)
        {
            continue;
        }

        subline << pt1;
        pt = pt1;
    }

    if(subline.size() > 1)
    {
        lines << subline;
    }

    for(i = 0; i < lines.count(); i++)
    {
        p.drawPolyline(lines[i]);
    }
}


QList<CRouteDB::keys_t> CRouteDB::keys()
{
    QList<keys_t> k;

    QString k1;
    QStringList ks = routes.keys();

    foreach(k1, ks)
    {
        keys_t k2;
        CRoute * route = routes[k1];

        k2.key      = k1;
        k2.name     = route->name;
        k2.icon     = route->getIcon();
        k2.time     = route->getTimestamp();

        k << k2;
    }

    CRouteToolWidget::sortmode_e sortmode = CRouteToolWidget::getSortMode();

    switch(sortmode)
    {
        case CRouteToolWidget::eSortByName:
            qSort(k.begin(), k.end(), CRouteDB::keyLessThanAlpha);
            break;
        case CRouteToolWidget::eSortByTime:
            qSort(k.begin(), k.end(), CRouteDB::keyLessThanTime);
            break;
    }

    return k;
}


void CRouteDB::makeVisible(const QStringList& keys)
{
    QRectF r;
    QString key;
    foreach(key, keys)
    {

        CRoute * rte =  routes[key];

        if(r.isNull())
        {
            r = rte->getBoundingRectF();
        }
        else
        {
            r |= rte->getBoundingRectF();
        }

    }

    if (!r.isNull ())
    {
        CMapDB::self().getMap().zoom(r.left() * DEG_TO_RAD, r.top() * DEG_TO_RAD, r.right() * DEG_TO_RAD, r.bottom() * DEG_TO_RAD);
    }

}


void CRouteDB::loadSecondaryRoute(const QString& key, QDomDocument& xml, CRoute::service_e service)
{
    if(routes.contains(key))
    {
        routes[key]->loadSecondaryRoute(xml, service);
    }
}


void CRouteDB::reset(const QString& key)
{
    if(routes.contains(key))
    {
        routes[key]->reset();
        routes[key]->resetRouteIdx();
    }
}


void CRouteDB::slotModified()
{
    CRoute * rte = qobject_cast<CRoute*>(sender());
    if(rte)
    {
        emitSigModified(rte->getKey());
    }

}
