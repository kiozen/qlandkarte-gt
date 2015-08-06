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

#include "config.h"

#include "CDeviceGarmin.h"
#include "CMainWindow.h"
#include "CCanvas.h"
#include "CWptDB.h"
#include "CWpt.h"
#include "CTrackDB.h"
#include "CTrack.h"
#include "CResources.h"
#include "CRouteDB.h"
#include "CRoute.h"
#include "CMapSelectionGarmin.h"
#include "CGarminExport.h"
#include "Platform.h"

#undef IDEVICE_H
#include <garmin/IDevice.h>

#include <QtGui>
#include <QMessageBox>
#include <QSound>
#include <QProgressDialog>

#include <limits>
#include <math.h>

#include <proj_api.h>
#ifdef __MINGW32__
#undef LP
#endif

#define XSTR(x) STR(x)
#define STR(x) #x

struct garmin_icon_t
{
    int16_t id;
    const char * name;
};

garmin_icon_t GarminIcons[] =
{
    /*    mps    pcx    desc */
    {  16384, "Airport" },
    {   8204, "Amusement Park" },
    {    169, "Ball Park" },
    {      6, "Bank" },
    {     13, "Bar" },
    {   8244, "Beach" },
    {      1, "Bell" },
    {    150, "Boat Ramp" },
    {   8205, "Bowling" },
    {   8233, "Bridge" },
    {   8234, "Building" },
    {    151, "Campground" },
    {    170, "Car" },
    {   8206, "Car Rental" },
    {   8207, "Car Repair" },
    {   8235, "Cemetery" },
    {   8236, "Church" },
    {    179, "Circle with X" },
    {   8203, "City (Capitol)" },
    {   8200, "City (Large)" },
    {   8199, "City (Medium)" },
    {   8198, "City (Small)" },
    {   8198, "Small City" },
    {   8237, "Civil" },
    {   8262, "Contact, Afro" },
    {   8272, "Contact, Alien" },
    {   8258, "Contact, Ball Cap" },
    {   8259, "Contact, Big Ears" },
    {   8271, "Contact, Biker" },
    {   8273, "Contact, Bug" },
    {   8274, "Contact, Cat" },
    {   8275, "Contact, Dog" },
    {   8263, "Contact, Dreadlocks" },
    {   8264, "Contact, Female1" },
    {   8265, "Contact, Female2" },
    {   8266, "Contact, Female3" },
    {   8261, "Contact, Goatee" },
    {   8268, "Contact, Kung-Fu" },
    {   8276, "Contact, Pig" },
    {   8270, "Contact, Pirate" },
    {   8267, "Contact, Ranger" },
    {   8257, "Contact, Smiley" },
    {   8260, "Contact, Spike" },
    {   8269, "Contact, Sumo" },
    {    165, "Controlled Area" },
    {   8220, "Convenience Store" },
    {   8238, "Crossing" },
    {    164, "Dam" },
    {    166, "Danger Area" },
    {   8218, "Department Store" },
    {      4, "Diver Down Flag 1" },
    {      5, "Diver Down Flag 2" },
    {    154, "Drinking Water" },
    {    177, "Exit" },
    {   8208, "Fast Food" },
    {      7, "Fishing Area" },
    {   8209, "Fitness Center" },
    {    178, "Flag" },
    {   8245, "Forest" },
    {      8, "Gas Station" },
    {   8255, "Geocache" },
    {   8256, "Geocache Found" },
    {   8239, "Ghost Town" },
    {  16393, "Glider Area" },
    {   8197, "Golf Course" },
    {      2, "Green Diamond" },
    {     15, "Green Square" },
    {  16388, "Heliport" },
    {      9, "Horn" },
    {    171, "Hunting Area" },
    {    157, "Information" },
    {   8240, "Levee" },
    {     12, "Light" },
    {   8221, "Live Theater" },
    {    173, "Lodging" },
    {    173, "Hotel" },
    {     21, "Man Overboard" },
    {      0, "Marina" },
    {    156, "Medical Facility" },
    {   8195, "Mile Marker" },
    {   8241, "Military" },
    {    174, "Mine" },
    {   8210, "Movie Theater" },
    {   8211, "Museum" },
    {     22, "Navaid, Amber" },
    {     23, "Navaid, Black" },
    {     24, "Navaid, Blue" },
    {     25, "Navaid, Green" },
    {     26, "Navaid, Green/Red" },
    {     27, "Navaid, Green/White" },
    {     28, "Navaid, Orange" },
    {     29, "Navaid, Red" },
    {     30, "Navaid, Red/Green" },
    {     31, "Navaid, Red/White" },
    {     32, "Navaid, Violet" },
    {     33, "Navaid, White" },
    {     34, "Navaid, White/Green" },
    {     35, "Navaid, White/Red" },
    {   8242, "Oil Field" },
    {  16395, "Parachute Area" },
    {    159, "Park" },
    {    158, "Parking Area" },
    {   8212, "Pharmacy" },
    {    160, "Picnic Area" },
    {   8213, "Pizza" },
    {   8214, "Post Office" },
    {  16389, "Private Field" },
    {     37, "Radio Beacon" },
    {      3, "Red Diamond" },
    {     16, "Red Square" },
    {     10, "Residence" },
    {     10, "House" },
    {     11, "Restaurant" },
    {    167, "Restricted Area" },
    {    152, "Restroom" },
    {   8215, "RV Park" },
    {   8226, "Scales" },
    {    161, "Scenic Area" },
    {   8216, "School" },
    {  16402, "Seaplane Base" },
    {     19, "Shipwreck" },
    {    172, "Shopping Center" },
    {  16392, "Short Tower" },
    {    153, "Shower" },
    {    162, "Skiing Area" },
    {     14, "Skull and Crossbones" },
    {  16390, "Soft Field" },
    {   8217, "Stadium" },
    {   8246, "Summit" },
    {    163, "Swimming Area" },
    {  16391, "Tall Tower" },
    {    155, "Telephone" },
    {   8227, "Toll Booth" },
    {   8196, "TracBack Point" },
    {    175, "Trail Head" },
    {    176, "Truck Stop" },
    {   8243, "Tunnel" },
    {  16394, "Ultralight Area" },
    {   8282, "Water Hydrant" },
    {     18, "Waypoint" },
    {     17, "White Buoy" },
    {     36, "White Dot" },
    {   8219, "Zoo" },

    /* Custom icons.   The spec reserves 7680-8191 for the custom
     * icons on the C units, Quest, 27xx, 276, 296,  and other units.
     * Note that firmware problems on the earlier unit result in these
     * being mangled, so be sure you're on a version from at least
     * late 2005.
     * {    -2,  7680, "Custom 0" },
     * ....
     * {    -2,  8192, "Custom 511" },
     */
    {   7680, "Custom 0" },
    {   7681, "Custom 1" },
    {   7682, "Custom 2" },
    {   7683, "Custom 3" },
    {   7684, "Custom 4" },
    {   7685, "Custom 5" },
    {   7686, "Custom 6" },
    {   7687, "Custom 7" },
    {   7688, "Custom 8" },
    {   7689, "Custom 9" },
    {   7690, "Custom 10" },
    {   7691, "Custom 11" },
    {   7692, "Custom 12" },
    {   7693, "Custom 13" },
    {   7694, "Custom 14" },
    {   7695, "Custom 15" },
    {   7696, "Custom 16" },
    {   7697, "Custom 17" },
    {   7698, "Custom 18" },
    {   7699, "Custom 19" },
    {   7700, "Custom 20" },
    {   7701, "Custom 21" },
    {   7702, "Custom 22" },
    {   7703, "Custom 23" },

    {   8227, "Micro-Cache" },
    {    161, "Virtual cache" },
    {   8217, "Multi-Cache" },
    {    157, "Unknown Cache"    },
    {    178, "Locationless (Reverse) Cache" },
    {   8214, "Post Office" },
    {    160, "Event Cache" },
    {  8221, "Webcam Cache" },

    /* MapSource V6.x */

    {   8286, "Flag, Red" },
    {   8284, "Flag, Blue" },
    {   8285, "Flag, Green" },
    {   8289, "Pin, Red" },
    {   8287, "Pin, Blue" },
    {   8288, "Pin, Green" },
    {   8292, "Block, Red" },
    {   8290, "Block, Blue" },
    {   8291, "Block, Green" },
    {   8293, "Bike Trail" },
    {    181, "Fishing Hot Spot Facility" },
    {   8249, "Police Station"},
    {   8251, "Ski Resort" },
    {   8252, "Ice Skating" },
    {   8253, "Wrecker" },
    {    184, "Anchor Prohibited" },
    {    185, "Beacon" },
    {    186, "Coast Guard" },
    {    187, "Reef" },
    {    188, "Weed Bed" },
    {    189, "Dropoff" },
    {    190, "Dock" },

    /* New in Garmin protocol spec from June 2006.  Extracted from
     * spec and fed through some horrible awk to add ones we didn't
     * have before but normalized for consistency. */
    {   8359, "Asian Food" },
    {   8296, "Blue Circle" },
    {   8299, "Blue Diamond" },
    {   8317, "Blue Letter A" },
    {   8318, "Blue Letter B" },
    {   8319, "Blue Letter C" },
    {   8320, "Blue Letter D" },
    {   8341, "Blue Number 0" },
    {   8342, "Blue Number 1" },
    {   8343, "Blue Number 2" },
    {   8344, "Blue Number 3" },
    {   8345, "Blue Number 4" },
    {   8346, "Blue Number 5" },
    {   8347, "Blue Number 6" },
    {   8348, "Blue Number 7" },
    {   8349, "Blue Number 8" },
    {   8350, "Blue Number 9" },
    {   8302, "Blue Oval" },
    {   8305, "Blue Rectangle" },
    {   8308, "Blue Square" },
    {   8351, "Blue Triangle" },
    {   8254, "Border Crossing (Port Of Entry)" },
    {    182, "Bottom Conditions" },
    {   8360, "Deli" },
    {   8228, "Elevation point" },
    {   8229, "Exit without services" },
    {  16398, "First approach fix" },
    {   8250, "Gambling/casino" },
    {   8232, "Geographic place name, land" },
    {   8230, "Geographic place name, Man-made" },
    {   8231, "Geographic place name, water" },
    {   8295, "Green circle" },
    {   8313, "Green Letter A" },
    {   8315, "Green Letter B" },
    {   8314, "Green Letter C" },
    {   8316, "Green Letter D" },
    {   8331, "Green Number 0" },
    {   8332, "Green Number 1" },
    {   8333, "Green Number 2" },
    {   8334, "Green Number 3" },
    {   8335, "Green Number 4" },
    {   8336, "Green Number 5" },
    {   8337, "Green Number 6" },
    {   8338, "Green Number 7" },
    {   8339, "Green Number 8" },
    {   8340, "Green Number 9" },
    {   8301, "Green Oval" },
    {   8304, "Green Rectangle" },
    {   8352, "Green Triangle" },
    {  16385, "Intersection" },
    {   8201, "Intl freeway hwy" },
    {   8202, "Intl national hwy" },
    {   8361, "Italian food" },
    {   8248, "Large exit without services" },
    {   8247, "Large Ramp intersection" },
    {  16399, "Localizer Outer Marker" },
    {  16400, "Missed approach point" },
    {  16386, "Non-directional beacon" },
    {    168, "Null" },
    {    180, "Open 24 Hours" },
    {   8222, "Ramp intersection" },
    {   8294, "Red circle" },
    {   8309, "Red Letter A" },
    {   8310, "Red Letter B" },
    {   8311, "Red Letter C" },
    {   8312, "Red Letter D" },
    {   8321, "Red Number 0" },
    {   8322, "Red Number 1" },
    {   8323, "Red Number 2" },
    {   8324, "Red Number 3" },
    {   8325, "Red Number 4" },
    {   8326, "Red Number 5" },
    {   8327, "Red Number 6" },
    {   8328, "Red Number 7" },
    {   8329, "Red Number 8" },
    {   8330, "Red Number 9" },
    {   8300, "Red Oval" },
    {   8303, "Red Rectangle" },
    {   8353, "Red Triangle" },
    {   8362, "Seafood" },
    {   8194, "State Hwy" },
    {   8363, "Steak" },
    {   8223, "Street Intersection" },
    {  16401, "TACAN" },
    {    183, "Tide/Current PRediction Station" },
    {    191, "U Marina" },
    {   8193, "US hwy" },
    {    193, "U stump" },
    {  16387, "VHF Omni-range" },
    {  16397, "VOR-DME" },
    {  16396, "VOR/TACAN" },
    {     -1, 0 },
};
#pragma pack(1)

