/**********************************************************************************************
    Copyright (C) 2008 Oliver Eichler oliver.eichler@gmx.de

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

#include "CMapDB.h"
#include "CMapToolWidget.h"
#include "CMapQMAP.h"
#include "CMapTDB.h"
#include "CMapRaster.h"
#include "CMapGeoTiff.h"
#include "CMapJnx.h"
#include "CMapRmap.h"
#include "CMapRmp.h"
#include "CMapWms.h"
#include "CMapTms.h"
#include "CMapDEM.h"
#include "CMainWindow.h"
#include "CMapEditWidget.h"
#include "CMapSearchWidget.h"
#include "GeoMath.h"
#include "CCanvas.h"
#include "CMap3D.h"
#include "CTabWidget.h"
#include "CMapSelectionGarmin.h"
#include "CMapSelectionRaster.h"
#include "CResources.h"
#include "IDevice.h"
#include "CQlb.h"
#include "IMouse.h"
#include "CSettings.h"

#include <QtGui>
#include <QMessageBox>
#include <QtXml/QDomDocument>

#include <gdal_priv.h>

CMapDB * CMapDB::m_self = 0;

CMapDB::CMapDB(QTabWidget * tb, QObject * parent)
: IDB(IDB::eTypeMap, tb, parent)
{
    SETTINGS;

    m_self              = this;
    CMapToolWidget * tw = new CMapToolWidget(tb);
    toolview            = tw;
    connect(tw, SIGNAL(sigChanged()), SIGNAL(sigChanged()));

    defaultMap = new CMapNoMap(theMainWindow->getCanvas());
    connect(defaultMap, SIGNAL(sigChanged()),  theMainWindow->getCanvas(), SLOT(update()));

    theMap = defaultMap;

    // static maps
    map_t m;
    m.description       = tr("--- No map ---");
    m.key               = "NoMap";
    m.type              = IMap::eNoMap;
    knownMaps[m.key]    = m;
    builtInKeys << m.key;

    QString map;
    QStringList maps = cfg.value("maps/knownMaps","").toString().split("|",QString::SkipEmptyParts);
    foreach(map, maps)
    {
        QFileInfo fi(map);
        QString ext     = fi.suffix().toLower();
        map_t m;
        m.filename      = map;
        if(ext == "tdb")
        {
            cfg.beginGroup("garmin/maps/alias");
            m.description = cfg.value(map,"").toString();
            cfg.endGroup();
        }
        else if(ext == "xml")
        {
            QFile file(map);
            file.open(QIODevice::ReadOnly);
            QDomDocument dom;
            dom.setContent(&file, false);
            m.description = dom.firstChildElement("GDAL_WMS").firstChildElement("Service").firstChildElement("Title").text();
            file.close();
            if(m.description.isEmpty()) m.description = fi.fileName();
        }
        else if(ext == "tms")
        {
            QFile file(map);
            file.open(QIODevice::ReadOnly);
            QDomDocument dom;
            dom.setContent(&file, false);
            m.description = dom.firstChildElement("TMS").firstChildElement("Title").text();
            file.close();
            if(m.description.isEmpty()) m.description = fi.fileName();
        }
        else if(ext == "rmap")
        {
            m.description = fi.baseName();
        }
        else
        {
            QSettings mapdef(map,QSettings::IniFormat);
            mapdef.setIniCodec(QTextCodec::codecForName("UTF-8"));
            m.description = mapdef.value("description/comment","").toString();
        }
        if(m.description.isEmpty()) m.description = QFileInfo(map).baseName();
        m.key            = map;
        m.type           = ext == "qmap" ? IMap::eRaster : ext == "tdb" ? IMap::eGarmin : ext == "xml" ? IMap::eWMS : ext == "tms" ? IMap::eTMS : IMap::eRaster;
        knownMaps[m.key] = m;
    }

    // add streaming maps
    {
        map_t m;

        QStringList keys = cfg.value("tms/knownMaps").toString().split("|",QString::SkipEmptyParts);
        foreach(const QString& key, keys)
        {
            cfg.beginGroup(QString("tms/maps/%1").arg(key));
            m.description   = cfg.value("name", "").toString();
            m.filename      = cfg.value("url", "").toString();
            m.copyright     = cfg.value("copyright", "").toString();
            m.key           = QString::number(qHash(m.filename));
            m.type          = IMap::eTMS;
            if(!m.description.isEmpty() && !m.filename.isEmpty())
            {
                knownMaps[m.key] = m;
            }
            cfg.endGroup();
        }
    }

    if(theMainWindow->didCrash())
    {
        int res = QMessageBox::warning(0,tr("Crash detected...."), tr("QLandkarte GT was terminated with a crash. This is really bad. A common reason for that is a bad map. Do you really want to load the last map?"), QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes);
        if(res == QMessageBox::No)
        {
            cfg.setValue("maps/visibleMaps","");
            emitSigChanged();
            return;
        }
    }

    maps = cfg.value("maps/visibleMaps","").toString().split("|",QString::SkipEmptyParts);
    cfg.setValue("maps/visibleMaps","");
    cfg.sync();

    foreach(map, maps)
    {
        openMap(map, false, *theMainWindow->getCanvas());
        break;
    }
    cfg.setValue("maps/visibleMaps",maps.join("|"));

    emitSigChanged();

}


CMapDB::~CMapDB()
{
    SETTINGS;
    QString maps;
    map_t map;
    QString mapsTMS;

    cfg.remove("tms/maps");

    foreach(map,knownMaps)
    {
        if(builtInKeys.contains(map.key))
        {
            continue;
        }
        if(map.filename.startsWith("http") || map.filename.startsWith("file")|| map.filename.startsWith("script"))
        {
            mapsTMS += map.key + "|";
            cfg.beginGroup(QString("tms/maps/%1").arg(map.key));
            cfg.setValue("name", map.description);
            cfg.setValue("url", map.filename);
            cfg.setValue("copyright", map.copyright);
            cfg.endGroup();
        }
        else
        {
            maps += map.filename + "|";
        }
    }
    cfg.setValue("maps/knownMaps",maps);
    cfg.setValue("tms/knownMaps",mapsTMS);

    GDALDestroyDriverManager();
}


CMap3D * CMapDB::getMap3D()
{
    return map3D;
}


void CMapDB::clear()
{
    if(selectedMaps.isEmpty()) return;
    selectedMaps.clear();
    emitSigChanged();
}


IMap& CMapDB::getMap()
{
    return (theMap.isNull() ? *defaultMap : *theMap);
}


IMap& CMapDB::getDEM()
{
    return (demMap.isNull() ? *defaultMap : *demMap);
}


void CMapDB::closeVisibleMaps()
{
    IMap * map = theMap;
    theMap = defaultMap;

    if(map && map != defaultMap) delete map;

    if(!demMap.isNull()) demMap->deleteLater();

    IMouse::resetPos1();
}


void CMapDB::openMap(const QString& filename, bool asRaster, CCanvas& canvas)
{

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    closeVisibleMaps();

    SETTINGS;

    map_t map;
    QFileInfo fi(filename);
    if(!fi.exists() && !filename.startsWith("http") && !filename.startsWith("file") && !filename.startsWith("script")) return;

    QString ext = fi.suffix().toLower();
    if(ext == "qmap")
    {

        // create map descritor
        QSettings mapdef(filename,QSettings::IniFormat);
        mapdef.setIniCodec(QTextCodec::codecForName("UTF-8"));
        map.filename    = filename;
        map.description = mapdef.value("description/comment","").toString();
        if(map.description.isEmpty()) map.description = fi.fileName();
        map.key         = filename;
        map.type        = IMap::eRaster;

        // create base map
        theMap = new CMapQMAP(map.key, filename,&canvas);

        // add map to known maps
        knownMaps[map.key] = map;

        // store current map filename for next session
        cfg.setValue("maps/visibleMaps",theMap->getFilename());

    }
    else if(ext == "tdb")
    {
        CMapTDB * maptdb;

        map.filename    = filename;
        map.key         = filename;
        map.type        = IMap::eGarmin;

        theMap = maptdb = new CMapTDB(map.key, filename, &canvas);

        map.description = maptdb->getName();
        if(map.description.isEmpty()) map.description = fi.fileName();

        // add map to known maps
        knownMaps[map.key] = map;

        // store current map filename for next session
        cfg.setValue("maps/visibleMaps",filename);
        cfg.beginGroup("garmin/maps/alias");
        cfg.setValue(map.filename, map.description);
        cfg.endGroup();
    }
    else if(ext == "jnx")
    {

        CMapJnx * mapjnx;

        map.filename    = filename;
        map.key         = filename;
        map.type        = IMap::eRaster;

        theMap = mapjnx = new CMapJnx(map.key, filename, &canvas);

        map.description = mapjnx->getName();
        if(map.description.isEmpty()) map.description = fi.fileName();

        // add map to known maps
        knownMaps[map.key] = map;

        // store current map filename for next session
        cfg.setValue("maps/visibleMaps",filename);
        cfg.beginGroup("garmin/maps/alias");
        cfg.setValue(map.filename, map.description);
        cfg.endGroup();
    }
    else if(ext == "rmap")
    {

        CMapRmap * maprmap;

        map.filename    = filename;
        map.key         = filename;
        map.type        = IMap::eRaster;

        theMap = maprmap = new CMapRmap(map.key, filename, &canvas);

        map.description = maprmap->getName();
        if(map.description.isEmpty()) map.description = fi.fileName();

        // add map to known maps
        knownMaps[map.key] = map;

        // store current map filename for next session
        cfg.setValue("maps/visibleMaps",filename);
    }
    else if(ext == "rmp")
    {

        CMapRmp * maprmp;

        map.filename    = filename;
        map.key         = filename;
        map.type        = IMap::eRaster;

        theMap = maprmp = new CMapRmp(map.key, filename, &canvas);

        map.description = maprmp->getName();
        if(map.description.isEmpty()) map.description = fi.fileName();

        // add map to known maps
        knownMaps[map.key] = map;

        // store current map filename for next session
        cfg.setValue("maps/visibleMaps",filename);
    }
    else if(ext == "xml")
    {
        CMapWms * mapwms;

        map.filename    = filename;
        map.key         = filename;
        map.type        = IMap::eWMS;

        theMap = mapwms = new CMapWms(map.key, filename, &canvas);

        map.description = mapwms->getName();
        if(map.description.isEmpty()) map.description = fi.fileName();

        // add map to known maps
        knownMaps[map.key] = map;
        // store current map filename for next session
        cfg.setValue("maps/visibleMaps",filename);
    }
    else if(ext == "tms")
    {
        CMapTms * maptms;

        map.filename    = filename;
        map.key         = filename;
        map.type        = IMap::eTMS;

        theMap = maptms = new CMapTms(map.key, &canvas);
        map.description = maptms->getName();
        if(map.description.isEmpty()) map.description = fi.fileName();

        // add map to known maps
        knownMaps[map.key] = map;
        // store current map filename for next session
        cfg.setValue("maps/visibleMaps",filename);

    }
    else if(filename.startsWith("http") || filename.startsWith("file") ||  filename.startsWith("script"))
    {
        theMap = new CMapTms(QString::number(qHash(filename)), theMainWindow->getCanvas());

        // store current map filename for next session
        cfg.setValue("maps/visibleMaps",filename);
    }
    else
    {
        if(asRaster)
        {
            theMap = new CMapRaster(filename,&canvas);
        }
        else
        {
            theMap = new CMapGeoTiff(filename,&canvas);
            // store current map filename for next session
            SETTINGS;
            cfg.setValue("maps/visibleMaps",theMap->getFilename());
        }
    }

    connect(theMap, SIGNAL(sigChanged()),  theMainWindow->getCanvas(), SLOT(slotMapChanged()));

    QString fileDEM = cfg.value(QString("map/dem/%1").arg(theMap->getKey()),"").toString();
    openDEM(fileDEM);

    emitSigChanged();

    QApplication::restoreOverrideCursor();
}


void CMapDB::openMap(const QString& key)
{
    if(!knownMaps.contains(key)) return;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    closeVisibleMaps();

    QString filename = knownMaps[key].filename;
    QFileInfo fi(filename);
    QString ext = fi.suffix().toLower();

    if(ext == "qmap")
    {
        // create base map
        if(filename.isEmpty())
        {
            theMap = defaultMap;
        }
        else
        {
            theMap = new CMapQMAP(key,filename,theMainWindow->getCanvas());
        }

    }
    else if(ext == "tdb")
    {
        theMap = new CMapTDB(key,filename,theMainWindow->getCanvas());
    }
    else if(ext == "jnx")
    {
        theMap = new CMapJnx(key,filename,theMainWindow->getCanvas());
    }
    else if(ext == "rmap")
    {
        theMap = new CMapRmap(key,filename,theMainWindow->getCanvas());
    }
    else if(ext == "rmp")
    {
        theMap = new CMapRmp(key,filename,theMainWindow->getCanvas());
    }
    else if(ext == "xml")
    {
        theMap = new CMapWms(key,filename,theMainWindow->getCanvas());
    }
    else if(ext == "tms")
    {
        theMap = new CMapTms(key, theMainWindow->getCanvas());
    }
    else if(filename.startsWith("http") || filename.startsWith("file") || filename.startsWith("script"))
    {
        theMap = new CMapTms(key, theMainWindow->getCanvas());
    }

    connect(theMap, SIGNAL(sigChanged()), theMainWindow->getCanvas(), SLOT(slotMapChanged()));

    // store current map filename for next session
    SETTINGS;
    cfg.setValue("maps/visibleMaps",filename);

    QString fileDEM = cfg.value(QString("map/dem/%1").arg(theMap->getKey()),"").toString();
    openDEM(fileDEM);

    double lon1, lon2, lat1, lat2;
    theMap->dimensions(lon1, lat1, lon2, lat2);
    if(((lon1 < IMap::midU) && (IMap::midU < lon2)) && ((lat2 < IMap::midV) && (IMap::midV < lat1)) && ((IMap::midU != 0) && (IMap::midV != 0)))
    {
        double midU = IMap::midU;
        double midV = IMap::midV;
        theMap->convertRad2Pt(midU, midV);
        theMap->move(QPoint(midU, midV), theMainWindow->getCanvas()->rect().center());
    }

    emitSigChanged();
    QApplication::restoreOverrideCursor();
}


IMap * CMapDB::createMap(const QString& key)
{
    if(!knownMaps.contains(key)) return 0;
    const map_t& mapdesc = knownMaps[key];
    if(mapdesc.type != IMap::eGarmin)
    {
        QMessageBox::critical(0, tr("Error..."), tr("Only vector maps are valid overlays."), QMessageBox::Abort, QMessageBox::Abort);
        return 0;
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    IMap * map = 0;
    QString filename = mapdesc.filename;
    QFileInfo fi(filename);
    QString ext = fi.suffix().toLower();

    if(ext == "tdb")
    {
        map = new CMapTDB(key, filename);
    }
    QApplication::restoreOverrideCursor();

    return map;
}


void CMapDB::openDEM(const QString& filename)
{
    SETTINGS;

    if(!demMap.isNull())
    {
        demMap->deleteLater();
        demMap = 0;
    }

    if(filename.isEmpty())
    {
        return;
    }

    try
    {
        CMapDEM * dem;
        dem = new CMapDEM(filename, theMainWindow->getCanvas());
        if (dem->loaded())
            demMap = dem, theMap->registerDEM(*dem);
        else
            dem->deleteLater();
    }
    catch(const QString& msg)
    {
        cfg.setValue(QString("map/dem/%1").arg(theMap->getKey()), "");
        cfg.setValue(QString("map/dem/%1/ignoreWarning").arg(theMap->getKey()), false);
        return;
    }

    cfg.setValue(QString("map/dem/%1").arg(theMap->getKey()), filename);
    if(filename.isEmpty())
    {
        cfg.setValue(QString("map/dem/%1/ignoreWarning").arg(theMap->getKey()), false);
    }

    emitSigChanged();
}


void CMapDB::closeMap()
{
    SETTINGS;
    cfg.setValue("maps/visibleMaps",theMap->getFilename());
    closeVisibleMaps();
}


void CMapDB::delKnownMap(const QStringList& keys)
{
    QString key;
    foreach(key, keys)
    {
        map_t& map = knownMaps[key];
        IMap::maptype_e type = map.type;
        QString filename = map.filename;

        knownMaps.remove(key);

        if(type == IMap::eGarmin)
        {
            SETTINGS;
            cfg.beginGroup("garmin/maps");
            cfg.beginGroup("alias");
            QString name = cfg.value(key,key).toString();
            cfg.endGroup();
            cfg.remove(name);
            cfg.endGroup();
            cfg.sync();
        }
        else if(type == IMap::eTMS)
        {
            SETTINGS;
            cfg.remove(QString("tms/maps/%1").arg(key));
            cfg.sync();
        }
        else if(type == IMap::eWMS)
        {
            SETTINGS;
            cfg.beginGroup("wms/maps");
            cfg.remove(key);
            cfg.endGroup();
            cfg.sync();
        }
        else if(QFileInfo(filename).completeSuffix() == "rmap")
        {
            SETTINGS;
            cfg.beginGroup("rmap/maps");
            cfg.remove(key);
            cfg.endGroup();
            cfg.sync();
        }

    }

    emitSigChanged();
}


void CMapDB::delSelectedMap(const QStringList& keys)
{
    QString key;
    foreach(key, keys)
    {
        delSelectedMap(key,true);
    }

    emitSigChanged();
}


void CMapDB::delSelectedMap(const QString& key, bool silent)
{
    if(selectedMaps.contains(key))
    {
        delete selectedMaps.take(key);
        if(!silent)
        {
            emitSigChanged();
        }
    }
}


void CMapDB::selSelectedMap(const QString& key)
{
    if(!selectedMaps.contains(key)) return;

    IMapSelection * ms = selectedMaps[key];
    if(mapsearch && (ms->type == IMapSelection::eRaster)) mapsearch->setArea((CMapSelectionRaster&)*ms);
}


void CMapDB::loadGPX(CGpx& gpx)
{
}


void CMapDB::saveGPX(CGpx& gpx, const QStringList& keys)
{
}


QDataStream& CMapDB::operator<<(QDataStream& s)
{

    qint32 type;
    qint32 subtype = IMapSelection::eNo;
    quint32 timestamp;
    QString key;
    QString mapkey;
    QString name;
    QString comment;
    QString description;

    double lon1;                 ///< top left longitude [rad]
    double lat1;                 ///< top left latitude [rad]
    double lon2;                 ///< bottom right longitude [rad]
    double lat2;                 ///< bottom right latitude [rad]

    QIODevice * dev = s.device();
    qint64      pos = dev->pos();

    char magic[9];
    s.readRawData(magic,9);

    if(strncmp(magic,"QLMapSel",9))
    {
        dev->seek(pos);
        return s;
    }

    QList<IMapSelection::sel_head_entry_t> entries;
    while(1)
    {
        IMapSelection::sel_head_entry_t entry;
        s >> entry.type >> entry.offset;
        entries << entry;
        if(entry.type == CWpt::eEnd) break;
    }

    QList<IMapSelection::sel_head_entry_t>::iterator entry = entries.begin();
    while(entry != entries.end())
    {
        qint64 o = pos + entry->offset;
        dev->seek(o);
        s >> entry->data;

        switch(entry->type)
        {
            case IMapSelection::eHeadBase:
            {

                QDataStream s1(&entry->data, QIODevice::ReadOnly);
                s1.setVersion(QDataStream::Qt_4_5);

                s1 >> type;
                s1 >> key;
                s1 >> mapkey;
                s1 >> timestamp;
                s1 >> name;
                s1 >> comment;
                s1 >> description;
                s1 >> lon1;      ///< top left longitude [rad]
                s1 >> lat1;      ///< top left latitude [rad]
                s1 >> lon2;      ///< bottom right longitude [rad]
                s1 >> lat2;      ///< bottom right latitude [rad]
                s1 >> subtype;
                break;
            }

            case IMapSelection::eHeadRaster:
            {
                QDataStream s1(&entry->data, QIODevice::ReadOnly);
                s1.setVersion(QDataStream::Qt_4_5);

                if(subtype == IMapSelection::eNo)
                {
                    subtype = IMapSelection::eGDAL;
                }

                CMapSelectionRaster * ms = new CMapSelectionRaster((IMapSelection::subtype_e)subtype, this);
                ms->setKey(key);
                ms->mapkey = mapkey;
                ms->setTimestamp(timestamp);
                ms->setName(name);
                ms->setComment(comment);
                ms->setDescription(description);
                ms->lon1 = lon1;
                ms->lat1 = lat1;
                ms->lon2 = lon2;
                ms->lat2 = lat2;

                s1 >> ms->selTiles ;

                selectedMaps[ms->getKey()] = ms;

                break;
            }

            case IMapSelection::eHeadGarmin:
            {
                int nMaps, nTiles, m, t;

                QDataStream s1(&entry->data, QIODevice::ReadOnly);
                s1.setVersion(QDataStream::Qt_4_5);

                CMapSelectionGarmin * ms = new CMapSelectionGarmin(this);
                ms->setKey(key);
                ms->mapkey = mapkey;
                ms->setTimestamp(timestamp);
                ms->setName(name);
                ms->setComment(comment);
                ms->setDescription(description);
                ms->lon1 = lon1;
                ms->lat1 = lat1;
                ms->lon2 = lon2;
                ms->lat2 = lat2;

                s1 >> nMaps;
                for(m = 0; m < nMaps; m++)
                {
                    QString key;
                    s1 >> key;
                    CMapSelectionGarmin::map_t map;
                    s1 >> map.unlockKey;
                    s1 >> map.name;
                    s1 >> map.typfile;
                    s1 >> map.mdrfile;
                    s1 >> map.fid;
                    s1 >> map.pid;

                    s1 >> nTiles;
                    for(t = 0; t < nTiles; t++)
                    {
                        QString key;
                        s1 >> key;
                        CMapSelectionGarmin::tile_t tile;
                        s1 >> tile.id;
                        s1 >> tile.name;
                        s1 >> tile.filename;
                        s1 >> tile.u;
                        s1 >> tile.v;
                        s1 >> tile.memSize;
                        s1 >> tile.area;
                        s1 >> tile.fid;
                        s1 >> tile.pid;

                        map.tiles[key] = tile;
                    }

                    ms->maps[key] = map;
                }

                QString key;
                foreach(key, selectedMaps.keys())
                {
                    IMapSelection * mapSel = selectedMaps[key];
                    if(mapSel->type == IMapSelection::eVector)
                    {
                        delete selectedMaps.take(key);
                    }
                }

                selectedMaps[ms->getKey()] = ms;
                break;
            }
            default:;
        }

        ++entry;
    }
    return s;
}


void CMapDB::loadQLB(CQlb& qlb, bool newKey)
{
    QDataStream stream(&qlb.mapsels(),QIODevice::ReadOnly);
    stream.setVersion(QDataStream::Qt_4_5);

    while(!stream.atEnd())
    {
        *this << stream;
    }
    if(selectedMaps.size())
    {
        emitSigChanged();
    }
}


void CMapDB::saveQLB(CQlb& qlb)
{
    QMap<QString, IMapSelection*>::iterator sel = selectedMaps.begin();
    while(sel != selectedMaps.end())
    {
        qlb << *(*sel);
        ++sel;
    }
}


void CMapDB::upload(const QStringList& keys)
{
    if(selectedMaps.isEmpty()) return;

    IDevice * dev = CResources::self().device();
    if(dev)
    {
        QList<IMapSelection*> tmpms = selectedMaps.values();
        dev->uploadMap(tmpms);
    }
}


void CMapDB::download()
{
}


void CMapDB::draw(QPainter& p, const QRect& rect, bool& needsRedraw)
{
    if(theMap.isNull())
    {
        defaultMap->draw(p);
        return;
    }
    needsRedraw = theMap->getNeedsRedraw();
    theMap->draw(p);

    if(!demMap.isNull())
    {
        demMap->draw(p);
    }

    if(tabbar-> currentWidget() != toolview)
    {
        return;
    }

    QMap<QString,IMapSelection*>::iterator ms = selectedMaps.begin();
    while(ms != selectedMaps.end())
    {
        (*ms)->draw(p, rect);
        ++ms;
    }
}


void CMapDB::editMap()
{
    if(mapedit.isNull())
    {
        mapedit = new CMapEditWidget(theMainWindow->getCanvas());
        theMainWindow->setTempWidget(mapedit, tr("Edit Map"));
    }
}


void CMapDB::show3DMap(bool show)
{
    if(map3D.isNull() && show)
    {
        if(!theMap.isNull())
        {
            map3D = new CMap3D(theMap, theMainWindow->getCanvas());
            theMainWindow->getCanvasTab()->addTab(map3D, tr("Map 3D..."));
        }
    }
    else if(!map3D.isNull() && !show)
    {
        map3D->deleteLater();
    }
}


void CMapDB::searchMap()
{
    if(mapsearch.isNull())
    {
        mapsearch = new CMapSearchWidget(theMainWindow->getCanvas());
        theMainWindow->setTempWidget(mapsearch, tr("Search Map"));
    }
    else
    {
        mapsearch->deleteLater();
    }
}


void CMapDB::select(const QRect& rect, const QMap< QPair<int,int>, bool>& selTiles)
{
    QString mapkey = theMap->getKey();
    if(mapkey.isEmpty())
    {
        QMessageBox::information(0,tr("Sorry..."), tr("You can't select subareas from single file maps. Create a collection with F1->F6."), QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    if(theMap->maptype == IMap::eRaster || theMap->maptype == IMap::eWMS || theMap->maptype == IMap::eTMS)
    {
        CMapSelectionRaster * ms;
        if(theMap->maptype == IMap::eRaster)
        {
            ms = new CMapSelectionRaster(IMapSelection::eGDAL, this);
        }
        else if(theMap->maptype == IMap::eWMS)
        {
            ms = new CMapSelectionRaster(IMapSelection::eWMS, this);
        }
        else if(theMap->maptype == IMap::eTMS)
        {
            ms = new CMapSelectionRaster(IMapSelection::eTMS, this);
        }

        ms->mapkey       = mapkey;
        ms->selTiles     = selTiles;
        ms->setName(knownMaps[mapkey].description);

        try
        {
            theMap->select(*ms, rect);

            selectedMaps[ms->getKey()] = ms;

            if(ms->isEmpty())
            {
                delete selectedMaps.take(ms->getKey());
            }
            else if(mapsearch)
            {
                mapsearch->setArea(*ms);
            }

            emitSigChanged();
        }
        catch(const QString& msg)
        {
            delete ms;
            QMessageBox::critical(0,tr("Error..."), msg, QMessageBox::Abort,QMessageBox::Abort);
        }

    }
    else if(theMap->maptype == IMap::eGarmin)
    {
        bool isEdit = false;
        IMapSelection * ms = 0;
        IMapSelection * mapSel;
        foreach(mapSel, selectedMaps)
        {
            if(mapSel->type == IMapSelection::eVector)
            {
                ms     = mapSel;
                isEdit = true;
                break;
            }
        }

        if(ms == 0)
        {
            ms = new CMapSelectionGarmin(this);
        }

        ms->mapkey       = mapkey;
        ms->setDescription("Garmin - gmapsupp.img");
        theMap->select(*ms, rect);

        selectedMaps[ms->getKey()] = ms;

        if(ms->isEmpty())
        {
            delete selectedMaps.take(ms->getKey());
            ms = 0;
        }

        if(ms && isEdit)
        {
            emitSigModified(ms->getKey());
        }
        emitSigChanged();
    }
    else
    {
        QMessageBox::critical(0,tr("Error..."), tr("This map does not support this feature."), QMessageBox::Abort,QMessageBox::Abort);
    }
}


IMapSelection * CMapDB::getSelectedMap(double lon, double lat)
{
    IMap& map = getMap();

    if(map.maptype != IMap::eRaster && map.maptype != IMap::eWMS && map.maptype != IMap::eTMS)
    {
        return 0;
    }

    if(tabbar->currentWidget() != toolview)
    {
        return 0;
    }

    QString mapkey = map.getKey();
    IMapSelection * mapSel = 0;
    foreach(mapSel, selectedMaps)
    {
        if(mapSel->mapkey != mapkey)
        {
            continue;
        }

        QRectF r(QPointF(mapSel->lon1, mapSel->lat1), QPointF(mapSel->lon2, mapSel->lat2));
        if(r.contains(lon, lat))
        {
            return mapSel;
        }
    }

    return 0;
}


IMapSelection * CMapDB::getMapSelectionByKey(const QString& key)
{
    if(selectedMaps.contains(key))
    {
        return selectedMaps[key];
    }

    return 0;
}


const CMapDB::map_t& CMapDB::getMapData(const QString& key)
{
    if(knownMaps.contains(key))
    {
        return knownMaps[key];
    }
    return emptyMap;
}


void CMapDB::setMapData(const map_t& map)
{
    map_t m = map;
    if(m.type == IMap::eTMS)
    {
        m.key = QString::number(qHash(m.filename));
    }

    knownMaps[m.key] = m;

    emitSigChanged();
}


void CMapDB::reloadMap()
{
    QString key;
    double  lon = 0;
    double  lat = 0;
    {
        IMap& map  = getMap();
        map.convertPt2Rad(lon, lat);
        key = map.getKey();
        closeVisibleMaps();
        emitSigChanged();
    }
    qApp->processEvents();
    {
        openMap(key);
        IMap& map  = getMap();
        map.convertRad2Pt(lon, lat);
        map.move(QPoint(lon, lat), QPoint(0,0));
        emitSigChanged();
    }
}
