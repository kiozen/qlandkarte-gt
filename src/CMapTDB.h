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
#ifndef CMAPTDB_H
#define CMAPTDB_H

#include "IMap.h"
#include "CGarminTile.h"
#include "IGarminTyp.h"
#include <QMap>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QFontMetrics>

class QTimer;
class QTextDocument;

class QCheckBox;
class QComboBox;

class CMapTDB : public IMap
{
    Q_OBJECT;
    public:
        CMapTDB(const QString& key, const QString& filename, CCanvas * parent);
        CMapTDB(const QString& key, const QString& filename);
        virtual ~CMapTDB();

        void convertPt2M(double& u, double& v);
        void convertM2Pt(double& u, double& v);
        void convertM2Pt(double* u, double* v, int n);
        void move(const QPoint& old, const QPoint& next);
        void zoom(bool zoomIn, const QPoint& p);
        void zoom(double lon1, double lat1, double lon2, double lat2);
        void zoom(qint32& level);
        void dimensions(double& lon1, double& lat1, double& lon2, double& lat2);
        const QString& getName(){return name;}
        void draw(QPainter& p);
        void draw();
        void draw(const QSize& s, bool needsRedraw, QPainter& p);
        void getArea_n_Scaling(projXY& p1, projXY& p2, float& my_xscale, float& my_yscale);
        void registerDEM(CMapDEM& dem);
        void select(IMapSelection& ms, const QRect& rect);
        void getClosePolyline(QPoint& pt1, QPoint& pt2, qint32 threshold, QPolygon& line);

        QString getCopyright();
        QString getMapLevelInfo();
        QString getLegendLines();
        QString getLegendArea();
        QString getLegendPoints();

        void config();
    protected:
        virtual void convertRad2Pt(double* u, double* v, int n);
        void resize(const QSize& s);
        bool eventFilter( QObject * watched, QEvent * event );

    private slots:
        void slotPoiLabels(bool checked);
        void slotNightView(bool checked);
        void slotDetailChanged(int idx);
        void slotLanguageChanged(int idx);
        void slotTypfileChanged(int idx);
        void slotToolTip();

    private:
        friend class CDlgMapTDBConfig;
        void setup();
        void checkTypFiles();
        void checkMdrFile();

        struct strlbl_t
        {
            strlbl_t() : type(IGarminTyp::eStandard){}

            QPoint  pt;
            QRect   rect;
            QString str;
            IGarminTyp::label_type_e type;
        };

        struct typ_section_t
        {
            typ_section_t() : dataOffset(0), dataLength(0), arrayOffset(0), arrayModulo(0), arraySize(0){}
            quint32  dataOffset;
            quint32  dataLength;
            quint32  arrayOffset;
            quint16  arrayModulo;
            quint32  arraySize;
        } ;

        void readTDB(const QString& filename);
        void readTYP();
        bool processPrimaryMapData();
        void drawPolylines(QPainter& p, polytype_t& lines);
        void drawPolygons(QPainter& p, polytype_t& lines);
        void drawPoints(QPainter& p, pointtype_t& points, QVector<QRect>& rectPois);
        void drawPois(QPainter& p, pointtype_t& points, QVector<QRect>& rectPois);
        void drawLabels(QPainter& p, const QVector<strlbl_t> &lbls);
        void drawText(QPainter& p);
        void drawInfo(QPainter& p);

        void getInfoPoints(const QPoint& pt, QMultiMap<QString, QString>& dict);
        void getInfoPois(const QPoint& pt, QMultiMap<QString, QString>& dict);
        void getInfoPolygons(const QPoint& pt, QMultiMap<QString, QString>& dict);
        void getInfoPolylines(QPoint& pt, QMultiMap<QString, QString>& dict);
        void collectText(const CGarminPolygon& item, const QPolygonF& line, const QFont& font, const QFontMetricsF& metrics, qint32 lineWidth);

        void drawLine(QPainter& p, CGarminPolygon& l, const IGarminTyp::polyline_property& property, const QFontMetricsF& metrics, const QFont& font);
        void drawLine(QPainter& p, const CGarminPolygon& l);

        void simplifyPolyline(QPolygonF & line) const;
        void simplifyPolyline(QPolygonF::iterator begin, QPolygonF::iterator end) const;

        QString createLegendString(const QMap<int,QString>& strings);

#pragma pack(1)
        struct tdb_hdr_t
        {
            quint8  type;
            quint16 size;
        };

        struct tdb_product_t : public tdb_hdr_t
        {
            quint32 id;
            quint16 version;
            char *  name[1];
        };

