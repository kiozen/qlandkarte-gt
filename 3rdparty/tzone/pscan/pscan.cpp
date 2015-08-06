// This is a very simple POSIX Rule Scanner
// I used it to find the possible variations in the fallback POSIX rules of the TZ DB

#include <QtCore>
#include "tzfile.h"

bool verbose=false;

void scanFile(QString fn)
{
	using namespace TimeZoneLib;
	QFile fd(fn);
	if(!fd.open(QIODevice::ReadOnly)){
		if(verbose)
			qDebug()<<"file unreadable"<<fn;
		return;
	}
	TZFile tz(fd.readAll());
	fd.close();
	if(tz.isValid()){
		if(verbose)
			qDebug()<<"file"<<fn<<":"<<tz.posixRule().asString();
		else
			qDebug()<<tz.posixRule().asString();
	}else
		if(verbose)
			qDebug()<<"invalid file"<<fn;
}

void scanDir(QString d)
{
	QDir dir(d);
	if(!dir.exists())return;
	if(verbose)
		qDebug()<<"entering dir"<<d;
	QStringList de=dir.entryList();
	for(int i=0;i<de.size();i++){
		if(de[i]=="." || de[i]=="..")continue;
		QString fn=d+"/"+de[i];
		QFileInfo fi(fn);
		if(fi.isFile()){
			if(fi.isReadable())
				scanFile(fn);
			else
				if(verbose)
					qDebug()<<"skipping unreadable"<<fn;
		}else
		if(fi.isDir())
			scanDir(fn);
		else
			if(verbose)
				qDebug()<<"skipping special"<<fn;
	}
}

int main(int argc,char**argv)
{
	QCoreApplication app(argc,argv);
	QStringList dl=app.arguments().mid(1);
	if(dl.size()==0){
		qDebug()<<"Usage: pscan [-v] directory ...";
		return 1;
	}
	if(dl[0]=="-v"){
		verbose=true;
		dl=dl.mid(1);
	}
	for(int i=0;i<dl.size();i++)
		scanDir(dl[i]);
}