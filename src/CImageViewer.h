/** ********************************************************************************************
    Copyright (c) 2012 Oliver Eichler

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
********************************************************************************************* */

#ifndef CIMAGEVIEWER_H
#define CIMAGEVIEWER_H

#include <QDialog>
#include "ui_IImageViewer.h"
#include "CWpt.h"

class CImageViewer : public QDialog, private Ui::IImageViewer
{
    Q_OBJECT;
    public:
        CImageViewer(QList<CWpt::image_t> &images, int idx, QWidget *parent);

        virtual ~CImageViewer();

    protected:
        void paintEvent(QPaintEvent * e);
        void resizeEvent(QResizeEvent * e);
        void mousePressEvent(QMouseEvent * e);

    private:
        void setImageAtIdx(int i);

        QList<CWpt::image_t> images;

        int idx;

        QRect rectImage;
        QRect rectClose;
        QRect rectPrev;
        QRect rectNext;
};
#endif                           //CIMAGEVIEWER_H
