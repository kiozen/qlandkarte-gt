TEMPLATE=app
TARGET=tztest
SOURCES+=tzt.cpp
HEADERS+=tzt.h
DESTDIR=../test
DEPENDPATH+=.. ../include ../src

LIBS += -L.. -lQtTzData
INCLUDEPATH += ../include ../src ..

CONFIG+=qtestlib debug
QT-=gui