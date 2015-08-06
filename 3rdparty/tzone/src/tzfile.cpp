//
// C++ Implementation: TZ file loader
//
// Description: internal interface to time zones
//
//
// Author: Konrad Rosenbaum <konrad@silmor.de>, (C) 2008
//
// Copyright: See README/COPYING files that come with this distribution
//
//

#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <QStringList>

#include "tzfile.h"
#include "tzhelp.h"
#include "tzsys.h"

//change this to enable/disable debugging
#define TZ_DEBUG_OUTPUT 0

using namespace TimeZoneLib;

#if TZ_DEBUG_OUTPUT > 0
 #include <QDebug>
 #define TZDEBUG qDebug()
#else
 #define TZDEBUG NoDebug()
 //this class should be optimized away by the compiler
 class NoDebug{public:
  NoDebug& operator<<(QString){return*this;}
  NoDebug& operator<<(const char*){return*this;}
  NoDebug& operator<<(int){return*this;}
  NoDebug& operator<<(qint64){return*this;}
  NoDebug& operator<<(bool){return*this;}
 };
#endif


/* **************************************************************
 * TZ File
 ****************************************************************/

static QString systemZone;
void TimeZoneLib::setSystemZoneFile(QString z)
{
	if(systemZone.isEmpty())systemZone=z;
}

static const int MAXCACHESIZE=256;

static const QStringList builtinpath=QStringList()
	<< ":/zoneinfo" 	//built in default
	<< ":/zoneinfo-posix"	//built in posix
	<< ":/zoneinfo-leaps"	//built in leap second corrected time
; // end of list

QStringList TZFile::m_search=QStringList()
	<< builtinpath
	<< "/etc/zoneinfo"	//global zoneinfo directory
	<< "/usr/share/zoneinfo"//typical Linux
	<< "/usr/lib/zoneinfo"  //some unices
	<< "/usr/local/etc/zoneinfo" //default path for the tzdata source package
; //end of list


QStringList TZFile::searchPath()
{
	return m_search;
}

void TZFile::setSearchPath(const QStringList& s)
{
	m_search=s;
}



TZFile::TZFile()
	:m_posix("UTC0UTC0")
	//above: make sure it behaves like UTC
{
	//valid in the sense that it is UTC
	m_valid=true;
	//call it UTC then...
	m_name="UTC";
}

TZFile::TZFile ( const char* tz )
{
	strConstruct(tz);
}

TZFile::TZFile(QString s)
{
	strConstruct(s);
}


void TZFile::strConstruct ( QString s)
{
	m_valid=false;
	//special rule names
	if(s.startsWith("system/")){
		//native localtime file
		if(s=="system/localtime" && !systemZone.isEmpty()){
			QFile f(systemZone);
			if(!f.open(QIODevice::ReadOnly))return;
			m_valid=load(f.readAll());
			if(!m_valid)return;
			m_name=s;
			m_fname=systemZone;
			return;
		}
		//no more specials left: abort
		return;
	}
	//search
	for(int i=0;i<m_search.size();i++){
		QFile f(m_search[i]+"/"+s);
		if(!f.open(QIODevice::ReadOnly))
			continue;
		m_valid=load(f.readAll());
		if(m_valid){
			m_name=s;
			m_fname=f.fileName();
			return;
		}
	}
}

TZFile::TZFile(QIODevice& d)
{
	if(!d.isReadable()){
		m_valid=false;
		return;
	}
	m_valid=load(d.readAll());
	if(m_valid){
		QFile*f=qobject_cast<QFile*>(&d);
		if(f)m_fname=f->fileName();
	}
}

TZFile::TZFile(const QByteArray& a)
{
	m_valid=load(a);
}

TZFile& TZFile::operator=(const TZFile& z)
{
	m_file=z.m_file;
	m_cache=z.m_cache;
	m_posix=z.m_posix;
	m_valid=z.m_valid;
	m_name=z.m_name;
	m_fname=z.m_fname;
	return *this;
}

TZFile::TZFile(const TZFile& z)
{
	operator=(z);
}

