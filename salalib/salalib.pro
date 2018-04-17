#-------------------------------------------------
#
# Project created by QtCreator 2017-02-09T16:19:46
#
#-------------------------------------------------
include(../defaults.pri)


QT       -= qt
CONFIG   -= qt
CONFIG   -= app_bundle
DEFINES       += _DEPTHMAP
TARGET = salalib
TEMPLATE = lib
CONFIG        += staticlib c++11

DEFINES += SALALIB_LIBRARY

SOURCES += \
    attributes.cpp \
    axialmap.cpp \
    connector.cpp \
    datalayer.cpp \
    isovist.cpp \
    MapInfoData.cpp \
    mgraph.cpp \
    nagent.cpp \
    ngraph.cpp \
    ntfp.cpp \
    pointdata.cpp \
    salaprogram.cpp \
    shapemap.cpp \
    spacepix.cpp \
    sparksieve2.cpp \
    tigerp.cpp \
    topomet.cpp \
    entityparsing.cpp \
    linkutils.cpp \
    gridproperties.cpp \
    geometrygenerators.cpp \
    importutils.cpp \
    point.cpp \
    pafcolor.cpp

HEADERS += \
    attributes.h \
    axialmap.h \
    connector.h \
    datalayer.h \
    fileproperties.h \
    isovist.h \
    MapInfoData.h \
    mgraph.h \
    nagent.h \
    ngraph.h \
    ntfp.h \
    pointdata.h \
    salaprogram.h \
    shapemap.h \
    spacepix.h \
    sparksieve2.h \
    tigerp.h \
    topomet.h \
    entityparsing.h \
    linkutils.h \
    gridproperties.h \
    isovistdef.h \
    mgraph_consts.h \
    geometrygenerators.h \
    importutils.h \
    importtypedefs.h \
    point.h \
    pixelref.h \
    displayparams.h \
    pafcolor.h \
    options.h

DISTFILES += \
    salascript-tests.txt

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
