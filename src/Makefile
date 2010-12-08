#
# Combined Makefile for Linux and OS/X
#
#
# Copyright 2009+ Joachim Glauche
#
# This file is part of RepSnapper and is made available under
# the terms of the GNU General Public License, version 2, or at your
# option, any later version, incorporated herein by reference.

CC=gcc
CXX=g++
LIB_DIR=../libraries
UNAME := $(shell uname)
ifeq ($(TARGET),)
	TARGET=RELEASE
endif
WARNING_FLAGS = -Wall

EXEC=repsnapper
EXEC_DEBUG=repsnapper_debug
ifeq ($(TARGET),RELEASE)
	CFLAGS ?= -O2 $(WARNING_FLAGS)
	EXECUTABLE=$(EXEC)
else
	CFLAGS ?= -g -O0 $(WARNING_FLAGS)
	EXECUTABLE=$(EXEC_DEBUG)
endif

# include SVN revision at compiletime
# see http://subversion.apache.org/faq.html#version-value-in-source
#VERSION := -D'VERSION="r$(shell svnversion -n .) $(shell uname) $(shell uname -m)"'
#CFLAGS += $(VERSION)

# Linux
ifeq ($(UNAME),Linux)
    GTK_LIBS=`pkg-config --libs gtk+-2.0 gthread-2.0`
    GTK_CFLAGS=`pkg-config --cflags gtk+-2.0 gthread-2.0` -DHAVE_GTK
    INC=$(GTK_CFLAGS) -I/usr/include -I$(LIB_DIR) -I$(LIB_DIR)/vmmlib/include -I/usr/include/boost -I/usr/include/lua5.1
    INC+=-I$(LIB_DIR)/polylib
    LDFLAGS=$(GTK_LIBS) -L/usr/lib -lGLU -lGL -lfltk -lfltk_gl -lfltk_forms -lglut -lboost_thread-mt -lboost_system-mt -lconfig++ 
    LDFLAGS+=-L$(LIB_DIR)/polylib -lpolylib
    TEST_LDFLAGS=-lboost_unit_test_framework
endif

# Mac
ifeq ($(UNAME),Darwin)
    # see notes in readme.macosx.txt
    MACPORTS_DIR=/opt/local
    INC=-I$(MACPORTS_DIR)/include -I$(LIB_DIR)/vmmlib/include -I$(LIB_DIR)
    INC+=-I$(LIB_DIR)/polylib
    INC+=-pthread
    LDFLAGS=-L$(MACPORTS_DIR)/lib -pthread -lfltk -lfltk_forms -lfltk_gl -lconfig++ -lboost_thread-mt -lboost_system-mt
    LDFLAGS+=-L$(LIB_DIR)/polylib -lpolylib
    LDFLAGS+=-framework Carbon -framework OpenGL -framework GLUT -framework AGL
    TEST_LDFLAGS=-lboost_unit_test_framework
endif

GENERATED=ui.cxx ui.h

MAIN_SOURCES=repsnapper.cpp
TEST_SOURCES=unittest.cpp
SHARED_SOURCES=asyncserial.cpp stl.cpp reprapserial.cpp \
	processcontroller.cpp printer.cpp modelviewcontroller.cpp \
	gllight.cpp gcode.cpp arcball.cpp stdafx.cpp ui.cxx \
	rfo.cpp flu_dnd.cpp flu_pixmaps.cpp flu_simplestring.cpp \
	flu_tree_browser.cpp ivcon.cpp file.cpp platform.cpp

HEADERS=arcball.h asyncserial.h convert.h flu_dnd.h flu_enumerations.h \
	flu_export.h flu_pixmaps.h flu_simplestring.h flu_tree_browser.h \
	gcode.h gllight.h ivcon.h miniball.h modelviewcontroller.h \
	printer.h processcontroller.h reprapserial.h rfo.h \
	stdafx.h stl.h triangle.h ui.h platform.h

