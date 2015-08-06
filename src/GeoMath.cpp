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

#include "GeoMath.h"
#include "CResources.h"
#include <stdlib.h>
#include <QtGui>
#include <QMessageBox>
#include <limits>
#ifndef QK_QT5_TZONE
#include <tzdata.h>
#endif

#include <math.h>

#if WIN32
#include <float.h>
#ifndef __MINGW32__
typedef __int32 int32_t;
#endif
#define isnan _isnan
#define FP_NAN NAN
#endif

const double WGS84_a = 6378137.0;
const double WGS84_b = 6356752.314245;
const double WGS84_f = 1.0/298.257223563;

const char * tblTimezone[] =
{
    "Africa/Abidjan",
    "Africa/Accra",
    "Africa/Addis_Ababa",
    "Africa/Algiers",
    "Africa/Asmara",
    "Africa/Bamako",
    "Africa/Bangui",
    "Africa/Banjul",
    "Africa/Bissau",
    "Africa/Blantyre",
    "Africa/Brazzaville",
    "Africa/Bujumbura",
    "Africa/Cairo",
    "Africa/Casablanca",
    "Africa/Conakry",
    "Africa/Dakar",
    "Africa/Dar_es_Salaam",
    "Africa/Djibouti",
    "Africa/Douala",
    "Africa/El_Aaiun",
    "Africa/Freetown",
    "Africa/Gaborone",
    "Africa/Harare",
    "Africa/Johannesburg",
    "Africa/Kampala",
    "Africa/Khartoum",
    "Africa/Kigali",
    "Africa/Kinshasa",
    "Africa/Lagos",
    "Africa/Libreville",
    "Africa/Lome",
    "Africa/Luanda",
    "Africa/Lubumbashi",
    "Africa/Lusaka",
    "Africa/Malabo",
    "Africa/Maputo",
    "Africa/Maseru",
    "Africa/Mbabane",
    "Africa/Mogadishu",
    "Africa/Monrovia",
    "Africa/Nairobi",
    "Africa/Ndjamena",
    "Africa/Niamey",
    "Africa/Nouakchott",
    "Africa/Ouagadougou",
    "Africa/Porto-Novo",
    "Africa/Sao_Tome",
    "Africa/Tripoli",
    "Africa/Tunis",
    "Africa/Windhoek",
    "America/Adak",
    "America/Anguilla",
    "America/Antigua",
    "America/Araguaina",
    "America/Argentina/Buenos_Aires",
    "America/Argentina/Catamarca",
    "America/Argentina/Cordoba",
    "America/Argentina/Jujuy",
    "America/Argentina/La_Rioja",
    "America/Argentina/Mendoza",
    "America/Argentina/Rio_Gallegos",
    "America/Argentina/San_Juan",
    "America/Argentina/San_Luis",
    "America/Argentina/Tucuman",
    "America/Argentina/Ushuaia",
    "America/Aruba",
    "America/Asuncion",
    "America/Atikokan",
    "America/Bahia",
    "America/Barbados",
    "America/Belem",
    "America/Belize",
    "America/Blanc-Sablon",
    "America/Boa_Vista",
    "America/Bogota",
    "America/Boise",
    "America/Cambridge_Bay",
    "America/Campo_Grande",
    "America/Cancun",
    "America/Caracas",
    "America/Cayenne",
    "America/Cayman",
    "America/Chicago",
    "America/Chihuahua",
    "America/Coral_Harbour",
    "America/Costa_Rica",
    "America/Cuiaba",
    "America/Curacao",
    "America/Dawson",
    "America/Dawson_Creek",
    "America/Denver",
    "America/Dominica",
    "America/Edmonton",
    "America/Eirunepe",
    "America/El_Salvador",
    "America/Fortaleza",
    "America/Glace_Bay",
    "America/Goose_Bay",
    "America/Grand_Turk",
    "America/Grenada",
    "America/Guadeloupe",
    "America/Guatemala",
    "America/Guayaquil",
    "America/Guyana",
    "America/Halifax",
    "America/Havana",
    "America/Hermosillo",
    "America/Indiana/Indianapolis",
    "America/Indiana/Knox",
    "America/Indiana/Marengo",
    "America/Indiana/Petersburg",
    "America/Indiana/Vevay",
    "America/Indiana/Vincennes",
    "America/Indiana/Winamac",
    "America/Inuvik",
    "America/Iqaluit",
    "America/Jamaica",
    "America/Juneau",
    "America/Kentucky/Louisville",
    "America/Kentucky/Monticello",
    "America/La_Paz",
    "America/Lima",
    "America/Los_Angeles",
    "America/Maceio",
    "America/Managua",
    "America/Manaus",
    "America/Marigot",
    "America/Martinique",
    "America/Mazatlan",
    "America/Menominee",
    "America/Merida",
    "America/Mexico_City",
    "America/Miquelon",
    "America/Moncton",
    "America/Monterrey",
    "America/Montevideo",
    "America/Montreal",
    "America/Montserrat",
    "America/Nassau",
    "America/New_York",
    "America/Nipigon",
    "America/Noronha",
    "America/North_Dakota/Center",
    "America/North_Dakota/Salem",
    "America/Panama",
    "America/Pangnirtung",
    "America/Paramaribo",
    "America/Phoenix",
    "America/Port-au-Prince",
    "America/Port_of_Spain",
    "America/Porto_Velho",
    "America/Puerto_Rico",
    "America/Rainy_River",
    "America/Rankin_Inlet",
    "America/Recife",
    "America/Regina",
    "America/Resolute",
    "America/Rio_Branco",
    "America/Santarem",
    "America/Santiago",
    "America/Santo_Domingo",
    "America/Sao_Paulo",
    "America/St_Barthelemy",
    "America/St_Johns",
    "America/St_Kitts",
    "America/St_Lucia",
    "America/St_Thomas",
    "America/St_Vincent",
    "America/Tegucigalpa",
    "America/Thunder_Bay",
    "America/Tijuana",
    "America/Toronto",
    "America/Tortola",
    "America/Vancouver",
    "America/Whitehorse",
    "America/Winnipeg",
    "America/Yellowknife",
    "Ameriica/Swift_Current",
    "Arctic/Longyearbyen",
    "Asia/Aden",
    "Asia/Almaty",
    "Asia/Amman",
    "Asia/Anadyr",
    "Asia/Aqtau",
    "Asia/Aqtobe",
    "Asia/Ashgabat",
    "Asia/Baghdad",
    "Asia/Bahrain",
    "Asia/Baku",
    "Asia/Bangkok",
    "Asia/Beirut",
    "Asia/Bishkek",
    "Asia/Brunei",
    "Asia/Choibalsan",
    "Asia/Chongqing",
    "Asia/Colombo",
    "Asia/Damascus",
    "Asia/Dhaka",
    "Asia/Dili",
    "Asia/Dubai",
    "Asia/Dushanbe",
    "Asia/Gaza",
    "Asia/Harbin",
    "Asia/Ho_Chi_Minh",
    "Asia/Hong_Kong",
    "Asia/Hovd",
    "Asia/Irkutsk",
    "Asia/Jakarta",
    "Asia/Jayapura",
    "Asia/Jerusalem",
    "Asia/Kabul",
    "Asia/Kamchatka",
    "Asia/Karachi",
    "Asia/Kashgar",
    "Asia/Katmandu",
    "Asia/Kolkata",
    "Asia/Krasnoyarsk",
    "Asia/Kuala_Lumpur",
    "Asia/Kuching",
    "Asia/Kuwait",
    "Asia/Macau",
    "Asia/Magadan",
    "Asia/Makassar",
    "Asia/Manila",
    "Asia/Muscat",
    "Asia/Nicosia",
    "Asia/Novosibirsk",
    "Asia/Omsk",
    "Asia/Oral",
    "Asia/Phnom_Penh",
    "Asia/Pontianak",
    "Asia/Pyongyang",
    "Asia/Qatar",
    "Asia/Qyzylorda",
    "Asia/Rangoon",
    "Asia/Riyadh",
    "Asia/Sakhalin",
    "Asia/Samarkand",
    "Asia/Seoul",
    "Asia/Shanghai",
    "Asia/Singapore",
    "Asia/Taipei",
    "Asia/Tashkent",
    "Asia/Tbilisi",
    "Asia/Tehran",
    "Asia/Thimphu",
    "Asia/Tokyo",
    "Asia/Ulaanbaatar",
    "Asia/Urumqi",
    "Asia/Vientiane",
    "Asia/Vladivostok",
    "Asia/Yakutsk",
    "Asia/Yekaterinburg",
    "Asia/Yerevan",
    "Atlantic/Azores",
    "Atlantic/Bermuda",
    "Atlantic/Canary",
    "Atlantic/Cape_Verde",
    "Atlantic/Faroe",
    "Atlantic/Madeira",
    "Atlantic/Reykjavik",
    "Atlantic/South_Georgia",
    "Atlantic/St_Helena",
    "Atlantic/Stanley",
    "Australia/Adelaide",
    "Australia/Brisbane",
    "Australia/Broken_Hill",
    "Australia/Currie",
    "Australia/Darwin",
    "Australia/Eucla",
    "Australia/Hobart",
    "Australia/Lindeman",
    "Australia/Lord_Howe",
    "Australia/Melbourne",
    "Australia/Perth",
    "Australia/Sydney",
    "Europe/Amsterdam",
    "Europe/Andorra",
    "Europe/Athens",
    "Europe/Belgrade",
    "Europe/Berlin",
    "Europe/Bratislava",
    "Europe/Brussels",
    "Europe/Bucharest",
    "Europe/Budapest",
    "Europe/Chisinau",
    "Europe/Copenhagen",
    "Europe/Dublin",
    "Europe/Gibraltar",
    "Europe/Guernsey",
    "Europe/Helsinki",
    "Europe/Isle_of_Man",
    "Europe/Istanbul",
    "Europe/Jersey",
    "Europe/Kaliningrad",
    "Europe/Kiev",
    "Europe/Lisbon",
    "Europe/Ljubljana",
    "Europe/London",
    "Europe/Luxembourg",
    "Europe/Madrid",
    "Europe/Malta",
    "Europe/Marienhamn",
    "Europe/Minsk",
    "Europe/Monaco",
    "Europe/Moscow",
    "Europe/Oslo",
    "Europe/Paris",
    "Europe/Podgorica",
    "Europe/Prague",
    "Europe/Riga",
    "Europe/Rome",
    "Europe/Samara",
    "Europe/San_Marino",
    "Europe/Sarajevo",
    "Europe/Simferopol",
    "Europe/Skopje",
    "Europe/Sofia",
    "Europe/Stockholm",
    "Europe/Tallinn",
    "Europe/Tirane",
    "Europe/Uzhgorod",
    "Europe/Vaduz",
    "Europe/Vatican",
    "Europe/Vienna",
    "Europe/Vilnius",
    "Europe/Volgograd",
    "Europe/Warsaw",
    "Europe/Zagreb",
    "Europe/Zaporozhye",
    "Europe/Zurich",
    "Indian/Antananarivo",
    "Indian/Chagos",
    "Indian/Christmas",
    "Indian/Cocos",
    "Indian/Comoro",
    "Indian/Kerguelen",
    "Indian/Mahe",
    "Indian/Maldives",
    "Indian/Mauritius",
    "Indian/Mayotte",
    "Indian/Reunion",
    "Pacific/Apia",
    "Pacific/Auckland",
    "Pacific/Chatham",
    "Pacific/Easter",
    "Pacific/Efate",
    "Pacific/Enderbury",
    "Pacific/Fakaofo",
    "Pacific/Fiji",
    "Pacific/Funafuti",
    "Pacific/Galapagos",
    "Pacific/Gambier",
    "Pacific/Guadalcanal",
    "Pacific/Guam",
    "Pacific/Honolulu",
    "Pacific/Johnston",
    "Pacific/Kiritimati",
    "Pacific/Kosrae",
    "Pacific/Kwajalein",
    "Pacific/Majuro",
    "Pacific/Marquesas",
    "Pacific/Midway",
    "Pacific/Nauru",
    "Pacific/Niue",
    "Pacific/Norfolk",
    "Pacific/Noumea",
    "Pacific/Pago_Pago",
    "Pacific/Palau",
    "Pacific/Pitcairn",
    "Pacific/Ponape",
    "Pacific/Port_Moresby",
    "Pacific/Rarotonga",
    "Pacific/Saipan",
    "Pacific/Tahiti",
    "Pacific/Tarawa",
    "Pacific/Tongatapu",
    "Pacific/Truk",
    "Pacific/Wake",
    "Pacific/Wallis",
    0
};

