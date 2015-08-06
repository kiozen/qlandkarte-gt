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
#include "CDlgCropMap.h"
#include "CMapFile.h"
#include "CSettings.h"

#include <QtGui>
#include <QScrollBar>

CDlgCropMap::CDlgCropMap(const QString &filename, quint32 x, quint32 y, quint32 w, quint32 h)
: filename(filename)
, x(x)
, y(y)
, w(w)
, h(h)
, tainted(false)
, totalNumberOfStates(0)
{
    setupUi(this);
    QFileInfo fi(filename);

    labelSrcFile->setText(filename);
    labelTarFile->setText(QString("%1/%2_crop.%3").arg(fi.absolutePath()).arg(fi.baseName()).arg(fi.completeSuffix()));

    connect(pushStart, SIGNAL(clicked()), this, SLOT(slotStart()));
    connect(pushCancel, SIGNAL(clicked()), this, SLOT(slotCancel()));
    connect(pushDetails, SIGNAL(clicked()), this, SLOT(slotDetails()));

    connect(&cmd, SIGNAL(readyReadStandardError()), this, SLOT(slotStderr()));
    connect(&cmd, SIGNAL(readyReadStandardOutput()), this, SLOT(slotStdout()));
    connect(&cmd, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(slotFinished(int,QProcess::ExitStatus)));

    SETTINGS;
    checkOverview2x->setChecked(cfg.value("map/export/over2x", true).toBool());
    checkOverview4x->setChecked(cfg.value("map/export/over4x", true).toBool());
    checkOverview8x->setChecked(cfg.value("map/export/over8x", true).toBool());
    checkOverview16x->setChecked(cfg.value("map/export/over16x", true).toBool());

    if(cfg.value("map/export/hidedetails", true).toBool())
    {
        textBrowser->hide();
    }
    else
    {
        textBrowser->show();
    }

    QFont f = font();
    f.setFamily("Mono");
    textBrowser->setFont(f);

    progressBar->setValue(0);

    adjustSize();

}


CDlgCropMap::~CDlgCropMap()
{
    SETTINGS;
    cfg.setValue("map/export/over2x",checkOverview2x->isChecked());
    cfg.setValue("map/export/over4x",checkOverview4x->isChecked());
    cfg.setValue("map/export/over8x",checkOverview8x->isChecked());
    cfg.setValue("map/export/over16x",checkOverview16x->isChecked());

    cfg.setValue("map/export/hidedetails", textBrowser->isHidden());

}


void CDlgCropMap::progress(const QString& str)
{
    QRegExp re("^(0[0-9\\.]*).*$");

    output += str;
    QStringList lines = output.split("\n");

    if(re.exactMatch(lines.last()))
    {
        QString prog    = re.cap(1);
        int points      = prog.count('.');
        int zeros       = prog.count('0');
        int p = (zeros - 1) * 10 + (points%3) * 2.5 + ((points/3) == zeros ? 7.5 : 0);
        if(p > 100) p = 100;

        progressBar->setValue(p);
    }

}


void CDlgCropMap::stdOut(const QString& str, bool gui)
{
    textBrowser->setTextColor(Qt::black);
    textBrowser->append(str);

    if(gui)
    {
        QPalette palette = labelStatus->palette();
        palette.setColor(labelStatus->foregroundRole(), Qt::black);
        labelStatus->setPalette(palette);
        labelStatus->setText(str.simplified());
    }
}


void CDlgCropMap::stdErr(const QString& str, bool gui)
{
    textBrowser->setTextColor(Qt::red);
    textBrowser->append(str);

    if(gui)
    {
        QPalette palette = labelStatus->palette();
        palette.setColor(labelStatus->foregroundRole(), Qt::red);
        labelStatus->setPalette(palette);
        labelStatus->setText(str.simplified());
    }
}


