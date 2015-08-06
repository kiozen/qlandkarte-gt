//
// C++ Implementation: TZ discovery for Windows
//
// Description: tries to match the windows timezone to an Olson timezone
//   the list shown below is floating around the Internet ... hope it is correct
//
//
// Author: Konrad Rosenbaum <konrad@silmor.de>, (C) 2008
//
// Copyright: See README/COPYING files that come with this distribution
//
//

#include <QSettings>
#include <QStringList>

#include <QDebug>

#include "tzsys.h"

using namespace TimeZoneLib;

// internal: accesses the registry to find the windows name of the currently set time zone
// helper to systemDefaultDiscover
static QString systemDefaultDiscover_win()
{
	//assign myself to the registry
	QSettings set("HKEY_LOCAL_MACHINE",QSettings::NativeFormat);
	//Try the Vista way, via the direct key into the registry
	set.beginGroup("SYSTEM/CurrentControlSet/Control/TimeZoneInformation/");
	if(set.contains("TimeZoneKeyName")){
		QString r=set.value("TimeZoneKeyName").toString();
//		qDebug()<<"vista thinks"<<r;
		if(r!="")return r;
	}
	//ok, its not Vista, so we search on
	//get the localized name
	QString tzl=set.value("StandardName").toString();
	set.endGroup();
	//did we actually get something?
	if(tzl.size()==0)return "";
//	qDebug()<<"localized"<<tzl;
	//now go through all time zones and compare
	set.beginGroup("SOFTWARE/Microsoft/Windows NT/CurrentVersion/Time Zones/");
	QStringList keys=set.childGroups();
	for(int i=0;i<keys.size();i++){
		if(set.value(keys[i]+"/Std")==tzl){
//			qDebug()<<"XP thinks"<<keys[i];
			return keys[i];
		}
	}
	//nothing found
	return "";
}

