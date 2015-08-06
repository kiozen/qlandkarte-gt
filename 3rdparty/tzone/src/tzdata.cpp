//
// C++ Implementation: TZ Data
//
// Description: Time Zone Data Calculator
//
//
// Author: Konrad Rosenbaum <konrad@silmor.de>, (C) 2008
//
// Copyright: See README/COPYING files that come with this distribution
//
//

#include <QSettings>
static inline void initResource(){Q_INIT_RESOURCE(zonefiles);}

#include "tzdata.h"
#include "tzfile.h"
#include "tzhelp.h"
#include "tzsys.h"
#include "tzreg.h"

using namespace TimeZoneLib;


//*********************************************************
// local registry

static QMap<QString,TZFile>registry;

static QRegExp zoneRegExp=QRegExp("[a-zA-Z0-9/_-]+");

static QString defaultzone;

static inline void initDefault()
{
	if(defaultzone==""){
		initResource();
		TimeStamp::resetRepository();
		TimeStamp::setDefaultZone("");
	}
}

TZFile& TimeZoneLib::getRegistryZone(QString zn)
{
	initDefault();
	//already known?
	if(registry.contains(zn))
		return registry[zn];
	//syntax check
	if(!zoneRegExp.exactMatch(zn))
		return registry["UTC"];
	//load it
	TZFile tf(zn);
	if(tf.isValid()){
		registry.insert(zn,tf);
		return registry[zn];
	}else
		return registry["UTC"];
}

//**********************************************************
// timestamp class

TimeStamp::TimeStamp()
{
	initDefault();
	m_unix=0;
	m_msec=0;
	setUTC();
}

TimeStamp::TimeStamp(qint64 ts, bool isLocal)
{
	initDefault();
	m_unix=ts;m_msec=0;
	if(isLocal)
		setZone(defaultzone);
	else
		setUTC();
}

TimeStamp::TimeStamp(qint64 ts, QString zone)
{
	initDefault();
	m_unix=ts;m_msec=0;
	setZone(zone);
}

TimeStamp::TimeStamp ( qint64 ts, quint16 msec, bool isLocal )
{
	initDefault();
	m_unix=ts+msec/1000;m_msec=msec%1000;
	if(isLocal)
		setZone(defaultzone);
	else
		setUTC();
}

TimeStamp::TimeStamp ( qint64 ts, quint16 msec, QString zone )
{
	initDefault();
	m_unix=ts+msec/1000;m_msec=msec%1000;
	setZone(zone);
}


TimeStamp::TimeStamp ( const QDate& dt, bool isLocal)
{
	initDefault();
	m_zone="UTC";
	m_year=dt.year();m_month=dt.month();m_day=dt.day();
	m_hour=m_min=m_sec=m_msec=0;
	if(isLocal)
		moveToZone(defaultzone);
	else
		moveToZone("UTC");
}

TimeStamp::TimeStamp ( const QDate& dt, QString zone)
{
	initDefault();
	m_zone="UTC";
	m_year=dt.year();m_month=dt.month();m_day=dt.day();
	m_hour=m_min=m_sec=m_msec=0;
	moveToZone(zone);
}

TimeStamp::TimeStamp ( const QTime& tm, bool isLocal )
{
	initDefault();
	m_zone="UTC";
	QDate dt=QDate::currentDate();
	m_year=dt.year();m_month=dt.month();m_day=dt.day();
	m_hour=tm.hour();m_min=tm.minute();m_sec=tm.second();m_msec=tm.msec();
	if(isLocal)
		moveToZone(defaultzone);
	else
		moveToZone("UTC");
}

TimeStamp::TimeStamp ( const QTime&tm , QString z)
{
	initDefault();
	m_zone="UTC";
	QDate dt=QDate::currentDate();
	m_year=dt.year();m_month=dt.month();m_day=dt.day();
	m_hour=tm.hour();m_min=tm.minute();m_sec=tm.second();m_msec=tm.msec();
	moveToZone(z);
}