// Header of Garmin 256-color BMP bitmap files
struct garmin_bmp_t
{
    quint16 bfType;
    quint32 bfSize;
    quint32 bfReserved;
    quint32 bfOffBits;

    quint32 biSize;
    qint32 biWidth;
    qint32 biHeight;
    quint16 biPlanes;
    quint16 biBitCount;
    quint32 biCompression;
    quint32 biSizeImage;
    quint32 biXPelsPerMeter;
    quint32 biYPelsPerMeter;
    quint32 biClrUsed;
    quint32 biClrImportant;

    quint32 clrtbl[0x100];

    quint8  data[];
};
#ifdef WIN32
#pragma pack()
#else
#pragma pack(0)
#endif

/**
  @param progress the progress as integer from 0..100, if -1 no progress bar needed.
  @param ok if this pointer is 0 no ok button needed, if non zero set to 1 if ok button pressed
  @param cancel if this pointer is 0 no cancel button needed, if non zero set to 1 if cancel button pressed
  @param title dialog title as C string
  @param msg dialog message C string to display
  @param self void pointer as provided while registering the callback
*/
void GUICallback(int progress, int * ok, int * cancel, const char * title, const char * msg, void * self)
{
    CDeviceGarmin * parent = static_cast<CDeviceGarmin *>(self);
    CDeviceGarmin::dlgdata_t& dd = parent->dlgData;

    if(progress != -1)
    {
        quint32 togo, hour, min, sec;
        QString message;

        if(dd.dlgProgress == 0)
        {
            dd.canceled     = false;
            dd.dlgProgress  = new QProgressDialog(QString(title),0,0,100,theMainWindow, Qt::WindowStaysOnTopHint);
            dd.timeProgress.start();
            if(cancel)
            {
                QPushButton * butCancel = new QPushButton(QObject::tr("Cancel"),dd.dlgProgress);
                parent->connect(butCancel, SIGNAL(clicked()), parent, SLOT(slotCancel()));
                dd.dlgProgress->setCancelButton(butCancel);
            }
        }

        if(title) dd.dlgProgress->setWindowTitle(QString(title));

        togo = (quint32)((100.0 * (double)dd.timeProgress.elapsed() / (double)progress) + 0.5);
        togo = (quint32)((double)(togo - dd.timeProgress.elapsed()) / 1000.0 + 0.5);

        hour = (togo / 3600);
        min  = (togo - hour * 3600) / 60;
        sec  = (togo - hour * 3600 - min * 60);

        message.sprintf(QObject::tr("\n\nEstimated finish: %02i:%02i:%02i [hh:mm:ss]").toUtf8(),hour,min,sec);

        dd.dlgProgress->setLabelText(QString(msg) + message);
        dd.dlgProgress->setValue(progress);

        if(progress == 100 && dd.dlgProgress)
        {
            delete dd.dlgProgress;
            dd.dlgProgress = 0;
        }

        if(cancel)
        {
            *cancel = dd.canceled;
        }

        qApp->processEvents();

    }
    else
    {
        if(ok && cancel)
        {
            QMessageBox::StandardButtons res = QMessageBox::question(theMainWindow,QString(title),QString(msg),QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Cancel);
            *ok     = res == QMessageBox::Ok;
            *cancel = res == QMessageBox::Cancel;
        }
        else if(ok && !cancel)
        {
            QMessageBox::question(theMainWindow,QString(title),QString(msg),QMessageBox::Ok,QMessageBox::Ok);
            *ok     = true;
        }
        else if(!ok && cancel)
        {
            QMessageBox::question(theMainWindow,QString(title),QString(msg),QMessageBox::Cancel,QMessageBox::Cancel);
            *cancel     = true;
        }
        else if(!ok && !cancel)
        {
            //kiozen - that doesn't work nicely
            //             QMessageBox * dlg = new QMessageBox(&parent->main);
            //             dlg->setWindowTitle(QString(title));
            //             dlg->setText(QString(msg));
            //             dlg->setStandardButtons(QMessageBox::NoButton);
            //             dlg->setIcon(QMessageBox::Information);
            //             dlg->show();
            //             qApp->processEvents(QEventLoop::AllEvents, 1000);
            //             sleep(3); // sleep for 3 seconds
            //             delete dlg;
        }
    }
}


