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
WARNING_FLAGS = -Wall -Wsign-compare -Wno-invalid-offsetof

repsnapper_CPPFLAGS = \
	-I$(LIB_DIR)/vmmlib/include \
	-I$(LIB_DIR)/lmfit/lmfit-3.2/lib \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/slicer \
	-I$(top_builddir)/src \
	-I$(LIB_DIR) \
	-DHAVE_GTK $(CFLAGS) $(EXTRA_CFLAGS) -I$(LIB_DIR) $(GTKMM_CFLAGS) \
	$(LIBREPRAP_CFLAGS) \
        -DRSDATADIR='$(repsnapperdatadir)' \
        -DSYSCONFDIR='$(repsnapperconfdir)' \
	-DLOCALEDIR='"$(localedir)"' \
	$(XMLPP_CFLAGS) \
	$(OPENMP_CFLAGS) \
	$(LIBZIP_CFLAGS) \
	-g -O3 $(WARNING_FLAGS)

SHARED_SRC= \
	src/model.cpp \
	src/model2.cpp \
	src/view.cpp \
	src/gllight.cpp \
	src/arcball.cpp \
	src/objtree.cpp \
	src/shape.cpp \
	src/triangle.cpp \
	src/flatshape.cpp \
	src/files.cpp \
	src/filechooser.cpp \
	src/platform.cpp \
	src/render.cpp \
	src/reprap_serial.cpp \
	src/printer.cpp \
	src/printer2.cpp \
	src/progress.cpp \
	src/connectview.cpp \
	src/settings.cpp \
	src/settings-ui.cpp

SHARED_INC= \
	src/arcball.h \
	src/convert.h \
	src/gllight.h \
	src/miniball.h \
	src/model.h \
	src/objtree.h \
	src/shape.h \
	src/triangle.h \
	src/flatshape.h \
	src/files.h \
	src/filechooser.h \
	src/stdafx.h \
	src/platform.h \
	src/render.h \
	src/reprap_serial.h \
	src/printer.h \
	src/progress.h \
	src/connectview.h \
	src/settings.h \
	src/settings-ui.h \
	src/view.h \
	src/types.h

include src/slicer/Makefile.am
include src/gcode/Makefile.am

# TEST_SOURCES=unittest.cpp

repsnapper_SOURCES = $(SHARED_SRC) $(SHARED_INC) src/repsnapper.cpp

src/gitversion.h: FORCE
	$(AM_V_GEN)sh $(top_builddir)/tools/gitversion.sh $(top_builddir)/src/gitversion.h $(top_srcdir)/src/gitversion.h
FORCE:

built_header_make=src/gitversion.h

BUILT_SOURCES += $(built_header_make)
EXTRA_DIST += $(built_header_make) \
	src/printer_libreprap.cpp \
        src/printer_iochannel.cpp

repsnapper_LDFLAGS = $(EXTRA_LDFLAGS)

repsnapper_LDADD = libreprap.la libclipper.la libpoly2tri.la liblmfit.la libamf.la $(OPENMP_CFLAGS) $(OPENVRML_LIBS) $(GTKMM_LIBS) $(GL_LIBS) $(XMLPP_LIBS) $(LIBZIP_LIBS) $(BOOST_LDFLAGS)

repsnapperdatadir = $(datadir)/@PACKAGE@
dist_repsnapperdata_DATA = src/repsnapper.ui

repsnapperconfdir = $(sysconfdir)/xdg/@PACKAGE@
dist_repsnapperconf_DATA = src/repsnapper.conf
