#include <QObject>
#include <QtTest/QtTest>

class MyTest:public QObject{
	Q_OBJECT
	
	private slots:
		void testHelpersDecode();
		void testHelpersDay();
		void testHelpersTime();
		void testHelpersReverse();
		
		void infos();
		
		void testZones();
		void testRegistry();
		void testPosix();
		void testStamps();
};
