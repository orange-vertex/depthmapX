include(../defaults.pri)
TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
INCLUDEPATH += ../ThirdParty/Catch ../ThirdParty/FakeIt

SOURCES += main.cpp \
    ../depthmapXcli/commandlineparser.cpp \
    testcommandlineparser.cpp \
    testradiusconverter.cpp \
    ../depthmapXcli/radiusconverter.cpp \
    testsimpletimer.cpp \
    testvgaparser.cpp \
    ../depthmapXcli/vgaparser.cpp \
    testlinkparser.cpp \
    ../depthmapXcli/linkparser.cpp \
    testagentparser.cpp \
    ../depthmapXcli/agentparser.cpp \
    testargumentholder.cpp \
    ../depthmapXcli/performancewriter.cpp \
    testperformancewriter.cpp \
    testselfcleaningfile.cpp \
    ../depthmapXcli/runmethods.cpp \
    ../depthmapXcli/modeparserregistry.cpp \
    testvisprepparser.cpp \
    ../depthmapXcli/visprepparser.cpp \
    testaxialparser.cpp \
    ../depthmapXcli/axialparser.cpp \
    testparsingutils.cpp \
    ../depthmapXcli/parsingutils.cpp \
    testisovistparser.cpp \
    ../depthmapXcli/isovistparser.cpp \
    testexportparser.cpp \
    ../depthmapXcli/exportparser.cpp \
    ../depthmapXcli/importparser.cpp \
    testimportparser.cpp \
    ../depthmapXcli/stepdepthparser.cpp \
    teststepdepthparser.cpp


HEADERS += \
    ../depthmapXcli/commandlineparser.h \
    ../depthmapXcli/radiusconverter.h \
    ../depthmapXcli/simpletimer.h \
    ../depthmapXcli/vgaparser.h \
    ../depthmapXcli/linkparser.h \
    ../depthmapXcli/agentparser.h \
    ../depthmapXcli/permformancewriter.h \
    argumentholder.h \
    selfcleaningfile.h

win32:Release:LIBS += -L../genlib/release -L../mgraph440/release -L../salalib/release
win32:Debug:LIBS += -L../genlib/debug -L../mgraph440/debug -L../salalib/debug
!win32:LIBS += -L../genlib -L../mgraph440 -L../salalib

LIBS += -lsalalib -lmgraph440 -lgenlib


## comment this out if you need a different version of R,
## and set set R_HOME accordingly as an environment variable
R_HOME = 		$$system(R RHOME)
#message("R_HOME is" $$R_HOME)

## include headers and libraries for R
RCPPFLAGS = 		$$system($$R_HOME/bin/R CMD config --cppflags)
RLDFLAGS = 		$$system($$R_HOME/bin/R CMD config --ldflags)
RBLAS = 		$$system($$R_HOME/bin/R CMD config BLAS_LIBS)
RLAPACK = 		$$system($$R_HOME/bin/R CMD config LAPACK_LIBS)

## if you need to set an rpath to R itself, also uncomment
RRPATH =		-Wl,-rpath,$$R_HOME/lib

## include headers and libraries for Rcpp interface classes
## note that RCPPLIBS will be empty with Rcpp (>= 0.11.0) and can be omitted
RCPPINCL = 		$$system($$R_HOME/bin/Rscript -e \"Rcpp:::CxxFlags\(\)\")
RCPPLIBS = 		$$system($$R_HOME/bin/Rscript -e \"Rcpp:::LdFlags\(\)\")

## include headers and libraries for RInside embedding classes
RINSIDEINCL = 		$$system($$R_HOME/bin/Rscript -e \"RInside:::CxxFlags\(\)\")
RINSIDELIBS = 		$$system($$R_HOME/bin/Rscript -e \"RInside:::LdFlags\(\)\")

## compiler etc settings used in default make rules
QMAKE_CXXFLAGS +=	$$RCPPWARNING $$RCPPFLAGS $$RCPPINCL $$RINSIDEINCL
QMAKE_LIBS +=           $$RLDFLAGS $$RBLAS $$RLAPACK $$RINSIDELIBS $$RCPPLIBS


mac {
    QMAKE_CXXFLAGS += -fopenmp
    QMAKE_LFLAGS +=  -fopenmp
    LIBS += -fopenmp
}

linux {
    QMAKE_CXXFLAGS += -fopenmp
    QMAKE_LFLAGS +=  -fopenmp
}

