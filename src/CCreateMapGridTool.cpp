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

#include "CCreateMapGridTool.h"
#include "CCreateMapGeoTiff.h"
#include "CMainWindow.h"
#include "CCanvas.h"
#include "CMapDB.h"
#include "GeoMath.h"
#include "CDlgProjWizzard.h"
#include "CSettings.h"

#include <QtGui>
#include <QMessageBox>

#include <gdal.h>

CCreateMapGridTool * CCreateMapGridTool::m_self = 0;

CCreateMapGridTool::CCreateMapGridTool(CCreateMapGeoTiff * geotifftool, QWidget * parent)
: QWidget(parent)
, geotifftool(geotifftool)
{
    m_self = this;
    setupUi(this);

    labelExample->setPixmap(QPixmap(":/pics/grid_example.png"));

    helpStep2a->setHelp(tr("Place Reference Points"),
        tr("The grid tool will place reference points with calculated longitude and latitude to the line crossings of a linear map grid. To do so you have to place the 4 initial reference points to the grid as shown in the example.\n\nAltenatively you might have chosen to use already existing reference points. In this case you simply have to define the grid step size."));
    helpStep2b->setHelp(tr("Add Source projection"),
        tr("Next you might want to add a source projection to do a grid shift to WGS84. And you have to define the longitude and the latitude of the top left reference point. And the spacing between point 1 and 2, and 1 and 4."));
    helpStep2c->setHelp(tr("Create grid"),
        tr("On ok, the grid tool will add equally spaced reference points over your map. Keep in mind to manually fine tune the location of each point to get good results."));

    connect(pushCancel, SIGNAL(clicked()), this, SLOT(deleteLater()));
    connect(pushOk, SIGNAL(clicked()), this, SLOT(slotOk()));
    connect(lineLongitude, SIGNAL(editingFinished()), this, SLOT(slotCheck()));
    connect(lineLatitude, SIGNAL(editingFinished()), this, SLOT(slotCheck()));
    connect(lineXSpacing, SIGNAL(editingFinished()), this, SLOT(slotCheck()));
    connect(lineYSpacing, SIGNAL(editingFinished()), this, SLOT(slotCheck()));

    QWidget * mapedit = theMainWindow->findChild<QWidget*>("CMapEditWidget");
    if(mapedit)
    {
        mapedit->hide();
    }

    SETTINGS;
    lineProjection->setText(cfg.value("create/ref.proj","").toString());
    lineXSpacing->setText(cfg.value("create/grid.x.spacing","1000").toString());
    lineYSpacing->setText(cfg.value("create/grid.y.spacing","1000").toString());

    if(geotifftool->treeWidget->topLevelItemCount() < 2)
    {
        place4GCPs();
    }
    else
    {
        int res = QMessageBox::question(0,tr("Reference points found."), tr("Do you want to take the existing reference points to calculate additional points on the grid?"), QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes);
        if(res == QMessageBox::No)
        {
            place4GCPs();
        }
        else
        {
            lineLongitude->setEnabled(false);
            lineLatitude->setEnabled(false);
            labelEasting->setEnabled(false);
            labelNorthing->setEnabled(false);
        }
    }

    slotCheck();

    toolProjWizard->setIcon(QPixmap(":/icons/iconWizzard16x16.png"));
    connect(toolProjWizard, SIGNAL(clicked()), this, SLOT(slotProjWizard()));

}


CCreateMapGridTool::~CCreateMapGridTool()
{
    m_self = 0;

    SETTINGS;
    cfg.setValue("create/ref.proj",lineProjection->text());
    cfg.setValue("create/grid.x.spacing",lineXSpacing->text());
    cfg.setValue("create/grid.y.spacing",lineYSpacing->text());

    for(int i = 1;  i != 5; ++i)
    {
        CCreateMapGeoTiff::refpt_t& pt = geotifftool->refpts[-i];
        if(pt.item) delete pt.item;
        geotifftool->refpts.remove(-i);
    }

    QWidget * mapedit = theMainWindow->findChild<QWidget*>("CMapEditWidget");
    if(mapedit)
    {
        mapedit->show();
    }

    geotifftool->rectSelArea = QRect(QPoint(0,0),geotifftool->sizeMap);
}


