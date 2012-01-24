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
WARNING_FLAGS = -Wall -Wno-invalid-offsetof

repsnapper_CPPFLAGS = \
	-I$(LIB_DIR)/vmmlib/include \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/slicer \
	-I$(LIB_DIR)/polylib \
	-I$(LIB_DIR) \
	-DHAVE_GTK $(CFLAGS) $(EXTRA_CFLAGS) -I$(LIB_DIR) $(GTKMM_CFLAGS) \
	$(LIBREPRAP_CFLAGS) \
        -DRSDATADIR='$(repsnapperdatadir)' \
        -DSYSCONFDIR='$(repsnapperconfdir)' \
	-DLOCALEDIR='"$(localedir)"' \
	$(OPENMP_CFLAGS) \
	-g -O3 $(WARNING_FLAGS)

SHARED_SRC= \
	src/model.cpp \
	src/model2.cpp \
	src/view.cpp \
	src/gllight.cpp \
	src/gcode.cpp \
	src/arcball.cpp \
	src/objtree.cpp \
	src/shape.cpp \
	src/file.cpp \
	src/platform.cpp \
	src/render.cpp \
	src/printer.cpp \
	src/printer2.cpp \
	src/progress.cpp \
	src/connectview.cpp \
	src/settings.cpp 

SHARED_INC= \
	src/arcball.h \
	src/convert.h \
	src/gcode.h \
	src/gllight.h \
	src/miniball.h \
	src/model.h \
	src/objtree.h \
	src/shape.h \
	src/file.h \
	src/stdafx.h \
	src/platform.h \
	src/render.h \
	src/printer.h \
	src/progress.h \
	src/connectview.h \
	src/settings.h \
	src/view.h \
	src/types.h 

include src/slicer/Makefile.am

# TEST_SOURCES=unittest.cpp

repsnapper_SOURCES = $(SHARED_SRC) $(SHARED_INC) src/repsnapper.cpp

repsnapper_LDFLAGS = $(EXTRA_LDFLAGS)

repsnapper_LDADD = libpolylib.la libreprap.la libclipper.la $(OPENMP_CFLAGS) $(OPENVRML_LIBS) $(GTKMM_LIBS) $(GL_LIBS) $(BOOST_LDFLAGS)

repsnapperdatadir = $(datadir)/@PACKAGE@
dist_repsnapperdata_DATA = src/repsnapper.ui

repsnapperconfdir = $(sysconfdir)/xdg/@PACKAGE@
dist_repsnapperconf_DATA = src/repsnapper.conf