TimeStamp::TimeStamp ( const QDateTime&d )
{
	initDefault();
	m_unix=d.toTime_t();m_msec=d.time().msec();
	if(d.timeSpec()!=Qt::UTC)
		setZone(defaultzone);
	else
		setUTC();
}


TimeStamp::TimeStamp ( const QDateTime& d, bool isLocal )
{
	initDefault();
	m_zone="UTC";
	QDate dt=d.date();
	QTime tm=d.time();
	m_year=dt.year();m_month=dt.month();m_day=dt.day();
	m_hour=tm.hour();m_min=tm.minute();m_sec=tm.second();m_msec=tm.msec();
	if(isLocal)
		moveToZone(defaultzone);
	else
		moveToZone("UTC");
}

TimeStamp::TimeStamp ( const QDateTime& d, QString z)
{
	initDefault();
	m_zone="UTC";
	QDate dt=d.date();
	QTime tm=d.time();
	m_year=dt.year();m_month=dt.month();m_day=dt.day();
	m_hour=tm.hour();m_min=tm.minute();m_sec=tm.second();m_msec=tm.msec();
	moveToZone(z);
}


TimeStamp::TimeStamp(const TimeStamp& t)
{
	operator=(t);
}
TimeStamp& TimeStamp::operator=(const TimeStamp& t)
{
	m_unix=t.m_unix;
	m_msec=t.m_msec;
	m_zone=t.m_zone;
	m_year=t.m_year;
	m_month=t.m_month;
	m_day=t.m_day;
	m_hour=t.m_hour;
	m_min=t.m_min;
	m_sec=t.m_sec;
	m_off=t.m_off;
	return *this;
}

QDateTime TimeStamp::toDateTime() const
{
	return QDateTime(toDate(),toTime(),m_zone=="UTC"?Qt::UTC:Qt::OffsetFromUTC);
}

QString TimeStamp::toISO() const
{
	//calculate zone offset
	QString z;
	if(m_off==0)z="Z";
	else if(m_off>0){
		int m=m_off/60;
		z=QString("+%1:%2")
		  .arg(m/60,2,10,QChar('0'))
		  .arg(m%60,2,10,QChar('0'));
	}else{
		int m=-(m_off/60);
		z=QString("-%1:%2")
		  .arg(m/60,2,10,QChar('0'))
		  .arg(m%60,2,10,QChar('0'));
	}
	//return final string
	return QString("%1-%2-%3T%4:%5:%6.%7%8")
		.arg(m_year) //%1 - year
		.arg(m_month,2,10,QChar('0')) //%2 - month
		.arg(m_day,2,10,QChar('0')) //%3 - day
		.arg(m_hour,2,10,QChar('0')) //%4 - hour
		.arg(m_min,2,10,QChar('0')) //%5 - minute
		.arg(m_sec,2,10,QChar('0')) //%6 - second
		.arg(m_msec,3,10,QChar('0')) //%7 - milli-second
		.arg(z) //%8 - zone info
	;
}


bool TimeStamp::setZone ( QString z)
{
	//get zone
	TZFile &zone=getRegistryZone(z);
	//get name
	m_zone=zone.name();
	//get offset
	TZRule r=zone.ruleForTime(m_unix);
	m_off=r.offsetFromUTC();
	//calculate members
	recalcToCached();
	//return success?
	return m_zone==z;
}

bool TimeStamp::moveToZone ( QString z)
{
	//get zone
	TZFile &zone=getRegistryZone(z);
	m_zone=zone.name();
	//get offset for local time
	m_unix=dateTime2stamp(m_year,m_month,m_day,m_hour,m_min,m_sec);
	TZRule r=zone.ruleForLocalTime(m_unix);
	m_off=r.offsetFromUTC();
	recalcToUnix();
	//return success?
	return z==m_zone;
}

void TimeStamp::recalcToCached()
{
	stamp2Time(m_unix+m_off,m_hour,m_min,m_sec);
	stamp2Date(m_unix+m_off,m_year,m_month,m_day);
}

void TimeStamp::recalcToUnix()
{
	m_unix=dateTime2stamp(m_year,m_month,m_day,m_hour,m_min,m_sec);
	m_unix-=m_off;
}

