#-------------------------------------------------
#
# Project created by QtCreator 2019-04-22T16:24:56
#
#-------------------------------------------------

QT       += core gui widgets serialport

lessThan(QT_MAJOR_VERSION, 5): error("requires Qt 5.12")
lessThan(QT_MINOR_VERSION, 12): error("requires Qt 5.12")

TARGET = RepsnapperQt
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
    ../libraries/clipper/clipper/polyclipping-code/cpp/clipper.cpp \
    ../libraries/lmfit/lmfit-5.0/lib/lmcurve.c \
    ../libraries/lmfit/lmfit-5.0/lib/lmmin.c \
    ../libraries/poly2tri/poly2tri/poly2tri/common/shapes.cc \
    ../libraries/poly2tri/poly2tri/poly2tri/sweep/advancing_front.cc \
    ../libraries/poly2tri/poly2tri/poly2tri/sweep/cdt.cc \
    ../libraries/poly2tri/poly2tri/poly2tri/sweep/sweep.cc \
    ../libraries/poly2tri/poly2tri/poly2tri/sweep/sweep_context.cc \
        ../src/arcball.cpp \
        ../src/files.cpp \
#        ../src/flatshape.cpp \
        ../src/gcode/command.cpp \
        ../src/gcode/gcode.cpp \
        ../src/gcode/gcodestate.cpp \
        ../src/gllight.cpp \
        ../src/model.cpp \
        ../src/model_slice.cpp \
        ../src/objlist.cpp \
        ../src/platform.cpp \
        ../src/printer/printer.cpp \
        ../src/render.cpp \
        ../src/settings.cpp \
        ../src/shape.cpp \
    ../src/slicer/antiooze.cpp \
        ../src/slicer/clipping.cpp \
        ../src/slicer/geometry.cpp \
        ../src/slicer/infill.cpp \
        ../src/slicer/infillpattern.cpp \
        ../src/slicer/layer.cpp \
        ../src/slicer/poly.cpp \
        ../src/slicer/printlines.cpp \
        ../src/transform3d.cpp \
        ../src/triangle.cpp \
        ../src/ui/prefs_dlg.cpp \
        ../src/ui/progress.cpp \
        main.cpp \
        mainwindow.cpp

HEADERS += \
    ../libraries/clipper/clipper/polyclipping-code/cpp/clipper.hpp \
    ../libraries/lmfit/lmfit-5.0/lib/lmcurve.h \
    ../libraries/lmfit/lmfit-5.0/lib/lmmin.h \
    ../libraries/lmfit/lmfit-5.0/lib/lmstruct.h \
    ../libraries/poly2tri/poly2tri/poly2tri/common/shapes.h \
    ../libraries/poly2tri/poly2tri/poly2tri/common/utils.h \
    ../libraries/poly2tri/poly2tri/poly2tri/poly2tri.h \
    ../libraries/poly2tri/poly2tri/poly2tri/sweep/advancing_front.h \
    ../libraries/poly2tri/poly2tri/poly2tri/sweep/cdt.h \
    ../libraries/poly2tri/poly2tri/poly2tri/sweep/sweep.h \
    ../libraries/poly2tri/poly2tri/poly2tri/sweep/sweep_context.h \
        ../src/arcball.h \
        ../src/files.h \
#        ../src/flatshape.h \
        ../src/gcode/command.h \
        ../src/gcode/gcode.h \
        ../src/gcode/gcodestate.h \
        ../src/gllight.h \
        ../src/miniball.h \
        ../src/model.h \
        ../src/objlist.h \
        ../src/platform.h \
        ../src/printer/custom_baud.h \
        ../src/printer/printer.h \
        ../src/render.h \
        ../src/settings.h \
        ../src/shape.h \
    ../src/slicer/antiooze.h \
        ../src/slicer/clipping.h \
        ../src/slicer/geometry.h \
        ../src/slicer/infill.h \
        ../src/slicer/infillpattern.h \
        ../src/slicer/layer.h \
        ../src/slicer/poly.h \
        ../src/slicer/printlines.h \
        ../src/stdafx.h \
        ../src/transform3d.h \
        ../src/triangle.h \
        ../src/types.h \
        ../src/ui/prefs_dlg.h \
        ../src/ui/progress.h \
        ../src/ui/widgets.h \
        mainwindow.h

FORMS += \
        temperaturepanel.ui \
        mainwindow.ui \
        preferences_dlg.ui

INCLUDEPATH += ../
INCLUDEPATH += ../libraries/vmmlib/include/
INCLUDEPATH += ../libraries/

QMAKE_CFLAGS += -fopenmp
QMAKE_CXXFLAGS += -fopenmp
LIBS += -fopenmp

CONFIG += link_pkgconfig

PKGCONFIG += glu
# glib-2.0
# glibmm-2.4 libxml++-2.6
#gstreamer-1.0


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /usr/local/bin
!isEmpty(target.path): INSTALLS += target

#QMAKE_CXX="ccache clang++80"