const int N_TIMEZONES = sizeof(tblTimezone)/sizeof(const char*);

bool GPS_Math_Deg_To_DegMin(float v, int32_t *d, float *m)
{
    bool sign = v < 0;
    int32_t deg = abs(v);
    double  min = (fabs(v) - deg) * 60.0;

    *d = deg;
    *m = min;

    return sign;
}


void GPS_Math_DegMin_To_Deg(bool sign, const int32_t d, const float m, float& deg)
{

    deg = abs(d) + m / 60.0;
    if(sign)
    {
        deg = -deg;
    }

    return;
}


void GPS_Math_DegMinSec_To_Deg(bool sign, const int32_t d, const int32_t m, const int32_t s, float& deg)
{

    deg = abs(d) + float(m) / 60.0 + float(s) / 3600;
    if(sign)
    {
        deg = -deg;
    }

    return;
}


namespace GeoMath
{
    QRegExp reCoord1("^\\s*([N|S]){1}\\W*([0-9]+)\\W*([0-9]+\\.[0-9]+)\\s+([E|W|O]){1}\\W*([0-9]+)\\W*([0-9]+\\.[0-9]+)\\s*$");

    QRegExp reCoord2("^\\s*([N|S]){1}\\s*([0-9]+\\.[0-9]+)\\W*\\s+([E|W|O]){1}\\s*([0-9]+\\.[0-9]+)\\W*\\s*$");

