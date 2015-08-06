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
#include "CDlgDeviceExportPath.h"

#include <QtGui>

CDlgDeviceExportPath::CDlgDeviceExportPath(const QString& what, QDir &dir, QString& subdir, mode_e mode, QWidget * parent)
: QDialog(parent)
, subdir(subdir)
{
    setupUi(this);

    labelHead->setText(tr("Where should I place all %1?").arg(what));

    if(mode == eDirectory)
    {
        QStringList dirs = dir.entryList(QStringList("*"), QDir::AllDirs|QDir::NoDotAndDotDot);
        foreach(const QString& d, dirs)
        {
            QListWidgetItem * item = new QListWidgetItem(listWidget);
            item->setText(d);
            item->setIcon(QIcon(":/icons/iconFolderBlue16x16.png"));
        }
    }
    else
    {
        QSet<QString> knownPrefix;
        qDebug() << dir;
        QStringList files = dir.entryList(QStringList("*.gpx"), QDir::Files|QDir::NoDotAndDotDot);
        foreach(const QString& f, files)
        {
            if(!f.contains('_'))
            {
                continue;
            }

            QString prefix = f.split('_')[0];
            if(prefix.isEmpty() || knownPrefix.contains(prefix))
            {
                continue;
            }
            knownPrefix << prefix;

            QListWidgetItem * item = new QListWidgetItem(listWidget);
            item->setText(prefix);
            item->setIcon(QIcon(":/icons/iconText16x16.png"));

        }
    }

    lineEdit->setText(QString("Data-%1").arg(QDateTime::currentDateTime().toUTC().toString("yyyy-MM-dd")));
    lineEdit->setFocus();
    lineEdit->selectAll();

    connect(listWidget, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(slotItemClicked(QListWidgetItem*)));
    connect(lineEdit, SIGNAL(returnPressed()), this, SLOT(slotReturnPressed()));
}


CDlgDeviceExportPath::~CDlgDeviceExportPath()
{

}


void CDlgDeviceExportPath::slotItemClicked(QListWidgetItem*item)
{
    if(item == 0) return;

    subdir = item->text();
    QDialog::accept();
}


void CDlgDeviceExportPath::slotReturnPressed()
{
    subdir = lineEdit->text();
    QDialog::accept();
}
