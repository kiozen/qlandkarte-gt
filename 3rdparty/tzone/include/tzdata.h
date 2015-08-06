//
// C++ Interface: TZ Data
//
// Description: Time Zone Calculator
//
//
// Author: Konrad Rosenbaum <konrad@silmor.de>, (C) 2010
//
// Copyright: See README/COPYING files that come with this distribution
//
//

#ifndef TIME_ZONE_DATA_H
#define TIME_ZONE_DATA_H

#include <QDate>
#include <QDateTime>
#include <QString>
#include <QTime>

namespace TimeZoneLib {class TZFile;}

/**represents the TZ's idea of a time stamp - it uses time zone names in the Olsen notation (eg. "Europe/Berlin")

This class always calculates according to the Gregorian calendar, even for dates preceding its introduction (1582 in most western regions, even after 1900 in some localities).

Valid time zones are strings of the form "Region/City", these are mapped to file names relative to the repository. Zones are checked for invalid characters before being accepted, invalid zones are generally replaced by UTC.

Warning: at the moment this class is not thread safe!*/
class TimeStamp
{
	public:
		/**numerical constants for week days*/
		enum WeekDay{
			Sunday=0,
			Monday=1,
			Tuesday=2,
			Wednesday=3,
			Thursday=4,
			Friday=5,
			Saturday=6
		};

		/**creates a zero timestamp (it matches Epoch and resides on the UTC zone)*/
		TimeStamp();
		
		/**creates a timestamp from a unix time
		\param ts the unix timestamp to use
		\param isLocal whether the default time zone should be used, true per default*/
		TimeStamp(qint64 ts,bool isLocal=true);
		/**creates a timestamp from a unix time
		\param ts the unix timestamp to use
		\param zone the zone it will reside on initially, if the zone is not found it will use UTC instead*/
		TimeStamp(qint64 ts,QString zone);
		/**creates a timestamp from a unix time
		\param ts the unix timestamp to use
		\param msec the milli-seconds beyond that timestamp
		\param isLocal whether the default time zone should be used, true per default*/
		TimeStamp(qint64 ts,quint16 msec,bool isLocal=true);
		/**creates a timestamp from a unix time
		\param ts the unix timestamp to use
		\param msec the milli-seconds beyond that timestamp
		\param zone the zone it will reside on initially, if the zone is not found it will use UTC instead*/
		TimeStamp(qint64 ts,quint16 msec,QString zone);
		
		/**creates a timestamp from a QDate, the internal time is set to 0:00:00.000, per default assumes local time*/
		TimeStamp(const QDate&,bool isLocal=true);
		/**creates a timestamp from a QDate assuming it to represent midnight on the given time zone*/
		TimeStamp(const QDate&,QString);
		
		/**creates a timestamp from a QTime, using today as date, per default assumes that the time is local - if the default time zone differs from the system time zone this may lead to misinterpretations*/
		TimeStamp(const QTime&,bool isLocal=true);
		/**creates a timestamp from QTime, using today on the given time zone as date, the timestamp is local to the given time zone*/
		TimeStamp(const QTime&,QString);
		
		/**creates a timestamp from QDateTime, it converts the QDateTime object to UTC before assigning it, most QDateTime objects created without specifying an explicit time zone are assigned local time by Qt; if the timestamp was of local time the timestamp then moves to the default zone, this may or may not match the zone set by the system, so the resulting timestamp may show time for a different zone
		\param ts timestamp to be converted*/
		TimeStamp(const QDateTime&ts);
		/**creates a timestamp from QDateTime, it ignores the settings of the ts object and uses isLocal to convert it into a UTC based timestamp
		\param ts timestamp to be converted
		\param isLocal if set to true the timestamp is interpreted to be of the default time zone*/
		TimeStamp(const QDateTime&ts,bool isLocal);
		/**creates a timestamp from QDateTime, the settings of the ts object are ignored and it is interpreted to be in the given time zone
		\param ts timestamp to be converted
		\param zone the time zone this QDateTime is interpreted to represent, if it does not exist it is interpreted to be UTC*/
		TimeStamp(const QDateTime&ts,QString zone);
		
		/**copies a timestamp*/
		TimeStamp(const TimeStamp&);
		/**copies a timestamp*/
		TimeStamp& operator=(const TimeStamp&);
		
		/**sets the timezone that this object represents, returns true on success, false if the timezone was not found - in the latter case it reverts to UTC
		
		the time the object represents will correspond to the same time in UTC before and after the conversion*/
		bool setZone(QString);
		
