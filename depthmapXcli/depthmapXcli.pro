include(../defaults.pri)
TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    commandlineparser.cpp \
    runmethods.cpp \
    radiusconverter.cpp \
    vgaparser.cpp \
    linkparser.cpp \
    performancewriter.cpp \
    modeparserregistry.cpp \
    visprepparser.cpp \
    axialparser.cpp \
    parsingutils.cpp \
    agentparser.cpp \
    isovistparser.cpp \
    exportparser.cpp \
    importparser.cpp \
    stepdepthparser.cpp \
    segmentparser.cpp \
    mapconvertparser.cpp

HEADERS += \
    commandlineparser.h \
    runmethods.h \
    radiusconverter.h \
    exceptions.h \
    simpletimer.h \
    vgaparser.h \
    linkparser.h \
    performancewriter.h \
    performancesink.h \
    imodeparser.h \
    modeparserregistry.h \
    imodeparserfactory.h \
    visprepparser.h \
    parsingutils.h \
    axialparser.h \
    agentparser.h \
    isovistparser.h \
    exportparser.h \
    importparser.h \
    stepdepthparser.h \
    segmentparser.h \
    mapconvertparser.h

win32:Release:LIBS += -L../genlib/release -L../mgraph440/release -L../salalib/release
win32:Debug:LIBS += -L../genlib/debug -L../mgraph440/debug -L../salalib/debug
!win32:LIBS += -L../genlib -L../mgraph440 -L../salalib

LIBS += -lsalalib -lmgraph440 -lgenlib

mac {
    QMAKE_CC = /usr/local/opt/llvm/bin/clang
    QMAKE_CXX = /usr/local/opt/llvm/bin/clang++
    QMAKE_LINK = /usr/local/opt/llvm/bin/clang++

    QMAKE_CXXFLAGS += -fopenmp
    QMAKE_LFLAGS +=  -fopenmp
    QMAKE_LFLAGS += -L/usr/local/opt/llvm/lib/
    LIBS += -fopenmp
}

linux {
    QMAKE_CXXFLAGS += -fopenmp
    QMAKE_LFLAGS +=  -fopenmp
}
