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
#ifndef CDLGEDITWPT_H
#define CDLGEDITWPT_H

#include <QDialog>

#include "ui_IDlgEditWpt.h"
#include "CImageSelect.h"

#ifdef HAS_DMTX
#include <dmtx.h>
#endif                           //HAS_DMTX

class CWpt;
class QNetworkAccessManager;

/// dialog to edit waypoint properties
class CDlgEditWpt : public QDialog, private Ui::IDlgEditWpt
{
    Q_OBJECT;
    public:
        CDlgEditWpt(CWpt &wpt, QWidget * parent);
        virtual ~CDlgEditWpt();

    public slots:
        int exec();
        void accept();

    private slots:
        void slotAddImage();
        void slotDelImage();
        void slotNextImage();
        void slotPrevImage();
        void slotSelectIcon();
        void slotOpenLink(const QString& link);
        void slotOpenLink(const QUrl& url);
        void slotEditLink();
        void slotSaveBarcode();
        void slotUpdateBarcode();
        void slotToggleHint(bool show);
        void slotSelectImage(const CImageSelect::img_t& img);
        void slotCreateBuddies();
        void slotTransparent(bool ok);
        void slotEditInfo();

        void slotCollectSpoiler();
        void slotRequestFinished(QNetworkReply * reply);
        void slotProxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*);

    protected:
        bool eventFilter(QObject *obj, QEvent *event);

    private:
        void showImage(int idx);
        void triggerSpoilerDownload();

        CWpt &wpt;
        qint32 idxImg;
        QString link;
        double oldLon;
        double oldLat;
        QSet<QString> wptBuddies;
#ifdef HAS_DMTX
        DmtxEncode * enc;
#endif
        QString name;

        QNetworkAccessManager * networkAccessManager;

        QMap<QNetworkReply*, QString> pendingRequests;

        bool silentSpoilerQuery;
};
#endif                           //CDLGEDITWPT_H
