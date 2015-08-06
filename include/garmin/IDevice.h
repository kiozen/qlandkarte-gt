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

  Garmin and MapSource are registered trademarks or trademarks of Garmin Ltd.
  or one of its subsidiaries.

**********************************************************************************************/

#ifndef IDEVICE_H
#define IDEVICE_H

// need integer type definitions with fixed width
#ifdef HAVE_INTTYPES_H
#  include <inttypes.h>
#elif HAVE_STDINT_H
#  include <stdint.h>
#elif WIN32

typedef __int8  int8_t;
typedef __int16 int16_t;
typedef __int32 int32_t;
typedef __int64 int64_t;
typedef unsigned __int8     uint8_t;
typedef unsigned __int16    uint16_t;
typedef unsigned __int32    uint32_t;
typedef unsigned __int64    uint64_t;

#else
#  error neither inttypes.h nor stdint.h are available
#endif

#include <list>
#include <string>
#include <vector>

#include <stdlib.h>
#include <string.h>

#ifndef _MKSTR_1
#define _MKSTR_1(x)    #x
#define _MKSTR(x)      _MKSTR_1(x)
#endif

#define INTERFACE_VERSION "01.18"

namespace Garmin
{

    /// common waypoint structure application side
    /**
        This structure has to be used to exchange a waypoint to QLandkarte.
        If an item is missing you can simply add it. This structure must
        never be copied assuming a certain alignment.

        Most values are the same like the ones used in the waypoint data
        structures used by Garmin.
    */
    struct Wpt_t
    {
        Wpt_t()
            : wpt_class(0)
            , dspl_color(0)
            , dspl_attr(0)
            , smbl(8287)
            , lat(1000.0f)
            , lon(1000.0f)
            , alt(1.0e25f)
            , dpth(1.0e25f)
            , dist(1.0e25f)
            , ete(0xFFFFFFFF)
            , temp(1.0e25f)
            , time(0xFFFFFFFF)
            , wpt_cat(0)
        {
            strncpy(state,"  ", 3);
            strncpy(cc,"  ",3);

        }
        /// same as Garmin spec.
        uint8_t  wpt_class;
        /// bit 0..4 of dspl_color
        uint8_t  dspl_color;
        /// bit 5..6 of dspl_color
        uint8_t  dspl_attr;
        /// same as Garmin spec.
        uint16_t smbl;
        /// the latitude as degrees
        double   lat;
        /// the longitude as degrees
        double   lon;
        /// same as Garmin spec.
        float    alt;
        /// same as Garmin spec.
        float    dpth;
        /// same as Garmin spec.
        float    dist;
        /// same as Garmin spec.
        char     state[3];
        /// same as Garmin spec.
        char     cc[3];
        /// same as Garmin spec.
        uint32_t ete;
        /// same as Garmin spec.
        float    temp;
        /// same as Garmin spec.
        uint32_t time;
        /// same as Garmin spec.
        uint16_t wpt_cat;
        /// same as Garmin spec.
        std::string ident;
        /// same as Garmin spec.
        std::string comment;
        /// same as Garmin spec.
        std::string facility;
        /// same as Garmin spec.
        std::string city;
        /// same as Garmin spec.
        std::string addr;
        /// same as Garmin spec.
        std::string crossroad;
    };

    /// common route point structure application side
    /**
        This structure has to be used to exchange a waypoint to QLandkarte.
        If an item is missing you can simply add it. This structure must
        never be copied assuming a certain alignment.

        Most values are the same like the ones used in the waypoint and
        route point data structures used by Garmin.

        It adds link information.  For more complex route operations, it
        is critical to note that this refers to the link with the *previous*
        waypoint.   For example, when a route is inverted, the link information
        must be transfered to the previous point first.
    */

    struct RtePt_t : public Wpt_t
    {
        RtePt_t()
            : rte_class(3)
            , subclass_1(0x0000)
            , subclass_2(0x00000000)
            , subclass_3(0xFFFFFFFF)
            , subclass_4(0xFFFFFFFF)
            , subclass_5(0xFFFFFFFF)
            {}

        uint16_t rte_class;

        uint16_t subclass_1;
        uint32_t subclass_2;
        uint32_t subclass_3;
        uint32_t subclass_4;
        uint32_t subclass_5;

        std::string ident;
    };