SHARED_OBJECTS=$(subst .c,.o,$(subst .cxx,.o,$(subst .cpp,.o,$(SHARED_SOURCES))))
MAIN_OBJECTS=$(subst .c,.o,$(subst .cxx,.o,$(subst .cpp,.o,$(MAIN_SOURCES)))) $(SHARED_OBJECTS)
TEST_OBJECTS=$(subst .c,.o,$(subst .cxx,.o,$(subst .cpp,.o,$(TEST_SOURCES)))) $(SHARED_OBJECTS)


all: $(EXECUTABLE)

$(EXECUTABLE): poly_lib $(MAIN_OBJECTS)
	$(CXX) ${INC} $(MAIN_OBJECTS) $(LDFLAGS) -o $@

unittest : poly_lib $(TEST_OBJECTS)
	$(CXX) ${INC} $(TEST_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) -o $@

%.cxx %.h:%.fl
	rm -f $@ # fluid doesn't remove on failure.
	fluid -c $<
%.o:%.cxx
	$(CXX) ${INC} -c $(CFLAGS) $< -o $@
%.o:%.cpp
	$(CXX) ${INC} -c $(CFLAGS) $< -o $@
%.o:%.c
	$(CC) ${INC} -c $(CFLAGS) $< -o $@

poly_lib:
	make -C ../libraries/polylib/ all

check: unittest
	./unittest
#	cd ../test ; python ./rpstest.py ../Src/repsnapper

clean:
	rm -f $(SHARED_OBJECTS) $(MAIN_OBJECTS) $(TEST_OBJECTS) \
	      $(EXEC) $(EXEC_DEBUG) $(GENERATED) unittest
	make -i -C ../libraries/polylib/ clean
	rm -rf repsnapper.app

# make update-deps will re-write the dependenciues below
update-depends:
	makedepend -Y $(SHARED_SOURCES) $(MAIN_SOURCES) $(TEST_SOURCES)

# need to include: jpeg zlib fltk libpng glut libconfig-hr bzip2 expat
osx-bundle: all
	mkdir -p repsnapper.app/Contents/
	mkdir repsnapper.app/Contents/MacOS/
	mkdir repsnapper.app/Contents/Resources/
	mkdir repsnapper.app/Contents/Frameworks/
	mkdir repsnapper.app/Contents/lib/
	cp Info.plist repsnapper.app/Contents/
	cp repsnapper repsnapper.app/Contents/
	cp repsnapper-osx.sh repsnapper.app/Contents/
	cp /opt/local/lib/libboost* repsnapper.app/Contents/lib/
	cp /opt/local/lib/libconfig* repsnapper.app/Contents/lib/
	cp /opt/local/lib/libz* repsnapper.app/Contents/lib/
	cp /opt/local/lib/libfltk* repsnapper.app/Contents/lib/
	cp /opt/local/lib/libpng* repsnapper.app/Contents/lib/
	cp /opt/local/lib/libexpat* repsnapper.app/Contents/lib/
	cp /opt/local/lib/libbz2* repsnapper.app/Contents/lib/
	cp /opt/local/lib/libjpeg* repsnapper.app/Contents/lib/
	rm repsnapper.app/Contents/lib/libboost*.a

# not needed
#	<Kulitorum> fillet.cpp
#	<Kulitorum> pathfinder.* (will someday keep the extruder inside the object for move commands, to avoid ooze)
#	<Kulitorum> reprapusb.h
#	<Kulitorum> search.*
#	<Kulitorum> simulator.*

# DO NOT DELETE THIS LINE -- make depend depends on it, deletes after here and updates the deps.

