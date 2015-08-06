//
// C++ Implementation: TZ discovery for Unixoid systems
//
// Description: 
//
//
// Author: Konrad Rosenbaum <konrad@silmor.de>, (C) 2008
//
// Copyright: See README/COPYING files that come with this distribution
//
//

#include "tzsys.h"

#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include <QStringList>
#include <QFile>
#include <QProcess>
#include <QFileInfo>


using namespace TimeZoneLib;

///list of files that contain an Olson time zone key
static QStringList efiles=QStringList()
	<<"/etc/timezone" //default for systems that have Olson
	<<"/etc/TIMEZONE" //Solaris #-(
	<<"/usr/local/etc/timezone" //Olson independent install
	<<"/usr/share/zoneinfo/timezone" //unusual system location (e.g. Mac)
	<<"/usr/lib/zoneinfo/timezone" //another unusual one
; // end of list

///list of files that contain either a symlink to or a copy of the binary time zone definition
static QStringList tfiles=QStringList()
	<<"/etc/localtime" //Linux, Mac, etc.
	<<"/usr/local/etc/localtime" //independent install
	<<"/usr/share/zoneinfo/localtime" //slightly unusual location
	<<"/usr/lib/zoneinfo/localtime" //Solaris
; // end of list

QString TimeZoneLib::systemDefaultDiscover()
{
	//do we have a TZ variable?
	const char*tze=getenv("TZ");
	if(tze!=0){
		//The TZ variable can contain the following formats:
		// "Europe/Berlin" (Olson, nicely formatted)
		// ":Europe/Berlin" (probably Olson, as per POSIX std.)
		// "UTC+2" (shortcut, Olson -> "Etc/UTC+2")
		// "XXX2" (simple POSIX)
		// "XXX2YYY3" (complex POSIX)
		// TODO: we need to cover these
		//   POSIX should be "system/POSIX/...rule..." or something
		QString r=QString::fromLocal8Bit(tze).trimmed();
		//remove leading colon
		if(r.size()>0 && r[0]==':')r=r.mid(1);
		//TODO: check the file exists
		return r;
	}
	//search list of environment files
	for(int i=0;i<efiles.size();i++){
		QFile fd(efiles[i]);
		if(fd.open(QIODevice::ReadOnly)){
			return QString::fromLocal8Bit(fd.read(256).data()).trimmed();
			//TODO: check the zone is known
		}
	}
	//check for /etc/localtime
	for(int i=0;i<tfiles.size();i++){
		QFileInfo fi(tfiles[i]);
		if(!fi.isFile() && !fi.isReadable())continue;
		setSystemZoneFile(tfiles[i]);
		char buf[1024];
		// if symlink: read, parse, return
		if(readlink(tfiles[i].toLocal8Bit().data(),buf,sizeof(buf))>0){
			QStringList r=QString::fromLocal8Bit(buf).split('/');
			//eliminate path leading up to the file
			while(r.size()>0){
				QString c=r.takeFirst();
				if(c=="zoneinfo")break;
			}
			if(r.size()==0)continue;
			//reconstruct zone name
			QString z=r.join("/");
			//TODO: check the zone exists
			return z;
		}
		// if file: return "system/localtime"
		else {
			return "system/localtime";
		}
	}
#ifdef Q_OS_ANDROID
	//special handling for Android
	QProcess getprop;
	getprop.start("getprop persist.sys.timezone");
	if(getprop.waitForFinished(5000)){
		QString r=getprop.readAllStandardOutput().trimmed();
		if(!r.isEmpty())return r;
	}
#endif
	//fall-back
	struct timezone tz;
	struct timeval tv;
	if(gettimeofday(&tv,&tz)==0){
		if(tz.tz_minuteswest==0)return "UTC";
		if((tz.tz_minuteswest%60)==0){
			const bool east=tz.tz_minuteswest<0;
			if(east)tz.tz_minuteswest*=-1;
			return QString("GMT%1%2").arg(east?"-":"+").arg(tz.tz_minuteswest/60);
		}
	}
	//nothing, the timezone is odd or indeterminable
	// TODO: "system/native"?
	return "";
}