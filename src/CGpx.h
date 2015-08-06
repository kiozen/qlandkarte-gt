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
#ifndef CGPX_H
#define CGPX_H

#include <QObject>
#include <QMap>
#include <QColor>
#include <QString>
#include <QtXml/QDomDocument>

/// handle geo data from GPX files
class CGpx : public QObject, public QDomDocument
{
    Q_OBJECT;

    public:
        // Those are standard GPX/XML namespaces
        static const QString gpx_ns;
        static const QString xsi_ns;

        // Those are the URIs of the GPX extensions we support
        static const QString gpxx_ns;
        static const QString gpxtpx_ns;
        static const QString wptx1_ns;
        static const QString rmc_ns;
        static const QString ql_ns;
        static const QString gs_ns;

        enum gpx_version
        {
            qlVer_foreign,       // file was not created by QLandkarteGT
            qlVer_1_0,           // file uses old, non XSD-compatible extensions
            qlVer_1_1            // file uses new, XSD-compatible extensions
        };

        enum exportMode_e
        {
            eQlgtExport
            , eCleanExport
            , eOcmExport
            , eBackupExport
            , eMagellan
        };

    public:
        CGpx(QObject * parent, exportMode_e mode);
        virtual ~CGpx();

        void load(const QString& filename);
        void save(const QString& filename);
        void makeExtensions();

        static QMap<QString,QDomElement> mapChildElements(const QDomNode&
                                                          parent);

        const QMap<QString, QColor>& getColorMap() const;
        const QMap<QString, int>& getTrackColorMap() const;

        QDomElement &getExtensions() { return extensions; }
        gpx_version version() { return file_version; }
        exportMode_e getExportMode() { return exportMode; }

    protected:
        void writeMetadata();

        QMap<QString, QColor> colorMap;
        QMap<QString, int> trackColorMap;

    private:
        QDomElement extensions;
        gpx_version file_version;
        exportMode_e exportMode;
};
#endif                           //CGPX_H