void CCreateMapGridTool::place4GCPs()
{
    for(int i = 1;  i != 5; ++i)
    {

        CCreateMapGeoTiff::refpt_t& pt = geotifftool->refpts[-i];
        pt.item         = new QTreeWidgetItem();

        pt.item->setData(CCreateMapGeoTiff::eLabel,Qt::UserRole,-i);
        pt.item->setText(CCreateMapGeoTiff::eLabel,tr("%1").arg(i));
        pt.item->setText(CCreateMapGeoTiff::eLonLat,tr(""));

        QPoint center   = theMainWindow->getCanvas()->rect().center();
        IMap& map       = CMapDB::self().getMap();
        switch(i)
        {
            case 1:
                pt.x            = center.x() - 50;
                pt.y            = center.y();
                break;
            case 2:
                pt.x            = center.x() + 50;
                pt.y            = center.y();
                break;
            case 3:
                pt.x            = center.x() + 50;
                pt.y            = center.y() + 100;
                break;
            case 4:
                pt.x            = center.x() - 50;
                pt.y            = center.y() + 100;
                break;
        }
        map.convertPt2M(pt.x,pt.y);
    }
    theMainWindow->getCanvas()->update();
}


void CCreateMapGridTool::slotCheck()
{
    pushOk->setEnabled(false);
    if(lineLongitude->isEnabled() && lineLongitude->text().isEmpty()) return;
    if(lineLatitude->isEnabled() && lineLatitude->text().isEmpty()) return;
    if(lineXSpacing->text().isEmpty()) return;
    if(lineYSpacing->text().isEmpty()) return;
    pushOk->setEnabled(true);
}