CDeviceGarmin::CDeviceGarmin(const QString& devkey, const QString& port, QObject * parent)
: IDevice(devkey, parent)
, port(port)
{
    qDebug() << "CDeviceGarmin::CDeviceGarmin()";

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(slotTimeout()));

}


CDeviceGarmin::~CDeviceGarmin()
{
    qDebug() << "~CDeviceGarmin::CDeviceGarmin()";
}


Garmin::IDevice * CDeviceGarmin::getDevice()
{
    Garmin::IDevice * (*func)(const char*) = 0;
    Garmin::IDevice * dev = 0;

#if defined(Q_OS_MAC)
    // MacOS X: plug-ins are stored in the bundle folder
    QString libname     = QString("%1/lib%2" XSTR(SHARED_LIB_EXT))
        .arg(QCoreApplication::applicationDirPath().replace(QRegExp("MacOS$"), "Resources/Drivers"))
        .arg(devkey);
#else
    QString libname     = QString("%1/lib%2" XSTR(SHARED_LIB_EXT)).arg(XSTR(PLUGINDIR)).arg(devkey);
#endif
    QString funcname    = QString("init%1").arg(devkey);

    QLibrary lib;
    lib.setFileName(libname);
    bool lib_loaded = lib.load();
    if (lib_loaded)
    {
        func = (Garmin::IDevice * (*)(const char*))lib.resolve(funcname.toLatin1());
    }

    if(!lib_loaded || func == 0)
    {
        QMessageBox warn;
        warn.setIcon(QMessageBox::Warning);
        warn.setWindowTitle(tr("Error ..."));
        warn.setText(tr("Failed to load driver."));
        warn.setDetailedText(lib.errorString());
        warn.setStandardButtons(QMessageBox::Ok);
        warn.exec();
        return 0;
    }

    dev = func(INTERFACE_VERSION);
    if(dev == 0)
    {
        QMessageBox warn;
        warn.setIcon(QMessageBox::Warning);
        warn.setWindowTitle(tr("Error ..."));
        warn.setText(tr("Driver version mismatch."));
        QString detail = QString(
            tr("The version of your driver plugin \"%1\" does not match the version QLandkarteGT expects (\"%2\").")
            ).arg(libname).arg(INTERFACE_VERSION);
        warn.setDetailedText(detail);
        warn.setStandardButtons(QMessageBox::Ok);
        warn.exec();
        func = 0;
    }

    if(dev)
    {
        dev->setPort(port.toLatin1());
        dev->setGuiCallback(GUICallback,this);
    }

    return dev;
}


