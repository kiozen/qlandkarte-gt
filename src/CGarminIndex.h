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

#ifndef CGARMININDEX_H
#define CGARMININDEX_H

#include "CGarminPolygon.h"
#include "CGarminPoint.h"

#include <QThread>
#include <QStringList>
#include <QSqlDatabase>
#include <QMutex>
#include <QSet>
#include <QRectF>

class CGarminIndex : public QThread
{
    Q_OBJECT;
    public:
        CGarminIndex(QObject * parent);
        virtual ~CGarminIndex();
        void setDBName(const QString& name);
        void create(const QStringList& files);
        bool created();

        void searchPolyline(const QString& text, QSet<QString>& result);
        void searchPolyline(const QString& text, QVector<CGarminPolygon>& result);

        void searchPoint(const QString& text, QSet<QString>& result);
        void searchPoint(const QString& text, QVector<CGarminPoint>& result);

        void searchPolyline(const QString& text, const QRectF& viewport, QSet<QString>& result);
        void searchPolyline(const QString& text, const QRectF& viewport, QVector<CGarminPolygon>& result);

        void searchPoint(const QString& text, const QRectF& viewport, QSet<QString>& result);
        void searchPoint(const QString& text, const QRectF& viewport, QVector<CGarminPoint>& result);

        signals:
        void sigProgress(const QString& status, const int progress);

    private slots:
        void slotFinished();

    protected:
        void run();

    private:
        QMutex          mutex;
        QString         dbName;
        QStringList     imgFiles;

};
#endif                           //CGARMININDEX_H