        struct tdb_map_t : public tdb_hdr_t
        {
            quint32 id;
            quint32 country;
            qint32  north;
            qint32  east;
            qint32  south;
            qint32  west;
            char    name[1];
        };

        struct tdb_map_size_t
        {
            quint16 dummy;
            quint16 count;
            quint32 sizes[1];
        };

        struct tdb_copyright_t
        {
            quint8  type;
            quint16 count;
            quint8  flag;
            char    str[1];
        };

        struct tdb_copyrights_t : public tdb_hdr_t
        {
            tdb_copyright_t entry;
        };
#ifdef WIN32
#pragma pack()
#else
#pragma pack(0)
#endif

        struct tile_t
        {
            tile_t() : img(0), selected(false){}
            quint32 id;
            QString key;
            QString name;
            //             std::string cname;
            QString file;
            double north;
            double east;
            double south;
            double west;
            QRectF area;
            //             QVector<projXY> definitionArea;
            CGarminTile * img;
            quint32 memSize;

            QPolygonF       defArea;
            QVector<double> defAreaU;
            QVector<double> defAreaV;

            bool selected;
        };

        struct map_level_t
        {
            quint8 bits;
            quint8 level;
            bool useBaseMap;

            bool operator==(const map_level_t &ml)  const
            {
                if (ml.bits != bits || ml.level != level || ml.useBaseMap != useBaseMap)
                    return false;
                else
                    return true;
            }

            static bool GreaterThan(const map_level_t &ml1, const map_level_t &ml2)
            {
                return ml1.bits < ml2.bits;
            }
        };

        /// scale entry
        struct scale_t
        {
            /// scale name
            QString label;
            /// scale factor
            double scale;
            /// minimum bits required for this resolution
            quint32 bits;
        };

        void readTile(tile_t& tile);

        bool tainted;

        /// tdb filename
        QString filename;
        /// typ filename
        QString typfile;
        /// mdr filename
        QString mdrfile;
        /// copyright string
        QString copyright;
        /// map collection name
        QString name;
        /// basemap filename
        QString basemap;
        /// north boundary of basemap []
        double north;
        /// east boundary of basemap []
        double east;
        /// south boundary of basemap []
        double south;
        /// west boundary of basemap []
        double west;
        /// the area in [m] covered by the basemap
        QRectF area;
        /// the unlock key
        QString mapkey;
        /// the basemap tile;
        CGarminTile * baseimg;
        /// high detail map tiles
        QMap<QString,tile_t> tiles;
        /// combined maplevels of basemap & submap tiles
        QVector<map_level_t> maplevels;
        /// flag for transparent maps
        bool isTransparent;

        /// different scale entries indexed by idxZoom,
        static scale_t scales[];
        /// the used scale
        double zoomFactor;
        /// top left corner as long / lat [rad]
        projXY topLeft;
        /// top bottom right as long / lat [rad]
        projXY bottomRight;

        QMap<quint32, IGarminTyp::polyline_property> polylineProperties;
        QMap<quint32, IGarminTyp::polygon_property> polygonProperties;
        QList<quint32> polygonDrawOrder;
        QMap<quint32, IGarminTyp::point_property> pointProperties;

        polytype_t polygons;
        polytype_t polylines;
        pointtype_t points;
        pointtype_t pois;

        QFontMetrics      fm;
        QVector<strlbl_t> labels;

        QTextDocument * info;
        QString         infotext;
        QPoint          topLeftInfo;

        QPoint          pointFocus;
        QPoint          pointMouse;

        int detailsFineTune;

        quint16 fid;
        quint16 pid;

        struct textpath_t
        {
            //            QPainterPath    path;
            QPolygonF       polyline;
            QString         text;
            QFont           font;
            QVector<qreal>  lengths;
            qint32          lineWidth;
            qint32          textOffset;
        };

        QVector<textpath_t> textpaths;

        QVector<CGarminPolygon> query1;
        QVector<CGarminPoint> query2;

        double lon_factor;
        double lat_factor;

        bool useTyp;
        bool textAboveLine;

        bool poiLabels;
        QCheckBox * checkPoiLabels;
        bool nightView;
        QCheckBox * checkNightView;

        QComboBox * comboDetails;

        QMap<quint8, QString> languages;

        QComboBox * comboLanguages;

        qint8 selectedLanguage;

        QComboBox * comboTypfiles;

        QTimer * toolTipTimer;

};
#endif                           //CMAPTDB_H
