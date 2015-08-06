//
// C++ Interface: TZ Data Helper functions
//
// Description: some calculator inlines
//
//
// Author: Konrad Rosenbaum <konrad@silmor.de>, (C) 2010
//
// Copyright: See README/COPYING files that come with this distribution
//
//

#ifndef TIMEZONE_INLINE_HELPERS_H
#define TIMEZONE_INLINE_HELPERS_H

#include <QtGlobal>
#include <QByteArray>

namespace TimeZoneLib {

/**the amount of seconds per Day*/
static const qint64 SecondsPerDay=86400;

/** decodes a signed big endian integer from a byte array, per default it decodes a 32bit int
\param a the array, it must be at least s bytes long
\param s the length of the int in bytes*/
inline qint64 decodeInt(const QByteArray&a,int s=4)
{
	if(a.size()<s)return 0;
	qint64 r=0;
        if(((signed char)a[0])<0)r=-1;
	for(int i=0;i<s;i++){
		r<<=8;
		r|=(unsigned char)a[i];
	}
	return r;
}

/**convenience function: decodes a 64bit integer from an array, see decodeInt */
inline qint64 decodeInt64(const QByteArray&a){return decodeInt(a,8);}

/**returns true if the given year is a leap year according to the Gregorian calendar*/
inline bool isLeapYear(int year)
{
	if((year%400)==0)return true;//every 400th is leapy
	if((year%100)==0)return false;//every 100th is not
	if((year%4)==0)return true;//every fourth is leapy
	return false;//all others are not leapy
}

/**returns how many days have passed since Epoch to this day
\returns 0 for 1/1/1970, positive values after Epoch, negative values befor Epoch
\param year the year to be calculated
\param month the month to be calculates, must be between 1 and 12, assumes january if none given
\param day the day of the month to be calculated, must be between 1 and 31, assumes the 1st if none given*/
inline qint64 daysSinceEpoch(int year,int month=1,int day=1)
{
	qint64 ret=0;
	//calculate jan 1st
	if(year>=1970){
		for(int y=1970;y<year;y++)
			if(isLeapYear(y))ret+=366;
			else ret+=365;
	}else{
		for(int y=1969;y>=year;y--)
			if(isLeapYear(y))ret-=366;
			else ret-=365;
	}
	//calculate 1st of month
	// for each month: add the days of the previous one, and the previous, ...
	switch(month){
		case 12:ret+=30;//nov
		case 11:ret+=31;//oct
		case 10:ret+=30;//sep
		case 9:ret+=31;//aug
		case 8:ret+=31;//jul
		case 7:ret+=30;//jun
		case 6:ret+=31;//may
		case 5:ret+=30;//apr
		case 4:ret+=31;//mar
		case 3:ret+=28;if(isLeapYear(year))ret++;//feb
		case 2:ret+=31;//jan
		default:break;
	}
	//add day
	ret+=day-1;
	
	return ret;
}

/** \internal returns a positive modulo*/
inline int pmod(qint64 num,int mod)
{
	num%=mod;
	while(num<0)num+=mod;
	return num;
}

/** \internal floor division: it always rounds to the next lower number if there is a remainder, this makes it behave just like normal division for positive numbers, but differently for negative numbers*/
inline qint64 fdiv(qint64 num,qint64 div)
{
	if(div<0){
		div*=-1;num*=-1;
	}
	if(num>0)return num/div;
	qint64 m=num%div;
	num/=div;if(m)num--;
	return num;
}

/**returns the week day of a specific date
\returns 0 for sunday, 1 for monday, ...etc., 6 for saturday
\param year the year to be calculated
\param month the month to be calculates, must be between 1 and 12, assumes january if none given
\param day the day of the month to be calculated, must be between 1 and 31, assumes the 1st if none given*/
inline int weekDayOf(int year,int month=1,int day=1)
{
	return pmod(daysSinceEpoch(year,month,day)+4,7);
}

/**returns the week day of a specific day offset from Epoch
\returns 0 for sunday, 1 for monday, ...etc., 6 for saturday*/
inline int weekDaySinceEpoch(qint64 daySinceEpoch)
{
	return pmod(daySinceEpoch+4,7);
}

/**returns the first day in a month that is a specific day of the week, 0 and 7 are sunday 
\returns offset in days from Epoch*/
inline qint64 firstWeekDayOf(int year,int month,int wday)
{
	wday=pmod(wday,7);
	qint64 day=daysSinceEpoch(year,month);
	do{
		if(weekDaySinceEpoch(day)==wday)return day;
		day++;
	}while(true);
}

/**returns the last day in a month that is a specific day of the week, 0 and 7 are sunday
\returns offset in days from Epoch*/
inline qint64 lastWeekDayOf(int year,int month,int wday)
{
	wday=pmod(wday,7);
	if(month>=12){
		month=1;
		year++;
	}else month++;
	qint64 day=daysSinceEpoch(year,month);day--;
	do{
		if(weekDaySinceEpoch(day)==wday)return day;
		day--;
	}while(true);
}

/**returns the time represented by this unix timestamp for UTC*/
inline void stamp2Time(qint64 stamp,quint8&h,quint8&m,quint8&s)
{
	stamp=pmod(stamp,SecondsPerDay);
	s=stamp%60;
	m=(stamp/60)%60;
	h=stamp/3600;
}

/**returns the date represented by this offset from Epoch (in days)*/
inline void offset2Date(qint64 off,qint16&y,quint8&m,quint8&d)
{
	//try to find the year
	y=1970;
	if(off>0){
		while(off>366){
			off-=365;
			if(isLeapYear(y))off--;
			y++;
		}
	}else{
		while(off<0){
			off+=365;
			y--;
			if(isLeapYear(y))off++;
		}
	}
	//find month
	if(off<31){m=1;d=off+1;return;}else off-=31;
	if(off<28){m=2;d=off+1;return;}else off-=28;
	if(isLeapYear(y)){
		if(off)off--;
		else{m=2;d=29;return;}
	}
	if(off<31){m=3;d=off+1;return;}else off-=31;
	if(off<30){m=4;d=off+1;return;}else off-=30;
	if(off<31){m=5;d=off+1;return;}else off-=31;
	if(off<30){m=6;d=off+1;return;}else off-=30;
	if(off<31){m=7;d=off+1;return;}else off-=31;
	if(off<31){m=8;d=off+1;return;}else off-=31;
	if(off<30){m=9;d=off+1;return;}else off-=30;
	if(off<31){m=10;d=off+1;return;}else off-=31;
	if(off<30){m=11;d=off+1;return;}else off-=30;
	if(off<31){m=12;d=off+1;return;}else off-=31;
	//something left over, move to next year
	y++;m=1;d=off+1;
}

/**converts the time stamp to a date (assumes UTC)*/
inline void stamp2Date(qint64 off,qint16&y,quint8&m,quint8&d)
{
	qint64 de=fdiv(off,SecondsPerDay);
	offset2Date(de,y,m,d);
}

/**converts date and time to a time stamp, UTC is assumed
\param days the date in days since Epoch
\param h:m:s exact time of the day (0<=h<24)
*/
inline qint64 dateTime2stamp(qint64 days,quint8 h,quint8 m,quint8 s)
{
	qint64 r=days*SecondsPerDay;
	r+=h*3600 + m*60 + s;
	return r;
}

/**converts date and time to a time stamp, UTC is assumed
\param y year
\param mon the month (1-12)
\param day day of the month (1-31)
\param hr:min:s exact time of the day (0<=hr<24)
*/
inline qint64 dateTime2stamp(int y,quint8 mon,quint8 day,quint8 hr,quint8 min,quint8 s)
{
	return dateTime2stamp(daysSinceEpoch(y,mon,day),hr,min,s);
}

//end of namespace
}

#endif
