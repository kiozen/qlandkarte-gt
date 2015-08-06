#include "tzt.h"

#include <QDebug>
#include <QtCore>

#include "tzdata.h"
#include "tzhelp.h"
#include "tzfile.h"
#include "tzreg.h"

void MyTest::testHelpersDecode()
{
	using namespace TimeZoneLib;
	QCOMPARE(decodeInt(QByteArray::fromHex("01020304"),4),0x01020304ll);
	QCOMPARE(decodeInt(QByteArray::fromHex("010203"),4),0ll);
	QCOMPARE(decodeInt(QByteArray::fromHex("ffffffff"),4),-1ll);
	QCOMPARE(decodeInt(QByteArray::fromHex("ffffff00"),4),-256ll);
	QCOMPARE(decodeInt(QByteArray::fromHex("ff01020300"),4),-0xfefdfdll);
	QCOMPARE(decodeInt(QByteArray::fromHex("010203"),2),0x0102ll);
	QCOMPARE(decodeInt(QByteArray::fromHex("010203"),3),0x010203ll);
}

void MyTest::testHelpersDay()
{
	using namespace TimeZoneLib;
	QCOMPARE(isLeapYear(1999),false);
	QCOMPARE(isLeapYear(1998),false);
	QCOMPARE(isLeapYear(2100),false);
	QCOMPARE(isLeapYear(2400),true);
	QCOMPARE(isLeapYear(1904),true);
	QCOMPARE(isLeapYear(2000),true);
	
	QCOMPARE(daysSinceEpoch(2010),14610ll);
	QCOMPARE(daysSinceEpoch(2012,12,15),15689ll);
	QCOMPARE(daysSinceEpoch(1901,4,15),-25098ll);
	
	QCOMPARE(weekDayOf(1970),(int)TimeStamp::Thursday);
	QCOMPARE(weekDayOf(2010,7,6),(int)TimeStamp::Tuesday);
	QCOMPARE(weekDaySinceEpoch(daysSinceEpoch(2010,7,6)),(int)TimeStamp::Tuesday);
	
	QCOMPARE(firstWeekDayOf(2010,7,TimeStamp::Wednesday),daysSinceEpoch(2010,7,7));
	QCOMPARE(firstWeekDayOf(2010,7,TimeStamp::Thursday),daysSinceEpoch(2010,7,1));
	QCOMPARE(lastWeekDayOf(2010,6,TimeStamp::Wednesday),daysSinceEpoch(2010,6,30));
	QCOMPARE(lastWeekDayOf(2010,6,TimeStamp::Thursday),daysSinceEpoch(2010,6,24));
}

void MyTest::testHelpersTime()
{
	using namespace TimeZoneLib;
	QCOMPARE(dateTime2stamp(2010,7,6,13,16,17),1278422177ll);
	QCOMPARE(dateTime2stamp(daysSinceEpoch(2010,7,6),13,16,17),1278422177ll);
	QCOMPARE(dateTime2stamp(1910,7,6,13,16,17),-1877337823ll);
	QCOMPARE(dateTime2stamp(daysSinceEpoch(1910,7,6),13,16,17),-1877337823ll);
}


inline QString i2str(int i1,int i2,int i3,int i4=0)
{
	return QString("%1:%2:%3:%4").arg(i1).arg(i2).arg(i3).arg(i4);
}

inline void testDateReverse(int yy,int mm,int dd)
{
	using namespace TimeZoneLib;
	qint16 y;quint8 m,d;
	offset2Date(daysSinceEpoch(yy,mm,dd),y,m,d);
	QCOMPARE(i2str(y,m,d),i2str(yy,mm,dd));
}

inline void testTimeReverse(int hh,int mm,int sc,int d)
{
	using namespace TimeZoneLib;
	quint8 h,m,s;
	int t=hh*3600+mm*60+sc+d*SecondsPerDay;
	stamp2Time(t,h,m,s);
	QCOMPARE(i2str(h,m,s,d),i2str(hh,mm,sc,d));
}

void MyTest::testHelpersReverse()
{
	testDateReverse(2010,7,7);
	testDateReverse(1901,4,15);
	testDateReverse(2011,12,31);
	testDateReverse(2011,1,1);
	testDateReverse(2012,1,1);
	testDateReverse(2012,12,31);
	testDateReverse(2013,12,31);
	testDateReverse(1943,12,31);
	testDateReverse(1943,1,1);
	testDateReverse(1944,1,1);
	testDateReverse(1944,12,31);
	testDateReverse(1945,12,31);
	
	testTimeReverse(12,2,3,0);
	testTimeReverse(0,0,0,0);
	testTimeReverse(12,2,3,1000);
	testTimeReverse(0,0,0,1000);
	testTimeReverse(12,2,3,-20);
	testTimeReverse(0,0,0,-20);
}


