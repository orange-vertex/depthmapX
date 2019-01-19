include(../defaults.pri)
include(dialogs/dialogs.pri)
include(views/views.pri)

QT            += core gui opengl widgets
DEFINES       += _DEPTHMAP
TEMPLATE      = app
TARGET        = depthmapX
ICON       = icons/depthmapX.icns
HEADERS       += GraphDoc.h \
                indexWidget.h \
                mainwindow.h \
                mdichild.h \
                treeWindow.h \
    compatibilitydefines.h \
    mainwindowfactory.h \
    version.h \
    settings.h \
    settingsimpl.h \
    coreapplication.h

SOURCES       += GraphDoc.cpp \
                indexWidget.cpp \
                mainwindow.cpp \
                mdichild.cpp \
                renderthread.cpp \
                treeWindow.cpp \
    mainwindowfactory.cpp \
    settingsimpl.cpp \
    coreapplication.cpp \
    main.cpp


win32:RC_ICONS += icons/depthmapX.ico

win32:Release:LIBS += -L../depthmapX/release -L../genlib/release -L../mgraph440/release  -L../salalib/release
win32:Debug:LIBS += -L../depthmapX/debug -L../genlib/debug -L../mgraph440/debug -L../salalib/debug
!win32:LIBS += -L../depthmapX -L../genlib -L../mgraph440 -L../salalib

LIBS += -ldepthmapX -lsalalib -lmgraph440 -lgenlib

!win32:!macx:LIBS += -L/usr/lib/i386-linux-gnu/

!win32:!macx:LIBS += -lGL -lGLU


win32:LIBS += -lOpenGl32 -lglu32 -lgdi32


RESOURCES     += resource.qrc

OTHER_FILES += \
    Libs/include/generic/lgpl.txt

QMAKE_CXXFLAGS_WARN_ON =

FORMS += \
    UI/TopoMetDlg.ui \
    UI/SegmentAnalysisDlg.ui \
    UI/RenameObjectDlg.ui \
    UI/PushDialog.ui \
    UI/PromptReplace.ui \
    UI/OptionsDlg.ui \
    UI/NewLayerDlg.ui \
    UI/MakeOptionsDlg.ui \
    UI/MakeLayerDlg.ui \
    UI/LicenceDialog.ui \
    UI/LayerChooserDlg.ui \
    UI/IsovistPathDlg.ui \
    UI/InsertColumnDlg.ui \
    UI/GridDialog.ui \
    UI/FindLocDlg.ui \
    UI/FilePropertiesDlg.ui \
    UI/FewestLineOptionsDlg.ui \
    UI/EditConnectionsDlg.ui \
    UI/DepthmapOptionsDlg.ui \
    UI/DepthmapAlert.ui \
    UI/ConvertShapesDlg.ui \
    UI/ColumnPropertiesDlg.ui \
    UI/ColourScaleDlg.ui \
    UI/AxialAnalysisOptionsDlg.ui \
    UI/AttributeSummary.ui \
    UI/AttributeChooserDlg.ui \
    UI/AgentAnalysisDlg.ui \
    UI/AboutDlg.ui \
    UI/licenseagreement.ui

mac {
    QMAKE_INFO_PLIST = resources/Info.plist
    BUNDLE_RESOURCES.files = icons/graph.icns
    BUNDLE_RESOURCES.path = Contents/Resources
    QMAKE_BUNDLE_DATA += BUNDLE_RESOURCES
}

win32: system(make_version_header.bat)
!win32: system(sh ./make_version_header.sh)
