/**********************************************************************************************
    Copyright (C) 2011 Oliver Eichler oliver.eichler@gmx.de

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

#include "CDlgMapTmsConfig.h"
#include "CMapTms.h"
#include <QtGui>

CDlgMapTmsConfig::CDlgMapTmsConfig(CMapTms& m)
{
    setupUi(this);

    map = CMapDB::self().getMapData(m.getKey());
    lineName->setText(map.description);
    lineUrl->setText(map.filename);
    lineCopyright->setText(map.copyright);

}


CDlgMapTmsConfig::CDlgMapTmsConfig()
{
    setupUi(this);
    lineName->setText(map.description);
    lineUrl->setText(map.filename);
    lineCopyright->setText(map.copyright);
}


CDlgMapTmsConfig::~CDlgMapTmsConfig()
{

}


void CDlgMapTmsConfig::accept()
{
    CMapDB& mapdb = CMapDB::self();
    mapdb.delKnownMap(QStringList(map.key));

    map.description = lineName->text();
    map.filename    = lineUrl->text();
    map.copyright   = lineCopyright->text();
    map.type        = IMap::eTMS;

    mapdb.setMapData(map);

    QDialog::accept();
}