void CDeviceGarmin::slotCancel()
{
    dlgData.canceled = true;
}


void CDeviceGarmin::slotTimeout()
{
    Garmin::IDevice * dev = getDevice();
    if(dev == 0) return;

    Garmin::Pvt_t pvt;
    try
    {
        dev->getRealTimePos(pvt);
    }
    catch(int)
    {
        timer->stop();
        log.fix = CLiveLog::eOff;
        emit sigLiveLog(log);

        QMessageBox::warning(0,tr("Device Link Error"),dev->getLastError().c_str(),QMessageBox::Ok,QMessageBox::NoButton);
        return;
    }

    log.fix = pvt.fix == 3 || pvt.fix == 5 ? CLiveLog::e3DFix : pvt.fix == 2 || pvt.fix == 4 ? CLiveLog::e2DFix : CLiveLog::eNoFix;
    if(log.fix != CLiveLog::eNoFix)
    {
        log.lon = pvt.lon;
        log.lat = pvt.lat;
        log.ele = pvt.alt + pvt.msl_hght;

        QDateTime t(QDate(1989,12,31), QTime(0,0), Qt::UTC);
        t = t.addDays(pvt.wn_days).addSecs((int)(pvt.tow + 0.5)).addSecs(-pvt.leap_scnds);
        log.timestamp = t.toLocalTime().toTime_t();

        // multiply by 100 to avoid leaving the float range.
        float heading = fabsf((100.0 * pvt.east) / (100.0 * pvt.north));
        heading = atanf(heading) / (2*M_PI) * 360.0;
        if( (pvt.north > 0.0) & (pvt.east > 0.0) )
        {
            // 1st quadrant
            heading = heading;
        }
        else if( (pvt.north > 0.0) & (pvt.east < 0.0) )
        {
            // 2nd quadrant
            heading = 360.0 - heading;
        }
        else if( (pvt.north < 0.0) & (pvt.east < 0.0) )
        {
            // 3rd quadrant
            heading = 180.0 + heading;
        }
        else if( (pvt.north < 0.0) & (pvt.east > 0.0) )
        {
            // 4th quadrant
            heading = 180.0 - heading;
        }
        else
        {
            heading = std::numeric_limits<float>::quiet_NaN();
        }

        log.heading     = heading;
        log.velocity    = sqrtf( pvt.north * pvt.north + pvt.east * pvt.east );
                                 //HS: moved division by 2 from CLiveLogDB.cpp
        log.error_horz  = pvt.eph/2.0;
                                 //HS: moved division by 2 from CLiveLogDB.cpp
        log.error_vert  = pvt.epv/2.0;
    }

    emit sigLiveLog(log);
}


