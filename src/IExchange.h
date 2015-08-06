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
#ifndef IEXCHANGE_H
#define IEXCHANGE_H

#include <QObject>
#include <QTreeWidgetItem>
class QTreeWidget;
class QDBusObjectPath;

class IExchange : public QObject
{
    Q_OBJECT;
    public:
        IExchange(const QString &vendor, QTreeWidget *treeWidget, QObject * parent);
        virtual ~IExchange();

    protected slots:
        virtual void slotUpdate();
        virtual void slotDeviceAdded(const QDBusObjectPath& path, const QVariantMap& map) = 0;
        virtual void slotDeviceRemoved(const QDBusObjectPath& path, const QStringList& list);
        virtual void slotItemExpanded(QTreeWidgetItem * item);
        virtual void slotItemCollapsed(QTreeWidgetItem * item);

    protected:
        QString checkForDevice(const QDBusObjectPath& path);

        QString vendor;

        QTreeWidget * treeWidget;
};

class ITreeWidgetItem : public QTreeWidgetItem
{
    public:
        ITreeWidgetItem(QTreeWidgetItem * parent) : QTreeWidgetItem(parent){}
        ITreeWidgetItem(QTreeWidget * parent) : QTreeWidgetItem(parent){}
        virtual void menu(const QPoint& pos){}

};

class IDeviceTreeWidgetItem : public ITreeWidgetItem
{
    public:
        IDeviceTreeWidgetItem(const QString& id, QTreeWidget * parent);

        const QString& getId(){return id;}

        void mount();
        void unmount();

        virtual void readDevice() = 0;

    protected:
        void readMountPoint();
        QString id;
        QString mountPoint;
};


#endif //IEXCHANGE_H