    QRegExp reCoord3("^\\s*([-0-9]+\\.[0-9]+)\\s+([-0-9]+\\.[0-9]+)\\s*$");

    QRegExp reCoord4("^\\s*([N|S]){1}\\s*([0-9]+)\\W+([0-9]+)\\W+([0-9]+)\\W*([E|W|O]){1}\\W*([0-9]+)\\W+([0-9]+)\\W+([0-9]+)\\W*\\s*$");

    QRegExp reCoord5("^\\s*([-0-9]+\\.[0-9]+)([N|S])\\s+([-0-9]+\\.[0-9]+)([W|E])\\s*$");
}


using namespace GeoMath;

//49.03196968N 12.10440376E

bool GPS_Math_Str_To_Deg(const QString& str, float& lon, float& lat, bool silent)
{

    if(reCoord2.exactMatch(str))
    {
        bool signLat    = reCoord2.cap(1) == "S";
        float absLat    = reCoord2.cap(2).toDouble();
        lat = signLat ? -absLat : absLat;

        bool signLon    = reCoord2.cap(3) == "W";
        float absLon    = reCoord2.cap(4).toDouble();
        lon = signLon ? -absLon : absLon;
    }
    else if(reCoord1.exactMatch(str))
    {

        bool signLat    = reCoord1.cap(1) == "S";
        int degLat      = reCoord1.cap(2).toInt();
        float minLat    = reCoord1.cap(3).toDouble();

        GPS_Math_DegMin_To_Deg(signLat, degLat, minLat, lat);

        bool signLon    = reCoord1.cap(4) == "W";
        int degLon      = reCoord1.cap(5).toInt();
        float minLon    = reCoord1.cap(6).toDouble();

        GPS_Math_DegMin_To_Deg(signLon, degLon, minLon, lon);
    }
    else if(reCoord3.exactMatch(str))
    {
        lat             = reCoord3.cap(1).toDouble();
        lon             = reCoord3.cap(2).toDouble();
    }
    else if(reCoord4.exactMatch(str))
    {
        bool signLat    = reCoord4.cap(1) == "S";
        int degLat    = reCoord4.cap(2).toInt();
        int minLat    = reCoord4.cap(3).toInt();
        int secLat    = reCoord4.cap(4).toInt();

        GPS_Math_DegMinSec_To_Deg(signLat, degLat, minLat, secLat, lat);

        bool signLon    = reCoord4.cap(5) == "W";
        int degLon    = reCoord4.cap(6).toInt();
        int minLon    = reCoord4.cap(7).toInt();
        int secLon    = reCoord4.cap(8).toInt();

        GPS_Math_DegMinSec_To_Deg(signLon, degLon, minLon, secLon, lon);

    }
    else if(reCoord5.exactMatch(str))
    {
        bool signLon    = reCoord4.cap(4) == "W";
        bool signLat    = reCoord4.cap(2) == "S";
        lat             = reCoord5.cap(1).toDouble();
        lon             = reCoord5.cap(3).toDouble();

        if(signLon) lon = -lon;
        if(signLat) lat = -lat;
    }
    else
    {
        if(!silent) QMessageBox::warning(0,QObject::tr("Error"),QObject::tr("Bad position format. Must be: \"[N|S] ddd mm.sss [W|E] ddd mm.sss\" or \"[N|S] ddd.ddd [W|E] ddd.ddd\""),QMessageBox::Ok,QMessageBox::NoButton);
        return false;
    }

    if(fabs(lon) > 180.0 || fabs(lat) > 90.0)
    {
        if(!silent) QMessageBox::warning(0,QObject::tr("Error"),QObject::tr("Position values out of bounds. "),QMessageBox::Ok,QMessageBox::NoButton);
        return false;
    }

    return true;
}


