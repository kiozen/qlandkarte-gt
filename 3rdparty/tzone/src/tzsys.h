//
// C++ Interface: TZ Data System Discovery
//
// Description: Time Zone System Discovery
//
//
// Author: Konrad Rosenbaum <konrad@silmor.de>, (C) 2010
//
// Copyright: See README/COPYING files that come with this distribution
//
//

#ifndef TIMEZONE_SYSTEM_DISCOVERY_H
#define TIMEZONE_SYSTEM_DISCOVERY_H

#include <QString>

namespace TimeZoneLib {
	/** \internal detect the default time zone of the system, the implementation differs between systems; implemented in tzsys_*.cpp */
	QString systemDefaultDiscover();
	
	/// \internal set the default system zone file, implemented in tzfile.cpp
	void setSystemZoneFile(QString);
}

#endif
