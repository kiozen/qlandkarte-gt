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
#include "CGpx.h"
#include "CTrack.h"
#include "version.h"

#include <QtCore>
#include <QMessageBox>
#include <iostream>

const QString CGpx::gpx_ns      = "http://www.topografix.com/GPX/1/1";
const QString CGpx::xsi_ns      = "http://www.w3.org/2001/XMLSchema-instance";
const QString CGpx::gpxx_ns     = "http://www.garmin.com/xmlschemas/GpxExtensions/v3";
const QString CGpx::gpxtpx_ns   = "http://www.garmin.com/xmlschemas/TrackPointExtension/v1";
const QString CGpx::wptx1_ns    = "http://www.garmin.com/xmlschemas/WaypointExtension/v1";
const QString CGpx::rmc_ns      = "urn:net:trekbuddy:1.0:nmea:rmc";
const QString CGpx::ql_ns       = "http://www.qlandkarte.org/xmlschemas/v1.1";
const QString CGpx::gs_ns       = "http://www.groundspeak.com/cache/1/0";

uint qHash(QColor color)
{
    return qHash(color.rgba());
}


CGpx::CGpx(QObject * parent, exportMode_e mode)
: QObject(parent)
, QDomDocument()
, exportMode(mode)
{

    writeMetadata();

    colorMap.insert("Black",       QColor(Qt::black));
    colorMap.insert("DarkRed",     QColor(Qt::darkRed));
    colorMap.insert("DarkGreen",   QColor(Qt::darkGreen));
    colorMap.insert("DarkYellow",  QColor(Qt::darkYellow));
    colorMap.insert("DarkBlue",    QColor(Qt::darkBlue));
    colorMap.insert("DarkMagenta", QColor(Qt::darkMagenta));
    colorMap.insert("DarkCyan",    QColor(Qt::darkCyan));
    colorMap.insert("LightGray",   QColor(Qt::gray));
    colorMap.insert("DarkGray",    QColor(Qt::darkGray));
    colorMap.insert("Red",         QColor(Qt::red));
    colorMap.insert("Green",       QColor(Qt::green));
    colorMap.insert("Yellow",      QColor(Qt::yellow));
    colorMap.insert("Blue",        QColor(Qt::blue));
    colorMap.insert("Magenta",     QColor(Qt::magenta));
    colorMap.insert("Cyan",        QColor(Qt::cyan));
    colorMap.insert("White",       QColor(Qt::white));
    colorMap.insert("Transparent", QColor(Qt::transparent));

    for (int i=0;;++i)
    {
        QColor trackColor = CTrack::lineColors[i];
        QString colorName = colorMap.key(trackColor);
        if (!colorName.isEmpty()) trackColorMap.insert(colorName, i);
        if (trackColor == Qt::transparent) break;
    }
}


CGpx::~CGpx()
{

}


const QMap<QString, QColor>& CGpx::getColorMap() const
{
    return colorMap;
}


const QMap<QString, int>& CGpx::getTrackColorMap() const
{
    return trackColorMap;
}


void CGpx::writeMetadata()
{
    QDomElement root = createElement("gpx");
    appendChild(root);
    root.setAttribute("version","1.1");
    root.setAttribute("creator","QLandkarteGT " VER_STR " http://www.qlandkarte.org/");
    root.setAttribute("xmlns",gpx_ns);
    root.setAttribute("xmlns:xsi",xsi_ns);
    root.setAttribute("xmlns:gpxx",gpxx_ns);
    root.setAttribute("xmlns:gpxtpx",gpxtpx_ns);
    root.setAttribute("xmlns:wptx1",wptx1_ns);
    root.setAttribute("xmlns:rmc",rmc_ns);
    //if (exportMode == eQlgtExport)
    {
        root.setAttribute("xmlns:ql",ql_ns);
    }

    QString schemaLocation = QString()
        + gpx_ns    + " http://www.topografix.com/GPX/1/1/gpx.xsd "
        + gpxx_ns   + " http://www.garmin.com/xmlschemas/GpxExtensionsv3.xsd "
        + gpxtpx_ns + " http://www.garmin.com/xmlschemas/TrackPointExtensionv1.xsd "
        + wptx1_ns  + " http://www.garmin.com/xmlschemas/WaypointExtensionv1.xsd";

    //if (exportMode == eQlgtExport)
    {
        schemaLocation += " ";
        schemaLocation += ql_ns;
        schemaLocation += " http://www.qlandkarte.org/xmlschemas/v1.1/ql-extensions.xsd";
    }
    root.setAttribute("xsi:schemaLocation", schemaLocation);

    QDomElement metadata = createElement("metadata");
    root.appendChild(metadata);

    QDomElement time = createElement("time");
    metadata.appendChild(time);
    QDomText _time_ = createTextNode(QDateTime::currentDateTime().toUTC().toString("yyyy-MM-dd'T'hh:mm:ss'Z'"));
    time.appendChild(_time_);
}


