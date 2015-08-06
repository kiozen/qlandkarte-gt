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
#ifndef CMAPQMAPEXPORT_H
#define CMAPQMAPEXPORT_H

#include <QDialog>
#include <QProcess>
#include <QFile>
#include <QList>
#include <QPointer>
#include <QTemporaryFile>
#include "ui_IMapQMAPExport.h"

class CMapSelectionRaster;
class CMapQMAPExport;
class QDir;

class IMapExportState : public QObject
{
    Q_OBJECT;
    public:
        IMapExportState(CMapQMAPExport * parent);
        virtual ~IMapExportState();

        virtual void explain() = 0;
        virtual void nextJob(QProcess& cmd) = 0;

        int getJobIdx(){return jobIdx;}
        virtual int getJobCnt() = 0;

        static QString getTempFilename();
    protected:
        CMapQMAPExport * gui;
        int jobIdx;
    private:
        static quint32 tmpFileCnt;
};

class CMapExportStateCutFiles : public IMapExportState
{
    Q_OBJECT;
    public:
        CMapExportStateCutFiles(CMapQMAPExport * parent);
        virtual ~CMapExportStateCutFiles();

        void explain();
        void nextJob(QProcess& cmd);
        int getJobCnt(){return jobs.count();}

        struct job_t
        {
            QString srcFile;
            QString tarFile;

            quint32 xoff;
            quint32 yoff;
            quint32 width;
            quint32 height;

            int level;
        };

        void addJob(const job_t& job){jobs << job;}
        const QList<job_t>& getJobs(){return jobs;}

    private:
        QList<job_t> jobs;

};

class CMapExportStateReadTileCache : public IMapExportState
{
    Q_OBJECT;
    public:
        CMapExportStateReadTileCache(const QString &app, CMapQMAPExport *parent);
        virtual ~CMapExportStateReadTileCache();

        void explain();
        void nextJob(QProcess& cmd);
        int getJobCnt(){return jobs.count();}

        struct job_t
        {
            QString srcFile;
            QString tarFile;

            double lon1;
            double lat1;
            double lon2;
            double lat2;

            int level;
        };

        void addJob(const job_t& job){jobs << job;}
        const QList<job_t>& getJobs(){return jobs;}

    private:
        QString app;
        QList<job_t> jobs;
};

class CMapExportStateCombineFiles : public IMapExportState
{
    Q_OBJECT;
    public:
        CMapExportStateCombineFiles(CMapQMAPExport * parent);
        virtual ~CMapExportStateCombineFiles();

        void explain();
        void nextJob(QProcess& cmd);
        int getJobCnt(){return jobs.count();}

        struct job_t
        {
            QStringList srcFile;
            QString tarFile;
        };

        void addJob(const job_t& job){jobs << job;}
        const QList<job_t>& getJobs(){return jobs;}

    private:
        QList<job_t> jobs;

};

class CMapExportStateConvColor : public IMapExportState
{
    Q_OBJECT;
    public:
        CMapExportStateConvColor(CMapQMAPExport * parent);
        virtual ~CMapExportStateConvColor();

        void explain();
        void nextJob(QProcess& cmd);
        int getJobCnt(){return jobs.count();}

        struct job_t
        {
            QString srcFile;
            QString tarFile;
        };

        void addJob(const job_t& job){jobs << job;}
        const QList<job_t>& getJobs(){return jobs;}

    private:
        QList<job_t> jobs;

};

class CMapExportStateReproject : public IMapExportState
{
    Q_OBJECT;
    public:
        CMapExportStateReproject(const QString& proj, CMapQMAPExport * parent);
        virtual ~CMapExportStateReproject();

        void explain();
        void nextJob(QProcess& cmd);
        int getJobCnt(){return jobs.count();}

        struct job_t
        {
            QString srcFile;
            QString tarFile;
        };

        void addJob(const job_t& job){jobs << job;}
        const QList<job_t>& getJobs(){return jobs;}

    private:
        QList<job_t> jobs;
        QString proj;

};

class CMapExportStateOptimize : public IMapExportState
{
    Q_OBJECT;
    public:
        CMapExportStateOptimize(CMapQMAPExport * parent);
        virtual ~CMapExportStateOptimize();

        void explain();
        void nextJob(QProcess& cmd);
        int getJobCnt(){return jobs.count();}

        struct job_t
        {
            QStringList overviews;
            QString srcFile;
        };

        void addJob(const job_t& job){jobs << job;}
        const QList<job_t>& getJobs(){return jobs;}

