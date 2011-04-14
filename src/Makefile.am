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

AM_CPPFLAGS = \
	-DHAVE_GTK $(CFLAGS) -I$(LIB_DIR) $(GTKMM_CFLAGS) \
        $(LIBREPRAP_CFLAGS) \
	-I$(LIB_DIR)/vmmlib/include \
	-I$(top_srcdir) \
	-I$(LIB_DIR)/polylib \
        -DDATADIR='$(repsnapperdatadir)' \
        -DSYSCONFDIR='$(repsnapperconfdir)' \
	-g -O0 $(WARNING_FLAGS)

SHARED_SRC= stl.cpp model.cpp view.cpp gllight.cpp gcode.cpp arcball.cpp \
	stdafx.cpp rfo.cpp file.cpp platform.cpp render.cpp	\
	progress.cpp connectview.cpp settings.cpp

SHARED_INC=arcball.h convert.h gcode.h gllight.h miniball.h	\
	model.h rfo.h file.h stdafx.h stl.h platform.h render.h		\
	progress.h connectview.h settings.h

# TEST_SOURCES=unittest.cpp

repsnapper_SOURCES = $(SHARED_SRC) $(SHARED_INC) repsnapper.cpp

repsnapper_LDADD = $(GTKMM_LIBS) $(LIBREPRAP_LIBS) \
		$(top_builddir)/libraries/polylib/libpolylib.la \
                -L/usr/lib -lGLU -lGL -lglut

repsnapperdatadir = $(datadir)/@PACKAGE@
dist_repsnapperdata_DATA = repsnapper.ui

repsnapperconfdir = $(sysconfdir)/xdg/@PACKAGE@
dist_repsnapperconf_DATA = repsnapper.conf