		/**sets the timezone of this object to the default time zone; the object will still reprsent the same point in time, but expressed relative to the default time zone; this is a shortcut for setZone(defaultZone()) */
		void setLocal(){setZone(defaultZone());}
		
		/**resets the object to UTC; it will still represent the same point in time, but expressed relative to UTC; this is a shortcut for setZone("UTC") */
		void setUTC(){setZone("UTC");}
		
		/**moves the timezone that this object represents, returns true on success, false if the timezone was not found - in the latter case it moves to UTC
		
		This method preserves the time as numerical values, so if the new zone has a different offset from UTC the object will represent a different point in time after the conversion. For example, if the time stamp is currently set to UTC "2010-07-06 12:11:22.000 +00:00" and is moved to "Europe/Berlin" it will now represent "2010-07-06 12:11:22.000 +02:00" or an actualy time two hours earlier.*/
		bool moveToZone(QString);
		
		/**move the timezone of this object to UTC; this is a shortcut for moveToZone("UTC") */
		void moveToUTC(){moveToZone("UTC");}
		
		/**move the timezone of this object to the default timezone; this is a shortcut for moveToZone(defaultZone()) */
		void moveToLocal(){moveToZone(defaultZone());}
		
		/**returns the currently set time zone or "UTC" if it has none*/
		QString zone()const{return m_zone;}
		
		/**returns an object that represents the same timestamp in UTC*/
		TimeStamp toUTC()const{return TimeStamp(m_unix,m_msec,false);}
		
		/**returns an object that represents the same timestamp in the default time zone*/
		TimeStamp toLocal()const{return TimeStamp(m_unix,m_msec,true);}
		
		/**returns an object that represents the same timestamp in the specified time zone, or UTC if the time zone is not valid*/
		TimeStamp toZone(QString zone)const{return TimeStamp(m_unix,zone);}
		
		/**converts the timestamp to QDate according to its own timezone*/
		QDate toDate()const{return QDate(m_year,m_month,m_day);}
		/**converts the timestamp to QDate according to its own timezone*/
		operator QDate()const{return QDate(m_year,m_month,m_day);}
		
		/**converts the timestamp to QTime according to its own timezone*/
		QTime toTime()const{return QTime(m_hour,m_min,m_sec,m_msec);}
		/**converts the timestamp to QTime according to its own timezone*/
		operator QTime()const{return QTime(m_hour,m_min,m_sec,m_msec);}
		
		/**converts the timestamp to QDateTime according to its own timezone, if the internal timezone is not UTC it will mark the resulting QDateTime as Qt::OffsetFromUTC unfortunately Qt has no way of storing the exact offset, so you have to remember it yourself*/
		QDateTime toDateTime()const;
		/**converts the timestamp to QDateTime according to its own timezone, if the internal timezone is not UTC it will mark the resulting QDateTime as Qt::OffsetFromUTC unfortunately Qt has no way of storing the exact offset, so you have to remember it yourself*/
		operator QDateTime()const{return toDateTime();}
		
		/**converts this timestamp to a QDateTime that uses UTC*/
		QDateTime toSystemDateTimeUTC()const{return toUTC().toDateTime();}
		/**converts this timestamp to a QDateTime that uses the system local time; this is done by converting to UTC first and then letting the operating system convert to its own zone*/
		QDateTime toSystemDateTime()const{return toSystemDateTimeUTC().toLocalTime();}
		
		/**adds msec milli-seconds to this timestamp*/
		void addMSecs(int msec);
		
		/**adds sec seconds to this timestamp*/
		void addSecs(int sec);
		
		/**adds d days to this timestamp*/
		void addDays(int d);
		
		/**returns the unix timestamp*/
		qint64 toUnix()const{return m_unix;}
		
		/**returns the year of this timestamp according to its own zone*/
		qint16 year()const{return m_year;}
		/**returns the month of this timestamp according to its own zone*/
		quint8 month()const{return m_month;}
		/**returns the day of month of this timestamp according to its own zone*/
		quint8 day()const{return m_day;}
		/**returns the hour of this timestamp according to its own zone*/
		quint8 hour()const{return m_hour;}
		/**returns the minute of this timestamp according to its own zone*/
		quint8 minute()const{return m_min;}
		/**returns the second of this timestamp according to its own zone*/
		quint8 second()const{return m_sec;}
		/**returns the milli-second of this timestamp*/
		quint16 msecs()const{return m_msec;}
		