TZFile::TZFile(int offset)
{
	//this one only sets the posix ruleset
	// the cache will be filled automatically while using it
	m_valid=true;
	//calculate hours, minutes and sign of the offset
	bool n=false;
	if(offset<0){
		n=true;
		offset*=-1;
	}
	int h=offset/60;
	int m=offset%60;
	//create the posix ruleset
	m_posix=PosixRule(QString("<UTC%1%2%3%4>%5%6:%7")
		.arg(n?"-":"+") //%1: sign of offset inside name
		.arg(h) //%2: hours
		.arg(m?":":"") //%3: if we have minutes create a ":"
		.arg(m?QString::number(m):QString()) //%4: if we have minutes print them
		.arg(n?"-":"") //%5: posix offset: if negative print "-"
		.arg(h).arg(m) //%6, %6: posix offset hours and minutes
	);
	//create name
	QString tname="UTC";
	if(n){tname+="-";}
	else tname+="+";
	tname+=QString("%1:%2").arg(h,2,10,QChar('0')).arg(m,2,10,QChar('0'));
	m_name=tname;
}

bool TZFile::load(const QByteArray& a)
{
	//reset
	m_file.clear();
	m_cache.clear();
	m_posix=PosixRule();
	//try 32 bit
	int off=load(a,0);
	if(off<0)return false;
	if(off==0)return true;
	//parse 64 bit section
	off=load(a.mid(off),'2');
	return off>=0;
}

int TZFile::load(const QByteArray& a,char tversion)
{
	//check for minimum size
	if(a.size()<44){
		TZDEBUG<<"TZ file too short.";
		return -1;
	}
	//check magic
	if(a.left(4)!="TZif"){
		TZDEBUG<<"not a TZ file";
		return -1;
	}
	//decode header
	char version=a[4];
	if(version!=0 && version!='2'){
		TZDEBUG<<"unknown TZ version";
		return -1;
	}
	qint32 tzh_ttisgmtcnt=decodeInt(a.mid(20,4));
	qint32 tzh_ttisstdcnt=decodeInt(a.mid(24,4));
	qint32 tzh_leapcnt=decodeInt(a.mid(28,4));
	qint32 tzh_timecnt=decodeInt(a.mid(32,4));
	qint32 tzh_typecnt=decodeInt(a.mid(36,4));
	qint32 tzh_charcnt=decodeInt(a.mid(40,4));
	
	//is this 32bit?
	int tsz=4;//transition time size in bytes
	if(tversion>0) tsz=8;
	qint64 coff=44;//current offset
	TZDEBUG<<"type is"<<(int)version<<"interpreting as"<<(int)tversion;
	
	QByteArray ttimes;
	QByteArray ttypeidx;
	QByteArray ttype;
	QByteArray zabbr;
	//decode current section
	ttimes=a.mid(coff,tzh_timecnt*tsz);coff+=tzh_timecnt*tsz;
	ttypeidx=a.mid(coff,tzh_timecnt);coff+=tzh_timecnt;
	ttype=a.mid(coff,tzh_typecnt*6);coff+=tzh_typecnt*6;
	zabbr=a.mid(coff,tzh_charcnt);coff+=tzh_charcnt;
	coff+=tzh_leapcnt*(tsz+4);//skip leap second calculations
	coff+=tzh_ttisstdcnt;//skip flag std vs. wall clock time
	coff+=tzh_ttisgmtcnt;//skip flag UTC vs local time
	
	//is this the target?
	if(version!=tversion)return coff;
	
	//is this tversion 2?
	QString posix;
	if(tversion>0)
		posix=QString::fromLatin1(a.mid(coff).data()).trimmed();
	
	//actually go through data
	for(int i=0;i<tzh_timecnt;i++){
		qint64 start=decodeInt(ttimes.mid(i*tsz,tsz),tsz);
		int idx=ttypeidx[i];
		qint32 off=0;bool isdst=false;
		if(idx>=0 && idx<(ttype.size()/6)){
			off=decodeInt(ttype.mid(idx*6,4));
			isdst=ttype[idx*6+4]!=(char)0;
			idx=ttype[idx*6+5];
		}else idx=-1;
		QString abbr;
		if(idx>=0 && idx<zabbr.size())
			abbr=QString::fromLatin1(zabbr.mid(idx).data());
		TZDEBUG<<"found entry"<<i<<"start"<<start<<"offset"<<off<<"dst"<<isdst<<"abbr"<<abbr;
		m_file.append(TZRule(start,off,abbr,isdst));
	}
	TZDEBUG<<"posix string:"<<posix;
	m_posix=PosixRule(posix);
	
	//sort and add end times
	qSort(m_file);
	for(int i=0;i<(m_file.size()-1);i++)
		m_file[i].setEnd(m_file[i+1].startTime());
	if(m_file.size())
		m_file[m_file.size()-1].setEnd(m_posix.nextRule(m_file[m_file.size()-1]).startTime());
	
	//return success
	return 0;
}