TimeStamp::WeekDay TimeStamp::weekDay() const
{
	return (WeekDay)weekDayOf(m_year,m_month,m_day);
}

void TimeStamp::addDays ( int d )
{
	addSecs(qint64(d)*SecondsPerDay);
}

void TimeStamp::addSecs ( int sec )
{
	m_unix+=sec;
	recalcToCached();
}

void TimeStamp::addMSecs ( int msec )
{
	qint64 tmp=m_msec;
	tmp+=msec;
	m_unix+=fdiv(tmp,1000);
	m_msec=pmod(tmp,1000);
	recalcToCached();
}

QString TimeStamp::zoneAbbreviation() const
{
	TZFile tf=getRegistryZone(m_zone);
	TZRule tr=tf.ruleForTime(m_unix);
	return tr.abbreviation();
}



//**************************************************************
// static members for registry access

QString TimeStamp::defaultZone()
{
	initDefault();
	return defaultzone;
}

bool TimeStamp::setDefaultZone(QString z)
{
	//is z empty?
	z=z.trimmed();
	if(z==""){
		//try to get my own default
		QSettings set;
		if(set.contains("timezone/default"))
			z=set.value("timezone/default").toString().trimmed();
	}
	//still empty: try to guess system default
	if(z==""){
		z=systemDefaultDiscover().trimmed();
	}
	//if still empty: get current diff from UTC
	if(z==""){
		//get local time
		QDateTime cur=QDateTime::currentDateTime();
		//get UTC
		QDateTime utc=cur.toUTC();
		//fake cur to UTC
		cur=QDateTime(cur.date(),cur.time(),Qt::UTC);
		//calc diff in minutes
		int d=utc.secsTo(cur)/60;
		//create name
		QString tname="UTC";
		if(d<0){tname+="-";d*=-1;}
		else tname+="+";
		tname+=QString("%1:%2").arg(d/60,2,10,QChar('0')).arg(d%60,2,10,QChar('0'));
		defaultzone=tname;
		//look it up
		if(registry.contains(tname))return true;
		//create it
		TZFile tzf(d);tzf.setName(tname);
		registry.insert(tname,tzf);
		return true;
	}
	//special case: UTC
	if(z=="UTC"){
		defaultzone="UTC";
		return true;
	}
	//get it from repo
	if(registry.contains(z)){
		defaultzone=z;
		return true;
	}
	//syntax check
	if(!zoneRegExp.exactMatch(z)){
		defaultzone="UTC";
		return false;
	}
	//try to load it
	TZFile tf(z);
	if(tf.isValid()){
		defaultzone=z;
		registry.insert(z,tf);
		return true;
	}
	//failed miserably
	defaultzone="UTC";
	return false;
}


void TimeStamp::storeDefaultZone()
{
	initDefault();
	QSettings().setValue("timezone/default",defaultzone);
}

void TimeStamp::forgetDefaultZone()
{
	QSettings().remove("timezone/default");
}

void TimeStamp::resetRepository()
{
	registry.clear();
	TZFile tzf;tzf.setName("UTC");
	registry.insert("UTC",tzf);
	setDefaultZone(defaultzone);
}

void TimeStamp::setSearchPath ( const QStringList& s)
{
	TZFile::setSearchPath(s);
}

bool TimeStamp::loadZone(QString zone)
{
	TZFile tf=getRegistryZone(zone);
	return tf.isValid();
}

QString TimeStamp::systemLocalZone()
{
	QString dz=defaultzone;
	setDefaultZone("");
	QString r=defaultzone;
	setDefaultZone(dz);
	return r;
}

TimeZoneLib::TZFile TimeStamp::zoneFile()const
{
	return getRegistryZone(m_zone);
}

TimeZoneLib::TZFile TimeStamp::defaultZoneFile()
{
	return getRegistryZone(defaultzone);
}

TimeZoneLib::TZFile TimeStamp::systemLocalZoneFile()
{
	return getRegistryZone(systemLocalZone());
}
