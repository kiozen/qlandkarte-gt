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
#ifndef CRESOURCES_H
#define CRESOURCES_H

#include <QObject>
#include <QFont>
#include <QPointer>
#include <QDir>
#include <QColor>

class IDevice;
class IUnit;

/// all global resources
class CResources : public QObject
{
    Q_OBJECT;
    public:
        virtual ~CResources();

        static CResources& self(){return *m_self;}

        /// get HTTP proxy settings
        /**
            @param url a string to store the proxy's URL
            @param port a 16bit unsigned integer to store the proxy's port

            @return The method will return true if the proxy is enabled.
        */
        bool getHttpProxy(QString& url, quint16& port);
        void getHttpProxyAuth(QString& user, QString& pwd);

        /// get pointer to current device handler
        IDevice * device();
        QString charset();

        /// the font used for text on the map
        const QFont& getMapFont(){return m_mapfont;}

        /// root path of all maps
        QString pathMaps;

        bool useGeoDB(){return m_useGeoDB;}
        bool saveGeoDBOnExit(){return m_saveGeoDBOnExit;}
        quint32 saveGeoDBMinutes(){return m_saveGeoDBMinutes;}
        QDir pathGeoDB(){return m_pathGeoDB;}
        bool flipMouseWheel(){return m_flipMouseWheel;}
        bool showTrackProfilePreview(){return m_showTrackProfile;}
        bool showTrackProfileEleInfo(){return m_showTrackEleInfo;}
        bool showNorthIndicator(){return m_showNorth;}
        bool showClock(){return m_showClock;}
        bool showScale(){return m_showScale;}
        bool showToolTip(){return m_showToolTip;}
        bool showElementInfo(){return m_showElementInfo;}
        bool showZoomLevel(){return m_showZoomLevel;}
        bool playSound(){return m_playSound;}
        bool useAntiAliasing(){return m_useAntiAliasing;}
        bool reducePoiIcons(){return m_reducePoiIcons;}

        double getZoomLevelThresholdPois(){return m_zoomLevelThresholdPois;}
        double getZoomLevelThresholdPoiLabels(){return m_zoomLevelThresholdPoiLabels;}

        QColor wptTextColor(){return m_WptTextColor;}

        QDir getPathMapCache(){return m_pathMapCache;}
        int getSizeMapCache(){return m_sizeMapCache;}
        int getExpireMapCache(){return m_expireMapCache;}

        enum TimezoneMode_e
        {
            eTZUtc
            ,eTZLocal
            ,eTZAuto
            ,eTZSelected
        };

        TimezoneMode_e getTimezoneMode(){return m_tzMode;}
        QString getSelectedTimezone(){return m_timezone;}

        QString getBRouterHost(){return m_brouterHost;}
        int getBRouterPort(){return m_brouterPort.toInt();}
        QStringList getBRouterProfiles();
        QStringList readBRouterProfiles(QString path);

        signals:
        void sigDeviceChanged();
        void sigBRouterChanged();

    private:
        friend class CMainWindow;
        friend class CDlgConfig;
        CResources(QObject * parent);

        static CResources * m_self;

        enum browser_e
        {
            eFirefox = 0
            ,eKonqueror = 1
            ,eOther = 2
        };

        /// use proxy for http requests
        bool m_useHttpProxy;
        /// the  proxy name or address
        QString m_httpProxy;
        /// the proxy port
        quint16 m_httpProxyPort;

        /// proxy logon data
        /**
        Currently caching of proxy authentication data
        is not supported. Hence each Network Access Manager
        requests proxy authentication data. Such data is
        only cached at Network Access Manager level.
        */
        //QString m_httpProxyUser;
        //QString m_httpProxyPwd;

        /// font used by the map
        QFont m_mapfont;

        /// this offset is needed to correct time in seconds until Dec. 30th, 1989 12:00 to POSIX standard
        quint32 time_offset;

        /// the device key for the desired device
        QString m_devKey;
        /// the actual device access object
        /**
            This can be different from m_devKey. In this case the next call
            to device() will destroy it and load the correct one.
        */
        IDevice * m_device;

        QString m_devIPAddress;
        quint16 m_devIPPort;
        QString m_devSerialPort;
        QString m_devBaudRate;

        bool m_watchdogEnabled;

        QString m_devType;
        QString m_devCharset;

        /// mouse wheel zoom direction
        bool m_flipMouseWheel;

        /// play sound after finishing transfers
        bool m_playSound;

        bool m_useGeoDB;
        bool m_saveGeoDBOnExit;
        quint32 m_saveGeoDBMinutes;
        QDir m_pathGeoDB;

        bool m_showTrackProfile;
        bool m_showTrackEleInfo;
        bool m_showNorth;
        bool m_showClock;
        bool m_showScale;
        bool m_showToolTip;
        bool m_showElementInfo;
        bool m_showZoomLevel;
        bool m_useAntiAliasing;
        bool m_reducePoiIcons;

        double m_zoomLevelThresholdPois;
        double m_zoomLevelThresholdPoiLabels;

        QColor m_WptTextColor;

        /// unit translator object
        QPointer<IUnit> unit;

        QDir m_pathMapCache;
        int  m_sizeMapCache;
        int  m_expireMapCache;

        TimezoneMode_e m_tzMode;
        QString m_timezone;

        QString m_brouterHost;
        QString m_brouterPort;
        QString m_brouterProfiles;
        QString m_brouterProfilePath;
        bool m_brouterLocal;
};
#endif                           //CRESOURCES_H