		/**returns the day of the week*/
		WeekDay weekDay()const;
		
		/**returns the abbreviated time zone name (not guaranteed to be globally unique)*/
		QString zoneAbbreviation()const;
		
		/**returns the time stamp as ISO date/time string*/
		QString toISO()const;
		
		
		/**returns the offset from UTC of this timestamp, positive values are east of UTC*/
		qint16 offsetFromUTC()const{return m_off;}
		
		/**returns true if those two timestamps represent the same time, regardless of time zone, eg. UTC 10:00 will be regarded as equivalent to 12:00 UTC+02:00*/
		bool operator==(const TimeStamp&ts)const
		 {return m_unix==ts.m_unix && m_msec==ts.m_msec;}
		/**returns true if those two timestamps represent differnt times, regardless of time zone, eg. UTC 10:00 will be regarded as different from to 10:00 UTC+02:00*/
		bool operator!=(const TimeStamp&ts)const
		 {return m_unix!=ts.m_unix || m_msec!=ts.m_msec;}
		
		/**returns true if this timestamp represents an earlier time than the argument*/
		bool operator<(const TimeStamp&ts)const
		 {return m_unix<ts.m_unix || (m_unix==ts.m_unix && m_msec<ts.m_msec);}
		/**returns true if this timestamp is earlier or equivalent to the argument*/
		bool operator<=(const TimeStamp&ts)const
		 {return m_unix<ts.m_unix || (m_unix==ts.m_unix && m_msec<=ts.m_msec);}

		/**returns true if this timestamp represents a later time than the argument*/
		bool operator>(const TimeStamp&ts)const
		 {return m_unix>ts.m_unix || (m_unix==ts.m_unix && m_msec>ts.m_msec);}
		/**returns true if this timestamp is later or equivalent to the argument*/
		bool operator>=(const TimeStamp&ts)const
		 {return m_unix>ts.m_unix || (m_unix==ts.m_unix && m_msec>=ts.m_msec);}
		
		// /////////////////////////////////////////////////////////////
		// statics
		/**convenience function: constructs a timestamp from a specific date, sets the time to 0:00:00*/
		static TimeStamp fromDate(int year,quint8 month,quint8 day,bool isLocal=true)
		 {return TimeStamp(QDate(year,month,day),isLocal);}
		/**convenience function: constructs a timestamp from a specific date, sets the time to 0:00:00*/
		static TimeStamp fromDate(QDate dt,bool isLocal=true)
		 {return TimeStamp(dt,isLocal);}
		/**convenience function: constructs a timestamp from a specific date, sets the time to 0:00:00*/
		static TimeStamp fromDate(int year,quint8 month,quint8 day,QString zone)
		 {return TimeStamp(QDate(year,month,day),zone);}
		/**convenience function: constructs a timestamp from a specific date, sets the time to 0:00:00*/
		static TimeStamp fromDate(QDate dt,QString zone)
		 {return TimeStamp(dt,zone);}
		
		/**convenience function: constructs a timestamp from a specific local time of today*/
		static TimeStamp fromTime(quint8 hour,quint8 minute,bool isLocal=true)
		 {return TimeStamp(QTime(hour,minute),isLocal);}
		/**convenience function: constructs a timestamp from a specific local time of today*/
		static TimeStamp fromTime(quint8 hour,quint8 minute,quint8 second,bool isLocal=true)
		 {return TimeStamp(QTime(hour,minute,second),isLocal);}
		/**convenience function: constructs a timestamp from a specific local time of today*/
		static TimeStamp fromTime(quint8 hour,quint8 minute,QString zone)
		 {return TimeStamp(QTime(hour,minute),zone);}
		/**convenience function: constructs a timestamp from a specific local time of today*/
		static TimeStamp fromTime(quint8 hour,quint8 minute,quint8 second,QString zone)
		 {return TimeStamp(QTime(hour,minute,second),zone);}
		/**convenience function: constructs a timestamp from a specific local time of today*/
		static TimeStamp fromTime(QTime tm,bool isLocal=true)
		 {return TimeStamp(tm,isLocal);}
		/**convenience function: constructs a timestamp from a specific local time of today*/
		static TimeStamp fromTime(QTime tm,QString zone)
		 {return TimeStamp(tm,zone);}

