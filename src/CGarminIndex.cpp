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

#include "CGarminIndex.h"
#include "CGarminTile.h"
#include "config.h"

#include <QtGui>
#include <QSqlQuery>
#include <QSqlError>

CGarminIndex::CGarminIndex(QObject * parent)
: QThread(parent)
{

}


CGarminIndex::~CGarminIndex()
{

}


bool CGarminIndex::created()
{
    return (QFile::exists(dbName) && QFileInfo(dbName).size() && !isRunning());
}


void CGarminIndex::setDBName(const QString& name)
{
    QMutexLocker lock(&mutex);

    QDir path(QDir::home().filePath(CONFIGDIR));
    dbName = path.filePath(name + ".db");
    qDebug() << "add" << dbName;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbName);
    db.open();
}


void CGarminIndex::create(const QStringList& files)
{
    if(isRunning()) return;
    imgFiles = files;

    QSqlDatabase db = QSqlDatabase::database(dbName);
    db.close();
    db.removeDatabase(dbName);
    QFile::remove(dbName);

    connect(this, SIGNAL(finished()), SLOT(slotFinished()));
    start();
}


void CGarminIndex::run()
{
    QMutexLocker lock(&mutex);

    const QRectF viewport(QPointF(-180 * DEG_TO_RAD, 90 * DEG_TO_RAD), QPointF(180 * DEG_TO_RAD, -90 * DEG_TO_RAD));
    const int size  = imgFiles.size();
    int cnt         = 0;
    QString filename;
    QSqlDatabase db;

    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbName);
    db.open();

    QSqlQuery query(db);
    if(!query.exec("PRAGMA temp_store = MEMORY"))
    {
        qDebug() << query.lastError();
    }

    if(!query.exec("PRAGMA synchronous = OFF"))
    {
        qDebug() << query.lastError();
    }

    if(!query.exec( "CREATE TABLE subfiles ("
        "id             INTEGER PRIMARY KEY,"
        "name           TEXT NOT NULL,"
        "filename       TEXT NOT NULL"
        ")"))
    {
        qDebug() << query.lastError();
    }

    if(!query.exec( "CREATE TABLE polylines ("
        "id             INTEGER PRIMARY KEY,"
        "type           INT UNSIGNED NOT NULL,"
        "subfile        INTEGER NOT NULL,"
        "subdiv         INT UNSIGNED NOT NULL,"
        "offset         INT UNSIGNED NOT NULL,"
        "lon1           REAL NOT NULL,"
        "lat1           REAL NOT NULL,"
        "lon2           REAL NOT NULL,"
        "lat2           REAL NOT NULL,"
        "label          TEXT"
        ")"))
    {
        qDebug() << query.lastError();
    }

    if(!query.exec( "CREATE TABLE points ("
        "id             INTEGER PRIMARY KEY,"
        "type           INT UNSIGNED NOT NULL,"
        "subfile        INTEGER NOT NULL,"
        "subdiv         INT UNSIGNED NOT NULL,"
        "offset         INT UNSIGNED NOT NULL,"
        "lon1           REAL NOT NULL,"
        "lat1           REAL NOT NULL,"
        "label          TEXT"
        ")"))
    {
        qDebug() << query.lastError();
    }

    foreach(filename, imgFiles)
    {
        CGarminTile tile(0);
        tile.readBasics(filename);
        emit sigProgress(tr("Create index... %1").arg(filename), cnt * 100 / size);

        tile.createIndex(db);
        ++cnt;
    }

}


void CGarminIndex::searchPolyline(const QString& text, QSet<QString>& result)
{
    QMutexLocker lock(&mutex);

    QSqlDatabase db = QSqlDatabase::database(dbName);
    QSqlQuery query(db);

    query.prepare("SELECT label FROM polylines WHERE label LIKE :label");
    query.bindValue(":label", "%" + text + "%");
    if(!query.exec())
    {
        qDebug() << query.lastError();
    }

    while (query.next())
    {
        result <<  query.value(0).toString();
    }
}


void CGarminIndex::searchPolyline(const QString& text, QVector<CGarminPolygon>& result)
{
    QMutexLocker lock(&mutex);

    QSqlDatabase db = QSqlDatabase::database(dbName);
    QSqlQuery query(db);

    query.prepare("SELECT label, subfile, subdiv, offset FROM polylines WHERE label = :label ");
    query.bindValue(":label", text);
    if(!query.exec())
    {
        qDebug() << query.lastError();
    }

    while (query.next())
    {
        quint32 subfile     = query.value(1).toUInt();
        quint32 subdiv      = query.value(2).toUInt();
        qint32  offset      = query.value(3).toInt();

        QSqlQuery query2(db);
        if(!query2.exec(QString("SELECT name, filename FROM subfiles WHERE id = %1").arg(subfile)))
        {
            qDebug() << query.lastError();
        }
        query2.next();

        QString name        = query2.value(0).toString();
        QString filename    = query2.value(1).toString();

        CGarminTile tile(0);
        tile.readBasics(filename);
        tile.readPolyline(name, subdiv, offset, result);
    }
}


void CGarminIndex::searchPoint(const QString& text, QSet<QString>& result)
{
    QMutexLocker lock(&mutex);

    QSqlDatabase db = QSqlDatabase::database(dbName);
    QSqlQuery query(db);

    query.prepare("SELECT label FROM points WHERE label LIKE :label");
    query.bindValue(":label", "%" + text + "%");
    if(!query.exec())
    {
        qDebug() << query.lastError();
    }

    while (query.next())
    {
        result <<  query.value(0).toString();
    }
}