    /// common route structure application side
    /**
        This structure has to be used to exchange a track to QLandkarte.
        If an item is missing you can simply add it. This structure must
        never be copied assuming a certain alignment.

        Most values are the same like the ones used in the point data
        structures used by Garmin.
    */

    struct Route_t
    {
        /// same as Garmin spec.
        std::string ident;
        /// route points
        std::vector<RtePt_t> route;
    };

    /// common track point structure application side
    /**
        This structure has to be used to exchange a track point to QLandkarte.
        If an item is missing you can simply add it. This structure must
        never be copied assuming a certain alignment.

        Most values are the same like the ones used in the track point data
        structures used by Garmin.
    */
    struct TrkPt_t
    {
        TrkPt_t()
            : lat(0.0)
            , lon(0.0)
            , time(0)
            , alt(1e25f)
            , dpth(1e25f)
            , distance(1e25f)
            , heart_rate(0xFF)
            , cadence(0xFF)
            , sensor(0xFF)
        {

        }
        /// the latitude as degrees
        double   lat;
        /// the longitude as degrees
        double   lon;
        /// the time in sec. as specified by Garmin
        uint32_t time;
        /// same as Garmin spec.
        float    alt;
        /// same as Garmin spec.
        float    dpth;

        float    distance;
        uint8_t  heart_rate;
        uint8_t  cadence;
        uint8_t sensor;

    };

    /// common track structure application side
    /**
        This structure has to be used to exchange a track to QLandkarte.
        If an item is missing you can simply add it. This structure must
        never be copied assuming a certain alignment.

        Most values are the same like the ones used in the point data
        structures used by Garmin.
    */
    struct Track_t
    {
        Track_t()
            : dspl(true)
        , color(0xFF) {

        }
        /// same as Garmin spec.
        bool    dspl;
        /// same as Garmin spec.
        uint8_t color;
        /// same as Garmin spec.
        std::string ident;
        /// trackpoints
        std::vector<TrkPt_t> track;
    };

    struct Map_t
    {
        std::string mapName;
        std::string tileName;
    };

    struct Pvt_t
    {
        /// same as Garmin spec.
        float    alt;
        /// same as Garmin spec.
        float    epe;
        /// same as Garmin spec.
        float    eph;
        /// same as Garmin spec.
        float    epv;
        /// same as Garmin spec.
        uint16_t fix;
        /// same as Garmin spec.
        double   tow;
        /// the latitude as degrees
        double   lat;
        /// the longitude as degrees
        double   lon;
        /// same as Garmin spec.
        float    east;
        /// same as Garmin spec.
        float    north;
        /// same as Garmin spec.
        float    up;
        /// same as Garmin spec.
        float    msl_hght;
        /// same as Garmin spec.
        int16_t  leap_scnds;
        /// same as Garmin spec.
        uint32_t wn_days;
    };

    struct Icon_t
    {
        Icon_t(){ memset(data,0,sizeof(data));}
        /// custom icon index (0..511)
        uint16_t idx;
        /// the bitmap's color table
        char clrtbl[0x400];
        /// the bitmap's image data
        char data[0x100];
    };

