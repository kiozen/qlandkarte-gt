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
#ifndef CEXCHANGEGARMIN_H
#define CEXCHANGEGARMIN_H

#include <IExchange.h>
#include <QDir>

class QDBusObjectPath;

class CGarminTreeWidgetItem : public IDeviceTreeWidgetItem
{
    public:
        CGarminTreeWidgetItem(const QString& id, QTreeWidget *parent);

        void readDevice();

    private:
        void readDeviceXml(const QString& filename);
        QString pathGpx;
        QString pathSpoiler;
        QString pathJpeg;
        QString pathAdventure;
};

class CGarminFolderTreeWidgetItem : public ITreeWidgetItem
{
    public:
        CGarminFolderTreeWidgetItem(const QString &path, QTreeWidgetItem *parent);

        void read();

    private:
        QDir dir;
};

class CGarminFileTreeWidgetItem : public ITreeWidgetItem
{
    public:
        CGarminFileTreeWidgetItem(const QString &path, QTreeWidgetItem *parent);

        void read();
        void save();

    private:
        QString filename;
};

class CGarminAdventureTreeWidgetItem : public ITreeWidgetItem
{
    public:
        CGarminAdventureTreeWidgetItem(const QString &path, QTreeWidgetItem *parent);
};


class CExchangeGarmin : public IExchange
{
    Q_OBJECT
    public:
        CExchangeGarmin(QTreeWidget *treeWidget, QObject * parent);
        virtual ~CExchangeGarmin();

    protected slots:
        virtual void slotItemExpanded(QTreeWidgetItem * item);
        virtual void slotItemCollapsed(QTreeWidgetItem * item);

    private slots:
        void slotDeviceAdded(const QDBusObjectPath& path, const QVariantMap& map);
};


#endif //CEXCHANGEGARMIN_H