void MyTest::infos()
{
	qDebug()<<"Info: the automatically detected time zone is"<<TimeStamp::defaultZone();
	TimeStamp ts(QDateTime::currentDateTime());
	qDebug()<<"Current time locally"<<ts.toISO();
	qDebug()<<"Current time at UTC"<<ts.toUTC().toISO();
}


void MyTest::testZones()
{
	using namespace TimeZoneLib;
	TZFile tf;
	//test defaults
	QCOMPARE(tf.name(),QString("UTC"));
	QCOMPARE(tf.ruleForTime(100).offsetFromUTC(),0);
	QCOMPARE(tf.ruleForTime(13474800).offsetFromUTC(),0);
	QCOMPARE(tf.ruleForLocalTime(100).offsetFromUTC(),0);
	//setting name
	tf.setName("Hallo");
	QCOMPARE(tf.name(),QString("Hallo"));
	//zurich time
	tf=TZFile("Europe/Zurich");
	QCOMPARE(tf.name(),QString("Europe/Zurich"));
	QCOMPARE(tf.ruleForTime(1278586618).offsetFromUTC(),7200);//summer 2010
	QCOMPARE(tf.ruleForTime(1292108400).offsetFromUTC(),3600);//winter 2010
	QCOMPARE(tf.ruleForTime(-444099600).offsetFromUTC(),3600);//winter 1955
	//this actually tests the posix rule generator
	QCOMPARE(tf.ruleForTime(4431967200).offsetFromUTC(),7200);//summer 2110
	QCOMPARE(tf.ruleForTime(4447782000).offsetFromUTC(),3600);//winter 2110
        //check offsets for far eastern zones are ok (qint16 has an overflow there)
        QCOMPARE(TZFile("Australia/Adelaide").ruleForTime(1362008681).offsetFromUTC(),37800);
}

void MyTest::testRegistry()
{
	using namespace TimeZoneLib;
	TZFile tf=getRegistryZone("<yadda-no-such-zone>");
	//test defaults
	QCOMPARE(tf.name(),QString("UTC"));
	QCOMPARE(tf.ruleForTime(100).offsetFromUTC(),0);
	QCOMPARE(tf.ruleForLocalTime(100).offsetFromUTC(),0);
	//explicit UTC
	tf=getRegistryZone("UTC");
	QCOMPARE(tf.name(),QString("UTC"));
	//zurich time
	tf=getRegistryZone("Europe/Zurich");
	QCOMPARE(tf.name(),QString("Europe/Zurich"));
	QCOMPARE(tf.ruleForTime(1278586618).offsetFromUTC(),7200);//summer 2010
	QCOMPARE(tf.ruleForTime(1292108400).offsetFromUTC(),3600);//winter 2010
}