		/**convenience function: constructs a timestamp from a specific local date and time*/
		static TimeStamp fromDateTime(QDateTime dt)
		 {return TimeStamp(dt);}
		/**convenience function: constructs a timestamp from a specific local date and time*/
		static TimeStamp fromDateTime(QDateTime dt,bool isLocal)
		 {return TimeStamp(dt,isLocal);}
		/**convenience function: constructs a timestamp from a specific local date and time*/
		static TimeStamp fromDateTime(QDateTime dt,QString zone)
		 {return TimeStamp(dt,zone);}

		/**convenience function: constructs a timestamp from a specific local date and time*/
		static TimeStamp fromDateTime(QDate dt,QTime tm)
		 {return TimeStamp(QDateTime(dt,tm));}
		/**convenience function: constructs a timestamp from a specific local date and time*/
		static TimeStamp fromDateTime(QDate dt,QTime tm,bool isLocal)
		 {return TimeStamp(QDateTime(dt,tm),isLocal);}
		/**convenience function: constructs a timestamp from a specific local date and time*/
		static TimeStamp fromDateTime(QDate dt,QTime tm,QString zone)
		 {return TimeStamp(QDateTime(dt,tm),zone);}

		/**convenience function: constructs a timestamp from a specific local date and time*/
		static TimeStamp fromDateTime(int year,quint8 month,quint8 day,quint8 hour,quint8 minute,bool isLocal=true)
		 {return fromDateTime(QDate(year,month,day),QTime(hour,minute),isLocal);}
		/**convenience function: constructs a timestamp from a specific local date and time*/
		static TimeStamp fromDateTime(int year,quint8 month,quint8 day,quint8 hour,quint8 minute,quint8 second,bool isLocal=true)
		 {return fromDateTime(QDate(year,month,day),QTime(hour,minute,second),isLocal);}

		/**returns the current date/time as localized time stamps
		\param zone the name of the target time zone*/
		static TimeStamp now(QString zone){return TimeStamp(QDateTime::currentDateTime().toTime_t(),zone);}
		/**returns the current date/time as time stamp
		\param isLocal if true the timestamp will be in the default time zone, if false on UTC*/
		static TimeStamp now(bool isLocal=true){return TimeStamp(QDateTime::currentDateTime().toTime_t(),isLocal);}
		
		/**sets the global search repository for time zones, this only affects time zones that have not been loaded yet*/
		static void setSearchPath(const QStringList&);
		
		/**resets the internal time zone repository, this forces all time zones to reload, it also tries to reset the default zone - if you changed the search path this may mean that the default zone is reset to UTC*/
		static void resetRepository();
		
		/**sets the default time zone - all objects that are created as "local" are regarded to be in this time zone, initially the system tries to guess what the system local time zone is; if setting fails the default will be UTC afterwards
		\returns true if the new zone is valid, otherwise resets the local zone to UTC
		\param zone the Olsen name of the local zone, "UTC" will set the local zone to UTC, an empty string will try to guess the system default*/
		static bool setDefaultZone(QString zone);
		
		/**returns the name of the current default time zone; returns "UTC" as a fallback*/
		static QString defaultZone();
		
		/**stores the current default time zone in the apps configuration (via QSettings), so this is always used instead of guessing*/
		static void storeDefaultZone();
		
		/**makes the app forget its default configuration (next time setDefaultZone is called with an empty string it will guess again)*/
		static void forgetDefaultZone();
		
		/**Tries to load the given zone, returns true on success. Other than loading it this method does not do anything with the loaded zone, it just checks that it is available to time stamps.*/
		static bool loadZone(QString zone);
		
		/**Tries to find the computers local time zone and returns it*/
		static QString systemLocalZone();
		
		///returns an instance of this time stamp's current zone file (or one that represents just the UTC-offset if there is none)
		TimeZoneLib::TZFile zoneFile()const;
		
		///returns a copy of the default zone file
		static TimeZoneLib::TZFile defaultZoneFile();
		
		///returns a copy of the system local zone file
		static TimeZoneLib::TZFile systemLocalZoneFile();
		
	private:
		//the main measure of time: the unix timestamp
		qint64 m_unix;
		//milliseconds beyond that time
		quint16 m_msec;
		//the time zone
		QString m_zone;
		//cache values to make calculations easier
		qint16 m_year;
                qint32 m_off;
		quint8 m_month,m_day,m_hour,m_min,m_sec;
		
		/** \internal helper function that converts unix+offset to cached values*/
		void recalcToCached();
		
		/** \internal helper function that converts the cached values plus offset back to a unix timestamp*/
		void recalcToUnix();
};



#endif