/// calc if 2 line segments intersect
/**
    The 1st line is defined by (x11,y11) - (x12,y12)
    The 2nd line is defined by (x21,y21) - (x22,y22)

*/
bool testLineSegForIntersect(float x11, float y11, float x12, float y12, float x21, float y21, float x22, float y22)
{
    /*
            float denom = ((other_line.end_.y_ - other_line.begin_.y_)*(end_.x_ - begin_.x_)) -
                          ((other_line.end_.x_ - other_line.begin_.x_)*(end_.y_ - begin_.y_));

            float nume_a = ((other_line.end_.x_ - other_line.begin_.x_)*(begin_.y_ - other_line.begin_.y_)) -
                           ((other_line.end_.y_ - other_line.begin_.y_)*(begin_.x_ - other_line.begin_.x_));

            float nume_b = ((end_.x_ - begin_.x_)*(begin_.y_ - other_line.begin_.y_)) -
                           ((end_.y_ - begin_.y_)*(begin_.x_ - other_line.begin_.x_));
    */
    float denom  = ((y22 - y21) * (x12 - x11)) - ((x22 - x21) * (y12 - y11));
    float nume_a = ((x22 - x21) * (y11 - y21)) - ((y22 - y21) * (x11 - x21));
    float nume_b = ((x12 - x11) * (y11 - y21)) - ((y12 - y11) * (x11 - x21));

    if(denom == 0.0f) return false;

    float ua = nume_a / denom;
    float ub = nume_b / denom;

    if(ua >= 0.0f && ua <= 1.0f && ub >= 0.0f && ub <= 1.0f)
    {
        return true;
    }
    return false;
}


