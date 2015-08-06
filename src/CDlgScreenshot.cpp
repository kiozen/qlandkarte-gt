/**********************************************************************************************
    Copyright (C) 2009 Oliver Eichler oliver.eichler@gmx.de

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

#include "CDlgScreenshot.h"
#include "CResources.h"
#include "IDevice.h"

#include "config.h"
#include "CSettings.h"

#include <QtGui>
#include <QFileDialog>

CDlgScreenshot::CDlgScreenshot(QWidget * parent)
: QDialog(parent)
{
    setupUi(this);

    connect(butAcquire, SIGNAL(clicked()), this, SLOT(slotAcquire()));
    connect(butSave, SIGNAL(clicked()), this, SLOT(slotSave()));
}


CDlgScreenshot::~CDlgScreenshot()
{

}


void CDlgScreenshot::slotAcquire()
{
    IDevice * dev = CResources::self().device();
    if(dev)
    {
        dev->downloadScreenshot(image);
        labelScreenshot->setPixmap(QPixmap::fromImage(image));

        butSave->setEnabled(true);
    }
}


void CDlgScreenshot::slotSave()
{
    SETTINGS;
    QString pathData = cfg.value("path/data","./").toString();
    QString filter = cfg.value("screenshot/imagetype","Bitmap (*.png)").toString();

    QString filename = QFileDialog::getSaveFileName( 0, tr("Select output file")
        ,pathData
        ,"Bitmap (*.png)"
        ,&filter
        , FILE_DIALOG_FLAGS
        );

    if(filename.isEmpty()) return;

    QFileInfo fi(filename);

    if(fi.suffix().toLower() != "png")
    {
        filename += ".png";
    }

    image.save(filename);

    pathData = fi.absolutePath();
    cfg.setValue("path/data", pathData);
    cfg.setValue("screenshot/imagetype", filter);
}
