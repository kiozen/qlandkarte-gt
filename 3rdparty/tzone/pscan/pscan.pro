# This is a very simple POSIX Rule Scanner
# I used it to find the possible variations in the fallback POSIX rules of the TZ DB

TEMPLATE = app
TARGET = pscan

QT -= gui

SOURCES += pscan.cpp ../src/tzfile.cpp ../zonefiles.cpp
HEADERS += ../include/tzfile.h
#RESOURCES += ../zonefiles.qrc

INCLUDEPATH += ../include ../src