bool testPointInPolygon(const projXY& pt, const QVector<projXY>& poly1)
{

    bool    c = false;
    int     npol;
    int     i = 0, j = 0;
    projXY      p1, p2;          // the two points of the polyline close to pt
    float  x = pt.u;
    float  y = pt.v;

    npol = poly1.count();
    if(npol > 2)
    {

        // see http://local.wasp.uwa.edu.au/~pbourke/geometry/insidepoly/
        for (i = 0, j = npol-1; i < npol; j = i++)
        {
            p1 = poly1[j];
            p2 = poly1[i];

            if ((((p2.v <= y) && (y < p1.v))  || ((p1.v <= y) && (y < p2.v))) &&
                (x < (p1.u - p2.u) * (y - p2.v) / (p1.v - p2.v) + p2.u))
            {
                c = !c;
            }
        }
    }
    return c;
}


bool testPolygonsForIntersect(const QVector<projXY>& poly1, const QVector<projXY>& poly2)
{

    int n;
    int npol1 = poly1.count();
    int npol2 = poly2.count();

    if(npol1 < 2 || npol2 < 2) return false;

    // test if points of poly1 are within poly2.
    for(n = 0; n < npol1; ++n)
    {
        if(testPointInPolygon(poly1[n],poly2)) return true;
    }

    // test if lines of poly1 intersect with lines from poly2
    int i1 = 0, j1 = 0;
    int i2 = 0, j2 = 0;

    projXY  p1, p2, p3, p4;

    for (i1 = 0, j1 = npol1-1; i1 < npol1; j1 = i1++)
    {
        p1 = poly1[j1];
        p2 = poly1[i1];
        for (i2 = 0, j2 = npol2-1; i2 < npol2; j2 = i2++)
        {
            p3 = poly2[j2];
            p4 = poly2[i2];
            if(testLineSegForIntersect(p1.u,p1.v,p2.u,p2.v,p3.u,p3.v,p4.u,p4.v))
            {
                return true;
            }
        }
    }
    return false;
}


// from http://www.movable-type.co.uk/scripts/LatLongVincenty.html
// additional antipodal convergence trick might be a bit lame, but it seems to work
double distance(const projXY& p1, const projXY& p2, double& a1, double& a2)
{
    double cosSigma = 0.0;
    double sigma = 0.0;
    double sinAlpha = 0.0;
    double cosSqAlpha = 0.0;
    double cos2SigmaM = 0.0;
    double sinSigma = 0.0;
    double sinLambda = 0.0;
    double cosLambda = 0.0;

    double L = p2.u - p1.u;

    double U1 = atan((1-WGS84_f) * tan(p1.v));
    double U2 = atan((1-WGS84_f) * tan(p2.v));
    double sinU1 = sin(U1), cosU1 = cos(U1);
    double sinU2 = sin(U2), cosU2 = cos(U2);
    double lambda = L, lambdaP = (double)(2*M_PI);
    unsigned iterLimit = 20;

    while (fabs(lambda - lambdaP) > 1e-12)
    {
        if (!iterLimit)
        {
            lambda = M_PI;
            qDebug() << "No lambda convergence, most likely due to near-antipodal points. Assuming antipodal.";
        }

        sinLambda = sin(lambda);
        cosLambda = cos(lambda);
        sinSigma = sqrt((cosU2*sinLambda) * (cosU2*sinLambda) + (cosU1*sinU2-sinU1*cosU2*cosLambda) * (cosU1*sinU2-sinU1*cosU2*cosLambda));

        if (sinSigma==0)
        {
            return 0;            // co-incident points
        }

        cosSigma = sinU1 * sinU2 + cosU1 * cosU2 * cosLambda;
        sigma = atan2(sinSigma, cosSigma);
        sinAlpha = cosU1 * cosU2 * sinLambda / sinSigma;
        cosSqAlpha = 1 - sinAlpha * sinAlpha;
        cos2SigmaM = cosSigma - 2 * sinU1 * sinU2 / cosSqAlpha;

        if (isnan(cos2SigmaM))
        {
            cos2SigmaM = 0;      // equatorial line: cosSqAlpha=0 (6)
        }

        double C = WGS84_f/16 * cosSqAlpha * (4 + WGS84_f * (4 - 3 * cosSqAlpha));
        lambdaP = lambda;

        if (iterLimit--) lambda = L + (1-C) * WGS84_f * sinAlpha * (sigma + C*sinSigma*(cos2SigmaM + C * cosSigma * (-1 + 2 * cos2SigmaM * cos2SigmaM)));
    }

    double uSq = cosSqAlpha * (WGS84_a*WGS84_a - WGS84_b*WGS84_b) / (WGS84_b*WGS84_b);
    double A = 1 + uSq/16384*(4096+uSq*(-768+uSq*(320-175*uSq)));
    double B = uSq/1024 * (256+uSq*(-128+uSq*(74-47*uSq)));
    double deltaSigma = B*sinSigma*(cos2SigmaM+B/4*(cosSigma*(-1+2*cos2SigmaM*cos2SigmaM)-B/6*cos2SigmaM*(-3+4*sinSigma*sinSigma)*(-3+4*cos2SigmaM*cos2SigmaM)));
    double s = WGS84_b*A*(sigma-deltaSigma);

    a1 = atan2(cosU2 * sinLambda, cosU1 * sinU2 - sinU1 * cosU2 * cosLambda) * 360 / (2*M_PI);
    a2 = atan2(cosU1 * sinLambda, -sinU1 * cosU2 + cosU1 * sinU2 * cosLambda) * 360 / (2*M_PI);
    return s;
}


