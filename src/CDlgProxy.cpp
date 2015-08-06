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

#include "CDlgProxy.h"
#include "CResources.h"
#include "CMainWindow.h"

#include <QtGui>

CDlgProxy::CDlgProxy(QString &user, QString &pwd, QWidget *parent)
: QDialog(parent)
, user(user)
, pwd(pwd)
{
    setupUi(this);

    QString url;
    quint16 port;
    CResources::self().getHttpProxy(url, port);

    iconLabel->setText(QString());
    iconLabel->setPixmap(theMainWindow->style()->standardIcon(QStyle::SP_MessageBoxQuestion, 0, theMainWindow).pixmap(32, 32));

    QString introMessage = tr("<qt>Connect to proxy \"%1\" using:</qt>");
#ifdef QK_QT5_PORT
    introMessage = introMessage.arg(url.toHtmlEscaped());
#else
    introMessage = introMessage.arg(Qt::escape(url));
#endif
    introLabel->setText(introMessage);
    introLabel->setWordWrap(true);

}


CDlgProxy::~CDlgProxy()
{

}


void CDlgProxy::accept()
{
    user    = userNameLineEdit->text();
    pwd     = passwordLineEdit->text();
    QDialog::accept();
}
