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

#include "CDlgMapQMAPConfig.h"
#include "CMapQMAP.h"

#include <QtGui>

#ifndef WIN32
#include <unistd.h>
#include <pwd.h>
#endif

const QString CDlgMapQMAPConfig::text =  QObject::tr(""
"<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.01 Transitional//EN'  'http://www.w3.org/TR/html4/loose.dtd'>"
"<html>"
"   <head>"
"       <title></title>"
"       <META HTTP-EQUIV='CACHE-CONTROL' CONTENT='NO-CACHE'>"
"       <meta http-equiv='Content-Type' content='text/html; charset=utf-8'>"
"       <style type='text/css'>"
"           p, li { white-space: pre-wrap;}"
"           td {padding-top: 3px;}"
"           th {background-color: darkBlue; color: white;}"
"       </style>"
"   </head>"
"   <body style=' font-family: sans-serif; font-size: 9pt; font-weight:400; font-style:normal;'>"
"       <p>${info}</p>"
"   </body>"
"</html>"
"");

CDlgMapQMAPConfig::CDlgMapQMAPConfig(CMapQMAP * map)
: map(map)
{
    setupUi(this);

    QDir tempDir;
#ifndef Q_OS_WIN32
    const char *envCache = getenv("QLGT_LEGEND");

    if (envCache)
    {
        tempDir = envCache;
    }
    else
    {
        struct passwd * userInfo = getpwuid(getuid());
        tempDir = QDir::tempPath() + "/qlandkarteqt-" + userInfo->pw_name + "/legend/";
    }
#else
    tempDir = QDir::tempPath() + "/qlandkarteqt/cache/";
#endif
    tempDir.mkpath(tempDir.path());

    QString cpytext = text;
    cpytext = cpytext.replace("${info}", map->getMapInfo());

    webView->page()->settings()->clearMemoryCaches();

    QFile tmp(tempDir.filePath("legend.html"));
    tmp.open(QIODevice::WriteOnly);
    tmp.write(cpytext.toLocal8Bit());
    tmp.close();

    webView->load(QUrl::fromLocalFile(tmp.fileName()));

    connect(this, SIGNAL(accepted()), SLOT(deleteLater()));
    connect(this, SIGNAL(rejected()), SLOT(deleteLater()));
}


CDlgMapQMAPConfig::~CDlgMapQMAPConfig()
{

}


void CDlgMapQMAPConfig::accept()
{

    QDialog::accept();
}
