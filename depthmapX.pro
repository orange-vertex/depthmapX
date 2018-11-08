TEMPLATE = subdirs
CONFIG+=ordered
SUBDIRS = \
    mgraph440 \
    genlib \
    salalib \
    depthmapX \
    GuiUnitTest \
    GuiApp \
    depthmapXcli \
    cliTest \
    salaTest \
    genlibTest \
    depthmapXTest

msvc {
  QMAKE_CXXFLAGS += -openmp
}

linux {
    QMAKE_CXXFLAGS += -fopenmp
    QMAKE_LFLAGS +=  -fopenmp
}

mac {
    QMAKE_CC = /usr/local/opt/llvm/bin/clang
    QMAKE_CXX = /usr/local/opt/llvm/bin/clang++
    QMAKE_LINK = /usr/local/opt/llvm/bin/clang++

    QMAKE_CXXFLAGS += -fopenmp
    QMAKE_LFLAGS +=  -fopenmp
    QMAKE_LFLAGS += -L/usr/local/opt/llvm/lib/
    LIBS += -fopenmp
}

GuiApp.depends = depthmapX genlib mgraph440 salalib
