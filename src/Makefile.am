#
# Combined Makefile for Linux and OS/X
#
#
# Copyright 2009+ Joachim Glauche
#
# This file is part of RepSnapper and is made available under
# the terms of the GNU General Public License, version 2, or at your
# option, any later version, incorporated herein by reference.

bin_PROGRAMS = repsnapper

LIB_DIR=$(top_srcdir)/libraries
WARNING_FLAGS = -Wall

AM_CPPFLAGS = \
	-DHAVE_GTK $(GTK_CFLAGS) -I$(LIB_DIR) \
	-I$(LIB_DIR)/vmmlib/include \
	-I$(top_srcdir) \
	-I/usr/include/boost \
	-I/usr/include/lua5.1 \
	-I$(LIB_DIR)/polylib \
	-g -O0 $(WARNING_FLAGS)

SHARED_SRC=asyncserial.cpp stl.cpp reprapserial.cpp \
	processcontroller.cpp printer.cpp modelviewcontroller.cpp \
	gllight.cpp gcode.cpp arcball.cpp stdafx.cpp \
	rfo.cpp flu_dnd.cpp flu_pixmaps.cpp flu_simplestring.cpp \
	flu_tree_browser.cpp ivcon.cpp file.cpp platform.cpp \
	ui.cxx view.cxx progress.cxx connectview.cpp

SHARED_INC=arcball.h asyncserial.h convert.h flu_dnd.h flu_enumerations.h \
	flu_export.h flu_pixmaps.h flu_simplestring.h flu_tree_browser.h \
	gcode.h gllight.h ivcon.h miniball.h modelviewcontroller.h \
	printer.h processcontroller.h reprapserial.h rfo.h \
	stdafx.h stl.h triangle.h platform.h view.h progress.h \
	connectview.h

# TEST_SOURCES=unittest.cpp

repsnapper_SOURCES = $(SHARED_SRC) $(SHARED_INC) repsnapper.cpp
repsnapper_LDADD = $(GTK_LIBS) -L/usr/lib -lGLU -lGL -lfltk -lfltk_gl \
		-lfltk_forms -lglut -lboost_thread-mt \
		-lboost_system-mt -lconfig++ \
	       $(top_builddir)/libraries/polylib/libpolylib.a


