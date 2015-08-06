/**********************************************************************************************
    Copyright (C) 2007-2009 Oliver Eichler oliver.eichler@gmx.de

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

#include <iostream>

#include <QtCore>
#include <QtGui>
#include <QRegExp>
#include <gdal_priv.h>
#include <proj_api.h>
#ifdef QK_QT5_PORT
#include <QQuickView>
#endif
#include <QSplashScreen>
#include <QMessageBox>

#include "CGetOpt.h"
#include "CAppOpts.h"

#ifdef __MINGW32__
#undef LP
#endif

#include "CApplication.h"
#include "CMainWindow.h"
#include "CGarminTyp.h"

#include "config.h"
#include "version.h"

static void usage(std::ostream &s)
{
    s << "usage: qlandkartegt [-d | --debug]\n"
        "                    [-h | --help]\n"
        "                    [-m FD | --monitor=FD]\n"
        "                    [-n | --no-splash]\n"
        "                    [-c | --config=file]\n"
        "                    [files...]\n"
        "\n"
        "The monitor function will read data from files if there is input on stream FD.\n"
        "For stdin use FD=0.\n\n";
}


static const QString text = QObject::tr(
"There is a problem with your Proj4 library and localization. The key issue is "
"that the floating point definition in your localization is different from what "
"Proj4 uses for it's correction tables ('1.2' vs '1,2'). That might cause an "
"offset when using raster maps. Vector maps are not affected, as they use a "
"projection that works without a textual table. "

"");

#ifdef QK_QT5_PORT
static void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
#else
static void myMessageOutput(QtMsgType type, const char *msg)
#endif
{
    switch (type)
    {
        case QtDebugMsg:
            if (qlOpts->debug)
            {
#ifdef QK_QT5_PORT
                std::cout << msg.toUtf8().constData() << std::endl;
#else
                puts(msg);
#endif
            }

            break;

        case QtWarningMsg:
#ifdef QK_QT5_PORT
            std::cerr << "Warning: " << msg.toUtf8().constData() << std::endl;
#else
            fprintf(stderr, "Warning: %s\n", msg);
#endif
            break;

        case QtCriticalMsg:
#ifdef QK_QT5_PORT
            std::cerr << "Critical: " <<  msg.toUtf8().constData() << std::endl;
#else
            fprintf(stderr, "Critical: %s\n", msg);
#endif
            break;
        case QtFatalMsg:
#ifdef QK_QT5_PORT
            std::cerr << "Fatal: " << msg.toUtf8().constData() << std::endl;
#else
            fprintf(stderr, "Fatal: %s\n", msg);
#endif
            abort();
    }
}


CAppOpts *qlOpts;

static void processOptions()
{
    CGetOpt opts;                // uses qApp->argc() and qApp->argv()
    bool dValue;
    opts.addSwitch('d', "debug", &dValue);
    bool hValue;
    opts.addSwitch('h', "help", &hValue);
    QString mValue;
    opts.addOptionalOption('m', "monitor", &mValue, "0");
    bool nValue;
    opts.addSwitch('n', "no-splash", &nValue);
    QStringList args;
    opts.addOptionalArguments("files", &args);
    QString config;
    opts.addOptionalOption('c', "config", &config, "");

    if (!opts.parse())
    {
        usage(std::cerr);
        exit(1);
    }

    if (hValue)
    {
        usage(std::cout);
        exit(0);
    }

    int m = -1;
    if (mValue != QString::null)
    {
        bool ok;
        m = mValue.toInt(&ok);
        if (!ok)
        {
            usage(std::cerr);
            exit(1);
        }
    }

    qDebug() << "use config file:" << config;
    qlOpts = new CAppOpts(dValue,// bool debug
        m,                       // int monitor
        nValue,                  // bool nosplash
        config,                  // optional config file
        args);                   // arguments
}


int main(int argc, char ** argv)
{
    QString str1, str2;

    {
        projPJ  pjWGS84 = pj_init_plus("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");
        projPJ  pjGK    = pj_init_plus("+proj=tmerc +lat_0=0 +lon_0=12 +k=1 +x_0=4500000 +y_0=0 +ellps=bessel +datum=potsdam +units=m +no_defs +towgs84=606.0,23.0,413.0");

        //     double x = 12.09    * DEG_TO_RAD;
        //     double y = 49.0336  * DEG_TO_RAD;
        double x = 0.211011;
        double y = 0.855797;
        pj_transform(pjWGS84,pjGK,1,0,&x,&y,0);

        //        printf("------------ %f %f\n", x, y);
        char * ptr = pj_get_def(pjGK,0);
        //        printf("------------ %s\n",ptr);
        str1 = ptr;

        pj_free(pjWGS84);
        pj_free(pjGK);
    }

    QDir path(QDir::home().filePath(CONFIGDIR));
    if(!path.exists())
    {
        path.mkpath("./");
    }

    QString locale = QLocale::system().name();

#ifndef WIN32
    setenv("LC_NUMERIC","C",1);
#endif
    CApplication theApp(argc,argv);
    processOptions();

#ifndef WIN32
#ifdef QK_QT5_PORT
    qInstallMessageHandler(myMessageOutput);
#else
    qInstallMsgHandler(myMessageOutput);
#endif
#endif

#ifdef WIN32
    // setup environment variables for GDAL/Proj4
    QString apppath = QCoreApplication::applicationDirPath();
    apppath = apppath.replace("/", "\\");

    QString env_path = qgetenv("PATH");
    env_path += QString(";%1;%1\\proj\\apps;%1\\gdal\\apps;%1\\curl;").arg(apppath);
    qputenv("PATH", env_path.toUtf8());

    qputenv("GDAL_DATA", QString("%1\\gdal-data").arg(apppath).toUtf8());
    qputenv("GDAL_DRIVER_PATH", QString("%1\\gdal\\plugins;").arg(apppath).toUtf8());
    qputenv("PROJ_LIB", QString("%1\\proj\\SHARE").arg(apppath).toUtf8());
#endif

#ifdef ENABLE_TRANSLATION
    {
        QString resourceDir = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
        QTranslator *qtTranslator = new QTranslator(0);
        if (qtTranslator->load(QLatin1String("qt_") + locale,resourceDir))
        {
            theApp.installTranslator(qtTranslator);
        }
        else if (qtTranslator->load(QLatin1String("qt_") + locale,QCoreApplication::applicationDirPath()))
        {
            theApp.installTranslator(qtTranslator);
        }

        QStringList dirList;
#ifndef Q_OS_MAC
        dirList << ".";
        dirList << "src";
#ifndef Q_OS_WIN32
        dirList << QCoreApplication::applicationDirPath().replace(QRegExp("bin$"), "share/qlandkartegt/translations");
#else
        dirList << QCoreApplication::applicationDirPath();
#endif
#else
        dirList << QCoreApplication::applicationDirPath().replace(QRegExp("MacOS$"), "Resources");
#endif
        QTranslator *qlandkartegtTranslator = new QTranslator(0);
        qDebug() << dirList;
        foreach(QString dir, dirList)
        {
            QString transName = QLatin1String("qlandkartegt_") + locale;
            if (qlandkartegtTranslator->load( transName, dir))
            {
                theApp.installTranslator(qlandkartegtTranslator);
                qDebug() << "using file '"+ QDir(dir).canonicalPath() + "/" + transName + ".qm' for translations.";
                break;
            }
        }
    }
#endif

    {
        projPJ  pjWGS84 = pj_init_plus("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");
        projPJ  pjGK    = pj_init_plus("+proj=tmerc +lat_0=0 +lon_0=12 +k=1 +x_0=4500000 +y_0=0 +ellps=bessel +datum=potsdam +units=m +no_defs +towgs84=606.0,23.0,413.0");

        //     double x = 12.09    * DEG_TO_RAD;
        //     double y = 49.0336  * DEG_TO_RAD;
        double x = 0.211011;
        double y = 0.855797;
        pj_transform(pjWGS84,pjGK,1,0,&x,&y,0);

        //        printf("------------ %f %f\n", x, y);
        char * ptr = pj_get_def(pjGK,0);
        //        printf("------------ %s\n",ptr);
        str2 = ptr;

        pj_free(pjWGS84);
        pj_free(pjGK);
    }

    GDALAllRegister();

#ifdef Q_OS_MAC
    QCoreApplication::setApplicationName("QLandkarteGT");
    QCoreApplication::setApplicationVersion(VER_STR);
    QCoreApplication::setOrganizationName("org.qlandkarte");
    QCoreApplication::setOrganizationDomain("org.qlandkarte");
#else
    QCoreApplication::setApplicationName("QLandkarteGT");
    QCoreApplication::setOrganizationName("QLandkarteGT");
    QCoreApplication::setOrganizationDomain("qlandkarte.org");
#endif
    QApplication::setWindowIcon(QIcon(":/icons/qlandkartegt.png"));

#ifdef WIN32
    QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath());
    qDebug() << QCoreApplication::applicationDirPath();
    qDebug() << QCoreApplication::libraryPaths();
#endif

    QSplashScreen *splash = 0;
    if (!qlOpts->nosplash)
    {
        QPixmap pic(":/pics/splash.png");
        QPainter p(&pic);
        QFont f = p.font();
        f.setBold(true);

        p.setPen(Qt::white);
        p.setFont(f);
        p.drawText(400,370,"V " VER_STR);

        splash = new QSplashScreen(pic);
        splash->show();
    }
    CMainWindow w;
    w.show();
    if (splash != 0)
    {
        splash->finish(&w);
        delete splash;
    }

    if(str1 != str2)
    {
        QMessageBox::warning(0, "There is a problem....", text, QMessageBox::Ok, QMessageBox::Ok);
    }

    int res  = theApp.exec();

    //delete qlOpts;

    return res;
}
