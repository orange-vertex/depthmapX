TEMPLATE = subdirs
CONFIG+=ordered
SUBDIRS = \
    mgraph440 \
    genlib \
    salalib \
    settingsdialog \
    depthmapX \
    GuiUnitTest \
    GuiApp \
    depthmapXcli \
    cliTest \
    salaTest \
    genlibTest \
    depthmapXTest
GuiApp.depends = depthmapX genlib mgraph440 salalib settingsdialog

mac {
    QMAKE_CC = /usr/local/opt/llvm/bin/clang
    QMAKE_CXX = /usr/local/opt/llvm/bin/clang++
    QMAKE_LINK = /usr/local/opt/llvm/bin/clang++

    QMAKE_CXXFLAGS += -fopenmp
    QMAKE_LFLAGS +=  -fopenmp
    QMAKE_LFLAGS += -L/usr/local/opt/llvm/lib/
    LIBS += -fopenmp
}