void MyTest::testPosix()
{
	using namespace TimeZoneLib;
	PosixRule rs("xxx0xxx0");
	QCOMPARE(rs.ruleForTime(100).offsetFromUTC(),0);
	QCOMPARE(rs.ruleForLocalTime(100).offsetFromUTC(),0);
	QCOMPARE(rs.ruleForTime(13474800).offsetFromUTC(),0);
	QCOMPARE(rs.daylightName(),rs.standardName());
	QCOMPARE(rs.daylightName(),QString("xxx"));
	QCOMPARE(rs.daylightOffset(),0);
	QCOMPARE(rs.standardOffset(),0);
	QCOMPARE(rs.haveDST(),false);
	rs=PosixRule("yyy2xxx");
	QCOMPARE(rs.standardName(),QString("yyy"));
	QCOMPARE(rs.daylightName(),QString("xxx"));
	QCOMPARE(rs.daylightOffset(),-3600);
	QCOMPARE(rs.standardOffset(),-7200);
	QCOMPARE(rs.haveDST(),true);
	QCOMPARE(rs.ruleForTime(100).offsetFromUTC(),-7200);
	QCOMPARE(rs.ruleForLocalTime(100).offsetFromUTC(),-7200);
	QCOMPARE(rs.ruleForTime(13474800).offsetFromUTC(),-3600);

	//test rules for year (replicates the Zurich rules)
	rs=PosixRule("STD-1SUM,M3.5.0,M10.5.0/3");
	QCOMPARE(rs.standardName(),QString("STD"));
	QCOMPARE(rs.daylightName(),QString("SUM"));
	QCOMPARE(rs.daylightOffset(),7200);
	QCOMPARE(rs.standardOffset(),3600);
	QCOMPARE(rs.haveDST(),true);
	QList<TZRule>rls=rs.rulesForYear(2010);
	qDebug()<<"retrieved rules for 2010:";
	for(int i=0;i<rls.size();i++)
		qDebug()<<" rule"<<i<<"offset"<<rls[i].offsetFromUTC()
		<<"starts at"<<TimeStamp(rls[i].startTime(),true).toISO()
		<<"ends at"<<TimeStamp(rls[i].endTime(),true).toISO();
	QCOMPARE(rls.size(),3);
	QCOMPARE(rls[0].startTime(),1256432400ll);
	QCOMPARE(rls[1].startTime(),1269738000ll);
	QCOMPARE(rls[2].startTime(),1288486800ll);
	QCOMPARE(rls[2].endTime(),  1301187600ll);

	QCOMPARE(rls[0].endTime(),rls[1].startTime());
	QCOMPARE(rls[1].endTime(),rls[2].startTime());
}


void MyTest::testStamps()
{
	qDebug()<<"attempting to set local zone to Europe/Zurich";
	//info: zurich has +200 in DST and +100 in normal time
	QCOMPARE(TimeStamp::setDefaultZone("Europe/Zurich"),true);
	QCOMPARE(TimeStamp::defaultZone(),(QString)"Europe/Zurich");
	TimeStamp ts=TimeStamp::fromDateTime(2010,7,6,12,3,4,false);
	TimeStamp tsl=ts.toLocal();
	TimeStamp tsm=TimeStamp::fromDateTime(2010,7,6,14,3,4,true);
	qDebug()<<"test timestamp 1:"<<ts.toISO();
	qDebug()<<" local version 1:"<<tsl.toISO();
	qDebug()<<" local version 2:"<<tsm.toISO();
	//equivalence
	QCOMPARE(ts,tsl);
	QCOMPARE(ts<=tsl,true);
	QCOMPARE(ts>=tsl,true);
	QCOMPARE(ts>tsl,false);
	QCOMPARE(ts<tsl,false);
	QCOMPARE(ts!=tsl,false);
	QCOMPARE(ts.toISO()!=tsl.toISO(),true);
	QCOMPARE(tsm.toISO()==tsl.toISO(),true);
	QCOMPARE(tsl.zone(),QString("Europe/Zurich"));
	QCOMPARE(tsm.zone(),QString("Europe/Zurich"));
	QCOMPARE(ts.zone(),QString("UTC"));
	QCOMPARE((int)ts.offsetFromUTC(),0);
	QCOMPARE((int)tsl.offsetFromUTC(),7200);
	//non-equivalence
	tsm.moveToUTC();
	QCOMPARE(ts!=tsm,true);
	QCOMPARE(ts<tsm,true);
	QCOMPARE(ts<=tsm,true);
	QCOMPARE(tsm>ts,true);
	QCOMPARE(tsm>=ts,true);
	QCOMPARE(ts>tsm,false);
	QCOMPARE(ts>=tsm,false);
	QCOMPARE(tsm<ts,false);
	QCOMPARE(tsm<=ts,false);
	QCOMPARE(tsm==ts,false);
	//assignment
	tsm=tsl;
	QCOMPARE(ts,tsm);
	//adding secs
	tsm.addSecs(100);
	QCOMPARE(tsm>ts,true);
	tsm.addSecs(-100);
	QCOMPARE(tsm,ts);
	//adding msecs
	tsm.addMSecs(100);
	QCOMPARE(tsm>ts,true);
	tsm.addMSecs(900);
	QCOMPARE(tsm.toUnix(),ts.toUnix()+1);
	tsm.addSecs(-1);
	QCOMPARE(tsm,ts);
	tsm.addMSecs(-100);
	QCOMPARE(tsm<ts,true);
	tsm.addMSecs(-900);
	QCOMPARE(tsm.toUnix(),ts.toUnix()-1);
	tsm.addSecs(1);
	QCOMPARE(tsm,ts);
}


QTEST_MAIN(MyTest)