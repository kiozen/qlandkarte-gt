/**********************************************************************************************
    Copyright (C) 2010 Oliver Eichler oliver.eichler@gmx.de

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

#include "CMapJnx.h"
#include "CResources.h"
#include "CDlgMapJNXConfig.h"
#include "GeoMath.h"
#include "CMainWindow.h"
#include "CSettings.h"
#include <QtGui>
#include <QCheckBox>

#define MAX_IDX_ZOOM 26
#define MIN_IDX_ZOOM 0

CMapJnx::scale_t CMapJnx::scales[] =
{
    {80000,  2083334 }
    ,{5000,   1302084 }
    ,{3000,   781250  }
    ,{2000,   520834  }
    ,{1200,   312500  }
    ,{800,    208334  }
    ,{500,    130209  }
    ,{300,    78125   }
    ,{200,    52084   }
    ,{120,    31250   }
    ,{80.0,   20834   }
    ,{50.0,   13021   }
    ,{30.0,   7813    }
    ,{20.0,   5209    }
    ,{12.0,   3125    }
    ,{8.00,   2084    }
    ,{5.00,   1303    }
    ,{3.00,   782     }
    ,{2.00,   521     }
    ,{1.20,   313     }
    ,{0.80,   209     }
    ,{0.50,   131     }
    ,{0.30,   79      }
    ,{0.20,   52      }
    ,{0.12,   32      }
    ,{0.08,   21      }
    ,{0.05,   14      }
};

void readCString(QDataStream& stream, QByteArray& ba)
{
    quint8 byte;

    ba.clear();

    stream >> byte;
    while(byte != 0)
    {
        ba += byte;
        stream >> byte;
    }

}


CMapJnx::CMapJnx(const QString& key, const QString& fn, CCanvas * parent)
: IMap(eRaster,key,parent)
, lon1(180.0)
, lat1(-90)
, lon2(-180)
, lat2(90)
, xscale(1.0)
, yscale(-1.0)
{
    qint32 productId = -1;

    filename = fn;

    QFileInfo fi(fn);
    name = fi.baseName();

    readFile(filename, productId);

    // find submaps in same directory
    QDir dir(fi.absoluteDir());
    QStringList subMaps = dir.entryList(QStringList("*.jnx"), QDir::Files);

    if(!files.isEmpty())
    {
        qDebug() << "load maps with product ID:" << productId;
        foreach(const QString& subMap, subMaps)
        {
            if(dir.absoluteFilePath(subMap) == filename)
            {
                continue;
            }
            readFile(dir.absoluteFilePath(subMap), productId);
        }
    }

    pjsrc = pj_init_plus("+proj=merc +ellps=WGS84 +datum=WGS84 +units=m +no_defs +towgs84=0,0,0");
    oSRS.importFromProj4(getProjection());

    x = lon1 * DEG_TO_RAD;
    y = lat2 * DEG_TO_RAD;

    pj_transform(pjtar, pjsrc,1,0,&x,&y,0);

    SETTINGS;
    cfg.beginGroup("birdseye/maps");
    cfg.beginGroup(name);
    zoomidx = cfg.value("zoomidx",23).toInt();
    x       = cfg.value("x",x).toDouble();
    y       = cfg.value("y",y).toDouble();
    cfg.endGroup();
    cfg.endGroup();

    zoom(zoomidx);

    setAngleNorth();

    theMainWindow->getCheckBoxQuadraticZoom()->hide();
}


CMapJnx::~CMapJnx()
{
    qDebug() << "CMapJnx::~CMapJnx()";

    midU = rect.center().x();
    midV = rect.center().y();
    convertPt2Rad(midU, midV);

    if(pjsrc) pj_free(pjsrc);

    SETTINGS;
    cfg.beginGroup("birdseye/maps");
    cfg.beginGroup(name);
    cfg.setValue("zoomidx",zoomidx);
    cfg.setValue("x",x);
    cfg.setValue("y",y);
    cfg.endGroup();
    cfg.endGroup();

    theMainWindow->getCheckBoxQuadraticZoom()->show();

}


void CMapJnx::readFile(const QString& fn, qint32& productId)
{

    hdr_t hdr;

    qDebug() << fn;

    QFile file(fn);
    file.open(QIODevice::ReadOnly);

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    stream >> hdr.version;       // byte 00000000..00000003
    stream >> hdr.devid;         // byte 00000004..00000007
    stream >> hdr.lat1;          // byte 00000008..0000000B
    stream >> hdr.lon2;          // byte 0000000C..0000000F
    stream >> hdr.lat2;          // byte 00000010..00000013
    stream >> hdr.lon1;          // byte 00000014..00000017
    stream >> hdr.details;       // byte 00000018..0000001B
    stream >> hdr.expire;        // byte 0000001C..0000001F
    stream >> hdr.productId;     // byte 00000020..00000023
    stream >> hdr.crc;           // byte 00000024..00000027
    stream >> hdr.signature;     // byte 00000028..0000002B
                                 // byte 0000002C..0000002F
    stream >> hdr.signature_offset;

    if(hdr.version > 3)
    {
        stream >> hdr.zorder;
    }
    else
    {
        hdr.zorder = -1;
    }

    if(productId != -1 && hdr.productId != productId)
    {
        return;
    }

    productId = hdr.productId;

    info  += "<h1>" + QFileInfo(fn).baseName() + "</h1>";

    files.append(file_t());
    file_t& mapFile = files.last();

    mapFile.filename = fn;

    mapFile.lat1 = hdr.lat1 * 180.0 / 0x7FFFFFFF;
    mapFile.lat2 = hdr.lat2 * 180.0 / 0x7FFFFFFF;
    mapFile.lon1 = hdr.lon1 * 180.0 / 0x7FFFFFFF;
    mapFile.lon2 = hdr.lon2 * 180.0 / 0x7FFFFFFF;
    mapFile.bbox = QRectF(QPointF(mapFile.lon1, mapFile.lat1), QPointF(mapFile.lon2, mapFile.lat2));

    qDebug() << hex << "Version:" << hdr.version << "DevId" <<  hdr.devid;
    qDebug() << mapFile.lon1 << mapFile.lat1 << mapFile.lon2 << mapFile.lat2;
    qDebug() << hex <<  hdr.lon1 <<  hdr.lat1 <<  hdr.lon2 <<  hdr.lat2;
    qDebug() << hex << "Details:" <<  hdr.details << "Expire:" <<  hdr.expire << "CRC:" <<  hdr.crc ;
    qDebug() << hex << "Signature:" <<  hdr.signature << "Offset:" <<  hdr.signature_offset;

    QString strTopLeft, strBottomRight;
    GPS_Math_Deg_To_Str(mapFile.lon1, mapFile.lat1, strTopLeft);
    GPS_Math_Deg_To_Str(mapFile.lon2, mapFile.lat2, strBottomRight);

    info += QString("<p><table>");
    info += QString("<tr><td style='background-color: blue; color: white;'>%1</td><td>%2</td></tr>").arg(tr("Product ID")).arg(hdr.productId);
    info += QString("<tr><td style='background-color: blue; color: white;'>%1</td><td>%2</td></tr>").arg(tr("Top/Left")).arg(strTopLeft.replace(QChar(0260),"&#176;"));
    info += QString("<tr><td style='background-color: blue; color: white;'>%1</td><td>%2</td></tr>").arg(tr("Bottom/Right")).arg(strBottomRight.replace(QChar(0260),"&#176;"));

    {
        projXY p1, p2;
        double a1,a2, width, height;
        float u1 = 0, v1 = 0, u2 = 0, v2 = 0;
        GPS_Math_Str_To_Deg(strTopLeft.replace("&#176;",""), u1, v1);
        GPS_Math_Str_To_Deg(strBottomRight.replace("&#176;",""), u2, v2);

        p1.u = u1 * DEG_TO_RAD;
        p1.v = v1 * DEG_TO_RAD;
        p2.u = u2 * DEG_TO_RAD;
        p2.v = v1 * DEG_TO_RAD;
        width   = distance(p1,p2,a1,a2)/1000;
        p1.u = u1 * DEG_TO_RAD;
        p1.v = v1 * DEG_TO_RAD;
        p2.u = u1 * DEG_TO_RAD;
        p2.v = v2 * DEG_TO_RAD;
        height  = distance(p1,p2,a1,a2)/1000;

        info += QString("<tr><td style='background-color: blue; color: white;'>%1</td><td>%2 km&#178; (%3 km x %4 km)</td></tr>").arg(tr("Area")).arg(width*height,0,'f',1).arg(width,0,'f',1).arg(height,0,'f',1);
        info += QString("<tr><td style='background-color: blue; color: white;'>%1</td><td>%2</td></tr>").arg(tr("Projection")).arg("+proj=merc +lon_0=0 +k=1 +x_0=0 +y_0=0 +ellps=WGS84 +datum=WGS84 +units=m +no_defs +towgs84=0,0,0");
        info += QString("<tr><td style='background-color: blue; color: white;'>%1</td><td>%2</td></tr>").arg(tr("Z-Order")).arg(hdr.zorder);
    }

    info += "</table></p>";

    qDebug() << "Levels:";
    mapFile.levels.resize(hdr.details);
    for(quint32 i = 0; i < hdr.details; i++)
    {
        level_t& level = mapFile.levels[i];
        stream >> level.nTiles >> level.offset >> level.scale;

        if(hdr.version > 3)
        {
            quint32 dummy;
            QTextCodec * codec = QTextCodec::codecForName("utf-8");
            QByteArray ba;

            stream >> dummy;
            readCString(stream, ba);
            level.copyright1 = codec->toUnicode(ba);

        }
        qDebug() << i << hex << level.nTiles << level.offset << level.scale;
    }

    quint32 infoBlockVersion;
    stream >> infoBlockVersion;
    if(infoBlockVersion == 0x9)
    {
        QTextCodec * codec = QTextCodec::codecForName("utf-8");
        QByteArray ba;
        quint8 dummy;
        QString groupId;
        QString groupName;
        QString groupTitle;

        readCString(stream, ba);
        groupId = codec->toUnicode(ba);
        readCString(stream, ba);
        groupName = codec->toUnicode(ba);

        stream >> dummy >> dummy >> dummy;
        readCString(stream, ba);
        groupTitle = codec->toUnicode(ba);
        qDebug() << groupId << groupName << groupTitle;

        info += QString("<p><table><tr><th>%1</th><th>%2</th><th>%3</th><th width='100%'>%4</th></tr>").arg(tr("Level")).arg(tr("#Tiles")).arg(tr("Scale")).arg(tr("Info"));
        for(quint32 i = 0; i < hdr.details; i++)
        {
            level_t& level = mapFile.levels[i];

            stream >> level.level;
            readCString(stream, ba);
            level.name1 = codec->toUnicode(ba);
            readCString(stream, ba);
            level.name2 = codec->toUnicode(ba);
            readCString(stream, ba);
            level.copyright2 = codec->toUnicode(ba);

            info+= QString("<tr><td>%1(%4)</td><td>%2</td><td>%3</td><td>%5</td></tr>").arg(i).arg(level.nTiles).arg(level.scale).arg(level.level).arg(level.name1 + "<br>" + level.name2 + "<br>" + level.copyright2);
        }
        info += "</table></p>";
    }

    for(quint32 i = 0; i < hdr.details; i++)
    {

        level_t& level = mapFile.levels[i];
        const quint32 M = level.nTiles;
        file.seek(level.offset);

        level.tiles.resize(M);

        for(quint32 m = 0; m < M; m++)
        {

            qint32 top, right, bottom, left;
            tile_t& tile = level.tiles[m];

            stream >> top >> right >> bottom >> left;
            stream >> tile.width >> tile.height >> tile.size >> tile.offset;

            tile.area.setTop(top * 180.0 / 0x7FFFFFFF);
            tile.area.setRight(right * 180.0 / 0x7FFFFFFF);
            tile.area.setBottom(bottom * 180.0 / 0x7FFFFFFF);
            tile.area.setLeft(left * 180.0 / 0x7FFFFFFF);
        }
    }

    if(mapFile.lon1 < lon1) lon1  = mapFile.lon1;
    if(mapFile.lat1 > lat1) lat1  = mapFile.lat1;
    if(mapFile.lon2 > lon2) lon2  = mapFile.lon2;
    if(mapFile.lat2 < lat2) lat2  = mapFile.lat2;

}


void CMapJnx::convertPt2M(double& u, double& v)
{
    u = x + u * xscale * zoomFactor;
    v = y + v * yscale * zoomFactor;
}


void CMapJnx::convertM2Pt(double& u, double& v)
{
    u = floor((u - x) / (xscale * zoomFactor) + 0.5);
    v = floor((v - y) / (yscale * zoomFactor) + 0.5);
}


void CMapJnx::move(const QPoint& old, const QPoint& next)
{
    projXY p2;
    p2.u = x;
    p2.v = y;
    convertM2Pt(p2.u, p2.v);

    // move top left point by difference
    p2.u += old.x() - next.x();
    p2.v += old.y() - next.y();

    // convert back to new top left geo coordinate
    convertPt2M(p2.u, p2.v);
    x = p2.u;
    y = p2.v;

    needsRedraw = true;
    emit sigChanged();

    setAngleNorth();
}


void CMapJnx::zoom(bool zoomIn, const QPoint& p0)
{
    projXY p1;

    needsRedraw = true;

    // convert point to geo. coordinates
    p1.u = p0.x();
    p1.v = p0.y();
    convertPt2Rad(p1.u, p1.v);

    zoomidx += zoomIn ? +1 : -1;
    // sigChanged will be sent at the end of this function
    blockSignals(true);
    zoom(zoomidx);

    // convert geo. coordinates back to point
    IMap::convertRad2Pt(p1.u, p1.v);

    projXY p2;
    p2.u = x;
    p2.v = y;
    convertM2Pt(p2.u, p2.v);

    // move top left point by difference point befor and after zoom
    p2.u += p1.u - p0.x();
    p2.v += p1.v - p0.y();

    // convert back to new top left geo coordinate
    convertPt2M(p2.u, p2.v);
    x = p2.u;
    y = p2.v;

    blockSignals(false);
    emit sigChanged();

}


void CMapJnx::zoom(double lon1, double lat1, double lon2, double lat2)
{

    double u[3];
    double v[3];
    double dU, dV;

    needsRedraw = true;

    u[0] = lon1;
    v[0] = lat1;
    u[1] = lon2;
    v[1] = lat1;
    u[2] = lon1;
    v[2] = lat2;

    pj_transform(pjtar, pjsrc,3,0,u,v,0);
    dU = u[1] - u[0];
    dV = v[2] - v[0];

    for(int i = MAX_IDX_ZOOM; i >= MIN_IDX_ZOOM; --i)
    {

        double z    = scales[i].qlgtScale;
        double pxU  = dU / (+1.0 * z);
        double pxV  = dV / (-1.0 * z);

        if(isLonLat())
        {
            pxU /= xscale;
            pxV /= yscale;
        }

        if((fabs(pxU) < size.width()) && (fabs(pxV) < size.height()))
        {
            zoomFactor  = z;
            zoomidx     = i;
            double u = lon1 + (lon2 - lon1)/2;
            double v = lat1 + (lat2 - lat1)/2;
            IMap::convertRad2Pt(u,v);
            move(QPoint(u,v), rect.center());
            return;
        }
    }
}


void CMapJnx::zoom(qint32& level)
{
    needsRedraw = true;

    zoomidx = level;
    if(zoomidx < MIN_IDX_ZOOM) zoomidx = MIN_IDX_ZOOM;
    if(zoomidx > MAX_IDX_ZOOM) zoomidx = MAX_IDX_ZOOM;
    zoomFactor = scales[zoomidx].qlgtScale;

    emit sigChanged();
}


void CMapJnx::dimensions(double& lon1, double& lat1, double& lon2, double& lat2)
{
    lon1 = this->lon1 * DEG_TO_RAD;
    lon2 = this->lon2 * DEG_TO_RAD;
    lat1 = this->lat2 * DEG_TO_RAD;
    lat2 = this->lat1 * DEG_TO_RAD;

}


void CMapJnx::draw(QPainter& p)
{
    if(pjsrc == 0) return IMap::draw(p);

    if(needsRedraw)
    {
        draw();
    }

    p.drawPixmap(0,0,pixBuffer);

    if(!ovlMap.isNull() && !doFastDraw)
    {
        ovlMap->draw(size, needsRedraw, p);
    }

    needsRedraw = false;
}


void CMapJnx::getArea_n_Scaling(projXY& p1, projXY& p2, float& my_xscale, float& my_yscale)
{
    p1.u = 0;
    p1.v = 0;
    convertPt2Rad(p1.u, p1.v);

    p2.u = size.width();
    p2.v = size.height();
    convertPt2Rad(p2.u, p2.v);

    my_xscale   = xscale*zoomFactor;
    my_yscale   = yscale*zoomFactor;
}


qint32 CMapJnx::zlevel2idx(quint32 zl, const file_t& file)
{
    qint32 idxLvl   = -1;
    quint32 actScale = scales[zl].jnxScale;

    for(int i = 0; i < file.levels.size(); i++)
    {
        const level_t& level = file.levels[i];

        if(actScale <= level.scale)
        {
            idxLvl = i;
        }
    }

    return idxLvl;
}


void CMapJnx::draw()
{
    if(pjsrc == 0) return IMap::draw();

    QImage image;
    QRectF viewport;

    pixBuffer.fill(Qt::white);
    QPainter p(&pixBuffer);

    p.setBrush(Qt::NoBrush);

    double u1 = 0;
    double v1 = size.height();
    double u2 = size.width();
    double v2 = 0;

    convertPt2Rad(u1,v1);
    convertPt2Rad(u2,v2);

    u1 *= RAD_TO_DEG;
    v1 *= RAD_TO_DEG;
    u2 *= RAD_TO_DEG;
    v2 *= RAD_TO_DEG;

    viewport.setTop(v2);
    viewport.setRight(u2);
    viewport.setBottom(v1);
    viewport.setLeft(u1);

    foreach(const file_t& mapFile, files)
    {
        if(!viewport.intersects(mapFile.bbox))
        {
            continue;
        }

        qint32 level = zlevel2idx(zoomidx, mapFile);

        if(level < 0)
        {
            double u1 = mapFile.lon1 * DEG_TO_RAD;
            double u2 = mapFile.lon2 * DEG_TO_RAD;
            double v2 = mapFile.lat2 * DEG_TO_RAD;
            double v1 = mapFile.lat1 * DEG_TO_RAD;

            convertRad2Pt(u1,v1);
            convertRad2Pt(u2,v2);

            QRectF r;
            r.setLeft(u1);
            r.setRight(u2);
            r.setTop(v2);
            r.setBottom(v1);

            p.setPen(QPen(Qt::darkBlue,2));
            p.setBrush(QBrush(QColor(230,230,255,100) ));
            p.drawRect(r);
            continue;
        }

        QByteArray data(1024*1024*4,0);
        //(char) typecast needed to avoid MSVC compiler warning
        //in MSVC, char is a signed type.
        //Maybe the QByteArray declaration should be fixed ;-)
        data[0] = (char) 0xFF;
        data[1] = (char) 0xD8;
        char * pData = data.data() + 2;

        QFile file(mapFile.filename);
        file.open(QIODevice::ReadOnly);

        const QVector<tile_t>& tiles = mapFile.levels[level].tiles;
        const quint32 M = tiles.size();
        for(quint32 m = 0; m < M; m++)
        {
            const tile_t& tile = tiles[m];

            if(viewport.intersects(tile.area))
            {

                double u1 = tile.area.left() * DEG_TO_RAD;
                double u2 = tile.area.right() * DEG_TO_RAD;
                double v2 = tile.area.top() * DEG_TO_RAD;
                double v1 = tile.area.bottom() * DEG_TO_RAD;

                convertRad2Pt(u1,v1);
                convertRad2Pt(u2,v2);

                QRectF r;
                r.setLeft(u1);
                r.setRight(u2);
                r.setTop(v2);
                r.setBottom(v1);

                if(!r.isValid() || r.width() > 3000 || r.height() > 3000)
                {
                    continue;
                }

                file.seek(tile.offset);
                file.read(pData, tile.size);
                image.loadFromData(data);

                p.drawImage(r.toRect(), image.scaled(r.size().toSize(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
            }
        }
    }
}


void CMapJnx::config()
{

    CDlgMapJNXConfig * dlg = new CDlgMapJNXConfig(this);
    dlg->show();

}
