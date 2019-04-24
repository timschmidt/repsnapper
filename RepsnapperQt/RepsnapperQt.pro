#-------------------------------------------------
#
# Project created by QtCreator 2019-04-22T16:24:56
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

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
        ../src/arcball.cpp \
        ../src/files.cpp \
        ../src/flatshape.cpp \
        ../src/gcode/command.cpp \
        ../src/gcode/gcode.cpp \
        ../src/gcode/gcodestate.cpp \
        ../src/gllight.cpp \
        ../src/glshader.cpp \
        ../src/model.cpp \
        ../src/model_slice.cpp \
        ../src/objlist.cpp \
        ../src/platform.cpp \
        ../src/pretty-print-vrml.cpp \
        ../src/printer/custom_baud.cpp \
        ../src/printer/printer.cpp \
        ../src/printer/printer_serial.cpp \
        ../src/printer/printer_serial_test.cpp \
        ../src/printer/printrun.cpp \
        ../src/printer/thread_buffer.cpp \
        ../src/printer/thread_buffer_test.cpp \
        ../src/printer/threaded_printer_serial.cpp \
        ../src/printer/threaded_printer_serial_test.cpp \
        ../src/render.cpp \
        ../src/repsnapper.cpp \
        ../src/settings.cpp \
        ../src/shape.cpp \
        ../src/slicer/clipping.cpp \
        ../src/slicer/geometry.cpp \
        ../src/slicer/infill.cpp \
        ../src/slicer/layer.cpp \
        ../src/slicer/poly.cpp \
        ../src/slicer/printlines.cpp \
        ../src/slicer/printlines_antiooze.cpp \
        ../src/transform3d.cpp \
        ../src/triangle.cpp \
        ../src/ui/connectview.cpp \
        ../src/ui/draw.cpp \
        ../src/ui/filechooser.cpp \
        ../src/ui/prefs_dlg.cpp \
        ../src/ui/progress.cpp \
        ../src/ui/view.cpp \
        ../src/ui/widgets.cpp \
        ../src/unittest.cpp \
        ../src/vrml.cpp \
        main.cpp \
        mainwindow.cpp

HEADERS += \
        ../src/arcball.h \
        ../src/files.h \
        ../src/flatshape.h \
        ../src/gcode/command.h \
        ../src/gcode/gcode.h \
        ../src/gcode/gcodestate.h \
        ../src/gitversion.h \
        ../src/gllight.h \
        ../src/glshader.h \
        ../src/librepsnapper/vmmlib/include/vmmlib/blas_daxpy.hpp \
        ../src/librepsnapper/vmmlib/include/vmmlib/cublas_includes.hpp \
        ../src/librepsnapper/vmmlib/include/vmmlib/cublas_types.hpp \
        ../src/librepsnapper/vmmlib/include/vmmlib/t3_converter.hpp \
        ../src/librepsnapper/vmmlib/include/vmmlib/t3_cublas_hooi.hpp \
        ../src/librepsnapper/vmmlib/include/vmmlib/t3_cublas_hosvd.hpp \
        ../src/librepsnapper/vmmlib/include/vmmlib/t3_cublas_ttm.hpp \
        ../src/librepsnapper/vmmlib/include/vmmlib/t3_ttm.hpp \
        ../src/librepsnapper/vmmlib/include/vmmlib/validator.hpp \
        ../src/linked_ptr.h \
        ../src/miniball.h \
        ../src/model.h \
        ../src/objlist.h \
        ../src/platform.h \
        ../src/printer/custom_baud.h \
        ../src/printer/printer.h \
        ../src/printer/printer_serial.h \
        ../src/printer/printrun.h \
        ../src/printer/thread.h \
        ../src/printer/thread_buffer.h \
        ../src/printer/threaded_printer_serial.h \
        ../src/render.h \
        ../src/settings.h \
        ../src/shape.h \
        ../src/slicer/clipping.h \
        ../src/slicer/geometry.h \
        ../src/slicer/infill.h \
        ../src/slicer/layer.h \
        ../src/slicer/poly.h \
        ../src/slicer/printlines.h \
        ../src/stdafx.h \
        ../src/transform3d.h \
        ../src/triangle.h \
        ../src/types.h \
        ../src/ui/connectview.h \
        ../src/ui/draw.h \
        ../src/ui/filechooser.h \
        ../src/ui/prefs_dlg.h \
        ../src/ui/progress.h \
        ../src/ui/view.h \
        ../src/ui/widgets.h \
        ../src/vrml.h \
        mainwindow.h

FORMS += \
        mainwindow.ui \
        preferences_dlg.ui

INCLUDEPATH += ../
INCLUDEPATH += ../libraries/vmmlib/include/
INCLUDEPATH += ../libraries/

INCLUDEPATH += /usr/local/include
INCLUDEPATH += /usr/local/include/giomm-2.4/
INCLUDEPATH += /usr/local/include/glibmm-2.4/
INCLUDEPATH += /usr/local/lib/glibmm-2.4/include
INCLUDEPATH += /usr/local/include/glib-2.0/
INCLUDEPATH += /usr/local/lib/glib-2.0/include
INCLUDEPATH += /usr/local/include/gtkmm-2.4/
INCLUDEPATH += /usr/local/include/sigc++-2.0/
INCLUDEPATH += /usr/local/lib/sigc++-2.0/include/
INCLUDEPATH += /usr/local/include/cairomm-1.0/
INCLUDEPATH += /usr/local/lib/cairomm-1.0/include/
INCLUDEPATH += /usr/local/include/cairo/
INCLUDEPATH += /usr/local/include/freetype2/
INCLUDEPATH += /usr/local/include/libxml++-2.6/
INCLUDEPATH += /usr/local/include/libxml++-2.6/include/

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
