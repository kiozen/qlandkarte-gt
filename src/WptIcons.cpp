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

#include "WptIcons.h"
#include "config.h"
#include "CSettings.h"
#include <QtCore>

const char * wptDefault = ":/icons/iconWaypoint16x16.png";

static QMap<QString, QString> wptIcons;

void initWptIcons()
{

    wptIcons["City (Capitol)"]      = ":/icons/wpt/capitol_city15x15.png";
    wptIcons["City (Large)"]        = ":/icons/wpt/large_city15x15.png";
    wptIcons["City (Medium)"]       = ":/icons/wpt/medium_city15x15.png";
    wptIcons["City (Small)"]        = ":/icons/wpt/small_city15x15.png";
    wptIcons["Small City"]          = ":/icons/wpt/small_city15x15.png";
    wptIcons["Geocache"]            = ":/icons/wpt/geocache15x15.png";
    wptIcons["Geocache Found"]      = ":/icons/wpt/geocache_fnd15x15.png";
    wptIcons["Custom 0"]            = ":/icons/wpt/custom15x15.bmp";
    wptIcons["Custom 1"]            = ":/icons/wpt/custom1.png";
    wptIcons["Custom 2"]            = ":/icons/wpt/custom2.png";
    wptIcons["Custom 3"]            = ":/icons/wpt/custom3.png";
    wptIcons["Custom 4"]            = ":/icons/wpt/custom4.png";
    wptIcons["Custom 5"]            = ":/icons/wpt/custom5.png";
    wptIcons["Custom 6"]            = ":/icons/wpt/custom15x15.bmp";
    wptIcons["Custom 7"]            = ":/icons/wpt/custom15x15.bmp";
    wptIcons["Custom 8"]            = ":/icons/wpt/custom15x15.bmp";
    wptIcons["Custom 9"]            = ":/icons/wpt/custom15x15.bmp";
    wptIcons["Custom 10"]           = ":/icons/wpt/custom15x15.bmp";
    wptIcons["Custom 11"]           = ":/icons/wpt/custom15x15.bmp";
    wptIcons["Custom 12"]           = ":/icons/wpt/custom15x15.bmp";
    wptIcons["Custom 13"]           = ":/icons/wpt/custom15x15.bmp";
    wptIcons["Custom 14"]           = ":/icons/wpt/custom15x15.bmp";
    wptIcons["Custom 15"]           = ":/icons/wpt/custom15x15.bmp";
    wptIcons["Custom 16"]           = ":/icons/wpt/custom15x15.bmp";
    wptIcons["Custom 17"]           = ":/icons/wpt/custom15x15.bmp";
    wptIcons["Custom 18"]           = ":/icons/wpt/custom15x15.bmp";
    wptIcons["Custom 19"]           = ":/icons/wpt/custom15x15.bmp";
    wptIcons["Custom 20"]           = ":/icons/wpt/custom15x15.bmp";
    wptIcons["Custom 21"]           = ":/icons/wpt/custom15x15.bmp";
    wptIcons["Custom 22"]           = ":/icons/wpt/custom15x15.bmp";
    wptIcons["Custom 23"]           = ":/icons/wpt/custom15x15.bmp";
    wptIcons["Flag, Red"]           = ":/icons/wpt/flag_pin_red15x15.png";
    wptIcons["Flag, Blue"]          = ":/icons/wpt/flag_pin_blue15x15.png";
    wptIcons["Flag, Green"]         = ":/icons/wpt/flag_pin_green15x15.png";
    wptIcons["Pin, Red"]            = ":/icons/wpt/pin_red15x15.png";
    wptIcons["Pin, Blue"]           = ":/icons/wpt/pin_blue15x15.png";
    wptIcons["Pin, Green"]          = ":/icons/wpt/pin_green15x15.png";
    wptIcons["Block, Red"]          = ":/icons/wpt/box_red15x15.png";
    wptIcons["Block, Blue"]         = ":/icons/wpt/box_blue15x15.png";
    wptIcons["Block, Green"]        = ":/icons/wpt/box_green15x15.png";
    wptIcons["Blue Diamond"]        = ":/icons/wpt/diamond_blue15x15.png";
    wptIcons["Green Diamond"]       = ":/icons/wpt/diamond_green15x15.png";
    wptIcons["Red Diamond"]         = ":/icons/wpt/diamond_red15x15.png";

    SETTINGS;

    setWptIconByName("Custom 0", cfg.value("garmin/icons/custom0", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 1", cfg.value("garmin/icons/custom1", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 2", cfg.value("garmin/icons/custom2", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 3", cfg.value("garmin/icons/custom3", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 4", cfg.value("garmin/icons/custom4", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 5", cfg.value("garmin/icons/custom5", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 6", cfg.value("garmin/icons/custom6", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 7", cfg.value("garmin/icons/custom7", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 8", cfg.value("garmin/icons/custom8", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 9", cfg.value("garmin/icons/custom9", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 10", cfg.value("garmin/icons/custom10", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 11", cfg.value("garmin/icons/custom11", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 12", cfg.value("garmin/icons/custom12", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 13", cfg.value("garmin/icons/custom13", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 14", cfg.value("garmin/icons/custom14", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 15", cfg.value("garmin/icons/custom15", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 16", cfg.value("garmin/icons/custom16", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 17", cfg.value("garmin/icons/custom17", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 18", cfg.value("garmin/icons/custom18", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 19", cfg.value("garmin/icons/custom19", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 20", cfg.value("garmin/icons/custom20", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 21", cfg.value("garmin/icons/custom21", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 22", cfg.value("garmin/icons/custom22", ":/icons/wpt/custom15x15.bmp").toString());
    setWptIconByName("Custom 23", cfg.value("garmin/icons/custom23", ":/icons/wpt/custom15x15.bmp").toString());

    setWptIconByName("Traditional Cache", ":/icons/cache/Traditional-Cache.png");
    setWptIconByName("Multi-cache", ":/icons/cache/Multi-cache.png");
    setWptIconByName("Unknown Cache", ":/icons/cache/Unknown-Cache.png");
    setWptIconByName("Wherigo Cache", ":/icons/cache/Wherigo-Cache.png");
    setWptIconByName("Event Cache", ":/icons/cache/Event-Cache.png");
    setWptIconByName("Earthcache", ":/icons/cache/Earthcache.png");
    setWptIconByName("Letterbox Hybrid", ":/icons/cache/Letterbox-Hybrid.png");
    setWptIconByName("Virtual Cache", ":/icons/cache/Virtual-Cache.png");
    setWptIconByName("Webcam Cache", ":/icons/cache/Webcam-Cache.png");

    QDir dirIcon(QDir::home().filePath(CONFIGDIR));
    dirIcon.mkdir("WaypointIcons");
    dirIcon.cd("WaypointIcons");

    QString filename;
    QStringList filenames = dirIcon.entryList(QDir::Files);

    foreach(filename, filenames)
    {
        QFileInfo fi(filename);
        QString name = fi.baseName();
        setWptIconByName(name, dirIcon.filePath(filename));
    }
}


QPixmap loadIcon(const QString& path)
{
    QFileInfo finfo(path);
    if(finfo.completeSuffix() != "bmp")
    {
        return QPixmap(path);
    }
    else
    {
        QImage img = QPixmap(path).toImage().convertToFormat(QImage::Format_Indexed8);
        img.setColor(0,qRgba(0,0,0,0));
        return QPixmap::fromImage(img);
    }

    return QPixmap();
}


void setWptIconByName(const QString& name, const QString& filename)
{
    wptIcons[name] = filename;
}


void setWptIconByName(const QString& name, const QPixmap& icon)
{
    QDir dirIcon(QDir::home().filePath(CONFIGDIR "WaypointIcons"));
    QString filename = dirIcon.filePath(name + ".png");

    icon.save(filename);
    wptIcons[name] = filename;
}


QPixmap getWptIconByName(const QString& name, QString * src)
{

    if(wptIcons.contains(name))
    {
        const QString& icon = wptIcons[name];
        if(src) *src = icon;
        return loadIcon(icon);
    }

    return QPixmap(wptDefault);
}


const QMap<QString, QString>& getWptIcons()
{
    return wptIcons;
}