    /// device property structure application side
    /**
        This structure is used to account for device properties such as
        the available memory and maximum number of maps.  If a property is
        missing, it can be added.  Both the item must be added and a still
        undefined bit in dev_property_list_t may have to be allocated to
        indicate that the property has indeed been set to a meaningful value.
    */
    // boolean quantitities in the form of a bit field
    struct dev_property_list_t
    {
        uint32_t memory_limit: 1;
        uint32_t maps_limit: 1;
        uint32_t allow_duplicated_map_IDs: 1;
        uint32_t routes_limit: 1;
        uint32_t route_pts_limit: 1;
        uint32_t waypts_limit: 1;
        uint32_t tracks_limit: 1;
        uint32_t track_pts_limit: 1;
        uint32_t screen_size: 1;
        uint32_t pvt_requestable: 1;
        uint32_t custom_POI_limit: 1;
        uint32_t protocols_requestable: 1;
        uint32_t protocols_set: 1;
        uint32_t product_ID: 1;
        uint32_t product_string: 1;
        uint32_t read_trailing_packets: 1;
        uint32_t undefined: 15;
        uint32_t ext_dev_properties: 1;
    };
    // make the bit field addressable as a single integer
    union device_properties_union_t
    {
        uint32_t all;
        dev_property_list_t item;
    };
    // the device properties structure
    struct DevProperties_t
    {
        /// bit encoded list of properties that have been set by the driver
        device_properties_union_t set;
        /// maximum map upload (GMAPSUPP.IMG) size in bytes (0 for no upload)
        uint64_t memory_limit;
        /// maximum number of map tiles allowed for upload
        uint32_t maps_limit;
        /// maximum number of routes
        uint32_t routes_limit;
        /// maximum number of points in a route
        uint32_t route_pts_limit;
        /// maximum number of waypoints
        uint32_t waypts_limit;
        /// maximum number of tracks
        uint32_t tracks_limit;
        /// maximum number of track points
        uint32_t track_pts_limit;
        /// screen size
        uint32_t screenwidth, screenheight;
        /// screen pixel order
        uint32_t pixel_order;
        /// maximum number of POI
        uint32_t custom_POI_limit;
        /// official protocols
        uint32_t L_Link;
        uint32_t A_Cmd;
        uint32_t A_Wpt, D_Wpt[3];
        uint32_t A_Prox_Wpt, D_Prox_Wpt[3];
        uint32_t A_Rt, D_Rt[3];
        uint32_t A_Trk, D_Trk[3];
        uint32_t A_Pvt, D_Pvt[3];
        /// inferred protocols
        uint32_t Q_Map_Limits;
        uint32_t Q_Map_Upload;
        uint32_t Q_Map_Info_Download;
        uint32_t Q_Screenshot;
        uint32_t Q_Custom_Icons;
        /// product_ID
        uint32_t product_ID;
        /// product_string
        const char * product_string;
    };

    /// exception error code
    enum exce_e
    {
        errOpen                  ///< error during opening the link
        ,errSync                 ///< error during sync. up sequence
        ,errWrite                ///< error during write access
        ,errRead                 ///< error during read access
        ,errNotImpl              ///< error because of missing implementation
        ,errRuntime              ///< error during operation
        ,errBlocked              ///< error because access is blocked by another process
    };

    /// exception type
    struct exce_t
    {

        exce_t(exce_e err, const std::string& msg) : err(err), msg(msg) {}
        exce_e err;
        std::string msg;
    };

    /// interface class for device driver plugins
    /**
        This is the common interface to all devices. The application uses
        this definition to gain access to the plugin. Thus if you are an
        application programmer simply load the plugin, resolve and call
        the init function. The object you will get will be of type IDevice.
        There is no need to link against libgarmin.a.

        If you are a plugin programmer you will rather use IDeviceDefault, than
        IDevice. The inheritance chain will look like:

        IDevice -> IDefaultDevice -> CDevice

        Thus if you miss a public method you have to add it here as pure virtual
        and as a default implementation to IDeviceDefault.

        NOTE: If you change this interface you _must_ increment the version
        number defined by INTERFACE_VERSION. This is important to prevent
        crashes due to different interface definitions.

        Most likely your device driver will implement the protected methods with
        leading '_' of IDeviceDefault.
    */
    class IDevice
    {
        public:
            IDevice()
                : _callback_(0), _self_(0){};
            virtual ~IDevice(){};

            /// setup gui callback for user interaction
            /**
                If you use the driver from a GUI you might want to react on some events or show
                the progress of the current operation. The registered callback will be for progress
                status as well as for user interaction depending on the parameters.

                The callback will require a progress dialog if the parameter "progress" is set to anthing
                else than -1. Developers using this callback to show a progress status should make sure:

                * The first call must have a progress of 0 and a title is set.
                * Subsequent calls have a progress from 0..100. The msg parameter can be set.
                * The last call must have a progress of 100.

                If you supply a pointer to a cancel variable from the first call on, you can cancel the
                operation if the value of the variable changes from 0 to 1.

                Developers implementing the callback function must make sure:

                * A progress of 0 will setup the progress dialog. Any subsequent progress of 0
                  is handled like any other progress. The dialog's title is set.
                  If there is a pointer to a cancel variable the dialog should provide a way to
                  cancel the operation. The dialog must make sure ther pointer is valid for
                  subsequent calls. The operation is canceled if "*cancel = 1;"
                * Any subsequent call can have a message parameter, but it mustn't have one.
                * A progress of 100 should remove the progress dialog. There can be subsequent
                  calls with a progress of 100.

                A progress of -1 will create a blocking message box. Depending on the pointers ok
                and cancel the dialog should show ok and cancel button. The integer variables will
                be set to true or false according to the button pressed.

                The supplied void pointer will be passed to every callback call and is free to be used
                by the GUI for what ever purpose it needs.

            */
            void setGuiCallback(void (*func)(int /*progress*/, int * /*ok*/, int * /*cancel*/, const char * /*title*/, const char * /*msg*/, void * /*self*/), void * p) {
            _self_        = p;
            _callback_    = func;
    }