TZRule TZFile::ruleForTime ( qint64 ts)
{
	//search file contents
	for(int i=0;i<m_file.size();i++)
		if(m_file[i].matches(ts))
			return m_file[i];
	//search cache
	for(int i=0;i<m_cache.size();i++)
		if(m_cache[i].matches(ts)){
			//move to end, to ensure it survives
			TZRule r=m_cache.takeAt(i);
			m_cache.append(r);
			//return it
			return r;
		}
	//get from posix
	TZRule r=m_posix.ruleForTime(ts);
	m_cache.append(r);
	//prune cache
	if(m_cache.size()>MAXCACHESIZE)
		m_cache=m_cache.mid(m_cache.size()-MAXCACHESIZE);
	//return
	return r;
}

TZRule TZFile::ruleForLocalTime ( qint64 ts)
{
	//search file contents
	for(int i=0;i<m_file.size();i++)
		if(m_file[i].matchesLocal(ts))
			return m_file[i];
	//search cache
	for(int i=0;i<m_cache.size();i++)
		if(m_cache[i].matchesLocal(ts)){
			//move to end, to ensure it survives
			TZRule r=m_cache.takeAt(i);
			m_cache.append(r);
			//return it
			return r;
		}
	//get from posix
	TZRule r=m_posix.ruleForLocalTime(ts);
	m_cache.append(r);
	//prune cache
	if(m_cache.size()>MAXCACHESIZE)
		m_cache=m_cache.mid(m_cache.size()-MAXCACHESIZE);
	//return
	return r;
}

QString TZFile::version()const
{
	//can only determine it if it is an internal or real file
	if(m_fname.isEmpty())return QString();
	//split out dir name
	QStringList ls=m_fname.split("/",QString::SkipEmptyParts);
	if(ls.size()<3)return QString();
	//reduce to zoneinfo dir
	for(int i=0;i<ls.size();i++)
		if(ls[i]=="zoneinfo"){
			ls=ls.mid(0,i+1);
		}
	QString fn=ls.join("/");
	if(m_fname.startsWith('/'))fn.prepend('/');
	fn.append("/+VERSION");
	//try to open version file
	QFile f(fn);
	if(f.open(QIODevice::ReadOnly))
		return QString::fromLatin1(f.readAll()).trimmed();
	else
		return QString();
}

QString TZFile::builtinVersion()
{
	foreach(QString bd,builtinpath){
		QFile f(bd+"/+VERSION");
		if(f.open(QIODevice::ReadOnly))
			return QString::fromLatin1(f.readAll()).trimmed();
	}
	return QString();
}

QString TZFile::libraryVersion()
{
    return "0.3.0.qlandkarte";
}

QString TZFile::dirName()const
{
	if(m_fname.isEmpty())return QString();
	return m_fname.mid(0,m_fname.size()-m_name.size());
}

/* **************************************************************
 * TZ Rule
 ****************************************************************/

TZRule::TZRule()
{
	m_start=m_end=0;
	m_off=0;
	m_isdst=false;
}

TZRule::TZRule(const TZRule& r)
{
	operator=(r);
}

TZRule::TZRule(qint64 start, qint32 off, QString abbr, bool isdst)
{
	m_start=start;
	m_off=off;
	m_abbr=abbr;
	m_isdst=isdst;
}


TZRule& TZRule::operator=(const TZRule& r )
{
	m_start=r.m_start;
	m_end=r.m_end;
	m_off=r.m_off;
	m_abbr=r.m_abbr;
	m_isdst=r.m_isdst;
	return *this;
}

/* **************************************************************
 * Posix Rule
 ****************************************************************/

PosixRule::PosixRule()
{
	m_off=0;
	m_offdst=3600;
	//according to POSIX we default on American rules
	m_months=4;m_weeks=1;m_days=0;//first sunday in april starts DST
	m_monthe=10;m_weeke=5;m_daye=0;//last sunday in october ends DST
	m_timee=m_times=7200;//2:00am
	m_types=m_typee=StartWeek;
}

PosixRule::PosixRule(const PosixRule& p)
{
	operator=(p);
}

PosixRule& PosixRule::operator=(const PosixRule& p)
{
	m_off=p.m_off;
	m_offdst=p.m_offdst;
	m_months=p.m_months;
	m_weeks=p.m_weeks;
	m_days=p.m_days;
	m_monthe=p.m_monthe;
	m_weeke=p.m_weeke;
	m_daye=p.m_daye;
	m_types=p.m_types;
	m_typee=p.m_typee;
	m_str=p.m_str;
	m_abbr=p.m_abbr;
	m_abbrdst=p.m_abbrdst;
	m_timee=p.m_timee;
	m_times=p.m_times;
	return *this;
}

