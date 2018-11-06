include(../defaults.pri)
TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
INCLUDEPATH += ../ThirdParty/Catch ../ThirdParty/FakeIt

SOURCES += main.cpp \
    testentityparsing.cpp \
    testpointmap.cpp \
    testlinkutils.cpp \
    testgridproperties.cpp \
    testisovistdef.cpp \
    testmgraph.cpp \
    testshapegraphs.cpp \
    teststructsizes.cpp \
    testsparksieve.cpp \
    testattributetable.cpp \
    testattributetableindex.cpp \
    testlayermanager.cpp \
    testattributetablehelpers.cpp \
    testattributetableview.cpp \
    testshapemaps.cpp \
    testgeometrygenerators.cpp \
    testmapinfodata.cpp \
    testsalaprogram.cpp

win32:Release:LIBS += -L../genlib/release -L../mgraph440/release -L../salalib/release
win32:Debug:LIBS += -L../genlib/debug -L../mgraph440/debug -L../salalib/debug
!win32:LIBS += -L../genlib -L../mgraph440 -L../salalib

LIBS += -lsalalib -lmgraph440 -lgenlib
