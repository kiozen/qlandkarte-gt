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
#ifndef CMAPSEARCHCANVAS_H
#define CMAPSEARCHCANVAS_H

#include <QWidget>
#include <QPixmap>

class CMapSearchCanvas : public QWidget
{
    Q_OBJECT;
    public:
        CMapSearchCanvas(QWidget * parent);
        virtual ~CMapSearchCanvas();

        void setBuffer(const QPixmap& pic);

        signals:
        void sigSelection(const QPixmap& pixmap);

    protected:
        void paintEvent(QPaintEvent * e);
        void mouseMoveEvent(QMouseEvent * e);
        void mousePressEvent(QMouseEvent * e);
        void mouseReleaseEvent(QMouseEvent * e);

    private:
        QPixmap buffer;

        QRect rectSelect;
};
#endif                           //CMAPSEARCHCANVAS_H
