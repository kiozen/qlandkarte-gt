/**********************************************************************************************
    Copyright (C) 2010 Oliver Eichler oliver.eichler@gmx.de

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
#ifndef CGEODB_H
#define CGEODB_H

#include <QWidget>
#include <QSqlDatabase>
#include "ui_IGeoToolWidget.h"

class QTabWidget;
class QTreeWidgetItem;
class QMenu;
class CDiary;
class CExchangeGarmin;
class CExchangeTwoNav;

class CGeoDB : public QWidget, private Ui::IGeoToolWidget
{
    Q_OBJECT;
    public:
        virtual ~CGeoDB();

        static CGeoDB& self(){return *m_self;}

        /// switch tabbar if project manager gains focus
        void gainFocus();

        bool getProjectDiaryData(quint64 id, CDiary& data);
        bool setProjectDiaryData(quint64 id, CDiary& data);

    private slots:
        void loadWorkspace();
        /// save all elements in the workspace branch to be restored in next application start
        void saveWorkspace();

        /// this slot is called each time CWptDB signales a change
        void slotWptDBChanged();
        /// this slot is called each time CTrackDB signales a change
        void slotTrkDBChanged();
        /// this slot is called each time CRouteDB signales a change
        void slotRteDBChanged();
        /// this slot is called each time COverlayDB signales a change
        void slotOvlDBChanged();
        /// this slot is called each time CMapDB signales a change
        void slotMapDBChanged();

        void slotDiaryDBChanged();

        /// this slot is called when a waypoint is modified
        void slotModifiedWpt(const QString&);
        /// this slot is called when a track is modified
        void slotModifiedTrk(const QString&);
        /// this slot is called when a route is modified
        void slotModifiedRte(const QString&);
        /// this slot is called when an overlay is modified
        void slotModifiedOvl(const QString&);
        /// this slot is called when a map selection is modified
        void slotModifiedMap(const QString&);

        /// query children when folder is expanded
        void slotItemExpanded(QTreeWidgetItem * item);
        /// make clicked item visible
        void slotItemDoubleClickedWks(QTreeWidgetItem * item, int column);
        void slotItemDoubleClickedDb(QTreeWidgetItem * item, int column);
        void slotItemClickedDb(QTreeWidgetItem * item, int column);
        /// test for name change on folders or checkstate change
        void slotItemChanged(QTreeWidgetItem * item, int column);
        /// display context menu for current item in the treeWidget
        void slotContextMenuDatabase(const QPoint&);
        /// display context menu for current item in the treeWorkspace
        void slotContextMenuWorkspace(const QPoint&);
        /// add folder to database
        void slotAddFolder();
        /// delete folder and all it's content from database
        void slotDelFolder();
        /// edit folder properties
        void slotEditFolder();
        /// add a diary to a project folder
        void slotAddDiary();
        /// hide/show diary for project folder
        void slotShowDiary();
        /// delete diary from project folder
        void slotDelDiary();
        /// add a new relation for a folder
        void slotCopyFolder();
        /// delete current relation and add new one
        void slotMoveFolder();
        /// remove item/folder relation
        void slotDelItems();
        /// add new folder relation for selected items
        void slotCopyItems();
        /// delete current relation and add new one
        void slotMoveItems();
        /// add new relation to item without relations
        void slotMoveLost();
        /// remove item from database
        void slotDelLost();
        /// add items to database
        void slotAddItems();
        /// update items in database with changed data in the workspace
        void slotSaveItems();
        /// create a real copy from item with own unique key
        void slotHardCopyItem();

        void slotExportProject();
        /// toggle the archive flag for folders
        void slotLockFolder(bool yes);

    private:
        friend class CGeoDBInternalEditLock;
        friend class CDlgSelGeoDBFolder;
        friend class CDlgEditFolder;
        friend bool sortItemsLessThan(QTreeWidgetItem * item1, QTreeWidgetItem * item2);
        friend class CMainWindow;

        CGeoDB(QTabWidget * tb, QWidget * parent);

        /// sort all items below an item
        void sortItems(QTreeWidgetItem * item);
        /// initialize database from scratch
        void initDB();
        /// move database from one version to most resent version
        void migrateDB(int version);
        /// call each time the workspace changed
        void changedWorkspace(quint32 what);
        /// update the item text in workspace with a "*" for chnaged items
        void updateModifyMarker(quint32 what);
        /// update the item text in workspace with a "*" for chnaged items
        void updateModifyMarker(QTreeWidgetItem * item, QSet<QString>& keys, const QString& label);
        /// update "in database" icon
        void updateDatabaseMarker(quint32 what);
        /// update "in database" icon
        void updateDatabaseMarker(QTreeWidgetItem * itemWks, QSet<quint64> &keysWks);
        /// initialize database treewidget on startup
        void initTreeWidget();
        /// build up the treewidget from a given parent item up to a given depth of levels
        void queryChildrenFromDB(QTreeWidgetItem * parent, int levels);
        /// parse database for items with no relation to a folder
        void updateLostFound();
        /// update checkmarks according to loaded items
        void updateCheckmarks();
        /// update checkmarks according to loaded items
        void updateCheckmarks(QTreeWidgetItem * item);
        /// set the diary icon to open or closed
        void updateDiaryIcon();
        void updateDiaryIcon(QTreeWidgetItem * item);
        /// update all folders with same ID
        void updateFolderById(quint64 id);
        /// load all items attached to given parent id into the workspace
        void addChildrenToWks(quint64 parentId);
        /// remove all items attached to given parent id from the workspace
        void delChildrenFromWks(quint64 parentId);
        /// add a new folder to the tree widget and the database
        void addFolder(QTreeWidgetItem * parent, const QString& name, const QString& comment, qint32 type);
        /// serach tree widget for the parent id and add folder as clone
        void addFolderById(quint64 parentId, QTreeWidgetItem * child);
        /// delete a folder and all sub items recursively from database and tree widget
        void delFolder(QTreeWidgetItem * item, bool isTopLevel);
        /// delete folder items from treewidget with given parent/child relation
        void delFolderById(quint64 parentId, quint64 childId);
        /// search tree widget for the parent id and add item as clone
        void addItemById(quint64 parentId, QTreeWidgetItem * child);
        /// remove items from tree widget with geven parent / item relation
        void delItemById(quint64 parentId, quint64 childId);
        /// write item data to database
        void addItemToDB(quint64 parentId, QTreeWidgetItem * item);
        /// search treeWidget for items with id and update their content from database
        void updateItemById(quint64 id);

        void exportProject(quint64 key, const QString &name, const QString &prefix);
        /// sync list of modified keys with actual list of tems. drop removed keys.
        void syncModifyMarker(QSet<QString> &markers, QList<QString>& keys);

        enum EntryType_e
        {
            eWpt        = QTreeWidgetItem::UserType + 3,
            eTrk        = QTreeWidgetItem::UserType + 4,
            eRte        = QTreeWidgetItem::UserType + 5,
            eOvl        = QTreeWidgetItem::UserType + 6,
            eMap        = QTreeWidgetItem::UserType + 7,
            eDry        = QTreeWidgetItem::UserType + 8,

            eFolder0    = QTreeWidgetItem::UserType + 100,
            eFolderT    = QTreeWidgetItem::UserType + 101,
            eFolder1    = QTreeWidgetItem::UserType + 102,
            eFolder2    = QTreeWidgetItem::UserType + 103,
            eFolderN    = QTreeWidgetItem::UserType + 104
        };

        enum ColumnType_e
        {
            eCoName     = 0,
            eCoState    = 1,
            eCoDiary    = 2
        };
        enum UserRoles_e
        {
            eUrDBKey  = Qt::UserRole,
            eUrQLKey  = Qt::UserRole + 1,
            eUrType   = Qt::UserRole + 2,
            eUrDiary  = Qt::UserRole + 3
        };

        static CGeoDB * m_self;

        /// left hand tool widget tabbar
        QTabWidget * tabbar;
        /// a counter that is used by CGeoDBInternalEditLock to block slotItemChanged() on internal item edits
        quint32 isInternalEdit;
        /// a copy from CResources::saveOnExit;
        bool saveOnExit;

        quint32 saveOnMinutes;
        /// the database object
        QSqlDatabase db;

        QTreeWidgetItem * itemDatabase;
        QTreeWidgetItem * itemLostFound;
        QTreeWidgetItem * itemWorkspace;

        QTreeWidgetItem * itemWksWpt;
        QTreeWidgetItem * itemWksTrk;
        QTreeWidgetItem * itemWksRte;
        QTreeWidgetItem * itemWksOvl;
        QTreeWidgetItem * itemWksMap;

        QSet<QString> keysWptModified;
        QSet<QString> keysTrkModified;
        QSet<QString> keysRteModified;
        QSet<QString> keysOvlModified;
        QSet<QString> keysMapModified;

        QSet<quint64> keysWksWpt;
        QSet<quint64> keysWksTrk;
        QSet<quint64> keysWksRte;
        QSet<quint64> keysWksOvl;
        QSet<quint64> keysWksMap;

        QMenu * contextMenuFolder;
        QAction * actAddDir;
        QAction * actDelDir;
        QAction * actEditDir;
        QAction * actMoveDir;
        QAction * actCopyDir;
        QAction * actLockDir;
        QAction * actAddDiary;
        QAction * actShowDiary;
        QAction * actDelDiary;
        QAction * actExportProject;

        QMenu * contextMenuItem;
        QAction * actMoveItem;
        QAction * actCopyItem;
        QAction * actDelItem;

        QMenu * contextMenuLost;
        QAction * actMoveLost;
        QAction * actDelLost;

        QMenu * contextMenuWks;
        QAction * actAddToDB;
        QAction * actSaveToDB;
        QAction * actHardCopy;


        CExchangeGarmin * xchngGarmin;
        CExchangeTwoNav * xchngTwoNav;

};
#endif                           //CGEODB_H
