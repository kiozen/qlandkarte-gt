//
// C++ Interface: TZ Registry
//
// Description: Time Zone Registry
//
//
// Author: Konrad Rosenbaum <konrad@silmor.de>, (C) 2010
//
// Copyright: See README/COPYING files that come with this distribution
//
//

#ifndef TIME_ZONE_REGISTRY_H
#define TIME_ZONE_REGISTRY_H

#include <QString>

namespace TimeZoneLib {

class TZFile;
/** \internal registry access for unit testing only!! */
TZFile& getRegistryZone(QString zn);

};

#endif