//mapping
static QString tzmap=
"Afghanistan|Asia/Kabul\n\
Afghanistan Standard Time|Asia/Kabul\n\
Alaskan|America/Anchorage\n\
Alaskan Standard Time|America/Anchorage\n\
Arab|Asia/Riyadh\n\
Arab Standard Time|Asia/Riyadh\n\
Arabian|Asia/Muscat\n\
Arabian Standard Time|Asia/Muscat\n\
Arabic Standard Time|Asia/Baghdad\n\
Atlantic|America/Halifax\n\
Atlantic Standard Time|America/Halifax\n\
AUS Central|Australia/Darwin\n\
AUS Central Standard Time|Australia/Darwin\n\
AUS Eastern|Australia/Sydney\n\
AUS Eastern Standard Time|Australia/Sydney\n\
Azerbaijan Standard Time|Asia/Baku\n\
Azores|Atlantic/Azores\n\
Azores Standard Time|Atlantic/Azores\n\
Bangkok|Asia/Bangkok\n\
Bangkok Standard Time|Asia/Bangkok\n\
Beijing|Asia/Shanghai\n\
Canada Central|America/Regina\n\
Canada Central Standard Time|America/Regina\n\
Cape Verde Standard Time|Atlantic/Cape_Verde\n\
Caucasus|Asia/Yerevan\n\
Caucasus Standard Time|Asia/Yerevan\n\
Cen. Australia|Australia/Adelaide\n\
Cen. Australia Standard Time|Australia/Adelaide\n\
Central|America/Chicago\n\
Central America Standard Time|America/Regina\n\
Central Asia|Asia/Dhaka\n\
Central Asia Standard Time|Asia/Dhaka\n\
Central Brazilian Standard Time|America/Manaus\n\
Central Europe|Europe/Prague\n\
Central Europe Standard Time|Europe/Prague\n\
Central European|Europe/Belgrade\n\
Central European Standard Time|Europe/Belgrade\n\
Central Pacific|Pacific/Guadalcanal\n\
Central Pacific Standard Time|Pacific/Guadalcanal\n\
Central Standard Time|America/Chicago\n\
Central Standard Time (Mexico)|America/Mexico_City\n\
China|Asia/Shanghai\n\
China Standard Time|Asia/Shanghai\n\
Dateline|GMT-1200\n\
Dateline Standard Time|GMT-1200\n\
E. Africa|Africa/Nairobi\n\
E. Africa Standard Time|Africa/Nairobi\n\
E. Australia|Australia/Brisbane\n\
E. Australia Standard Time|Australia/Brisbane\n\
E. Europe|Europe/Minsk\n\
E. Europe Standard Time|Europe/Minsk\n\
E. South America|America/Sao_Paulo\n\
E. South America Standard Time|America/Sao_Paulo\n\
Eastern|America/New_York\n\
Eastern Standard Time|America/New_York\n\
Egypt|Africa/Cairo\n\
Egypt Standard Time|Africa/Cairo\n\
Ekaterinburg|Asia/Yekaterinburg\n\
Ekaterinburg Standard Time|Asia/Yekaterinburg\n\
Fiji|Pacific/Fiji\n\
Fiji Standard Time|Pacific/Fiji\n\
FLE|Europe/Helsinki\n\
FLE Standard Time|Europe/Helsinki\n\
Georgian Standard Time|Asia/Tbilisi\n\
GFT|Europe/Athens\n\
GFT Standard Time|Europe/Athens\n\
GMT|Europe/London\n\
GMT Standard Time|Europe/London\n\
GMT Standard Time|GMT\n\
Greenland Standard Time|America/Godthab\n\
Greenwich|GMT\n\
Greenwich Standard Time|GMT\n\
GTB|Europe/Athens\n\
GTB Standard Time|Europe/Athens\n\
Hawaiian|Pacific/Honolulu\n\
Hawaiian Standard Time|Pacific/Honolulu\n\
India|Asia/Calcutta\n\
India Standard Time|Asia/Calcutta\n\
Iran|Asia/Tehran\n\
Iran Standard Time|Asia/Tehran\n\
Israel|Asia/Jerusalem\n\
Israel Standard Time|Asia/Jerusalem\n\
Jordan Standard Time|Asia/Amman\n\
Korea|Asia/Seoul\n\
Korea Standard Time|Asia/Seoul\n\
Mexico|America/Mexico_City\n\
Mexico Standard Time|America/Mexico_City\n\
Mexico Standard Time 2|America/Chihuahua\n\
Mid-Atlantic|Atlantic/South_Georgia\n\
Mid-Atlantic Standard Time|Atlantic/South_Georgia\n\
Middle East Standard Time|Asia/Beirut\n\
Mountain|America/Denver\n\
Mountain Standard Time|America/Denver\n\
Mountain Standard Time (Mexico)|America/Chihuahua\n\
Myanmar Standard Time|Asia/Rangoon\n\
N. Central Asia Standard Time|Asia/Novosibirsk\n\
Namibia Standard Time|Africa/Windhoek\n\
Nepal Standard Time|Asia/Katmandu\n\
New Zealand|Pacific/Auckland\n\
New Zealand Standard Time|Pacific/Auckland\n\
Newfoundland|America/St_Johns\n\
Newfoundland Standard Time|America/St_Johns\n\
North Asia East Standard Time|Asia/Ulaanbaatar\n\
North Asia Standard Time|Asia/Krasnoyarsk\n\
Pacific|America/Los_Angeles\n\
Pacific SA|America/Santiago\n\
Pacific SA Standard Time|America/Santiago\n\
Pacific Standard Time|America/Los_Angeles\n\
Pacific Standard Time (Mexico)|America/Tijuana\n\
Prague Bratislava|Europe/Prague\n\
Romance|Europe/Paris\n\
Romance Standard Time|Europe/Paris\n\
Russian|Europe/Moscow\n\
Russian Standard Time|Europe/Moscow\n\
SA Eastern|America/Buenos_Aires\n\
SA Eastern Standard Time|America/Buenos_Aires\n\
SA Pacific|America/Bogota\n\
SA Pacific Standard Time|America/Bogota\n\
SA Western|America/Caracas\n\
SA Western Standard Time|America/Caracas\n\
Samoa|Pacific/Apia\n\
Samoa Standard Time|Pacific/Apia\n\
Saudi Arabia|Asia/Riyadh\n\
Saudi Arabia Standard Time|Asia/Riyadh\n\
SE Asia|Asia/Bangkok\n\
SE Asia Standard Time|Asia/Bangkok\n\
Singapore|Asia/Singapore\n\
Singapore Standard Time|Asia/Singapore\n\
South Africa|Africa/Harare\n\
South Africa Standard Time|Africa/Harare\n\
Sri Lanka|Asia/Colombo\n\
Sri Lanka Standard Time|Asia/Colombo\n\
Sydney Standard Time|Australia/Sydney\n\
Taipei|Asia/Taipei\n\
Taipei Standard Time|Asia/Taipei\n\
Tasmania|Australia/Hobart\n\
Tasmania Standard Time|Australia/Hobart\n\
Tasmania Standard Time|Australia/Hobart\n\
Tokyo|Asia/Tokyo\n\
Tokyo Standard Time|Asia/Tokyo\n\
Tonga Standard Time|Pacific/Tongatapu\n\
US Eastern|America/Indianapolis\n\
US Eastern Standard Time|America/Indianapolis\n\
US Mountain|America/Phoenix\n\
US Mountain Standard Time|America/Phoenix\n\
Vladivostok|Asia/Vladivostok\n\
Vladivostok Standard Time|Asia/Vladivostok\n\
W. Australia|Australia/Perth\n\
W. Australia Standard Time|Australia/Perth\n\
W. Central Africa Standard Time|Africa/Luanda\n\
W. Europe|Europe/Berlin\n\
W. Europe Standard Time|Europe/Berlin\n\
Warsaw|Europe/Warsaw\n\
West Asia|Asia/Karachi\n\
West Asia Standard Time|Asia/Karachi\n\
West Pacific|Pacific/Guam\n\
West Pacific Standard Time|Pacific/Guam\n\
Western Brazilian Standard Time|America/Rio_Branco\n\
Yakutsk|Asia/Yakutsk\n\
Yakutsk Standard Time|Asia/Yakutsk";

//windows implementation of system default time zone discovery
// takes a windows time zone name (see above) and maps it to an Olson DB name
QString TimeZoneLib::systemDefaultDiscover()
{
	QString win=systemDefaultDiscover_win().trimmed();
//	qDebug()<<"system"<<win;
	//if none found, assume UTC+offset
	if(win=="")return "";
	//go through mapping
	QStringList sl=tzmap.split("\n");
//	qDebug()<<"having"<<sl.size()<<"options";
	for(int i=0;i<sl.size();i++){
		QStringList tz=sl[i].trimmed().split("|");
		if(tz.size()!=2)continue;
		if(win==tz[0].trimmed()){
//			qDebug()<<"found one"<<tz[1];
			return tz[1].trimmed();
		}
	}
	//none found again
	return "";
}

