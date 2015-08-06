/**********************************************************************************************
    Copyright (C) 2014 Oliver Eichler oliver.eichler@gmx.de

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
#include "CExchangeGarmin.h"
#include "WptIcons.h"

#include <QtDBus>
#include <QtXml>
#include <QtGui>
#include <QMessageBox>

CGarminTreeWidgetItem::CGarminTreeWidgetItem(const QString& id, QTreeWidget *parent)
    : IDeviceTreeWidgetItem(id,parent)
{
    setIcon(0, QIcon("://icons/iconDeviceGarmin16x16.png"));

//    Qt::NoItemFlags	0	It does not have any properties set.
//    Qt::ItemIsSelectable	1	It can be selected.
//    Qt::ItemIsEditable	2	It can be edited.
//    Qt::ItemIsDragEnabled	4	It can be dragged.
//    Qt::ItemIsDropEnabled	8	It can be used as a drop target.
//    Qt::ItemIsUserCheckable	16	It can be checked or unchecked by the user.
//    Qt::ItemIsEnabled	32	The user can interact with the item.
//    Qt::ItemIsTristate	64	The item is checkable with three separate states.
    setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
}

void CGarminTreeWidgetItem::readDevice()
{

    QDir dir(mountPoint);
    if(dir.exists("Garmin/GarminDevice.xml"))
    {
        readDeviceXml(dir.absoluteFilePath("Garmin/GarminDevice.xml"));
    }
    else
    {
        QTreeWidgetItem * item = new QTreeWidgetItem(this);
        item->setText(0,QObject::tr("No 'Garmin/GarminDevice.xml' found"));
        item->setIcon(0,QIcon("://icons/iconWarning16x16.png"));
        return;
    }

    QDir dirGpx(dir.absoluteFilePath(pathGpx));
    QStringList subdirs = dirGpx.entryList(QStringList("*"), QDir::Dirs|QDir::NoDotAndDotDot, QDir::Name);
    foreach(const QString& dirname, subdirs)
    {
        new CGarminFolderTreeWidgetItem(dirGpx.absoluteFilePath(dirname), this);

    }

    QStringList files = dirGpx.entryList(QStringList("*.gpx"), QDir::Files|QDir::NoDotAndDotDot, QDir::Name);
    foreach(const QString& filename, files)
    {
        new CGarminFileTreeWidgetItem(dirGpx.absoluteFilePath(filename), this);
    }

}

void CGarminTreeWidgetItem::readDeviceXml(const QString& filename)
{
    QFile file(filename);
    file.open(QIODevice::ReadOnly);

    QDomDocument dom;
    dom.setContent(&file);

    QDomElement device              = dom.firstChildElement("Device");
    QDomElement MassStorageMode     = device.firstChildElement("MassStorageMode");
    const QDomNodeList& DataTypes   = MassStorageMode.elementsByTagName("DataType");

    pathGpx.clear();
    pathSpoiler.clear();
    pathJpeg.clear();
    pathAdventure.clear();

    for(int i = 0; i < DataTypes.size(); i++)
    {
        QDomNode dataType       = DataTypes.at(i);
        QDomElement Name        = dataType.firstChildElement("Name");
        QDomElement File        = dataType.firstChildElement("File");
        QDomElement Location    = File.firstChildElement("Location");
        QDomElement Path        = Location.firstChildElement("Path");

//        qDebug() << Name.text().simplified() << Path.text().simplified();

        QString name = Name.text().simplified();

        if(name == "GPSData")
        {
            pathGpx = Path.text().simplified();
        }
        else if(name == "UserDataSync")
        {
            pathGpx = Path.text().simplified();
        }
        else if(name == "GeotaggedPhotos")
        {
            pathJpeg = Path.text().simplified();
        }
        else if(name == "GeocachePhotos")
        {
            pathSpoiler = Path.text().simplified();
        }
        else if(name == "Adventures")
        {
            pathAdventure = Path.text().simplified();
        }
    }
}
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------
CGarminFolderTreeWidgetItem::CGarminFolderTreeWidgetItem(const QString& path, QTreeWidgetItem *parent)
    : ITreeWidgetItem(parent)
    , dir(path)
{
    QFileInfo fi(path);
    setText(0, fi.baseName());
    setIcon(0, QIcon("://icons/iconFolderOrange16x16.png"));
    setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);

    //    Qt::NoItemFlags	0	It does not have any properties set.
    //    Qt::ItemIsSelectable	1	It can be selected.
    //    Qt::ItemIsEditable	2	It can be edited.
    //    Qt::ItemIsDragEnabled	4	It can be dragged.
    //    Qt::ItemIsDropEnabled	8	It can be used as a drop target.
    //    Qt::ItemIsUserCheckable	16	It can be checked or unchecked by the user.
    //    Qt::ItemIsEnabled	32	The user can interact with the item.
    //    Qt::ItemIsTristate	64	The item is checkable with three separate states.
    setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);

}

void CGarminFolderTreeWidgetItem::read()
{
    QList<QTreeWidgetItem *> children =	takeChildren();
    qDeleteAll(children);

    QStringList subdirs = dir.entryList(QStringList("*"), QDir::Dirs|QDir::NoDotAndDotDot, QDir::Name);
    foreach(const QString& dirname, subdirs)
    {
        new CGarminFolderTreeWidgetItem(dir.absoluteFilePath(dirname), this);

    }

    QStringList files = dir.entryList(QStringList("*.gpx"), QDir::Files|QDir::NoDotAndDotDot, QDir::Name);
    foreach(const QString& filename, files)
    {
        new CGarminFileTreeWidgetItem(dir.absoluteFilePath(filename), this);
    }

}

// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------
CGarminFileTreeWidgetItem::CGarminFileTreeWidgetItem(const QString &path, QTreeWidgetItem *parent)
    : ITreeWidgetItem(parent)
    , filename(path)
{
    QFileInfo fi(path);
    setText(0, fi.baseName());
    setIcon(0, QIcon("://icons/textjustify.png"));
    setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);

    //    Qt::NoItemFlags	0	It does not have any properties set.
    //    Qt::ItemIsSelectable	1	It can be selected.
    //    Qt::ItemIsEditable	2	It can be edited.
    //    Qt::ItemIsDragEnabled	4	It can be dragged.
    //    Qt::ItemIsDropEnabled	8	It can be used as a drop target.
    //    Qt::ItemIsUserCheckable	16	It can be checked or unchecked by the user.
    //    Qt::ItemIsEnabled	32	The user can interact with the item.
    //    Qt::ItemIsTristate	64	The item is checkable with three separate states.
    setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled|Qt::ItemIsDragEnabled|Qt::ItemIsDropEnabled);

}

void CGarminFileTreeWidgetItem::read()
{
    int N;
    QList<QTreeWidgetItem *> children =	takeChildren();
    qDeleteAll(children);
    children.clear();

    QFile file(filename);
    file.open(QIODevice::ReadOnly);
    QDomDocument xml;

    QString msg;
    int line;
    int column;
    if(!xml.setContent(&file, false, &msg, &line, &column))
    {
        file.close();
        QMessageBox::critical(0, QObject::tr("Failed to read..."), QObject::tr("Failed to read: %1\nline %2, column %3:\n %4").arg(filename).arg(line).arg(column).arg(msg), QMessageBox::Abort);
        return;
    }
    file.close();

    const QDomElement& gpx = xml.documentElement();
    if(gpx.tagName() != "gpx")
    {
        QMessageBox::critical(0, QObject::tr("Failed to read..."), QObject::tr("Not a GPX file: ") + filename, QMessageBox::Abort);
        return;
    }

    const QDomNodeList& waypoints = gpx.elementsByTagName("wpt");
    N = waypoints.count();
    for(int n = 0; n < N; ++n)
    {
        QString name, sym, str;
        const QDomNode& waypoint = waypoints.item(n);

        QTextStream s(&str, QIODevice::WriteOnly);
        waypoint.save(s,2);

        if(waypoint.namedItem("name").isElement())
        {
            name = waypoint.namedItem("name").toElement().text();
        }

        if(waypoint.namedItem("sym").isElement())
        {
            sym = waypoint.namedItem("sym").toElement().text();

        }

        QTreeWidgetItem * item = new QTreeWidgetItem();
        item->setText(0, name);
        item->setIcon(0, getWptIconByName(sym));
        item->setData(0, Qt::UserRole,str);

        //    Qt::NoItemFlags	0	It does not have any properties set.
        //    Qt::ItemIsSelectable	1	It can be selected.
        //    Qt::ItemIsEditable	2	It can be edited.
        //    Qt::ItemIsDragEnabled	4	It can be dragged.
        //    Qt::ItemIsDropEnabled	8	It can be used as a drop target.
        //    Qt::ItemIsUserCheckable	16	It can be checked or unchecked by the user.
        //    Qt::ItemIsEnabled	32	The user can interact with the item.
        //    Qt::ItemIsTristate	64	The item is checkable with three separate states.
        item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled|Qt::ItemIsDragEnabled);


        children << item;
    }

    const QDomNodeList& tracks = gpx.elementsByTagName("trk");
    N = tracks.count();
    for(int n = 0; n < N; ++n)
    {
        QString name, str;
        const QDomNode& track = tracks.item(n);

        QTextStream s(&str, QIODevice::WriteOnly);
        track.save(s,2);

        if(track.namedItem("name").isElement())
        {
            name = track.namedItem("name").toElement().text();
        }

        QTreeWidgetItem * item = new QTreeWidgetItem();
        item->setText(0, name);
        item->setIcon(0, QIcon("://icons/iconTrack16x16.png"));
        item->setData(0, Qt::UserRole,str);

        //    Qt::NoItemFlags	0	It does not have any properties set.
        //    Qt::ItemIsSelectable	1	It can be selected.
        //    Qt::ItemIsEditable	2	It can be edited.
        //    Qt::ItemIsDragEnabled	4	It can be dragged.
        //    Qt::ItemIsDropEnabled	8	It can be used as a drop target.
        //    Qt::ItemIsUserCheckable	16	It can be checked or unchecked by the user.
        //    Qt::ItemIsEnabled	32	The user can interact with the item.
        //    Qt::ItemIsTristate	64	The item is checkable with three separate states.
        item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled|Qt::ItemIsDragEnabled);

        children << item;
    }

    const QDomNodeList& routes = gpx.elementsByTagName("rte");
    N = routes.count();
    for(int n = 0; n < N; ++n)
    {
        QString name, str;
        const QDomNode& route = routes.item(n);

        QTextStream s(&str, QIODevice::WriteOnly);
        route.save(s,2);

        if(route.namedItem("name").isElement())
        {
            name = route.namedItem("name").toElement().text();
        }

        QTreeWidgetItem * item = new QTreeWidgetItem();
        item->setText(0, name);
        item->setIcon(0, QIcon("://icons/iconRoute16x16.png"));
        item->setData(0, Qt::UserRole,str);

        //    Qt::NoItemFlags	0	It does not have any properties set.
        //    Qt::ItemIsSelectable	1	It can be selected.
        //    Qt::ItemIsEditable	2	It can be edited.
        //    Qt::ItemIsDragEnabled	4	It can be dragged.
        //    Qt::ItemIsDropEnabled	8	It can be used as a drop target.
        //    Qt::ItemIsUserCheckable	16	It can be checked or unchecked by the user.
        //    Qt::ItemIsEnabled	32	The user can interact with the item.
        //    Qt::ItemIsTristate	64	The item is checkable with three separate states.
        item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled|Qt::ItemIsDragEnabled);

        children << item;
    }

    addChildren(children);
}

void CGarminFileTreeWidgetItem::save()
{
    QFile file(filename);
    file.open(QIODevice::ReadOnly);
    QDomDocument xml;

    qDebug() << "void CGarminFileTreeWidgetItem::save()";

    QString msg;
    int line;
    int column;
    if(!xml.setContent(&file, false, &msg, &line, &column))
    {
        file.close();
        //QMessageBox::critical(0, QObject::tr("Failed to read..."), QObject::tr("Failed to read: %1\nline %2, column %3:\n %4").arg(filename).arg(line).arg(column).arg(msg), QMessageBox::Abort);
        return;
    }
    file.close();

    QDomElement gpx = xml.documentElement();
    if(gpx.tagName() != "gpx")
    {
        //QMessageBox::critical(0, QObject::tr("Failed to read..."), QObject::tr("Not a GPX file: ") + filename, QMessageBox::Abort);
        return;
    }

    QDomNodeList children = gpx.childNodes();
    for(int i = (children.count() - 1); i >= 0; i--)
    {
        QDomNode child = children.at(i);
        QString tag = child.toElement().tagName();
        if(tag == "trk" || tag == "wpt" || tag == "rte")
        {
            gpx.removeChild(child);
        }
    }

    for(int c = 0;  c < childCount(); c++)
    {
        QTreeWidgetItem * item = child(c);
        QString str = item->data(0,Qt::UserRole).toString();
        QDomDocument tmp;
        tmp.setContent(str);
        gpx.appendChild(tmp.documentElement());
    }

    file.open(QIODevice::WriteOnly);
    QTextStream out(&file);
    out.setCodec("UTF-8");
    xml.save(out,2);
    file.close();

}

// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------
CGarminAdventureTreeWidgetItem::CGarminAdventureTreeWidgetItem(const QString &path, QTreeWidgetItem *parent)
    : ITreeWidgetItem(parent)
{
    setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
}

// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------
CExchangeGarmin::CExchangeGarmin(QTreeWidget * treeWidget, QObject * parent)
    : IExchange("Garmin", treeWidget,parent)
{

}

CExchangeGarmin::~CExchangeGarmin()
{

}

void CExchangeGarmin::slotDeviceAdded(const QDBusObjectPath& path, const QVariantMap& map)
{
#ifdef Q_OS_LINUX
//    qDebug() << "-----------CExchangeGarmin::slotDeviceAdded----------";
//    qDebug() << path.path() << map;

    QString device = checkForDevice(path);
    if(!device.isEmpty())
    {
        CGarminTreeWidgetItem * item = new CGarminTreeWidgetItem(path.path(), treeWidget);
        item->setText(0, device);

    }
#endif //Q_OS_LINUX
}

void CExchangeGarmin::slotItemExpanded(QTreeWidgetItem * item)
{
    CGarminFolderTreeWidgetItem * folder = dynamic_cast<CGarminFolderTreeWidgetItem*>(item);
    if(folder)
    {
        folder->read();
        return;
    }

    CGarminFileTreeWidgetItem * file = dynamic_cast<CGarminFileTreeWidgetItem*>(item);
    if(file)
    {
        file->read();
        return;
    }

    IExchange::slotItemExpanded(item);
}

void CExchangeGarmin::slotItemCollapsed(QTreeWidgetItem * item)
{
    CGarminFileTreeWidgetItem * file = dynamic_cast<CGarminFileTreeWidgetItem*>(item);
    if(file)
    {
        file->save();
        return;
    }

    IExchange::slotItemCollapsed(item);
}

