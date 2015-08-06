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

#include "IExchange.h"
#include <QtGui>
#include <QtDBus>
#include <QMetaType>


IDeviceTreeWidgetItem::IDeviceTreeWidgetItem(const QString& id, QTreeWidget *parent)
    : ITreeWidgetItem(parent)
    , id(id)
{
    setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
}

void IDeviceTreeWidgetItem::readMountPoint()
{
#ifdef Q_OS_LINUX
    QStringList points;
    QDBusMessage message = QDBusMessage::createMethodCall("org.freedesktop.UDisks2",
                                                          id,
                                                          "org.freedesktop.DBus.Properties",
                                                          "Get");

    QList<QVariant> args;
    args << "org.freedesktop.UDisks2.Filesystem" << "MountPoints";
    message.setArguments(args);

    QDBusMessage reply = QDBusConnection::systemBus().call(message);

    QList<QByteArray> l;
    foreach (QVariant arg, reply.arguments())
    {
        arg.value<QDBusVariant>().variant().value<QDBusArgument>() >> l;
    }

    foreach (QByteArray p, l)
    {
        points.append(p);
    }

    if(!points.isEmpty())
    {
        setIcon(1,QIcon("://icons/iconEject16x16.png"));
        mountPoint = points[0];
    }
    else
    {
        mountPoint.clear();
        setIcon(1,QIcon());
    }
#endif //Q_OS_LINUX
}

void IDeviceTreeWidgetItem::mount()
{
#ifdef Q_OS_LINUX
//    qDebug() << "IDeviceTreeWidgetItem::mount()";
    readMountPoint();

    QDBusMessage message = QDBusMessage::createMethodCall("org.freedesktop.UDisks2",id,"org.freedesktop.UDisks2.Filesystem","Mount");
    QVariantMap args;
    args.insert("options", "flush");
    message << args;
    QDBusConnection::systemBus().call(message);

    readMountPoint();
#endif //Q_OS_LINUX
}

void IDeviceTreeWidgetItem::unmount()
{
#ifdef Q_OS_LINUX
//    qDebug() << "IDeviceTreeWidgetItem::unmount()";
    QDBusMessage message = QDBusMessage::createMethodCall("org.freedesktop.UDisks2",id,"org.freedesktop.UDisks2.Filesystem","Unmount");
    QVariantMap args;
    message << args;
    QDBusConnection::systemBus().call(message);

    readMountPoint();
#endif //Q_OS_LINUX
}



IExchange::IExchange(const QString& vendor, QTreeWidget *treeWidget, QObject *parent)
    : QObject(parent)
    , vendor(vendor)
    , treeWidget(treeWidget)
{
#ifdef Q_OS_LINUX
    QDBusConnection::systemBus().connect("org.freedesktop.UDisks2",
                   "/org/freedesktop/UDisks2",
                   "org.freedesktop.DBus.ObjectManager",
                   "InterfacesAdded",
                   this,
                   SLOT(slotDeviceAdded(QDBusObjectPath,QVariantMap)));

    QDBusConnection::systemBus().connect("org.freedesktop.UDisks2",
                   "/org/freedesktop/UDisks2",
                   "org.freedesktop.DBus.ObjectManager",
                   "InterfacesRemoved",
                   this,
                   SLOT(slotDeviceRemoved(QDBusObjectPath,QStringList)));

    QTimer::singleShot(1000, this, SLOT(slotUpdate()));
#endif //Q_OS_LINUX
    connect(treeWidget, SIGNAL(itemExpanded(QTreeWidgetItem*)), this, SLOT(slotItemExpanded(QTreeWidgetItem*)));
    connect(treeWidget, SIGNAL(itemCollapsed(QTreeWidgetItem*)), this, SLOT(slotItemCollapsed(QTreeWidgetItem*)));
}

IExchange::~IExchange()
{

}