void CGarminIndex::searchPoint(const QString& text, QVector<CGarminPoint>& result)
{
    QMutexLocker lock(&mutex);

    QSqlDatabase db = QSqlDatabase::database(dbName);
    QSqlQuery query(db);

    query.prepare("SELECT label, subfile, subdiv, offset FROM points WHERE label = :label ");
    query.bindValue(":label", text);
    if(!query.exec())
    {
        qDebug() << query.lastError();
    }

    while (query.next())
    {
        quint32 subfile     = query.value(1).toUInt();
        quint32 subdiv      = query.value(2).toUInt();
        qint32  offset      = query.value(3).toInt();

        QSqlQuery query2(db);
        if(!query2.exec(QString("SELECT name, filename FROM subfiles WHERE id = %1").arg(subfile)))
        {
            qDebug() << query.lastError();
        }
        query2.next();

        QString name        = query2.value(0).toString();
        QString filename    = query2.value(1).toString();

        CGarminTile tile(0);
        tile.readBasics(filename);
        tile.readPoint(name, subdiv, offset, result);
    }
}


void CGarminIndex::searchPolyline(const QString& text, const QRectF& viewport, QSet<QString>& result)
{
    QMutexLocker lock(&mutex);

    QSqlDatabase db = QSqlDatabase::database(dbName);
    QSqlQuery query(db);

    query.prepare("SELECT label, lon1, lat1, lon2, lat2 FROM polylines WHERE label LIKE :label");
    query.bindValue(":label", "%" + text + "%");
    if(!query.exec())
    {
        qDebug() << query.lastError();
    }

    while (query.next())
    {
        QPointF p1(query.value(1).toDouble(), query.value(2).toDouble());
        QPointF p2(query.value(3).toDouble(), query.value(4).toDouble());

        if(viewport.intersects(QRectF(p1,p2)))
        {
            result <<  query.value(0).toString();
        }
    }
}


void CGarminIndex::searchPolyline(const QString& text, const QRectF& viewport, QVector<CGarminPolygon>& result)
{
    QMutexLocker lock(&mutex);

    QSqlDatabase db = QSqlDatabase::database(dbName);
    QSqlQuery query(db);

    query.prepare("SELECT label, subfile, subdiv, offset, lon1, lat1, lon2, lat2 FROM polylines WHERE label = :label ");
    query.bindValue(":label", text);
    if(!query.exec())
    {
        qDebug() << query.lastError();
    }

    while (query.next())
    {
        QPointF p1(query.value(4).toDouble(), query.value(5).toDouble());
        QPointF p2(query.value(6).toDouble(), query.value(7).toDouble());
        if(!viewport.intersects(QRectF(p1,p2))) continue;

        quint32 subfile     = query.value(1).toUInt();
        quint32 subdiv      = query.value(2).toUInt();
        qint32  offset      = query.value(3).toInt();

        QSqlQuery query2(db);
        if(!query2.exec(QString("SELECT name, filename FROM subfiles WHERE id = %1").arg(subfile)))
        {
            qDebug() << query.lastError();
        }
        query2.next();

        QString name        = query2.value(0).toString();
        QString filename    = query2.value(1).toString();

        CGarminTile tile(0);
        tile.readBasics(filename);
        tile.readPolyline(name, subdiv, offset, result);
    }
}


void CGarminIndex::searchPoint(const QString& text, const QRectF& viewport, QSet<QString>& result)
{
    QMutexLocker lock(&mutex);

    QSqlDatabase db = QSqlDatabase::database(dbName);
    QSqlQuery query(db);

    query.prepare("SELECT label, lon1, lat1 FROM points WHERE label LIKE :label");
    query.bindValue(":label", "%" + text + "%");
    if(!query.exec())
    {
        qDebug() << query.lastError();
    }

    while (query.next())
    {
        QPointF p1(query.value(1).toDouble(), query.value(2).toDouble());
        if(viewport.contains(p1))
        {
            result <<  query.value(0).toString();
        }
    }
}


void CGarminIndex::searchPoint(const QString& text, const QRectF& viewport, QVector<CGarminPoint>& result)
{
    QMutexLocker lock(&mutex);

    QSqlDatabase db = QSqlDatabase::database(dbName);
    QSqlQuery query(db);

    query.prepare("SELECT label, subfile, subdiv, offset, lon1, lat1 FROM points WHERE label = :label ");
    query.bindValue(":label", text);
    if(!query.exec())
    {
        qDebug() << query.lastError();
    }

    while (query.next())
    {
        QPointF p1(query.value(4).toDouble(), query.value(5).toDouble());
        if(!viewport.contains(p1)) continue;

        quint32 subfile     = query.value(1).toUInt();
        quint32 subdiv      = query.value(2).toUInt();
        qint32  offset      = query.value(3).toInt();

        QSqlQuery query2(db);
        if(!query2.exec(QString("SELECT name, filename FROM subfiles WHERE id = %1").arg(subfile)))
        {
            qDebug() << query.lastError();
        }
        query2.next();

        QString name        = query2.value(0).toString();
        QString filename    = query2.value(1).toString();

        CGarminTile tile(0);
        tile.readBasics(filename);
        tile.readPoint(name, subdiv, offset, result);
    }
}


void CGarminIndex::slotFinished()
{
    emit sigProgress(tr("Done"), 0);
    disconnect(this, 0, 0, 0);
}
