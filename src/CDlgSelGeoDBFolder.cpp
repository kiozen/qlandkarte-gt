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
#include "CDlgSelGeoDBFolder.h"
#include "CGeoDB.h"

#include <QtGui>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

CDlgSelGeoDBFolder::CDlgSelGeoDBFolder(QSqlDatabase& db, quint64& result, bool topLevelToo)
: db(db)
, result(result)
, topLevelToo(topLevelToo)
{
    result = 0;
    setupUi(this);

    QTreeWidgetItem item;
    item.setData(CGeoDB::eCoName, CGeoDB::eUrDBKey, 1);
    queryChildrenFromDB(&item);

    treeWidget->addTopLevelItems(item.takeChildren());
    treeWidget->expandAll();

    showMaximized();
}


CDlgSelGeoDBFolder::~CDlgSelGeoDBFolder()
{

}


void CDlgSelGeoDBFolder::accept()
{
    QTreeWidgetItem * item = treeWidget->currentItem();
    if(item == 0)
    {
        return;
    }

    result = item->data(CGeoDB::eCoName, CGeoDB::eUrDBKey).toULongLong();

    QDialog::accept();
}


void CDlgSelGeoDBFolder::queryChildrenFromDB(QTreeWidgetItem * parent)
{
    QSqlQuery query(db);
    quint64 parentId = parent ? parent->data(CGeoDB::eCoName, CGeoDB::eUrDBKey).toULongLong() : 1;

    if(parentId == 0)
    {
        return;
    }

    query.prepare("SELECT t1.child FROM folder2folder AS t1, folders AS t2 WHERE t1.parent = :id AND t2.id = t1.child ORDER BY t2.name");
    query.bindValue(":id", parentId);
    if(!query.exec())
    {
        qDebug() << query.lastQuery();
        qDebug() << query.lastError();
        return;
    }

    while(query.next())
    {
        quint64 childId = query.value(0).toULongLong();

        QSqlQuery query2(db);
        query2.prepare("SELECT icon, name, comment, type, locked FROM folders WHERE id = :id");
        query2.bindValue(":id", childId);
        if(!query2.exec())
        {
            qDebug() << query2.lastQuery();
            qDebug() << query2.lastError();
            continue;
        }
        query2.next();

        // check for archive flag
        if(query2.value(4).toBool())
        {
            continue;
        }

        QString comment = query2.value(2).toString();
        QTreeWidgetItem * item = new QTreeWidgetItem(parent, query2.value(3).toInt());
        item->setData(CGeoDB::eCoName, CGeoDB::eUrDBKey, childId);
        item->setIcon(CGeoDB::eCoName, QIcon(query2.value(0).toString()));
        if(comment.isEmpty())
        {
            item->setText(CGeoDB::eCoName, query2.value(1).toString());
        }
        else
        {
            item->setText(CGeoDB::eCoName, query2.value(1).toString() + " - " + comment);
        }

        if(item->type() < CGeoDB::eFolder1)
        {
            item->setFlags(item->flags()&~Qt::ItemIsSelectable);
        }

        queryChildrenFromDB(item);
    }

#ifdef QK_QT5_PORT
    treeWidget->header()->setSectionResizeMode(CGeoDB::eCoName,QHeaderView::ResizeToContents);
#else
    treeWidget->header()->setResizeMode(CGeoDB::eCoName,QHeaderView::ResizeToContents);
#endif
}