QString IExchange::checkForDevice(const QDBusObjectPath& path)
{
#ifdef Q_OS_LINUX
    QString model;
    QDBusInterface * blockIface = 0;
    QDBusInterface * driveIface = 0;

    try
    {
        // ignore all path that are no block devices
        if(!path.path().startsWith("/org/freedesktop/UDisks2/block_devices/"))
        {
            throw QString();
        }

        // create path of to drive the block device belongs to
        blockIface = new QDBusInterface("org.freedesktop.UDisks2", path.path(), "org.freedesktop.UDisks2.Block", QDBusConnection::systemBus(), this);
        Q_ASSERT(blockIface);
        QDBusObjectPath drive_object = blockIface->property("Drive").value<QDBusObjectPath>();


        // read vendor string attached to drive
        driveIface = new QDBusInterface("org.freedesktop.UDisks2", drive_object.path(),"org.freedesktop.UDisks2.Drive", QDBusConnection::systemBus(), this);
        Q_ASSERT(driveIface);
        QString _vendor_ = driveIface->property("Vendor").toString();

        // vendor must match
        if(_vendor_.toUpper() != vendor.toUpper())
        {
            throw QString();
        }

        // single out block devices with attached filesystem
        // devices with partitions will have a block device for the drive and one for each partition
        // we are interested in the ones with filesystem as they are the ones to mount
        QDBusMessage call = QDBusMessage::createMethodCall("org.freedesktop.UDisks2", path.path(), "org.freedesktop.DBus.Introspectable", "Introspect");
        QDBusPendingReply<QString> reply = QDBusConnection::systemBus().call(call);

        QXmlStreamReader xml(reply.value());
        while (!xml.atEnd())
        {
            xml.readNext();
            if (xml.tokenType() == QXmlStreamReader::StartElement && xml.name().toString() == "interface" )
            {

                QString name = xml.attributes().value("name").toString();
                if(name == "org.freedesktop.UDisks2.Filesystem")
                {
                    throw  driveIface->property("Model").toString();
                }
            }
        }
    }
    catch(const QString& str)
    {
        model = str;
    }
    delete driveIface;
    delete blockIface;
    return model;
#else // Q_OS_LINUX
    return "";
#endif // Q_OS_LINUX
}

void IExchange::slotUpdate()
{
#ifdef Q_OS_LINUX
    QList<QDBusObjectPath> paths;
    QDBusMessage call = QDBusMessage::createMethodCall("org.freedesktop.UDisks2",
                                                       "/org/freedesktop/UDisks2/block_devices",
                                                       "org.freedesktop.DBus.Introspectable",
                                                       "Introspect");
    QDBusPendingReply<QString> reply = QDBusConnection::systemBus().call(call);

    if (!reply.isValid())
    {
        qWarning("UDisks2Manager: error: %s", qPrintable(reply.error().name()));
        return;
    }

    QXmlStreamReader xml(reply.value());
    while (!xml.atEnd())
    {
        xml.readNext();
        if (xml.tokenType() == QXmlStreamReader::StartElement && xml.name().toString() == "node" )
        {
            QString name = xml.attributes().value("name").toString();
            if(!name.isEmpty())
                paths << QDBusObjectPath("/org/freedesktop/UDisks2/block_devices/" + name);
        }
    }

    foreach (QDBusObjectPath i, paths)
    {
        slotDeviceAdded(i, QVariantMap());
    }
#endif //Q_OS_LINUX
}

void IExchange::slotDeviceRemoved(const QDBusObjectPath& path, const QStringList& list)
{
#ifdef Q_OS_LINUX
//    qDebug() << "-----------dbusDeviceRemoved----------";
//    qDebug() << path.path() << list;
    if(!path.path().startsWith("/org/freedesktop/UDisks2/block_devices/"))
    {
        return;
    }

    const int N = treeWidget->topLevelItemCount();
    for(int i = 0; i<N; i++)
    {
        IDeviceTreeWidgetItem * item = dynamic_cast<IDeviceTreeWidgetItem*>(treeWidget->topLevelItem(i));
        if(item && item->getId() == path.path())
        {
            delete item;
            return;
        }
    }

#endif //Q_OS_LINUX
}

void IExchange::slotItemExpanded(QTreeWidgetItem * item)
{
    IDeviceTreeWidgetItem * devItem = dynamic_cast<IDeviceTreeWidgetItem*>(item);
    if(!devItem)
    {
        return;
    }
    if(devItem->childCount() != 0)
    {
        return;
    }

    devItem->mount();
    devItem->readDevice();

}

void IExchange::slotItemCollapsed(QTreeWidgetItem * item)
{
    IDeviceTreeWidgetItem * devItem = dynamic_cast<IDeviceTreeWidgetItem*>(item);
    if(!devItem)
    {
        return;
    }
    devItem->unmount();
    QList<QTreeWidgetItem *> children =	devItem->takeChildren();
    qDeleteAll(children);

}


