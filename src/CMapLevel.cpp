/**********************************************************************************************
    Copyright (C) 2007 Oliver Eichler oliver.eichler@gmx.de

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

#include "CMapLevel.h"
#include "CMapFile.h"
#include "CMapDEM.h"
#include "CMapQMAP.h"
#include "CWpt.h"

#include <QtGui>

CMapLevel::CMapLevel(quint32 min, quint32 max, CMapQMAP * parent)
: QObject(parent)
, min(min)
, max(max)
, pjtar(0)
, pjsrc(0)
, westbound(180)
, northbound(-90)
, eastbound(-180)
, southbound(90)
, has32BitRgbFile(false)
{
    pjtar = pj_init_plus("+proj=longlat  +datum=WGS84 +no_defs");
}


CMapLevel::~CMapLevel()
{
    if(pjtar) pj_free(pjtar);
    if(pjsrc) pj_free(pjsrc);
}


void CMapLevel::addMapFile(const QString& filename)
{
    CMapFile * mapfile = new CMapFile(filename,this);
    if(mapfile && !mapfile->ok)
    {
        qDebug() << "skip" << filename;
        delete mapfile;
        return;
    }
    mapfiles << mapfile;
    Q_ASSERT((*mapfiles.begin())->strProj == mapfile->strProj);
    if(pjsrc == 0)
    {
        pjsrc = pj_init_plus(mapfile->strProj.toLatin1());
    }

    double n = 0, e = 0 , s = 0, w = 0;

    w = mapfile->xref1;
    n = mapfile->yref1;
    pj_transform(pjsrc, pjtar, 1, 0, &w, &n, 0);

    e = mapfile->xref2;
    s = mapfile->yref2;
    pj_transform(pjsrc, pjtar, 1, 0, &e, &s, 0);

    if(w < westbound)   westbound = w;
    if(e > eastbound)   eastbound = e;
    if(n > northbound)  northbound = n;
    if(s < southbound)  southbound = s;

    if(mapfile->is32BitRgb()) has32BitRgbFile = true;

    //qDebug() << filename;
    //printf("%f %f\n",(westbound * RAD_TO_DEG), (northbound * RAD_TO_DEG));
    //printf("%f %f\n",(eastbound * RAD_TO_DEG), (southbound * RAD_TO_DEG));
    //qDebug() << "topleft"     << (westbound * RAD_TO_DEG) << (northbound * RAD_TO_DEG);
    //qDebug() << "bottomright" << (eastbound * RAD_TO_DEG) << (southbound * RAD_TO_DEG);

}


void CMapLevel::dimensions(double& lon1, double& lat1, double& lon2, double& lat2)
{
    lon1 = westbound;
    lat1 = northbound;
    lon2 = eastbound;
    lat2 = southbound;
}
