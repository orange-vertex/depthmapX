include(../defaults.pri)

QT         += core gui opengl widgets
DEFINES    += _DEPTHMAP
TEMPLATE   = app
TARGET     = depthmapX
ICON       = icons/depthmapX.icns
SOURCES    = main.cpp \
    coreapplication.cpp

win32:RC_ICONS += icons/depthmapX.ico

win32:Release:LIBS += -L../depthmapX/release -L../genlib/release -L../mgraph440/release  -L../salalib/release -L../settingsdialog/release
win32:Debug:LIBS += -L../depthmapX/debug -L../genlib/debug -L../mgraph440/debug -L../salalib/debug -L../settingsdialog/debug
!win32:LIBS += -L../depthmapX -L../genlib -L../mgraph440 -L../salalib -L../settingsdialog

LIBS += -ldepthmapX -lsalalib -lmgraph440 -lgenlib -lsettingsdialog

!win32:!macx:LIBS += -L/usr/lib/i386-linux-gnu/

!win32:!macx:LIBS += -lGL -lGLU


win32:LIBS += -lOpenGl32 -lglu32 -lgdi32

HEADERS += \
    coreapplication.h

mac {
    QMAKE_INFO_PLIST = resources/Info.plist
    BUNDLE_RESOURCES.files = icons/graph.icns
    BUNDLE_RESOURCES.path = Contents/Resources
    QMAKE_BUNDLE_DATA += BUNDLE_RESOURCES
}

FORMS += \
    ../depthmapX/UI/ColourScaleDlg.ui


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
