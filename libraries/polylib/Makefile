#    This file is a part of the RepSnapper project.
#    Copyright (C) 2010  Kulitorum
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.

#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.

#    You should have received a copy of the GNU General Public License along
#    with this program; if not, write to the Free Software Foundation, Inc.,
#    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

# polylib makefile
INC=../vmmlib/include
CXXFLAGS=-c -I$(INC) -Wall

all: libpolylib.a
	
libpolylib.a: GeometryBase.o Polygon2f.o
	ar -r libpolylib.a GeometryBase.o Polygon2f.o

clean:
	rm -f *.o
	rm -f *.a
