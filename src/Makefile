#
# Combined Makefile for Linux and OS/X
#

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
    LDFLAGS=$(GTK_LIBS) -L/usr/lib -lGLU -lGL -lfltk -lfltk_gl -lfltk_forms -lglut -lboost_thread-mt -lboost_system-mt 
    LDFLAGS+=-L$(LIB_DIR)/polylib -lpolylib
    TEST_LDFLAGS=-lboost_unit_test_framework
endif

# Mac
ifeq ($(UNAME),Darwin)
    # assumes you have installed MacPorts from http://www.macports.org and run:
    # sudo port install boost fltk lua
    # assumes you have built boost as in the Readme.MacOsx.txt

    BOOST_HOME=../libraries/boost-darwin
    BOOST_INC=-I$(BOOST_HOME)/include/boost-1_43
	ifeq ($(TARGET),RELEASE)
		BOOST_LIB=$(BOOST_HOME)/lib/libboost_thread-xgcc40-mt.a $(BOOST_HOME)/lib/libboost_system-xgcc40-mt.a
	else
		BOOST_LIB=$(BOOST_HOME)/lib/libboost_thread-xgcc40-mt-d.a $(BOOST_HOME)/lib/libboost_system-xgcc40-mt-d.a
	endif
    MACPORTS_DIR=/opt/local
	INC=$(BOOST_INC) -I$(MACPORTS_DIR)/include -I$(LIB_DIR)/vmmlib/include -I$(LIB_DIR)
	INC+=-I$(LIB_DIR)/polylib
	INC+=-pthread
	LDFLAGS=$(BOOST_LIB) -L$(MACPORTS_DIR)/lib -pthread -lfltk -lfltk_forms -lfltk_gl
	LDFLAGS+=-L$(LIB_DIR)/polylib -lpolylib
    LDFLAGS+=-framework Carbon -framework OpenGL -framework GLUT -framework AGL
    TEST_LDFLAGS=-lboost_unit_test_framework
endif

GENERATED=ui.cxx ui.h

MAIN_SOURCES=RepSnapper.cpp
TEST_SOURCES=unittest.cpp
SHARED_SOURCES=asyncserial.cpp stl.cpp RepRapSerial.cpp \
	ProcessController.cpp printer.cpp modelviewcontroller.cpp \
	glutils.cpp gcode.cpp arcball.cpp stdafx.cpp ui.cxx \
	RFO.cpp flu_dnd.cpp flu_pixmaps.cpp flu_simplestring.cpp \
	flu_tree_browser.cpp ivcon.cpp file.cpp platform.cpp \
	$(LIB_DIR)/xml/xml.cpp

HEADERS=arcball.h asyncserial.h convert.h flu_dnd.h flu_enumerations.h \
	flu_export.h flu_pixmaps.h flu_simplestring.h flu_tree_browser.h \
	gcode.h glutils.h ivcon.h miniball.h modelviewcontroller.h \
	printer.h ProcessController.h RepRapSerial.h RFO.h \
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

# make update-deps will re-write the dependenciues below
update-depends:
	makedepend -Y $(SHARED_SOURCES) $(MAIN_SOURCES) $(TEST_SOURCES)

# not needed
#	<Kulitorum> fillet.cpp
#	<Kulitorum> pathfinder.* (will someday keep the extruder inside the object for move commands, to avoid ooze)
#	<Kulitorum> reprapusb.h
#	<Kulitorum> search.*
#	<Kulitorum> simulator.*

# DO NOT DELETE THIS LINE -- make depend depends on it, deletes after here and updates the deps.