void CDlgCropMap::slotStderr()
{
    QString str;
    textBrowser->setTextColor(Qt::red);

    str = cmd.readAllStandardError();

#ifndef WIN32
    if(str[0] == '\r')
    {
        textBrowser->moveCursor( QTextCursor::End, QTextCursor::MoveAnchor );
        textBrowser->moveCursor( QTextCursor::StartOfLine, QTextCursor::MoveAnchor );
        textBrowser->moveCursor( QTextCursor::End, QTextCursor::KeepAnchor );
        textBrowser->textCursor().removeSelectedText();

        str = str.split("\r").last();
    }
#endif

    textBrowser->insertPlainText(str);
    textBrowser->verticalScrollBar()->setValue(textBrowser->verticalScrollBar()->maximum());

    tainted = true;
}


void CDlgCropMap::slotStdout()
{
    QString str;
    textBrowser->setTextColor(Qt::blue);
    str = cmd.readAllStandardOutput();

#ifndef WIN32
    if(str[0] == '\r')
    {
        textBrowser->moveCursor( QTextCursor::End, QTextCursor::MoveAnchor );
        textBrowser->moveCursor( QTextCursor::StartOfLine, QTextCursor::MoveAnchor );
        textBrowser->moveCursor( QTextCursor::End, QTextCursor::KeepAnchor );
        textBrowser->textCursor().removeSelectedText();

        str = str.split("\r").last();
    }
#endif

    progress(str);

    textBrowser->insertPlainText(str);
    textBrowser->verticalScrollBar()->setValue(textBrowser->verticalScrollBar()->maximum());
}


void CDlgCropMap::setNextState()
{
    output.clear();

    if(!state.isNull()) state->deleteLater();

    if(states.isEmpty())
    {
        stdOut(tr("*** done ***\n"), true);
        if(tainted)
        {
            stdErr(tr("Warnings. See \"Details\" for more information.\n"), true);
        }
        progressBar->setValue(100);
        pushStart->setEnabled(true);
        pushCancel->setText(tr("Close"));
    }
    else
    {
        state = states.takeFirst();
        state->explain();
        state->nextJob(cmd);

        labelStep->setText(tr("Step %1/%2,").arg(totalNumberOfStates - states.count()).arg(totalNumberOfStates));
    }
}


void CDlgCropMap::slotFinished(int exitCode, QProcess::ExitStatus status)
{
    output.clear();

    if(exitCode || status)
    {

        textBrowser->setTextColor(Qt::red);
        textBrowser->append(tr("!!! failed !!!\n"));

        labelStatus->setText(tr("Failed. See \"Details\" for more information."));

        qDeleteAll(states);
        states.clear();
        state->deleteLater();

        pushStart->setEnabled(true);
        pushCancel->setText(tr("Close"));
        return;
    }

    QApplication::processEvents();
    state->nextJob(cmd);
}


void CDlgCropMap::slotCancel()
{
    if(cmd.state() != QProcess::NotRunning)
    {
        stdOut(tr("\nCanceled by user's request.\n"));

        cmd.kill();
        cmd.waitForFinished(1000);

        QFile::remove(labelTarFile->text());
    }
    else
    {
        reject();
    }
}


void CDlgCropMap::slotStart()
{
    if(!states.isEmpty() || !state.isNull()) return;
    pushStart->setEnabled(false);
    pushCancel->setText(tr("Cancel"));
    textBrowser->clear();

    tainted = false;

    CMapCropStateCrop * state1 =  new CMapCropStateCrop(this);
    {
        CMapCropStateCrop::job_t job;
        job.srcFile = labelSrcFile->text();
        job.tarFile = labelTarFile->text();
        job.xoff    = x;
        job.yoff    = y;
        job.width   = w;
        job.height  = h;

        state1->addJob(job);
        states << state1;
    }

    QStringList overviews;
    if(checkOverview2x->isChecked())  overviews << "2";
    if(checkOverview4x->isChecked())  overviews << "4";
    if(checkOverview8x->isChecked())  overviews << "8";
    if(checkOverview16x->isChecked()) overviews << "16";

    if(!overviews.isEmpty())
    {
        CMapCropStateOptimize * state2 =  new CMapCropStateOptimize(this);
        CMapCropStateOptimize::job_t job;

        job.srcFile     = labelTarFile->text();
        job.overviews   = overviews;
        state2->addJob(job);
        states << state2;
    }

    totalNumberOfStates = states.count();
    // start the statemachine
    setNextState();
}