void CGpx::makeExtensions()
{
    QDomElement root = documentElement();

    extensions = createElement("extensions");
    root.appendChild(extensions);
}


void CGpx::save(const QString& filename)
{
    QFile file(filename);

    if(file.exists() && (exportMode == eQlgtExport))
    {
        CGpx gpx(0,exportMode);
        try
        {
            gpx.load(filename);
            const  QDomElement& docElem = gpx.documentElement();
            const QDomNamedNodeMap& attr = docElem.attributes();
            if(!attr.namedItem("creator").nodeValue().startsWith("QLandkarteGT"))
            {
                throw tr("bad application");
            }
        }
        catch(const QString&)
        {
            int res = QMessageBox::warning(0,tr("File exists ...")
                ,tr("The file exists and it has not been created by QLandkarte GT. "
                "If you press 'yes' all data in this file will be lost. "
                "Even if this file contains GPX data, QLandkarte GT might not "
                "load and store all elements of this file. Those elements "
                "will be lost. I recommend to use another file. "
                "<b>Do you really want to overwrite the file?</b>")
                ,QMessageBox::Yes|QMessageBox::No,QMessageBox::No);
            if(res == QMessageBox::No)
            {
                return;
            }
        }
    }

    if(!file.open(QIODevice::WriteOnly))
    {
        throw tr("Failed to create %1").arg(filename);
    }
    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>" << endl;;
    out << toString();
    file.close();
    if(file.error() != QFile::NoError)
    {
        throw tr("Failed to write %1").arg(filename);
        return;
    }
}


void CGpx::load(const QString& filename)
{
    clear();
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly))
    {
        throw tr("Failed to open: ") + filename;
    }

    QString msg;
    int line;
    int column;
    if(!setContent(&file, true, &msg, &line, &column))
    {
        file.close();
        throw tr("Failed to read: %1\nline %2, column %3:\n %4").arg(filename).arg(line).arg(column).arg(msg);
    }
    file.close();

    const  QDomElement& docElem = documentElement();
    if(docElem.tagName() != "gpx")
    {
        throw tr("Not a GPX file: ") + filename;
    }

    if (!docElem.hasAttribute("creator"))
    {
        //throw tr("GPX schema violation: no \"creator\" attribute.");
        qWarning() << tr("GPX schema violation: no 'creator' attribute.");
    }

    QString creator = docElem.attribute("creator");
    if (creator.startsWith("QLandkarte"))
    {
        // QLandkarteGT file

        // Test whether this is an old or new file.  New files use
        // "QLandkarteGT <versionnummber> http://www.qlandkarte.org/"
        // as creator string, old files use only "QLandkarteGT".
        // Very old files use just "QLandkarte".
        if (creator == "QLandkarteGT" || creator == "QLandkarte")
        {
            file_version = qlVer_1_0;
            qDebug() << "CGpx::load(): Detected old" << creator
                << "format, using compat mode";
        }
        else
        {
            file_version = qlVer_1_1;
            qDebug() << "CGpx::load(): Detected new QLandkarteGT format";
        }
    }
    else
    {
        // Foreign GPX file
        file_version = qlVer_foreign;
        qDebug() << "CGpx::load(): Detected foreign GPX format";
    }
}


QMap<QString,QDomElement> CGpx::mapChildElements(const QDomNode& parent)
{
    // I tried to use QDomNamedNodeMap first, but it did not work. After
    // setNamedItem(child) the size() remained 0. XML support in QT sucks.

    QMap<QString,QDomElement> map;

    QDomNode child = parent.firstChild();
    while (!child.isNull())
    {
        if (child.isElement())
        {
            if (child.prefix().isEmpty())
            {
                map.insert(child.nodeName(), child.toElement());
            }
            else
            {
                map.insert(child.namespaceURI()+":"+child.localName(), child.toElement());
                qDebug() << (child.namespaceURI()+":"+child.localName());
            }
        }
        child = child.nextSibling();
    }

    return map;
}