asyncserial.o: stdafx.h config.h platform.h arcball.h ivcon.h asyncserial.h
repsnapper.o: stdafx.h config.h platform.h arcball.h ivcon.h
repsnapper.o: modelviewcontroller.h ui.h file.h flu_tree_browser.h
repsnapper.o: flu_enumerations.h flu_export.h flu_simplestring.h gcode.h stl.h
repsnapper.o: processcontroller.h printer.h rfo.h gllight.h reprapserial.h
repsnapper.o: asyncserial.h
stl.o: stdafx.h config.h platform.h arcball.h ivcon.h stl.h gcode.h ui.h
stl.o: file.h modelviewcontroller.h processcontroller.h printer.h rfo.h
stl.o: flu_tree_browser.h flu_enumerations.h flu_export.h flu_simplestring.h
stl.o: gllight.h reprapserial.h asyncserial.h
reprapserial.o: stdafx.h config.h platform.h arcball.h ivcon.h reprapserial.h
reprapserial.o: ui.h file.h modelviewcontroller.h gcode.h stl.h
reprapserial.o: processcontroller.h printer.h rfo.h flu_tree_browser.h
reprapserial.o: flu_enumerations.h flu_export.h flu_simplestring.h gllight.h
reprapserial.o: asyncserial.h convert.h
processcontroller.o: stdafx.h config.h platform.h arcball.h ivcon.h
processcontroller.o: modelviewcontroller.h ui.h file.h flu_tree_browser.h
processcontroller.o: flu_enumerations.h flu_export.h flu_simplestring.h
processcontroller.o: gcode.h stl.h processcontroller.h printer.h rfo.h
processcontroller.o: gllight.h reprapserial.h asyncserial.h
printer.o: stdafx.h config.h platform.h arcball.h ivcon.h processcontroller.h
printer.o: printer.h gcode.h rfo.h flu_tree_browser.h flu_enumerations.h
printer.o: flu_export.h flu_simplestring.h stl.h
modelviewcontroller.o: stdafx.h config.h platform.h arcball.h ivcon.h
modelviewcontroller.o: modelviewcontroller.h ui.h file.h flu_tree_browser.h
modelviewcontroller.o: flu_enumerations.h flu_export.h flu_simplestring.h
modelviewcontroller.o: gcode.h stl.h processcontroller.h printer.h rfo.h
modelviewcontroller.o: gllight.h reprapserial.h asyncserial.h
gllight.o: stdafx.h config.h platform.h arcball.h ivcon.h gllight.h
gcode.o: stdafx.h config.h platform.h arcball.h ivcon.h gcode.h ui.h file.h
gcode.o: modelviewcontroller.h stl.h processcontroller.h printer.h rfo.h
gcode.o: flu_tree_browser.h flu_enumerations.h flu_export.h flu_simplestring.h
gcode.o: gllight.h reprapserial.h asyncserial.h
arcball.o: stdafx.h config.h platform.h arcball.h ivcon.h
stdafx.o: stdafx.h config.h platform.h arcball.h ivcon.h stl.h
ui.o: ui.h config.h stdafx.h platform.h arcball.h ivcon.h file.h
ui.o: modelviewcontroller.h gcode.h stl.h processcontroller.h printer.h rfo.h
ui.o: flu_tree_browser.h flu_enumerations.h flu_export.h flu_simplestring.h
ui.o: gllight.h reprapserial.h asyncserial.h
rfo.o: stdafx.h config.h platform.h arcball.h ivcon.h ui.h file.h
rfo.o: modelviewcontroller.h gcode.h stl.h processcontroller.h printer.h
rfo.o: rfo.h flu_tree_browser.h flu_enumerations.h flu_export.h
rfo.o: flu_simplestring.h gllight.h reprapserial.h asyncserial.h flu_pixmaps.h
flu_dnd.o: flu_dnd.h flu_enumerations.h flu_export.h
flu_pixmaps.o: flu_pixmaps.h flu_export.h
flu_simplestring.o: flu_simplestring.h flu_enumerations.h flu_export.h
flu_tree_browser.o: flu_tree_browser.h flu_enumerations.h flu_export.h
flu_tree_browser.o: flu_simplestring.h flu_pixmaps.h
ivcon.o: ivcon.h
file.o: stdafx.h config.h platform.h arcball.h ivcon.h file.h
file.o: modelviewcontroller.h ui.h flu_tree_browser.h flu_enumerations.h
file.o: flu_export.h flu_simplestring.h gcode.h stl.h processcontroller.h
file.o: printer.h rfo.h gllight.h reprapserial.h asyncserial.h
