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
    LDFLAGS=$(GTK_LIBS) -L/usr/lib -lGLU -lfltk -lfltk_gl -lfltk_forms -lglut -lboost_thread-mt -lboost_system-mt 
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

GENERATED=UI.cxx UI.h

MAIN_SOURCES=RepSnapper.cpp
TEST_SOURCES=unittest.cpp
SHARED_SOURCES=asyncserial.cpp stl.cpp RepRapSerial.cpp \
	ProcessController.cpp Printer.cpp ModelViewController.cpp \
	glutils.cpp GCode.cpp arcball.cpp stdafx.cpp UI.cxx \
	RFO.cpp flu_dnd.cpp flu_pixmaps.cpp flu_simplestring.cpp \
	Flu_Tree_Browser.cpp ivcon.cpp file.cpp platform.cpp \
	$(LIB_DIR)/xml/xml.cpp

HEADERS=arcball.h asyncserial.h convert.h flu_dnd.h flu_enumerations.h \
	flu_export.h flu_pixmaps.h flu_simplestring.h Flu_Tree_Browser.h \
	gcode.h glutils.h ivcon.h miniball.h ModelViewController.h \
	Printer.h ProcessController.h RepRapSerial.h RFO.h \
	stdafx.h stl.h triangle.h UI.h platform.h

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

AsyncSerial.o: stdafx.h config.h platform.h arcball.h ivcon.h asyncserial.h
RepSnapper.o: stdafx.h config.h platform.h arcball.h ivcon.h
RepSnapper.o: ModelViewController.h UI.h file.h Flu_Tree_Browser.h
RepSnapper.o: flu_enumerations.h flu_export.h flu_simplestring.h gcode.h stl.h
RepSnapper.o: ProcessController.h Printer.h RFO.h glutils.h RepRapSerial.h
RepSnapper.o: asyncserial.h
stl.o: stdafx.h config.h platform.h arcball.h ivcon.h stl.h gcode.h UI.h
stl.o: file.h ModelViewController.h ProcessController.h Printer.h RFO.h
stl.o: Flu_Tree_Browser.h flu_enumerations.h flu_export.h flu_simplestring.h
stl.o: glutils.h RepRapSerial.h asyncserial.h
RepRapSerial.o: stdafx.h config.h platform.h arcball.h ivcon.h RepRapSerial.h
RepRapSerial.o: UI.h file.h ModelViewController.h gcode.h stl.h
RepRapSerial.o: ProcessController.h Printer.h RFO.h Flu_Tree_Browser.h
RepRapSerial.o: flu_enumerations.h flu_export.h flu_simplestring.h glutils.h
RepRapSerial.o: asyncserial.h convert.h
ProcessController.o: stdafx.h config.h platform.h arcball.h ivcon.h
ProcessController.o: ModelViewController.h UI.h file.h Flu_Tree_Browser.h
ProcessController.o: flu_enumerations.h flu_export.h flu_simplestring.h
ProcessController.o: gcode.h stl.h ProcessController.h Printer.h RFO.h
ProcessController.o: glutils.h RepRapSerial.h asyncserial.h
Printer.o: stdafx.h config.h platform.h arcball.h ivcon.h ProcessController.h
Printer.o: Printer.h gcode.h RFO.h Flu_Tree_Browser.h flu_enumerations.h
Printer.o: flu_export.h flu_simplestring.h stl.h
ModelViewController.o: stdafx.h config.h platform.h arcball.h ivcon.h
ModelViewController.o: ModelViewController.h UI.h file.h Flu_Tree_Browser.h
ModelViewController.o: flu_enumerations.h flu_export.h flu_simplestring.h
ModelViewController.o: gcode.h stl.h ProcessController.h Printer.h RFO.h
ModelViewController.o: glutils.h RepRapSerial.h asyncserial.h
glutils.o: stdafx.h config.h platform.h arcball.h ivcon.h glutils.h
GCode.o: stdafx.h config.h platform.h arcball.h ivcon.h gcode.h UI.h file.h
GCode.o: ModelViewController.h stl.h ProcessController.h Printer.h RFO.h
GCode.o: Flu_Tree_Browser.h flu_enumerations.h flu_export.h flu_simplestring.h
GCode.o: glutils.h RepRapSerial.h asyncserial.h
ArcBall.o: stdafx.h config.h platform.h arcball.h ivcon.h
stdafx.o: stdafx.h config.h platform.h arcball.h ivcon.h stl.h
UI.o: UI.h config.h stdafx.h platform.h arcball.h ivcon.h file.h
UI.o: ModelViewController.h gcode.h stl.h ProcessController.h Printer.h RFO.h
UI.o: Flu_Tree_Browser.h flu_enumerations.h flu_export.h flu_simplestring.h
UI.o: glutils.h RepRapSerial.h asyncserial.h
RFO.o: stdafx.h config.h platform.h arcball.h ivcon.h UI.h file.h
RFO.o: ModelViewController.h gcode.h stl.h ProcessController.h Printer.h
RFO.o: RFO.h Flu_Tree_Browser.h flu_enumerations.h flu_export.h
RFO.o: flu_simplestring.h glutils.h RepRapSerial.h asyncserial.h flu_pixmaps.h
Flu_DND.o: flu_dnd.h flu_enumerations.h flu_export.h
flu_pixmaps.o: flu_pixmaps.h flu_export.h
FluSimpleString.o: flu_simplestring.h flu_enumerations.h flu_export.h
Flu_Tree_Browser.o: Flu_Tree_Browser.h flu_enumerations.h flu_export.h
Flu_Tree_Browser.o: flu_simplestring.h flu_pixmaps.h
ivcon.o: ivcon.h
File.o: stdafx.h config.h platform.h arcball.h ivcon.h file.h
File.o: ModelViewController.h UI.h Flu_Tree_Browser.h flu_enumerations.h
File.o: flu_export.h flu_simplestring.h gcode.h stl.h ProcessController.h
File.o: Printer.h RFO.h glutils.h RepRapSerial.h asyncserial.h
../libraries/xml/xml.o: ../libraries/xml/xml.h