    /// upload a single map file to device
    /**
        This will handle just a single file. Map tiles must be concatenated into
        one big file (gmapsupp.img). If the file containes tiles with locked data
        an array of 25 ASCII digits has to be passed as key.

        @param mapdata pointer to the gmapsupp.img data array
        @param size the size of the gmapsupp.img data array
        @param key pointer to 25 digit key or 0 for no key
    */
    virtual void uploadMap(const uint8_t * mapdata, uint32_t size, const char * key) = 0;

    /// alternative map uppload API
    /**
        This will handle just a single file. Map tiles must be concatenated into
        one big file (gmapsupp.img). If the file containes tiles with locked data
        an array of 25 ASCII digits has to be passed as key.

        @param mapdata pointer to the gmapsupp.img data array
        @param size the size of the gmapsupp.img data array
        @param key pointer to 25 digit key or 0 for no key
    */
    virtual void uploadMap(const char * filename, uint32_t size, const char * key) = 0;

    /// query loaded map list
    /**
        This is not a real download of maps as just the information about the
        loaded maps is transfered.
    */
    virtual void queryMap(std::list<Map_t>& maps) = 0;

    /// download waypoints from device
    /**
        @param waypoints list object to receive waypoints
    */
    virtual void downloadWaypoints(std::list<Garmin::Wpt_t>& waypoints) = 0;

    /// upload waypoints to device
    /**
        @param waypoints list of waypoints to send
    */
    virtual void uploadWaypoints(std::list<Garmin::Wpt_t>& waypoints) = 0;

    /// download track from device
    /**
        @param tracks list object to receive tracks
    */
    virtual void downloadTracks(std::list<Garmin::Track_t>& tracks) = 0;

    /// upload track to device
    /**
        @param tracks list of tracks to send
    */
    virtual void uploadTracks(std::list<Garmin::Track_t>& tracks) = 0;

    /// download routes from device
    /**
        @param routes list of routes
    */
    virtual void downloadRoutes(std::list<Garmin::Route_t>& routes) = 0;

    /// upload route to device
    /**
        @param routes list of routes
    */
    virtual void uploadRoutes(std::list<Garmin::Route_t>& routes) = 0;

    /// upload custom icons to device
    /**
        @param routes list of icons
    */
    virtual void uploadCustomIcons(std::list<Garmin::Icon_t>& icons) = 0;

    /// download a screenshot from the device
    /**
        @param clrtbl a pointer reference to be set to the downloaded color table of size 0x100
        @param data a pointer reference to be set to the downloaded image data array of size width x height
        @param width a integer reference to store the image width at
        @param height a integer reference to store the image height at
    */
    virtual void screenshot(char *& clrtbl, char *& data, int& width, int& height) = 0;

    /// switch device into realtime position mode
    virtual void setRealTimeMode(bool on) = 0;

    /// request real time position
    virtual void getRealTimePos(Garmin::Pvt_t& pvt) = 0;

    /// get the device's properties.
    virtual void getDevProperties(Garmin::DevProperties_t& properties) = 0;

    /// get the copyright notice of this driver
    virtual const std::string& getCopyright() = 0;

    /// get reason string for last exception
    virtual const std::string& getLastError() = 0;

    /// set port string used for communication
    /**
        This should be called prior to an operation. As an operation will
        create a new ILink object, a changed port setting will apply imediately.
        If the ILink object does not need any port settings this value is ignored.
    */
    virtual void setPort(const char * port) = 0;

    protected:
        /// see setGuiCallback()
        void (*_callback_)(int /*progress*/, int * /*ok*/, int * /*cancel*/, const char * /*title*/, const char * /*msg*/, void * /*self*/);
        /// see setGuiCallback()
        void * _self_;
};

}
#endif                           //IDEVICE_H
