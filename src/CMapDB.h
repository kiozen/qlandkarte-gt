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
#ifndef CMAPDB_H
#define CMAPDB_H

#include "IDB.h"
#include "IMap.h"
#include "CMapNoMap.h"
#include "IMapSelection.h"
#include <QList>
#include <QMap>
#include <QPointer>
#include <QSet>

class QPainter;
class CCanvas;
class CMapNoMap;
class CMapEditWidget;
class CMapSearchWidget;
class CMap3D;

class CMapDB : public IDB
{
    Q_OBJECT;
    public:
        virtual ~CMapDB();

        static CMapDB& self(){return *m_self;}

        struct map_t
        {
            IMap::maptype_e   type;
            QString filename;
            QString description;
            QString key;
            QString copyright;
        };

        /// open a map collection from disc
        void openMap(const QString& filename, bool asRaster ,CCanvas& canvas);
        /// open a known map by it's key
        void openMap(const QString& key);
        /// create a map object by it's key for external use (like overlays)
        IMap * createMap(const QString& key);
        /// close the current map
        void closeMap();

        /// open DEM overlay
        void openDEM(const QString& filename);

        /// get current main map
        IMap& getMap();

        /// get current DEM map
        IMap& getDEM();

        /// get map selection under position
        /**
            Only selections of the current map are evaluated. The current map
            must be a raster map.

            @param lon  the longitude [rad]
            @param lon  the latitude [rad]

            @return A pointer to a selection under the given point or 0.
        */
        IMapSelection * getSelectedMap(double lon, double lat);

        IMapSelection * getMapSelectionByKey(const QString& key);

        CMap3D * getMap3D();

        /// delete known maps by keys
        void delKnownMap(const QStringList& keys);
        /// delete selected maps by keys
        void delSelectedMap(const QStringList& keys);

        void delSelectedMap(const QString& key, bool silent);

        void selSelectedMap(const QString& key);

        /// draw visible maps
        void draw(QPainter& p, const QRect& rect, bool& needsRedraw);

        void loadGPX(CGpx& gpx);
        void saveGPX(CGpx& gpx, const QStringList& keys);

        void loadQLB(CQlb& qlb, bool newKey);
        void saveQLB(CQlb& qlb);

        void upload(const QStringList& keys);
        void download();

        /// remove all selected map areas
        void clear();
        /// create map edit dialog
        void editMap();
        /// create tab with 3D map
        void show3DMap(bool show);
        /// create map search dialog
        void searchMap();

        /// select an area of the map for export [px]
        /**
            @param rect area within the current viewport
        */
        void select(const QRect& rect,  const QMap< QPair<int,int>, bool>& selTiles);

        /// get access to selected map list
        const QMap<QString,IMapSelection*>& getSelectedMaps(){return selectedMaps;}

        bool contains(const QString& key){return knownMaps.contains(key);}

        /// test if map with given key is a built-in map
        bool isBuiltIn(const QString& key){return builtInKeys.contains(key);}

        /// get read access to a map's internal side information
        const map_t& getMapData(const QString& key);

        /// register a map via it's side information structure.
        void setMapData(const map_t& map);

        void reloadMap();

    private:
        friend class CMainWindow;
        friend class CMapToolWidget;
        friend class CMapQMAPExport;

        QDataStream& operator<<(QDataStream&);

        CMapDB(QTabWidget * tb, QObject * parent);

        /// get access to known map dictionary, CMapToolWidget only
        const QMap<QString,map_t>& getKnownMaps(){return knownMaps;}

        void closeVisibleMaps();

        static CMapDB * m_self;

        /// a dictionary of previous opened maps
        QMap<QString,map_t> knownMaps;

        /// the default map if no map is selected
        QPointer<IMap> defaultMap;
        /// the base map
        /**
            The base map will supply the projection.
            All other layers have to use the same projection;
        */
        QPointer<IMap> theMap;

        /// the DEM attached to the map
        QPointer<IMap> demMap;

        /// the map edit widget used to alter and create maps
        QPointer<CMapEditWidget> mapedit;

        /// the 3D view of the map
        QPointer<CMap3D> map3D;

        QPointer<CMapSearchWidget> mapsearch;

        /// list of selected areas on maps
        QMap<QString,IMapSelection*> selectedMaps;

        /// key list of built-in maps
        QSet<QString> builtInKeys;

        map_t emptyMap;
};
#endif                           //CMAPDB_H