double parallel_distance(const projXY& p1, const projXY& p2)
{
    // Assure same latitude V
    if (p1.v != p2.v) return std::numeric_limits<double>::quiet_NaN();

    // Compute the distance between Earth center and latitude V
    double cosV = cos(p1.v);
    double r = WGS84_a*WGS84_b / sqrt(cosV*cosV*WGS84_b*WGS84_b + (1-cosV*cosV)*WGS84_a*WGS84_a);

    // Return the lenght of U2-U1 arc at latitude V
    return fabs(p2.u-p1.u)*r*cosV;
}


void GPS_Math_Deg_To_Str(const float& x, const float& y, QString& str)
{
    qint32 degN,degE;
    float minN,minE;

    bool signLat = GPS_Math_Deg_To_DegMin(y, &degN, &minN);

    bool signLon = GPS_Math_Deg_To_DegMin(x, &degE, &minE);

    QString lat,lng;
    lat = signLat ? "S" : "N";
    lng = signLon ? "W" : "E";
    str = QString("%2%3%1 %4 %5%6%1 %7").arg(QChar(0260)).arg(lat).arg(abs(degN), 2, 10, QLatin1Char('0')).arg(minN, 6, 'f', 3, QLatin1Char('0')).arg(lng).arg(abs(degE), 2, 10, QLatin1Char('0')).arg(minE, 6, 'f', 3, QLatin1Char('0'));
    //print fully metric str.sprintf("%s%06.8f\260 %s%06.8f\260 ",lat.toUtf8().data(),y,lng.toUtf8().data(),x);
}


bool GPS_Math_Str_To_LongLat(const QString& str, float& lon, float& lat, const QString& srcproj, const QString& tarproj)
{
    double u = 0, v = 0;
    QRegExp re("^\\s*([\\-0-9\\.]+)\\s+([\\-0-9\\.]+)\\s*$");

    projPJ  pjTar = 0;
    if(!tarproj.isEmpty())
    {
        pjTar = pj_init_plus(tarproj.toLatin1());
        if(pjTar == 0)
        {
            QMessageBox::warning(0,QObject::tr("Error ..."), QObject::tr("Failed to setup projection. Bad syntax?\n%1").arg(tarproj), QMessageBox::Abort,QMessageBox::Abort);
            return false;
        }
    }

    projPJ  pjSrc = 0;
    if(!srcproj.isEmpty())
    {
        pjSrc = pj_init_plus(srcproj.toLatin1());
        if(pjSrc == 0)
        {
            if(pjTar) pj_free(pjTar);

            QMessageBox::warning(0,QObject::tr("Error ..."), QObject::tr("Failed to setup projection. Bad syntax?\n%1").arg(srcproj), QMessageBox::Abort,QMessageBox::Abort);
            return false;
        }
    }
    else
    {
        pjSrc = pj_init_plus("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");
    }

    if(GPS_Math_Str_To_Deg(str, lon, lat,true))
    {
        u = lon * DEG_TO_RAD;
        v = lat * DEG_TO_RAD;
    }
    else
    {
        if(!re.exactMatch(str))
        {
            QMessageBox::warning(0,QObject::tr("Error ..."), QObject::tr("Failed to read reference coordinate. Bad syntax?\n%1").arg(str), QMessageBox::Abort,QMessageBox::Abort);
            if(pjSrc) pj_free(pjSrc);
            if(pjTar) pj_free(pjTar);
            return false;
        }
        u = re.cap(1).toDouble();
        v = re.cap(2).toDouble();

        if((abs(u) <= 180) && (abs(v) <= 90) && pjTar)
        {
            u = u * DEG_TO_RAD;
            v = v * DEG_TO_RAD;
        }
    }

    if(pjTar && pjSrc)
    {
        pj_transform(pjSrc,pjTar,1,0,&u,&v,0);
    }

    lon = u;
    lat = v;

    if(pjSrc) pj_free(pjSrc);
    if(pjTar) pj_free(pjTar);
    return true;
}