static inline QString parseTZname(QString&s)
{
	if(s=="")return "";
	QString r;
	if(s[0]=='<'){
		for(int i=1;i<=s.size();i++){
			if(i==s.size()){
				s="";
				break;
			}
			if(s[i]=='>'){
				s=s.mid(i+1);
				break;
			}else
				r+=s[i];
		}
	}else{
		for(int i=0;i<=s.size();i++){
			if(i==s.size()){
				s="";
				break;
			}
			if(s[i].isLetter())
				r+=s[i];
			else{
				s=s.mid(i);
				break;
			}
		}
	}
	return r;
}

static inline int parseTime(QString&s)
{
	if(s=="")return 0;
	//negative?
	int f=1;
	if(s[0]=='-'){
		f=-1;
		s=s.mid(1);
	}
	//parse out
	QString ts;
	for(int i=0;i<s.size();i++){
		if(s[i]==':' || s[i].isDigit())
			ts+=s[i];
		else{
			s=s.mid(i);
			break;
		}
	}
	//split
	QStringList sl=ts.split(":");
	int tm=0;
	for(int i=0;i<sl.size();i++){
		int t=sl[i].toInt(0,10);
		switch(i){
			case 0:tm+=t*3600;break;
			case 1:tm+=t*60;break;
			case 2:tm+=t;break;
		}
	}
	return f*tm;
}

PosixRule::PosixRule(QString s)
{
	//init
	operator=(PosixRule());
	//copy string
	m_str=s.trimmed();
	if(m_str.size()==0)return;
	
	//parse it
	QStringList l1=m_str.split(",");
	
	//parse stanza 1: stdoff[dst[off]]
	QString s1=l1[0];
	// std: abbreviation 1
	m_abbr=parseTZname(s1);
	// off: offset 1
	m_off=-parseTime(s1);
	// dst: abbreviation 2
	if(s1.size()){
		m_abbrdst=parseTZname(s1);
		if(s1.size())
			m_offdst=-parseTime(s1);
		else
			//POSIX default: if no DST offset given it is standard + 1h ahead
			m_offdst=m_off+3600;
	}else{
		m_abbrdst=m_abbr;//DST has no special name
		m_offdst=m_off+3600;//but its own time (according to POSIX)
	}
	
	//syntax ok?
	if(l1.size()<2)return;
	
	//parse stanza 2: start of DST
	QStringList l2=l1[1].split("/");
	//scan date
	s1=l2[0];
	if(s1[0]=='J'){
		m_types=StartDay1;
		m_days=s1.mid(1).toInt();
	}else
	if(s1[0].isDigit()){
		m_types=StartDay0;
		m_days=s1.toInt();
	}else
	if(s1[0]=='M'){
		QStringList l3=s1.mid(1).split(".");
		if(l3.size()==3){
			m_types=StartWeek;
			m_months=l3[0].toInt();
			m_weeks=l3[1].toInt();
			m_days=l3[2].toInt();
		}
	}
	//scan time
	if(l2.size()>1)
		m_times=parseTime(l2[1]);
	else
		m_times=7200;//default is 2:00 of previous zone
	
	if(l1.size()<3)return;
	//parse stanza 3: end of DST
	l2=l1[2].split("/");
	//scan date
	s1=l2[0];
	if(s1[0]=='J'){
		m_typee=StartDay1;
		m_daye=s1.mid(1).toInt();
	}else
	if(s1[0].isDigit()){
		m_typee=StartDay0;
		m_daye=s1.toInt();
	}else
	if(s1[0]=='M'){
		QStringList l3=s1.mid(1).split(".");
		if(l3.size()==3){
			m_typee=StartWeek;
			m_monthe=l3[0].toInt();
			m_weeke=l3[1].toInt();
			m_daye=l3[2].toInt();
		}
	}
	//scan time
	if(l2.size()>1)
		m_timee=parseTime(l2[1]);
	else
		m_timee=7200;//default is 2:00 of previous zone
}

//helper for rulesForYear: calculates the exact time of a transition
static inline qint64 calcTimeStamp(int year,PosixRule::StartType type,int month,int week,int day,int time)
{
	qint64 ret=0;
	switch(type){
		case PosixRule::StartDay1:
			ret=daysSinceEpoch(year)+day-1;
			if(day>59 && isLeapYear(year))ret++;
			break;
		case PosixRule::StartDay0:
			ret=daysSinceEpoch(year)+day;
			break;
		case PosixRule::StartWeek:
			if(week>=5)ret=lastWeekDayOf(year,month,day);
			else{
				ret=lastWeekDayOf(year,month,day);
				if(week>1)
					ret+=7*(week-1);
			}
			break;
		default:return 0;
	}
	return ret*SecondsPerDay+time;
}