    private:
        QList<job_t> jobs;

};

class CMapExportStateGCM : public IMapExportState
{
    Q_OBJECT;
    public:
        CMapExportStateGCM(const QString& app, CMapQMAPExport * parent);
        virtual ~CMapExportStateGCM();

        void explain();
        void nextJob(QProcess& cmd);
        int getJobCnt(){return jobs.count();}

        struct job_t
        {
            QString jpegQuality;
            QString jpegSubSmpl;
            QString zOrder;
            QString tileFile;
            QStringList srcFile;
            QString tarFile;
        };

        void addJob(const job_t& job){jobs << job;}
        const QList<job_t>& getJobs(){return jobs;}

    private:
        QList<job_t> jobs;

        const QString app;

};

class CMapExportStateJNX : public IMapExportState
{
    Q_OBJECT;
    public:
        CMapExportStateJNX(const QString& app, CMapQMAPExport * parent);
        virtual ~CMapExportStateJNX();

        void explain();
        void nextJob(QProcess& cmd);
        int getJobCnt(){return jobs.count();}

        struct job_t
        {
            QString jpegQuality;
            QString jpegSubSmpl;
            QString zOrder;
            QString productId;
            QString productName;
            QString description;
            QString copyright;
            QStringList srcFile;
            QString tarFile;
        };

        void addJob(const job_t& job){jobs << job;}
        const QList<job_t>& getJobs(){return jobs;}

    private:
        QList<job_t> jobs;

        const QString app;

};

class CMapExportStateRMAP : public IMapExportState
{
    Q_OBJECT;
    public:
        CMapExportStateRMAP(const QString& app, CMapQMAPExport * parent);
        virtual ~CMapExportStateRMAP();

        void explain();
        void nextJob(QProcess& cmd);
        int getJobCnt(){return jobs.count();}

        struct job_t
        {
            QString jpegQuality;
            QString jpegSubSmpl;
            QStringList srcFile;
            QString tarFile;
        };

        void addJob(const job_t& job){jobs << job;}
        const QList<job_t>& getJobs(){return jobs;}

    private:
        QList<job_t> jobs;

        const QString app;

};

class CMapExportStateRMP : public IMapExportState
{
    Q_OBJECT;
    public:
        CMapExportStateRMP(const QString& app, CMapQMAPExport * parent);
        virtual ~CMapExportStateRMP();

        void explain();
        void nextJob(QProcess& cmd);
        int getJobCnt(){return jobs.count();}

        struct job_t
        {
            QString jpegQuality;
            QString jpegSubSmpl;
            QString provider;
            QString product;
            QString copyright;
            QStringList srcFile;
            QString tarFile;
        };

        void addJob(const job_t& job){jobs << job;}
        const QList<job_t>& getJobs(){return jobs;}

    private:
        QList<job_t> jobs;

        const QString app;

};

class CMapQMAPExport : public QDialog, private Ui::IMapQMAPExport
{
    Q_OBJECT;
    public:
        CMapQMAPExport(const CMapSelectionRaster& mapsel, QWidget * parent);
        virtual ~CMapQMAPExport();

        void stdOut(const QString& str, bool gui = false);
        void stdErr(const QString& str, bool gui = false);

        void setNextState();

    public slots:
        void slotFinished(int exitCode, QProcess::ExitStatus status);

    private slots:
        void slotBirdsEyeToggled(bool checked);
        void slotQLMToggled(bool checked);
        void slotGCMToggled(bool checked);
        void slotRMAPToggled(bool checked);
        void slotRMPToggled(bool checked);
        void slotOutputPath();

        void slotStderr();
        void slotStdout();
        void slotStart();
        void slotCancel();
        void slotDetails();

        void slotSetupProj();
        void slotSetupProjFromMap();

        void slotSelectCopyright();

    private:
        void startExportGDAL();
        void startExportStreaming();
        void startExportCommon(QStringList& srcFiles, QDir& tarPath, const QString& prefix);
        void progress(const QString& str);

        const CMapSelectionRaster& mapsel;

        bool tainted;
        bool has_map2jnx;
        QString path_map2jnx;
        QString path_map2gcm;
        QString path_cache2gtiff;
        QString path_map2rmap;
        QString path_map2rmp;

        QProcess cmd;

        QList<IMapExportState*> states;
        QPointer<IMapExportState> state;

        QString output;

        int totalNumberOfStates;

        QString copyright;
};
#endif                           //CMAPQMAPEXPORT_H