asyncserial.o: stdafx.h config.h platform.h arcball.h ivcon.h asyncserial.h
RepSnapper.o: stdafx.h config.h platform.h arcball.h ivcon.h
RepSnapper.o: modelviewcontroller.h ui.h file.h flu_tree_browser.h
RepSnapper.o: flu_enumerations.h flu_export.h flu_simplestring.h gcode.h stl.h
RepSnapper.o: ProcessController.h printer.h RFO.h glutils.h RepRapSerial.h
RepSnapper.o: asyncserial.h
stl.o: stdafx.h config.h platform.h arcball.h ivcon.h stl.h gcode.h ui.h
stl.o: file.h modelviewcontroller.h ProcessController.h printer.h RFO.h
stl.o: flu_tree_browser.h flu_enumerations.h flu_export.h flu_simplestring.h
stl.o: glutils.h RepRapSerial.h asyncserial.h
RepRapSerial.o: stdafx.h config.h platform.h arcball.h ivcon.h RepRapSerial.h
RepRapSerial.o: ui.h file.h modelviewcontroller.h gcode.h stl.h
RepRapSerial.o: ProcessController.h printer.h RFO.h flu_tree_browser.h
RepRapSerial.o: flu_enumerations.h flu_export.h flu_simplestring.h glutils.h
RepRapSerial.o: asyncserial.h convert.h
ProcessController.o: stdafx.h config.h platform.h arcball.h ivcon.h
ProcessController.o: modelviewcontroller.h ui.h file.h flu_tree_browser.h
ProcessController.o: flu_enumerations.h flu_export.h flu_simplestring.h
ProcessController.o: gcode.h stl.h ProcessController.h printer.h RFO.h
ProcessController.o: glutils.h RepRapSerial.h asyncserial.h
printer.o: stdafx.h config.h platform.h arcball.h ivcon.h ProcessController.h
printer.o: printer.h gcode.h RFO.h flu_tree_browser.h flu_enumerations.h
printer.o: flu_export.h flu_simplestring.h stl.h
modelviewcontroller.o: stdafx.h config.h platform.h arcball.h ivcon.h
modelviewcontroller.o: modelviewcontroller.h ui.h file.h flu_tree_browser.h
modelviewcontroller.o: flu_enumerations.h flu_export.h flu_simplestring.h
modelviewcontroller.o: gcode.h stl.h ProcessController.h printer.h RFO.h
modelviewcontroller.o: glutils.h RepRapSerial.h asyncserial.h
glutils.o: stdafx.h config.h platform.h arcball.h ivcon.h glutils.h
gcode.o: stdafx.h config.h platform.h arcball.h ivcon.h gcode.h ui.h file.h
gcode.o: modelviewcontroller.h stl.h ProcessController.h printer.h RFO.h
gcode.o: flu_tree_browser.h flu_enumerations.h flu_export.h flu_simplestring.h
gcode.o: glutils.h RepRapSerial.h asyncserial.h
arcball.o: stdafx.h config.h platform.h arcball.h ivcon.h
stdafx.o: stdafx.h config.h platform.h arcball.h ivcon.h stl.h
ui.o: ui.h config.h stdafx.h platform.h arcball.h ivcon.h file.h
ui.o: modelviewcontroller.h gcode.h stl.h ProcessController.h printer.h RFO.h
ui.o: flu_tree_browser.h flu_enumerations.h flu_export.h flu_simplestring.h
ui.o: glutils.h RepRapSerial.h asyncserial.h
RFO.o: stdafx.h config.h platform.h arcball.h ivcon.h ui.h file.h
RFO.o: modelviewcontroller.h gcode.h stl.h ProcessController.h printer.h
RFO.o: RFO.h flu_tree_browser.h flu_enumerations.h flu_export.h
RFO.o: flu_simplestring.h glutils.h RepRapSerial.h asyncserial.h flu_pixmaps.h
flu_dnd.o: flu_dnd.h flu_enumerations.h flu_export.h
flu_pixmaps.o: flu_pixmaps.h flu_export.h
flu_simplestring.o: flu_simplestring.h flu_enumerations.h flu_export.h
flu_tree_browser.o: flu_tree_browser.h flu_enumerations.h flu_export.h
flu_tree_browser.o: flu_simplestring.h flu_pixmaps.h
ivcon.o: ivcon.h
file.o: stdafx.h config.h platform.h arcball.h ivcon.h file.h
file.o: modelviewcontroller.h ui.h flu_tree_browser.h flu_enumerations.h
file.o: flu_export.h flu_simplestring.h gcode.h stl.h ProcessController.h
file.o: printer.h RFO.h glutils.h RepRapSerial.h asyncserial.h
../libraries/xml/xml.o: ../libraries/xml/xml.h