projXY GPS_Math_Wpt_Projection(const projXY& pt1, double distance, double bearing)
{
    projXY pt2;

    double d    = distance / 6378130.0;
    double lon1 = pt1.u;
    double lat1 = pt1.v;

    double lat2 = asin(sin(lat1) * cos(d) + cos(lat1) * sin(d) * cos(-bearing));
    double lon2 = cos(lat1) == 0 ? lon1 : fmod(lon1 - asin(sin(-bearing) * sin(d) / cos(lat1)) + M_PI, (2*M_PI)) - M_PI;

    pt2.u = lon2;
    pt2.v = lat2;
    return pt2;
}


void GPS_Math_SubPolyline( const QPoint& pt1, const QPoint& pt2, int threshold, const QPolygon& line1, QPolygon& line2)
{
    int i, len;
    projXY p1, p2;
    double dx,dy;                // delta x and y defined by p1 and p2
    double d_p1_p2;              // distance between p1 and p2
    double u;                    // ratio u the tangent point will divide d_p1_p2
    double x,y;                  // coord. (x,y) of the point on line defined by [p1,p2] close to pt
    double distance;             // the distance to the polyline
    double shortest1 = threshold;
    double shortest2 = threshold;
    int idx11 = -1, idx21 = -1, idx12 = -1;

    QPoint pt11;
    QPoint pt21;

    line2.clear();

    len = line1.size();

    // find points on line closest to pt1 and pt2
    for(i=1; i<len; ++i)
    {
        p1.u = line1[i - 1].x();
        p1.v = line1[i - 1].y();
        p2.u = line1[i].x();
        p2.v = line1[i].y();

        dx = p2.u - p1.u;
        dy = p2.v - p1.v;
        d_p1_p2 = sqrt(dx * dx + dy * dy);

        // find point on line closest to pt1
        u = ((pt1.x() - p1.u) * dx + (pt1.y() - p1.v) * dy) / (d_p1_p2 * d_p1_p2);

        if(u >= 0.0 && u <= 1.0)
        {
            x = p1.u + u * dx;
            y = p1.v + u * dy;

            distance = sqrt((x - pt1.x())*(x - pt1.x()) + (y - pt1.y())*(y - pt1.y()));

            if(distance < shortest1)
            {
                idx11 = i - 1;
                idx12 = i;
                pt11.setX(x);
                pt11.setY(y);
                shortest1 = distance;
            }
        }

        // find point on line closest to pt2
        u = ((pt2.x() - p1.u) * dx + (pt2.y() - p1.v) * dy) / (d_p1_p2 * d_p1_p2);

        if(u >= 0.0 && u <= 1.0)
        {

            x = p1.u + u * dx;
            y = p1.v + u * dy;

            distance = sqrt((x - pt2.x())*(x - pt2.x()) + (y - pt2.y())*(y - pt2.y()));

            if(distance < shortest2)
            {
                idx21 = i - 1;
                pt21.setX(x);
                pt21.setY(y);
                shortest2 = distance;
            }
        }
    }

    // if 1st point can't be found test for distance to both ends
    if(idx11 == -1)
    {
        QPoint px = line1.first();
        distance = sqrt((double)((px.x() - pt1.x())*(px.x() - pt1.x()) + (px.y() - pt1.y())*(px.y() - pt1.y())));
        if(distance < (threshold<<1))
        {
            idx11 = 0;
            idx12 = 1;
            pt11 = px;
        }
        else
        {
            px = line1.last();
            distance = sqrt((double)((px.x() - pt1.x())*(px.x() - pt1.x()) + (px.y() - pt1.y())*(px.y() - pt1.y())));
            if(distance < (threshold<<1))
            {
                idx11 = line1.size() - 2;
                idx12 = line1.size() - 1;
                pt11 = px;
            }
        }
    }

    // if 2nd point can't be found test for distance to both ends
    if(idx21 == -1)
    {
        QPoint px = line1.first();
        distance = sqrt((double)((px.x() - pt2.x())*(px.x() - pt2.x()) + (px.y() - pt2.y())*(px.y() - pt2.y())));

        if(distance < (threshold<<1))
        {
            idx21 = 0;
            pt21 = px;
        }
        else
        {
            px = line1.last();
            distance = sqrt((double)((px.x() - pt2.x())*(px.x() - pt2.x()) + (px.y() - pt2.y())*(px.y() - pt2.y())));
            if(distance < (threshold<<1))
            {
                idx21 = line1.size() - 2;
                pt21 = px;
            }
        }
    }

    //    qDebug() << line1.size() << idx11 << idx12 << idx21 << pt1 << pt2 << pt11 << pt21;

    // copy segment of line 1 to line2
    if(idx11 != -1 && idx21 != -1)
    {
        if(idx11 == idx21)
        {
            line2.push_back(pt11);
            line2.push_back(pt21);
        }
        else if(idx12 == idx21)
        {
            line2.push_back(pt11);
            line2.push_back(line1[idx12]);
            line2.push_back(pt21);
        }
        else if(idx11 < idx21)
        {
            line2.push_back(pt11);
            for(i = idx12; i <= idx21; i++)
            {
                line2.push_back(line1[i]);
            }
            line2.push_back(pt21);
        }
        else if(idx11 > idx21)
        {
            line2.push_back(pt11);
            for(i = idx11; i > idx21; i--)
            {
                line2.push_back(line1[i]);
            }
            line2.push_back(pt21);
        }

    }
}


