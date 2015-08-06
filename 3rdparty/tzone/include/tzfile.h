//
// C++ Interface: TZ file loader
//
// Description: internal interface to time zones
//
//
// Author: Konrad Rosenbaum <konrad@silmor.de>, (C) 2008
//
// Copyright: See README/COPYING files that come with this distribution
//
//


#ifndef TZFILE_H
#define TZFILE_H

#include <QStringList>

class QByteArray;
class QIODevice;

namespace TimeZoneLib {

/** \internal represents a single transition rule in the TZ system*/
class TZRule
{
	public:
		/**creates an invalid rule, all requests to it are handled as if they were for the UTC zone*/
		TZRule();
		/**copies a rule*/
		TZRule(const TZRule&);
		
		/**copies a rule*/
		TZRule& operator=(const TZRule&);
		
		/**returns the time stamp at which this rule becomes active*/
		qint64 startTime()const{return m_start;}
		/**returns the time stamp at which this rule becomes inactive*/
		qint64 endTime()const{return m_end;}
		/**returns the abbreviated time zone name of this rule (eg. MET, or MEST)*/
		QString abbreviation()const{return m_abbr;}
		/**returns true if this rule refers to daylight saving time*/
		bool isDST()const{return m_isdst;}
		/**returns the offset of this rule towards UTC in seconds, positive values are east*/
		qint32 offsetFromUTC()const{return m_off;}
		
		/**returns true if the given time stamp matches the validity period of this rule*/
		bool matches(qint64 ts)const{return m_start<=ts && m_end>ts;}
		
		/**returns true if the given time stamp matches the validity period of this rule moved by its internal offset*/
		bool matchesLocal(qint64 ts)const{return matches(ts-m_off);}
		
		/** \internal used for sorting rules, returns true if this rules start time is before the start time or r*/
		bool operator<(const TZRule&r)const{return m_start<r.m_start;}
		/** \internal used for sorting rules, returns true if this rules start time is before the start time or r*/
		bool operator<=(const TZRule&r)const{return m_start<=r.m_start;}
		
	private:
		friend class TZFile;
		friend class PosixRule;
		/** \internal used by TZFile and PosixRule to instantiate rules*/
		TZRule(qint64 start,qint32 off,QString abbr,bool isdst);
		/** \internal used by TZFile and PosixRule to set the end time of rules*/
		void setEnd(qint64 e){m_end=e;}
		
		qint64 m_start,m_end;
		qint32 m_off;
		QString m_abbr;
		bool m_isdst;
};

/** \internal represents a POSIX style time calculation rule, it is used as intermediary to calculate simpler conversion rules*/
class PosixRule
{
	public:
		/**instantiates an empty ruleset, always returns a UTC/empty rule*/
		PosixRule();
		/**copies a ruleset*/
		PosixRule(const PosixRule&);
		/** \internal used by TZFile to instantiate rulesets */
		PosixRule(QString);
		
		/**copies a ruleset*/
		PosixRule& operator=(const PosixRule&);
		
		/**returns the posix string of this ruleset*/
		QString asString()const{return m_str;}
		
		/**returns all rules that match the given year*/
		QList<TZRule> rulesForYear(int)const;
		
		/**takes the given rule and calculates the one that would follow it*/
		TZRule nextRule(const TZRule&)const;
		
		/**calculates and returns a rule for a specific timestamp*/
		TZRule ruleForTime(qint64)const;
		
		/**calculates and returns a rule for a specific timestamp moved by the prospective offset*/
		TZRule ruleForLocalTime(qint64)const;
		
		/** \internal how the ruleset calculates start and end times of rules*/
		enum StartType{
			/**no calculation, return a rule with a simple offset*/
			StartNone,
			/**a specific day of year, start counting at zero, feb 29th counts if present*/
			StartDay0,
			/**a specific day of year, start counting at one, feb 29th does not count if present*/
			StartDay1,
			/**start at a specifc week day in a specific week of a specific month*/
			StartWeek
		};
		
