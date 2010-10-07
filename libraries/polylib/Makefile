# polylib makefile
INC=../vmmlib/include
CXXFLAGS=-c -I$(INC) -Wall

all: libpolylib.a
	
libpolylib.a: GeometryBase.o Polygon2f.o
	ar -r libpolylib.a GeometryBase.o Polygon2f.o

clean:
	rm -f *.o
	rm -f *.a
