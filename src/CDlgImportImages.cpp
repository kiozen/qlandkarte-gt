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
#include "CDlgImportImages.h"
#include "CSettings.h"
#include "config.h"
#include "CWptDB.h"
#include "GeoMath.h"

#include <QtGui>
#include <QFileDialog>

CDlgImportImages::CDlgImportImages(QWidget *parent)
: QDialog(parent)
{
    setupUi(this);

    SETTINGS;
    QString path = cfg.value("path/images", "./").toString();

    labelPath->setText(path);

    radioCopySmall->setChecked(cfg.value("imageImport/copy/small", true).toBool());
    radioCopyLarge->setChecked(cfg.value("imageImport/copy/large", false).toBool());
    radioCopyOriginal->setChecked(cfg.value("imageImport/copy/original", false).toBool());
    radioCopyLink->setChecked(cfg.value("imageImport/copy/link", false).toBool());

    connect(toolPath, SIGNAL(clicked()), this, SLOT(slotSelectPath()));

    radioRefExif->setChecked(cfg.value("imageImport/ref/exif", true).toBool());
    radioRefTime->setChecked(cfg.value("imageImport/ref/time", false).toBool());
    radioRefPosition->setChecked(cfg.value("imageImport/ref/position", false).toBool());

    connect(radioRefExif, SIGNAL(clicked()), this, SLOT(slotSelectRefMethod()));
    connect(radioRefTime, SIGNAL(clicked()), this, SLOT(slotSelectRefMethod()));
    connect(radioRefPosition, SIGNAL(clicked()), this, SLOT(slotSelectRefMethod()));

    searchForFiles(path);

    slotSelectRefMethod();

    connect(listImages, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(slotSelectPicture(QListWidgetItem*)));
}


CDlgImportImages::~CDlgImportImages()
{
}


void CDlgImportImages::accept()
{
    QStringList files;

    for(int i = 0; i < listImages->count(); i++)
    {
        files << listImages->item(i)->data(Qt::UserRole).toString();
    }

    CWptDB::exifMode_e mode = CWptDB::eExifModeSmall;
    if(radioCopyLarge->isChecked())
    {
        mode = CWptDB::eExifModeLarge;
    }
    else if(radioCopyOriginal->isChecked())
    {
        mode = CWptDB::eExifModeOriginal;
    }
    else if(radioCopyLink->isChecked())
    {
        mode = CWptDB::eExifModeLink;
    }

    if(radioRefExif->isChecked())
    {
        CWptDB::self().createWaypointsFromImages(files, mode);
    }
    else if(radioRefTime->isChecked())
    {
        quint32 timestamp = dateTimeEdit->dateTime().toUTC().toTime_t();
        CWptDB::self().createWaypointsFromImages(files, mode, selectedFile, timestamp);
    }
    else if(radioRefPosition->isChecked())
    {
        float lon = 0;
        float lat = 0;

        if(GPS_Math_Str_To_Deg(linePosition->text(), lon, lat))
        {
            CWptDB::self().createWaypointsFromImages(files, mode, selectedFile, lon, lat);
        }
    }

    SETTINGS;
    cfg.setValue("path/images", labelPath->text());
    cfg.setValue("imageImport/copy/small", radioCopySmall->isChecked());
    cfg.setValue("imageImport/copy/large", radioCopyLarge->isChecked());
    cfg.setValue("imageImport/copy/original", radioCopyOriginal->isChecked());
    cfg.setValue("imageImport/copy/link", radioCopyLink->isChecked());

    cfg.setValue("imageImport/ref/exif", radioRefExif->isChecked());
    cfg.setValue("imageImport/ref/time", radioRefTime->isChecked());
    cfg.setValue("imageImport/ref/position", radioRefPosition->isChecked());

    QDialog::accept();
}


void CDlgImportImages::searchForFiles(const QString& path)
{
    QDir dir(path);
    QStringList filter;
    filter << "*.jpg" << "*.jpeg" << "*.png";
    QStringList files = dir.entryList(filter, QDir::Files);

    listImages->clear();
    foreach(const QString& file, files)
    {
        QListWidgetItem * item = new QListWidgetItem(listImages);
        item->setText(file);
        item->setData(Qt::UserRole, dir.filePath(file));
    }
}


void CDlgImportImages::slotSelectPath()
{
    QString path = labelPath->text();
    path = QFileDialog::getExistingDirectory(0, tr("Select path..."), path, FILE_DIALOG_FLAGS);
    if(path.isEmpty()) return;
    labelPath->setText(path);

    searchForFiles(path);
}


void CDlgImportImages::slotSelectRefMethod()
{
    if(radioRefTime->isChecked())
    {
        groupRefTime->show();
    }
    else
    {
        groupRefTime->hide();
    }

    if(radioRefPosition->isChecked())
    {
        groupRefPosition->show();
    }
    else
    {
        groupRefPosition->hide();
    }
}


void CDlgImportImages::slotSelectPicture(QListWidgetItem * item)
{
    labelRefTimeFile->setText(item->text());
    labelRefPositionFile->setText(item->text());

    dateTimeEdit->setEnabled(true);
    linePosition->setEnabled(true);

    QFileInfo fi(item->data(Qt::UserRole).toString());
    dateTimeEdit->setDateTime(fi.created());

    selectedFile = fi.absoluteFilePath();
}