		/**returns the name of standard time*/
		QString standardName()const{return m_abbr;}
		
		/**returns the name of daylight saving time*/
		QString daylightName()const{return m_abbrdst;}
		
		/**returns the offset from UTC in seconds for standard time*/
		qint32 standardOffset()const{return m_off;}
		
		/**returns the offset from UTC in seconds for daylight saving time*/
		qint32 daylightOffset()const{return m_offdst;}
		
		/**returns true of this rule has daylight saving time*/
		bool haveDST()const{return m_abbr!=m_abbrdst;}
		
	private:
		
		QString m_str,m_abbr,m_abbrdst;
		qint32 m_off,m_offdst;
		StartType m_types,m_typee;
		qint32 m_months,m_weeks,m_days,m_times;
		qint32 m_monthe,m_weeke,m_daye,m_timee;
		
		/** \internal create specific rule, does not create the end time of the rule*/
		TZRule createRule(qint32 year,bool start)const;
};

/** \internal represents a time zone file with its internal rules*/
class TZFile
{
	public:
		/**constructs a time zone file which handles UTC as local time zone*/
		TZFile();
		/**tries to find the zone tz in the search path, tz is always regarded as a relative file name*/
		TZFile(QString tz);
		/**tries to find the zone tz in the search path, tz is always regarded as a relative file name*/
		TZFile(const char* tz);
		/**loads the time zone from the given device*/
		TZFile(QIODevice&);
		/**loads the time zone from the given byte array, the array must contain a complete time zone file*/
		TZFile(const QByteArray&);
		/**copies a zone file object*/
		TZFile(const TZFile&);
		/**does not actually load a file, but constructs a virtual file that corresponds to a specific offset east of UTC in minutes*/
		TZFile(int offset);
		
		TZFile& operator=(const TZFile&);
		
		/**returns whether this is a valid time zone file, invalid files handle all requests as though they were for the UTC zone*/
		bool isValid()const{return m_valid;}
		
		/**returns the zone name, this may be empty if the zone was loaded from an already open device or an byte array*/
		QString name()const{return m_name;}
		
		/**overrides the zone name; this has no effect one the zones contents and behavior*/
		void setName(QString n){m_name=n;}
		
		/**returns a rule that matches a specific UTC based time*/
		TZRule ruleForTime(qint64);
		
		/**returns a rule that matches a specific local time, the timestamp given is a Unix timestamp plus the offset of the rule that will match; in other words it was calculated assuming UTC and now looks for a rule that will provide a correcting offset*/
		TZRule ruleForLocalTime(qint64);
		
		/**returns the list of rules that were encoded in the file*/
		QList<TZRule> fileRules()const{return m_file;}
		
		/**returns the fallback POSIX rule that was encoded in the file (or an invalid rule if there was none)*/
		PosixRule posixRule()const{return m_posix;}
		
		/**returns all currently cached rules that were auto-generated during the lifetime of this file object*/
		QList<TZRule> cachedRules()const{return m_cache;}
		
		///returns the full file name of the zone file, if known
		QString fileName()const{return m_fname;}
		
		///returns the DB directory name only
		QString dirName()const;
		
		///tries to determine version info for this zone file
		QString version()const;
		
		///returns the built-in version of the Olson-DB
		static QString builtinVersion();
		
		///returns the library version
		static QString libraryVersion();
		
		/**returns the search paths for time zone files*/
		static QStringList searchPath();
		/**sets new search paths for time zone files*/
		static void setSearchPath(const QStringList&);
	private:
		/** \internal helper for constructor taking string*/
		void strConstruct(QString);
		/** \internal helper to load time zones (the complete file)*/
		bool load(const QByteArray&);
		/** \internal helper to load time zones (one section - 32 or 64 bit)*/
		int load(const QByteArray&,char);
		
		bool m_valid;
		QList<TZRule>m_file;
		QList<TZRule>m_cache;
		PosixRule m_posix;
		QString m_name,m_fname;
		
		static QStringList m_search;
};

//end of namespace
};

#endif