TEMPLATE = lib
TARGET = QtTzData
DESTDIR = test/..
CONFIG += dll create_prl
QT -= gui
OBJECTS_DIR = .ctmp
MOC_DIR = .ctmp
RCC_DIR = .ctmp

#prevent win32 from attaching a version number to the DLL
VERSION = 
#define actual version
DEFINES += TZLIB_VERSION=\\\"0.3.0\\\"


HEADERS += \
	include/tzfile.h \
	include/tzdata.h \
	src/tzsys.h

SOURCES += \
	zonefiles.cpp \
	src/tzfile.cpp \
	src/tzdata.cpp 

INCLUDEPATH += include src .
DEPENDPATH += include src .

#discovery routines
win32-* {
  SOURCES += src/tzsys_win.cpp
} else {
  SOURCES += src/tzsys_unix.cpp
}