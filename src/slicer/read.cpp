/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010  Kulitorum

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include "slicer.h"
#include <ieee754.h>

// Constructor
Slicer::Slicer()
{
	Min.x = Min.y = Min.z = 0.0f;
	Max.x = Max.y = Max.z = 200.0f;

	CalcCenter();
}

static float read_float(ifstream &file) {
	// Read platform independent 32 bit ieee 754 little-endian float.
	union ieee_union {
		char buffer[4];
		struct ieee_struct {
			unsigned int mantissa:23;
			unsigned int exponent:8;
			unsigned int negative:1;
		} ieee;
	} ieee;
	file.read (ieee.buffer, 4);
	ieee754_float ret;
	ret.ieee.mantissa = ieee.ieee.mantissa;
	ret.ieee.exponent = ieee.ieee.exponent;
	ret.ieee.negative = ieee.ieee.negative;
	return ret.f;
}

/* Loads an binary STL file by filename
 * Returns 0 on success and -1 on failure */
int Slicer::loadBinarySTL(string filename) {

    if(getFileType(filename) != BINARY_STL) {
        return -1;
    }

    triangles.clear();
	Min.x = Min.y = Min.z = numeric_limits<float>::infinity();
	Max.x = Max.y = Max.z = -numeric_limits<float>::infinity();

    ifstream file;
    file.open(filename.c_str());

    if(file.fail()) {
        cerr << "Error: Unable to open stl file - " << filename << endl;
        return -1;
    }

    /* Binary STL files have a meaningless 80 byte header
     * followed by the number of triangles */
    file.seekg(80, ios_base::beg);
    unsigned int num_triangles;
    unsigned char buffer[4];
    file.read(reinterpret_cast <char *> (buffer), 4);
    // Read platform independent 32-bit little-endian int.
    num_triangles = buffer[0] | buffer[1] << 8 | buffer[2] << 16 | buffer[3] << 24;
    triangles.reserve(num_triangles);

    for(uint i = 0; i < num_triangles; i++)
    {
        float a,b,c;
        a = read_float (file);
        b = read_float (file);
        c = read_float (file);
        Vector3f N(a,b,c);
        a = read_float (file);
        b = read_float (file);
        c = read_float (file);
        Vector3f Ax(a,b,c);
        a = read_float (file);
        b = read_float (file);
        c = read_float (file);
        Vector3f Bx(a,b,c);
        a = read_float (file);
        b = read_float (file);
        c = read_float (file);
        Vector3f Cx(a,b,c);

        /* Recalculate normal vector - can't trust an STL file! */
        Vector3f AA=Cx-Ax;
        Vector3f BB=Cx-Bx;
        N.x = AA.y * BB.z - BB.y * AA.z;
        N.y = AA.z * BB.x - BB.z * AA.x;
        N.z = AA.x * BB.y - BB.x * AA.y;
        N.normalize();

        /* attribute byte count - sometimes contains face color
            information but is useless for our purposes */
        unsigned short byte_count;
        file.read(reinterpret_cast <char *> (buffer), 2);
	byte_count = buffer[0] | buffer[1] << 8;
	// Repress unused variable warning.
	(void)&byte_count;

        Triangle T(N,Ax,Bx,Cx);

        triangles.push_back(T);
        T.AccumulateMinMax (Min, Max);
    }
    file.close();
    return 0;
}

/* Loads an ASCII STL file by filename
 * Returns 0 on success and -1 on failure */
int Slicer::loadASCIISTL(string filename) {

    // Check filetype
    if(getFileType(filename) != ASCII_STL) {
        cerr << "None ASCII STL file passed to loadASCIIFile" << endl;
        return -1;
    }
    triangles.clear();
	Min.x = Min.y = Min.z = numeric_limits<float>::infinity();
	Max.x = Max.y = Max.z = -numeric_limits<float>::infinity();

    ifstream file;
    file.open(filename.c_str());

    if(file.fail()) {
        cerr << "Error: Unable to open stl file - " << filename << endl;
        return -1;
    }

    /* ASCII files start with "solid [Name_of_file]"
     * so get rid of them to access the data */
    string solid;
    getline (file, solid);

    while(!file.eof()) { // Loop around all triangles
        string facet;
        file >> facet;

        // Parse possible end of file - "endsolid"
        if(facet == "endsolid") {
            break;
        }

        if(facet != "facet") {
            cerr << "Error: Facet keyword not found in STL file!" << endl;
            return -1;
        }

        // Parse Face Normal - "normal %f %f %f"
        string normal;
        Vector3f normal_vec;
        file >> normal
             >> normal_vec.x
             >> normal_vec.y
             >> normal_vec.z;

        if(normal != "normal") {
            cerr << "Error: normal keyword not found in STL file!" << endl;
            return -1;
        }

        // Parse "outer loop" line
        string outer, loop;
        file >> outer >> loop;
        if(outer != "outer" || loop != "loop") {
            cerr << "Error: Outer/Loop keywords not found!" << endl;
            return -1;
        }

        // Grab the 3 vertices - each one of the form "vertex %f %f %f"
        Vector3f vertices[3];
        for(int i=0; i<3; i++) {
            string vertex;
            file >> vertex
                 >> vertices[i].x
                 >> vertices[i].y
                 >> vertices[i].z;

            if(vertex != "vertex") {
                cerr << "Error: Vertex keyword not found" << endl;
                return -1;
            }
        }

        // Parse end of vertices loop - "endloop endfacet"
        string endloop, endfacet;
        file >> endloop >> endfacet;

        if(endloop != "endloop" || endfacet != "endfacet") {
            cerr << "Error: Endloop or endfacet keyword not found" << endl;
            return -1;
        }

        /* Recalculate normal vector - can't trust an STL file! */
        Vector3f AA=vertices[2]-vertices[0];
        Vector3f BB=vertices[2]-vertices[1];
        normal_vec.x = AA.y * BB.z - BB.y * AA.z;
        normal_vec.y = AA.z * BB.x - BB.z * AA.x;
        normal_vec.z = AA.x * BB.y - BB.x * AA.y;
        normal_vec.normalize();

        // Create triangle object and push onto the vector
        Triangle triangle(normal_vec,
                            vertices[0],
                            vertices[1],
                            vertices[2]);

        triangle.AccumulateMinMax(Min, Max);

        triangles.push_back(triangle);
    }
    file.close();
    return 0;
} // STL::loadASCIIFile(string filename)

/* Returns the filetype of the given file */
filetype_t Slicer::getFileType(string filename) {

    // Extract file extension (i.e. "stl")
    string extension = filename.substr(filename.find_last_of(".")+1);

    if(extension != "stl" && extension != "STL") {
        return NONE_STL;
    }

    ifstream file;
    file.open(filename.c_str());

    if(file.fail()) {
        cerr << "Error: Unable to open stl file - " << filename << endl;
        return NONE_STL;
    }

    // ASCII files start with "solid [Name_of_file]"
    string first_word;
    file >> first_word;

    file.close();

    if(first_word == "solid") { // ASCII
        return ASCII_STL;
    } else {
        return BINARY_STL;
    }
}

/* Loads an stl file by filename
 * Returns -1 on error, and 0 on success
 * Error messages placed on stderr */
int Slicer::load(string filename)
{
    filetype_t type = getFileType (filename);
    if(type == ASCII_STL) {
        loadASCIISTL(filename);
    } else if (type == BINARY_STL) { // Binary
        loadBinarySTL(filename);
    } else {
	cerr << "unrecognized file - " << filename << endl;
	return -1;
    }

    OptimizeRotation();
    CalcCenter();

    scale_factor = 1.0;
    return 0;
}