bool GPS_Math_LineCrossesRect(const QPoint& p1, const QPoint& p2, const QRect& rect)
{

    // the trival case
    if(rect.contains(p1) || rect.contains(p2))
    {
        return true;
    }

    double slope    = double(p2.y() - p1.y()) / (p2.x() - p1.x());
    double offset   = p1.y() - slope * p1.x();
    double y1       = offset + slope * rect.left();
    double y2       = offset + slope * rect.right();

    if((y1 < rect.top()) && (y2 < rect.top()))
    {
        return false;
    }
    else if((y1 > rect.bottom()) && (y2 > rect.bottom()))
    {
        return false;
    }

    return true;
}


double GPS_Math_distPointLine3D(point3D& x1, point3D& x2, point3D& x0)
{

    point3D v1, v2, v3, v1x2;

    double a1x2, a3;

    // (x0 - x1)
    v1.x    = x0.x - x1.x;
    v1.y    = x0.y - x1.y;
    v1.z    = x0.z - x1.z;

    // (x0 - x2)
    v2.x    = x0.x - x2.x;
    v2.y    = x0.y - x2.y;
    v2.z    = x0.z - x2.z;

    // (x2 - x1)
    v3.x    = x2.x - x1.x;
    v3.y    = x2.y - x1.y;
    v3.z    = x2.z - x1.z;

    // (x0 - x1)x(x0 - x2)
    v1x2.x  = v1.y * v2.z - v1.z * v2.y;
    v1x2.y  = v1.z * v2.x - v1.x * v2.z;
    v1x2.z  = v1.x * v2.y - v1.y * v2.x;

    // |(x0 - x1)x(x0 - x2)|
    a1x2    = sqrt(v1x2.x*v1x2.x + v1x2.y*v1x2.y + v1x2.z*v1x2.z);
    // |(x2 - x1)|
    a3      = sqrt(v3.x*v3.x + v3.y*v3.y + v3.z*v3.z);

    return a1x2/a3;
}


struct segment
{
    segment(): idx1(0), idx2(0){}
    segment(qint32 idx1, quint32 idx2) : idx1(idx1), idx2(idx2) {}
    qint32 idx1;
    qint32 idx2;
};

void GPS_Math_DouglasPeucker(QVector<pointDP> &line, double d)
{
    if(line.count() < 3) return;

    QStack<segment> stack;
    stack << segment(0, line.size() - 1);

    while(!stack.isEmpty())
    {
        int idx = -1;
        segment seg = stack.pop();

        pointDP& x1 = line[seg.idx1];
        pointDP& x2 = line[seg.idx2];

        for(int i = seg.idx1 + 1; i < seg.idx2; i++)
        {
            double distance = GPS_Math_distPointLine3D(x1, x2, line[i]);
            if(distance > d)
            {
                idx = i;
                break;
            }
        }

        if(idx > 0)
        {
            stack << segment(seg.idx1, idx);
            stack << segment(idx, seg.idx2);
        }
        else
        {
            for(int i = seg.idx1 + 1; i < seg.idx2; i++)
            {
                line[i].used = false;
            }
        }
    }
}


const char * GPS_Timezone(double lon, double lat)
{
    CResources::TimezoneMode_e mode = CResources::self().getTimezoneMode();

    if(mode == CResources::eTZUtc)
    {
        return "UTC";
    }
    else if(mode == CResources::eTZLocal)
    {
#ifdef QK_QT5_TZONE
        return QTimeZone::systemTimeZoneId().data();
#else
        return TimeStamp::defaultZone().toLatin1();
#endif
    }
    else if(mode == CResources::eTZSelected)
    {
        return CResources::self().getSelectedTimezone().toLatin1();
    }

    static QImage imgTimezone = QPixmap(":/pics/timezones.png").toImage();

    int x = qRound(2048.0 / 360.0 * (180.0 + lon));
    int y = qRound(1024.0 / 180.0 * (90.0  - lat));

    QRgb rgb = imgTimezone.pixel(x,y);

    if(qRed(rgb) == 0 && qGreen(rgb) == 0)
    {
        return "UTC";
    }

    int tz   = ((qRed(rgb) & 248) << 1) + ((qGreen(rgb) >> 4) & 15);
    if(tz >= N_TIMEZONES)
    {
        return 0;
    }

    return tblTimezone[tz];
}


QPoint getPolygonCentroid(const QPolygon& polygon)
{

    int i, len, x = 0, y = 0;

    len = polygon.size();

    for(i = 0; i < len; i++)
    {
        x = x + polygon[i].x();
        y = y + polygon[i].y();
    }
    x = x / len;
    y = y / len;

    return QPoint(x,y);
}