void CDeviceGarmin::uploadWpts(const QList<CWpt*>& wpts)
{
    qDebug() << "CDeviceGarmin::uploadWpts()";
    Garmin::IDevice * dev = getDevice();
    if(dev == 0) return;

    std::list<Garmin::Wpt_t> garwpts;
    QTextCodec *codec = QTextCodec::codecForName(CResources::self().charset().toLatin1());

    QList<CWpt*>::const_iterator wpt = wpts.begin();
    while(wpt != wpts.end())
    {
        Garmin::Wpt_t garwpt;

        garmin_icon_t * icon = GarminIcons;
        while(icon->name != 0)
        {
            if((*wpt)->getIconString() == icon->name)
            {
                garwpt.smbl = icon->id;
                break;
            }
            ++icon;
        }

        garwpt.lat      = (*wpt)->lat;
        garwpt.lon      = (*wpt)->lon;
        garwpt.alt      = (*wpt)->ele;
        garwpt.dist     = (*wpt)->prx;
        garwpt.ident    = codec->fromUnicode((*wpt)->getName()).data();

        QString comment = (*wpt)->getComment();
        if((*wpt)->getComment() == "" )
        {
            comment = (*wpt)->getDescription();
        }
        IItem::removeHtml(comment);

        garwpt.comment  = codec->fromUnicode(comment).data();

        garwpts.push_back(garwpt);

        ++wpt;
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    try
    {
        dev->uploadWaypoints(garwpts);
        if (CResources::self().playSound())
        {
            QSound::play(":/sounds/xfer-done.wav");
        }

        QApplication::restoreOverrideCursor();
    }
    catch(int)
    {
        QMessageBox::warning(0,tr("Device Link Error"),dev->getLastError().c_str(),QMessageBox::Ok,QMessageBox::NoButton);
        QApplication::restoreOverrideCursor();
        return;
    }

    theMainWindow->getCanvas()->setFadingMessage(tr("Upload waypoints finished!"));

}


void CDeviceGarmin::downloadWpts(QList<CWpt*>& wpts)
{
    qDebug() << "CDeviceGarmin::downloadWpts()";
    Garmin::IDevice * dev = getDevice();
    if(dev == 0) return;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    std::list<Garmin::Wpt_t> garwpts;
    try
    {
        dev->downloadWaypoints(garwpts);
        if (CResources::self().playSound())
        {
            QSound::play(":/sounds/xfer-done.wav");
        }

        QApplication::restoreOverrideCursor();
    }
    catch(int)
    {
        QMessageBox::warning(0,tr("Device Link Error"),dev->getLastError().c_str(),QMessageBox::Ok,QMessageBox::NoButton);
        QApplication::restoreOverrideCursor();
        return;
    }

    QTextCodec *codec = QTextCodec::codecForName(CResources::self().charset().toLatin1());

    std::list<Garmin::Wpt_t>::const_iterator garwpt = garwpts.begin();
    while(garwpt != garwpts.end())
    {
        CWpt * wpt = new CWpt(&CWptDB::self());

        wpt->setName(codec->toUnicode(garwpt->ident.c_str()));
        wpt->setComment(codec->toUnicode(garwpt->comment.c_str()));
        wpt->lon        = garwpt->lon;
        wpt->lat        = garwpt->lat;
        wpt->ele        = garwpt->alt;
        wpt->prx        = garwpt->dist;

        garmin_icon_t * icon = GarminIcons;
        while(icon->name != 0)
        {
            if(garwpt->smbl == icon->id)
            {
                wpt->setIcon(icon->name);
                break;
            }
            ++icon;
        }

        wpts << wpt;
        ++garwpt;
    }

    theMainWindow->getCanvas()->setFadingMessage(tr("Download waypoints finished!"));
}


void CDeviceGarmin::uploadTracks(const QList<CTrack*>& trks)
{
    qDebug() << "CDeviceGarmin::uploadTracks()";

    Garmin::IDevice * dev = getDevice();
    if(dev == 0) return;

    std::list<Garmin::Track_t> gartrks;
    QTextCodec *codec = QTextCodec::codecForName(CResources::self().charset().toLatin1());

    QList<CTrack*>::const_iterator trk = trks.begin();
    while(trk != trks.end())
    {
        Garmin::Track_t gartrk;

        gartrk.ident = codec->fromUnicode((*trk)->getName()).data();
        gartrk.color = (*trk)->getColorIdx();

        const QList<CTrack::pt_t>& trkpts           = (*trk)->getTrackPoints();
        QList<CTrack::pt_t>::const_iterator trkpt   =  trkpts.begin();
        while(trkpt != trkpts.end())
        {
            Garmin::TrkPt_t gartrkpt;

            QDateTime t = QDateTime::fromTime_t(trkpt->timestamp);
            t = t.addYears(-20).addDays(1);

            gartrkpt.time   = t.toTime_t();
            gartrkpt.lon    = trkpt->lon;
            gartrkpt.lat    = trkpt->lat;
            gartrkpt.alt    = trkpt->ele;

            gartrk.track.push_back(gartrkpt);
            ++trkpt;
        }

        gartrks.push_back(gartrk);
        ++trk;
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    try
    {
        dev->uploadTracks(gartrks);
        if (CResources::self().playSound())
        {
            QSound::play(":/sounds/xfer-done.wav");
        }

        QApplication::restoreOverrideCursor();
    }
    catch(int)
    {
        QMessageBox::warning(0,tr("Device Link Error"),dev->getLastError().c_str(),QMessageBox::Ok,QMessageBox::NoButton);
        QApplication::restoreOverrideCursor();
        return;
    }

    theMainWindow->getCanvas()->setFadingMessage(tr("Upload tracks finished!"));
}


void CDeviceGarmin::downloadTracks(QList<CTrack*>& trks)
{
    qDebug() << "CDeviceGarmin::downloadTracks()";
    Garmin::IDevice * dev = getDevice();
    if(dev == 0) return;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    std::list<Garmin::Track_t> gartrks;
    try
    {
        dev->downloadTracks(gartrks);
        if (CResources::self().playSound())
        {
            QSound::play(":/sounds/xfer-done.wav");
        }

        QApplication::restoreOverrideCursor();
    }
    catch(int)
    {
        QMessageBox::warning(0,tr("Device Link Error"),dev->getLastError().c_str(),QMessageBox::Ok,QMessageBox::NoButton);
        QApplication::restoreOverrideCursor();
        return;
    }

    QTextCodec *codec = QTextCodec::codecForName(CResources::self().charset().toLatin1());
    std::list<Garmin::Track_t>::const_iterator gartrk = gartrks.begin();
    while(gartrk != gartrks.end())
    {

        CTrack * trk = new CTrack(&CTrackDB::self());

        trk->setName(codec->toUnicode(gartrk->ident.c_str()));
        trk->setColor(gartrk->color);

        std::vector<Garmin::TrkPt_t>::const_iterator gartrkpt = gartrk->track.begin();
        while(gartrkpt != gartrk->track.end())
        {
            QDateTime t = QDateTime::fromTime_t(gartrkpt->time);
            t = t.addYears(20).addDays(-1);

            CTrack::pt_t trkpt;
            trkpt.lon       = gartrkpt->lon;
            trkpt.lat       = gartrkpt->lat;
            trkpt.timestamp = t.toTime_t();
            trkpt.ele       = gartrkpt->alt;

            *trk << trkpt;
            ++gartrkpt;
        }

        if(trk->getTrackPoints().count() > 0)
        {
            trks << trk;
        }
        else
        {
            delete trk;
        }
        ++gartrk;
    }

    theMainWindow->getCanvas()->setFadingMessage(tr("Download tracks finished!"));

}


void CDeviceGarmin::downloadScreenshot(QImage& image)
{
    qDebug() << "CDeviceGarmin::downloadScreenshot()";
    Garmin::IDevice * dev = getDevice();
    if(dev == 0) return;

    char *  clrtbl  = 0;
    char *  data    = 0;
    int     width   = 0;
    int     height  = 0;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    try
    {
        dev->screenshot(clrtbl, data, width, height);
        if (CResources::self().playSound())
        {
            QSound::play(":/sounds/xfer-done.wav");
        }

        QApplication::restoreOverrideCursor();
    }
    catch(int)
    {
        QMessageBox::warning(0,tr("Device Link Error"),dev->getLastError().c_str(),QMessageBox::Ok,QMessageBox::NoButton);
        QApplication::restoreOverrideCursor();
        return;
    }

    if(data != 0 && clrtbl != 0)
    {
        /* Hack hack hack - add two dummy bytes at the beginning of the array which are
         * removed below, and let the bmp record start at offs 2.  This shifts all 4-byte
         * data in the record to adresses which are a multiple of 4. */
        QByteArray buffer(sizeof(garmin_bmp_t) + width * height + 2, 0);
        garmin_bmp_t * pBmp = (garmin_bmp_t*)(buffer.data() + 2);
        pBmp->bfType        = gar_endian(uint16_t, 0x4d42);
        pBmp->bfSize        = gar_endian(uint32_t, buffer.size());
        pBmp->bfReserved    = 0;
        pBmp->bfOffBits     = gar_endian(uint32_t, sizeof(garmin_bmp_t));
        pBmp->biSize        = gar_endian(uint32_t, 0x28);
        pBmp->biWidth       = gar_endian(int32_t, width);
        pBmp->biHeight      = gar_endian(int32_t, height);
        pBmp->biPlanes      = gar_endian(uint16_t, 1);
        pBmp->biBitCount    = gar_endian(uint16_t, 8);
        pBmp->biCompression = 0;
        pBmp->biSizeImage   = gar_endian(uint32_t, width * height);
        pBmp->biYPelsPerMeter = 0;
        pBmp->biXPelsPerMeter = 0;
        pBmp->biClrUsed       = gar_endian(uint32_t, 0x100);
        pBmp->biClrImportant  = gar_endian(uint32_t, 0x100);
        memcpy(pBmp->clrtbl,clrtbl,sizeof(pBmp->clrtbl));
        memcpy(pBmp->data,data,width * height);

        image.loadFromData((const uchar *)pBmp, sizeof(garmin_bmp_t) + width * height);

    }

}


void CDeviceGarmin::uploadRoutes(const QList<CRoute*>& rtes)
{
    Garmin::IDevice * dev = getDevice();
    if(dev == 0) return;

    std::list<Garmin::Route_t> garrtes;
    int id = 0;

    QTextCodec *codec = QTextCodec::codecForName(CResources::self().charset().toLatin1());

    QList<CRoute*>::const_iterator rte = rtes.begin();
    while(rte != rtes.end())
    {
        Garmin::Route_t garrte;

        uint16_t smbl = 8198;
        garmin_icon_t * icon = GarminIcons;
        while(icon->name != 0)
        {
            if((*rte)->getIconString() == icon->name)
            {
                smbl = icon->id;
                break;
            }
            ++icon;
        }

        QString name = (*rte)->getName();
        garrte.ident = codec->fromUnicode(name).data();

        unsigned cnt = 0;
        const QVector<CRoute::pt_t>& rtepts         = (*rte)->getSecRtePoints().isEmpty() ? (*rte)->getPriRtePoints() : (*rte)->getSecRtePoints();
        QVector<CRoute::pt_t>::const_iterator rtept =  rtepts.begin();
        while(rtept != rtepts.end())
        {
            Garmin::RtePt_t garrtept;

            garrtept.lon            = rtept->lon;
            garrtept.lat            = rtept->lat;
            garrtept.Wpt_t::ident   = QString("%1.%2").arg(id).arg(++cnt,3,10,QChar('0')).toLatin1().data();
            garrtept.Wpt_t::comment = codec->fromUnicode(name).data();
            garrtept.Wpt_t::smbl    = smbl;

            garrte.route.push_back(garrtept);
            ++rtept;
        }

        garrtes.push_back(garrte);
        ++rte; ++id;
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    try
    {
        dev->uploadRoutes(garrtes);
        if (CResources::self().playSound())
        {
            QSound::play(":/sounds/xfer-done.wav");
        }

        QApplication::restoreOverrideCursor();
    }
    catch(int)
    {
        QMessageBox::warning(0,tr("Device Link Error"),dev->getLastError().c_str(),QMessageBox::Ok,QMessageBox::NoButton);
        QApplication::restoreOverrideCursor();
        return;
    }

    theMainWindow->getCanvas()->setFadingMessage(tr("Upload routes finished!"));
}


void CDeviceGarmin::downloadRoutes(QList<CRoute*>& rtes)
{
    qDebug() << "CDeviceGarmin::downloadRoutes()";
    Garmin::IDevice * dev = getDevice();
    if(dev == 0) return;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    std::list<Garmin::Route_t> garrtes;
    try
    {
        dev->downloadRoutes(garrtes);
        if (CResources::self().playSound())
        {
            QSound::play(":/sounds/xfer-done.wav");
        }

        QApplication::restoreOverrideCursor();
    }
    catch(int)
    {
        QMessageBox::warning(0,tr("Device Link Error"),dev->getLastError().c_str(),QMessageBox::Ok,QMessageBox::NoButton);
        QApplication::restoreOverrideCursor();
        return;
    }

    QTextCodec *codec = QTextCodec::codecForName(CResources::self().charset().toLatin1());

    std::list<Garmin::Route_t>::const_iterator garrte = garrtes.begin();
    while(garrte != garrtes.end())
    {

        CRoute * rte = new CRoute(&CRouteDB::self());

        rte->setName(codec->toUnicode(garrte->ident.c_str()));

        std::vector<Garmin::RtePt_t>::const_iterator garrtept = garrte->route.begin();
        while(garrtept != garrte->route.end())
        {
            rte->addPosition(garrtept->lon, garrtept->lat, garrtept->ident.c_str());
            ++garrtept;
        }

        if(rte->getPriRtePoints().count() > 0)
        {
            rtes << rte;
        }
        else
        {
            delete rte;
        }
        ++garrte;
    }

    theMainWindow->getCanvas()->setFadingMessage(tr("Download routes finished!"));
}


void CDeviceGarmin::uploadMap(const QList<IMapSelection*>& mss)
{
    Garmin::IDevice * dev = getDevice();
    if(dev == 0) return;

    QList<IMapSelection*>::const_iterator ms = mss.begin();

    while(ms != mss.end())
    {
        if((*ms)->type == IMapSelection::eVector)
        {
            break;
        }
        ++ms;
    }
    if(ms == mss.end()) return;

    CMapSelectionGarmin * gms = (CMapSelectionGarmin*)(*ms);
    QTemporaryFile tmpfile;
    tmpfile.open();

    CGarminExport dlg(0);
    dlg.exportToFile(*gms, tmpfile.fileName());
    if(dlg.hadErrors())
    {
        QMessageBox::warning(0,tr("Error..."), tr("Failed to create image file."),QMessageBox::Abort,QMessageBox::Abort);
        return;
    }

    QStringList keys;
    QMap<QString, CMapSelectionGarmin::map_t>::const_iterator map = gms->maps.begin();
    while(map != gms->maps.end())
    {
        if(!map->unlockKey.isEmpty())
        {
            keys << map->unlockKey;
        }
        ++map;
    }

    QFileInfo fi(tmpfile.fileName());

    qDebug() << fi.size();
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    try
    {
        dev->uploadMap(tmpfile.fileName().toLocal8Bit(), (quint32)fi.size() , keys.isEmpty() ? 0 : keys[0].toLatin1().data());
        if (CResources::self().playSound())
        {
            QSound::play(":/sounds/xfer-done.wav");
        }
        QApplication::restoreOverrideCursor();
    }
    catch(int)
    {
        QMessageBox::warning(0,tr("Device Link Error"),dev->getLastError().c_str(),QMessageBox::Ok,QMessageBox::NoButton);
        QApplication::restoreOverrideCursor();
        return;
    }

    theMainWindow->getCanvas()->setFadingMessage(tr("Upload maps finished!"));
}


void CDeviceGarmin::setLiveLog(bool on)
{
    Garmin::IDevice * dev = getDevice();
    if(dev == 0) return;

    log.error_unit = "m";

    try
    {
        dev->setRealTimeMode(on);
    }
    catch(int)
    {
        timer->stop();
        log.fix = CLiveLog::eOff;
        emit sigLiveLog(log);

        QMessageBox::warning(0,tr("Device Link Error"),dev->getLastError().c_str(),QMessageBox::Ok,QMessageBox::NoButton);
        return;
    }

    if(on && !timer->isActive())
    {
        timer->start(1000);
    }

    else
    {
        timer->stop();
        log.fix = CLiveLog::eOff;
        emit sigLiveLog(log);
    }

}


bool CDeviceGarmin::liveLog()
{
    return timer->isActive();
}
