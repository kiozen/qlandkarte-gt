/**********************************************************************************************
    Copyright (C) 2012 Oliver Eichler oliver.eichler@gmx.de

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

#ifndef CTEXTBROWSER_H
#define CTEXTBROWSER_H

#include <QTextBrowser>

class CTextBrowser : public QTextBrowser
{
    Q_OBJECT;
    public:
        CTextBrowser(QWidget * parent);
        virtual ~CTextBrowser();

        void resetAreas();

        void addArea(const QString& key, const QRect& rect);

    public slots:
        void slotHighlightArea(const QString& key);

        signals:
        void sigHighlightArea(const QString& key);

    protected:
        void paintEvent(QPaintEvent * e);
        void mouseMoveEvent(QMouseEvent * e);

    private:

        QMap<QString, QRect> areas;

        QString areaKey;
};
#endif                           //CTEXTBROWSER_H
