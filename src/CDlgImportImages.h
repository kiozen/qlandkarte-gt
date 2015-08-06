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
#ifndef CDLGIMPORTIMAGES_H
#define CDLGIMPORTIMAGES_H

#include <QDialog>
#include <ui_IDlgImportImages.h>

class CDlgImportImages : public QDialog, private Ui::IDlgImportImages
{
    Q_OBJECT;
    public:
        CDlgImportImages(QWidget * parent);
        virtual ~CDlgImportImages();

    public slots:
        void accept();

    private slots:
        void slotSelectPath();
        void slotSelectRefMethod();
        void slotSelectPicture(QListWidgetItem * item);

    private:
        void searchForFiles(const QString& path);

        QString selectedFile;
};
#endif                           //CDLGIMPORTIMAGES_H