void CDlgCropMap::slotDetails()
{
    if(textBrowser->isHidden())
    {
        textBrowser->show();
    }
    else
    {
        textBrowser->hide();
    }

    adjustSize();
}


// --------------------------------------------------------------------------------------------

quint32 IMapCropState::tmpFileCnt = 0;

IMapCropState::IMapCropState(CDlgCropMap * parent)
: QObject(parent)
, gui(parent)
, jobIdx(0)
{

}


IMapCropState::~IMapCropState()
{

}


QString IMapCropState::getTempFilename()
{
    QTemporaryFile * tmp = new QTemporaryFile(QDir::temp().absoluteFilePath(QString("qlgt_%1.XXXXXX.tif").arg(tmpFileCnt++)));
    tmp->open();
    QString fn =  tmp->fileName();
    tmp->close();
    delete tmp;

    return fn;
}


// --------------------------------------------------------------------------------------------
CMapCropStateCrop::CMapCropStateCrop(CDlgCropMap * parent)
: IMapCropState(parent)

{
}


CMapCropStateCrop::~CMapCropStateCrop()
{
    qDebug() << "~CMapCropStateCrop()";
}


void CMapCropStateCrop::explain()
{
    gui->stdOut(   "*************************************");
    gui->stdOut(tr("Cut area from files..."), true);
    gui->stdOut(   "-------------------------------------");
}


void CMapCropStateCrop::nextJob(QProcess& cmd)
{
    if(jobIdx < jobs.count())
    {
        job_t& job = jobs[jobIdx];

        QStringList args;
        args << "-co" << "tiled=yes" << "-co" << "compress=DEFLATE";
        args << "-srcwin";
        args << QString::number(job.xoff) << QString::number(job.yoff);
        args << QString::number(job.width) << QString::number(job.height);
        args << job.srcFile;
        args << job.tarFile;

        jobIdx++;

        gui->stdOut(GDALTRANSLATE " " +  args.join(" ") + "\n");
        cmd.start(GDALTRANSLATE, args);

    }
    else
    {
        gui->setNextState();
    }
}


// --------------------------------------------------------------------------------------------
CMapCropStateOptimize::CMapCropStateOptimize(CDlgCropMap * parent)
: IMapCropState(parent)

{

}


CMapCropStateOptimize::~CMapCropStateOptimize()
{
    qDebug() << "~CMapCropStateOptimize()";
}


void CMapCropStateOptimize::explain()
{
    gui->stdOut(   "*************************************");
    gui->stdOut(tr("Optimize file..."), true);
    gui->stdOut(   "-------------------------------------");
}


void CMapCropStateOptimize::nextJob(QProcess& cmd)
{
    if(jobIdx < jobs.count())
    {
        job_t& job = jobs[jobIdx];

        if(job.overviews.isEmpty())
        {
            gui->stdOut(tr("nothing to do\n"));
            jobIdx++;

            gui->slotFinished(0, QProcess::NormalExit);
            return;
        }

        QStringList args;
        args << "-r" << "cubic";
        args << "--config" << "COMPRESS_OVERVIEW" << "JPEG";
        args << job.srcFile;
        args += job.overviews;

        jobIdx++;

        gui->stdOut(GDALADDO " " +  args.join(" ") + "\n");
        cmd.start(GDALADDO, args);
    }
    else
    {
        gui->setNextState();
    }
}
