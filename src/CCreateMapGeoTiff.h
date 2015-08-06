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
#ifndef CCREATEMAPGEOTIFF_H
#define CCREATEMAPGEOTIFF_H

#include <QWidget>
#include <QProcess>
#include <QPointer>
#include <QDir>

#include <gdal.h>

#include "ui_ICreateMapGeoTiff.h"

class QTemporaryFile;

class CCreateMapGeoTiff : public QWidget, private Ui::ICreateMapGeoTiff
{
    Q_OBJECT;
    public:
        CCreateMapGeoTiff(QWidget * parent);
        virtual ~CCreateMapGeoTiff();

        static CCreateMapGeoTiff * self(){return m_self;}

        struct refpt_t
        {
            refpt_t() : x(0), y(0), item(0){}

            double x;
            double y;

            QTreeWidgetItem * item;
        };

        enum columns_e
        {

            eLabel        = 0
            , eLonLat       = 1
            , eX            = 2
            , eY            = 3
            , eMaxColumn    = 4
        };

        enum mode_e
        {
            eThinPlate = -1
            , eSquare = -2
            , eLinear = 1
            , eQuadratic = 2

        };

        QMap<quint32,refpt_t>& getRefPoints(){return refpts;}

        void selRefPointByKey(const quint32 key);

        bool eventFilter(QObject * watched, QEvent * event);

        QRect& getSelArea(){return rectSelArea;}

    protected:
        void keyPressEvent(QKeyEvent * e);

    private slots:
        void slotOpenFile();
        void slotOutFile();
        void slotReload();
        void slotModeChanged(int);
        void slotAddRef();
        void slotDelRef();
        void slotLoadRef();
        void slotSaveRef();
        void slotGridTool();
        void slotSelectionChanged();
        void slotItemChanged(QTreeWidgetItem * item, int column);
        void slotItemDoubleClicked(QTreeWidgetItem * item);
        void slotGoOn();
        void slotStderr();
        void slotStdout();
        void slotFinished( int exitCode, QProcess::ExitStatus status);
        void slotClearAll();
        void slotProjWizard();
        void slotGCPProjWizard();

    private:
        friend class CCreateMapGridTool;
        static CCreateMapGeoTiff * m_self;

        void enableStep2();
        void enableStep3(bool doEnable);
        void cleanupTmpFiles();
        int getNumberOfGCPs();

        void loadGCP(const QString& filename);
        void loadTAB(const QString& filename);

        void saveGCP(const QString& filename);
        void saveTAB(const QString& filename);

        void gdalGCP2RefPt(const GDAL_GCP* gcps, int n);

        void addRef(double x, double y, double u, double v);

        QMap<quint32,refpt_t> refpts;
        quint32 refcnt;

        QProcess cmd;

        enum state_e {eNone, eTranslate, eWarp, eTile, eOverview};
        state_e state;

        QPointer<QTemporaryFile> tmpfile1;
        QPointer<QTemporaryFile> tmpfile2;

        QDir path;
        bool closemap;

        QSize sizeMap;
        QRect rectSelArea;
};
#endif                           //CCREATEMAPGEOTIFF_H
