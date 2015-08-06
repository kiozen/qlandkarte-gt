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
#ifndef CDLGCROPMAP_H
#define CDLGCROPMAP_H

#include <QObject>
#include <QDialog>
#include <QProcess>
#include <QPointer>
#include <QList>

#include "ui_IDlgCropMap.h"

class CDlgCropMap;

class IMapCropState : public QObject
{
    Q_OBJECT;
    public:
        IMapCropState(CDlgCropMap * parent);
        virtual ~IMapCropState();

        virtual void explain() = 0;
        virtual void nextJob(QProcess& cmd) = 0;

        static QString getTempFilename();
    protected:
        CDlgCropMap * gui;
        int jobIdx;
    private:
        static quint32 tmpFileCnt;
};

class CMapCropStateCrop : public IMapCropState
{
    Q_OBJECT;
    public:
        CMapCropStateCrop(CDlgCropMap * parent);
        virtual ~CMapCropStateCrop();

        void explain();
        void nextJob(QProcess& cmd);

        struct job_t
        {
            QString srcFile;
            QString tarFile;

            quint32 xoff;
            quint32 yoff;
            quint32 width;
            quint32 height;
        };

        void addJob(const job_t& job){jobs << job;}

    private:
        QList<job_t> jobs;

};

class CMapCropStateOptimize : public IMapCropState
{
    Q_OBJECT;
    public:
        CMapCropStateOptimize(CDlgCropMap * parent);
        virtual ~CMapCropStateOptimize();

        void explain();
        void nextJob(QProcess& cmd);

        struct job_t
        {
            QStringList overviews;
            QString srcFile;
        };

        void addJob(const job_t& job){jobs << job;}

    private:
        QList<job_t> jobs;

};

class CDlgCropMap : public QDialog, private Ui::IDlgCropMap
{
    Q_OBJECT;
    public:
        CDlgCropMap(const QString& filename, quint32 x, quint32 y, quint32 w, quint32 h);
        virtual ~CDlgCropMap();

        void stdOut(const QString& str, bool gui = false);
        void stdErr(const QString& str, bool gui = false);

        void setNextState();

    public slots:
        void slotFinished(int exitCode, QProcess::ExitStatus status);

    private slots:
        void slotStderr();
        void slotStdout();
        void slotStart();
        void slotCancel();
        void slotDetails();

    private:
        void progress(const QString& str);

        const QString filename;
        const quint32 x;
        const quint32 y;
        const quint32 w;
        const quint32 h;

        QProcess cmd;

        QList<IMapCropState*> states;
        QPointer<IMapCropState> state;

        QString output;
        bool tainted;

        int totalNumberOfStates;
};
#endif                           //CDLGCROPMAP_H