TZRule PosixRule::createRule (qint32 year, bool start )const
{
	PosixRule::StartType type;
	qint32 month,week,day,time,prevoffset,newoffset;
	QString abbr;
	bool isdst;
	if(start){
		type=m_types;
		month=m_months;
		week=m_weeks;
		day=m_days;
		time=m_times;
		abbr=m_abbrdst;
		prevoffset=m_off;
		newoffset=m_offdst;
		isdst=true;
	}else{
		type=m_typee;
		month=m_monthe;
		week=m_weeke;
		day=m_daye;
		time=m_timee;
		abbr=m_abbr;
		prevoffset=m_offdst;
		newoffset=m_off;
		isdst=false;
	}
	TZRule r;
	r.m_start=calcTimeStamp(year,type,month,week,day,time);
	r.m_start-=prevoffset;
	r.m_abbr=abbr;
	r.m_isdst=isdst;
	r.m_off=newoffset;
	r.m_end=r.m_start;
	return r;
}


QList< TZRule > PosixRule::rulesForYear(int year) const
{
	//do we need to calculate, if not: create a rule for the full year
	if(m_types==StartNone || m_typee==StartNone){
		TZRule tz(daysSinceEpoch(year)*SecondsPerDay,m_off,m_abbr,false);
		tz.setEnd(daysSinceEpoch(year+1)*SecondsPerDay+m_off);
		return QList<TZRule>()<<tz;
	}
	
	//ok, now calculate
	TZRule start,end,prev;
	QList<TZRule>ret;
	
	//calculate previous year
	start=createRule(year-1,true);
	end=createRule(year-1,false);
	if(start<end)ret<<end;
	else ret<<start;
	
	//calculate this year
	ret<<createRule(year,true)<<createRule(year,false);
	
	//calculate next year - just for the end of it all
	start=createRule(year+1,true);
	end=createRule(year+1,false);
	if(start<end)ret<<start;
	else ret<<end;
	
	//calculate end of rules
	qSort(ret);
	for(int i=1;i<ret.size();i++)
		ret[i-1].m_end=ret[i].m_start;
	//truncate last rule, we do not need it
	ret=ret.mid(0,ret.size()-1);
	
	return ret;
}

TZRule PosixRule::nextRule(const TZRule& r) const
{
	//find the year
	qint16 y;
	quint8 mo,d;
	stamp2Date(r.m_start,y,mo,d);
	//create rules around
	QList<TZRule>rl;
	rl<<createRule(y-1,true)<<createRule(y-1,false);
	rl<<createRule(y,true)<<createRule(y,false);
	rl<<createRule(y+1,true)<<createRule(y+1,false);
	rl<<createRule(y+2,true)<<createRule(y+2,false);
	//sort and extend
	qSort(rl);
	for(int i=1;i<rl.size();i++)
		rl[i-1].m_end=rl[i].m_start;
	//find the first one that is later and on a different offset
	for(int i=0;i<rl.size();i++){
		if(rl[i]<r)continue;
		if(rl[i].m_off == r.m_off)continue;
		return rl[i];
	}
	//fall back to the first one that is later
	for(int i=0;i<rl.size();i++){
		if(rl[i]<=r)continue;
		return rl[i];
	}
	//should not be reachable
	qDebug()<<"oops in PosixRule::nextRule";
	return TZRule();
}

TZRule PosixRule::ruleForTime(qint64 ts) const
{
	qint16 y;
	quint8 mo,d;
	stamp2Date(ts,y,mo,d);
	QList<TZRule>yr=rulesForYear(y);
	for(int i=0;i<yr.size();i++)
		if(yr[i].matches(ts))
			return yr[i];
	//should not be reachable...
	qDebug()<<"oops in PosixRule::ruleForTime";
	return TZRule();
}

TZRule PosixRule::ruleForLocalTime ( qint64 ts) const
{
	qint16 y;
	quint8 mo,d;
	stamp2Date(ts,y,mo,d);
	QList<TZRule>yr=rulesForYear(y);
	for(int i=0;i<yr.size();i++)
		if(yr[i].matchesLocal(ts))
			return yr[i];
	//should not be reachable...
	qDebug()<<"oops in PosixRule::ruleForLocalTime";
	return TZRule();
}