void CCreateMapGridTool::slotOk()
{
    bool isLongLat = false;
    int res;
    double adfGeoTransform1[6], adfGeoTransform2[6];
    double northing = lineLatitude->text().toDouble();
    double easting  = lineLongitude->text().toDouble();
    double stepx    = lineXSpacing->text().toDouble();
    double stepy    = lineYSpacing->text().toDouble();

    int         n    = 0;
    GDAL_GCP  * gcps = 0;

    if(lineLatitude->isEnabled() && lineLongitude->isEnabled())
    {

        CCreateMapGeoTiff::refpt_t& pt1 = geotifftool->refpts[-1];
        CCreateMapGeoTiff::refpt_t& pt2 = geotifftool->refpts[-2];
        CCreateMapGeoTiff::refpt_t& pt3 = geotifftool->refpts[-3];
        CCreateMapGeoTiff::refpt_t& pt4 = geotifftool->refpts[-4];

        n = 4;
        gcps = new GDAL_GCP[n];
        memset(gcps,0,n*sizeof(GDAL_GCP));

        gcps[0].dfGCPPixel  = pt1.x;
        gcps[0].dfGCPLine   = pt1.y;
        gcps[0].dfGCPX      = easting;
        gcps[0].dfGCPY      = northing;

        gcps[1].dfGCPPixel  = pt2.x;
        gcps[1].dfGCPLine   = pt2.y;
        gcps[1].dfGCPX      = easting + stepx;
        gcps[1].dfGCPY      = northing;

        gcps[2].dfGCPPixel  = pt3.x;
        gcps[2].dfGCPLine   = pt3.y;
        gcps[2].dfGCPX      = easting + stepx;
        gcps[2].dfGCPY      = northing - stepy;

        gcps[3].dfGCPPixel  = pt4.x;
        gcps[3].dfGCPLine   = pt4.y;
        gcps[3].dfGCPX      = easting;
        gcps[3].dfGCPY      = northing - stepy;
    }
    else if(geotifftool->treeWidget->topLevelItemCount() > 2)
    {
        n = geotifftool->treeWidget->topLevelItemCount();
        gcps = new GDAL_GCP[n];
        memset(gcps,0,n*sizeof(GDAL_GCP));

        QMap<quint32,CCreateMapGeoTiff::refpt_t>::iterator pt = geotifftool->refpts.begin();
        for(int i = 0; i < n; ++i)
        {

            float lon, lat;
            if(!GPS_Math_Str_To_LongLat(pt->item->text(CCreateMapGeoTiff::eLonLat), lon, lat, "", lineProjection->text()))
            {
                delete [] gcps;
                return;
            }

            gcps[i].dfGCPPixel  = pt->x;
            gcps[i].dfGCPLine   = pt->y;
            gcps[i].dfGCPX      = lon;
            gcps[i].dfGCPY      = lat;

            ++pt;
        }
    }
    else
    {
        float lon, lat;
        n = 3;
        gcps = new GDAL_GCP[n];
        memset(gcps,0,n*sizeof(GDAL_GCP));

        // point 1
        QMap<quint32,CCreateMapGeoTiff::refpt_t>::iterator pt = geotifftool->refpts.begin();

        if(!GPS_Math_Str_To_LongLat(pt->item->text(CCreateMapGeoTiff::eLonLat), lon, lat, "", lineProjection->text()))
        {
            delete [] gcps;
            return;
        }
        gcps[0].dfGCPPixel  = pt->x;
        gcps[0].dfGCPLine   = pt->y;
        gcps[0].dfGCPX      = lon;
        gcps[0].dfGCPY      = lat;

        // point 2
        ++pt;

        if(!GPS_Math_Str_To_LongLat(pt->item->text(CCreateMapGeoTiff::eLonLat), lon, lat, "", lineProjection->text()))
        {
            delete [] gcps;
            return;
        }

        gcps[1].dfGCPPixel  = pt->x;
        gcps[1].dfGCPLine   = pt->y;
        gcps[1].dfGCPX      = lon;
        gcps[1].dfGCPY      = lat;

        // point 3
        double dx, dy, dX, dY, delta;
        double a, b;

        dx =   gcps[1].dfGCPPixel - gcps[0].dfGCPPixel;
        dy = -(gcps[1].dfGCPLine - gcps[0].dfGCPLine);

        dX =   gcps[1].dfGCPX - gcps[0].dfGCPX;
        dY =   gcps[1].dfGCPY - gcps[0].dfGCPY;

        delta = dx * dx + dy * dy;

        a = (dX * dx + dY * dy) / delta;
        b = (dY * dx - dX * dy) / delta;

        gcps[2].dfGCPPixel  = 0;
        gcps[2].dfGCPLine   = 0;
        gcps[2].dfGCPX      = gcps[1].dfGCPX - a * gcps[1].dfGCPPixel - b * gcps[1].dfGCPLine;
        gcps[2].dfGCPY      = gcps[1].dfGCPY - b * gcps[1].dfGCPPixel + a * gcps[1].dfGCPLine;

    }

    res = GDALGCPsToGeoTransform(n,gcps,adfGeoTransform1,TRUE);
    if(gcps) delete [] gcps; gcps = 0; n = 0;

    if(res == FALSE)
    {
        QMessageBox::warning(0,tr("Error ..."), tr("Failed to calculate transformation for ref. points. Are all 4 points placed propperly?"), QMessageBox::Abort,QMessageBox::Abort);
        return;
    }

    GDALInvGeoTransform(adfGeoTransform1, adfGeoTransform2);

    projPJ  pjWGS84 = 0, pjSrc = 0;
    if(!lineProjection->text().isEmpty())
    {
        pjSrc   = pj_init_plus(lineProjection->text().toLatin1());
        if(pjSrc == 0)
        {
            QMessageBox::warning(0,tr("Error ..."), tr("Failed to setup projection. Bad syntax?"), QMessageBox::Abort,QMessageBox::Abort);
            return;
        }
        pjWGS84 = pj_init_plus("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");

        isLongLat = lineProjection->text().contains("longlat");
    }

    double u1 = ((int)((adfGeoTransform1[0] + stepx) / stepx)) * stepx;
    double v1 = ((int)(adfGeoTransform1[3] / stepy)) * stepy;

    QRect rect = geotifftool->getSelArea();

    double u = u1, v = v1;
    double x = 0,  y = 0;
    bool go         = true;
    bool validLine  = false;
    int top         = rect.top();
    while(go)
    {
        GDALApplyGeoTransform(adfGeoTransform2, u, v, &x, &y);
        if(rect.contains(QPoint(x,y)))
        {
            // add ref point
            double _u = u;
            double _v = v;

            if(pjSrc)
            {
                if(isLongLat)
                {
                    _u *= DEG_TO_RAD;
                    _v *= DEG_TO_RAD;
                }
                pj_transform(pjSrc,pjWGS84,1,0,&_u,&_v,0);
                _u = _u * RAD_TO_DEG;
                _v = _v * RAD_TO_DEG;
            }

            geotifftool->addRef(x,y,_u,_v);
            validLine = true;
        }
        else
        {
            if(x > rect.right())
            {
                if(!validLine && (y > top)) break;
                validLine = false;

                u  = u1;
                v -= stepy;
                continue;
            }
        }

        u += stepx;
    }

    if(pjWGS84) pj_free(pjWGS84);
    if(pjSrc) pj_free(pjSrc);

    geotifftool->lineGCPProjection->setText("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");
    geotifftool->comboMode->setCurrentIndex(geotifftool->comboMode->findData(CCreateMapGeoTiff::eQuadratic));

    theMainWindow->getCanvas()->update();
    deleteLater();
}


void CCreateMapGridTool::slotProjWizard()
{
    CDlgProjWizzard dlg(*lineProjection, this);
    dlg.exec();
}
