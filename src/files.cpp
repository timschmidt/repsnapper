/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010  Kulitorum
    Copyright (C) 2011-12  martin.dieringer@gmx.de

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

#include "files.h"

#include <iostream>

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

	GFloatIEEE754 ret;
	ret.mpn.mantissa = ieee.ieee.mantissa;
	ret.mpn.biased_exponent = ieee.ieee.exponent;
	ret.mpn.sign = ieee.ieee.negative;

	return ret.v_float;
}

static double read_double(ifstream &file) {
  return double(read_float(file));
}


File::File(Glib::RefPtr<Gio::File> file)
 : _file(file)
{
  _path = _file->get_path();
  _type = getFileType(_path);
}

void File::loadTriangles(vector< vector<Triangle> > &triangles,
			 vector<ustring> &names,
			 uint max_triangles)
{
  if(_type == ASCII_STL) {
    // multiple shapes per file
    load_asciiSTL(triangles, names, max_triangles);
  } else {
    // single shape per file
    triangles.resize(1);
    names.resize(1);
    names[0] = _file->get_basename();
    size_t found = names[0].find_last_of(".");
    names[0] = (ustring)names[0].substr(0,found);
    if (_type == BINARY_STL) {
      load_binarySTL(triangles[0], max_triangles);
    } else if (_type == VRML) {
      load_VRML(triangles[0], max_triangles);
    } else if (_type == AMF) {
      load_AMF(triangles[0], max_triangles);
    } else {
      cerr << _("Unrecognized file - ") << _file->get_parse_name() << endl;
      cerr << _("Known extensions: ") << "STL, WRL, AMF." << endl;
    }
  }
}



filetype_t File::getFileType(ustring filename)
{

    // Extract file extension (i.e. "stl")
  ustring extension = filename.substr(filename.find_last_of(".")+1);


    if(extension == "wrl" || extension == "WRL") {
        return VRML;
    }

    if(extension == "amf" || extension == "AMF") {
        return AMF;
    }

    if(extension != "stl" && extension != "STL") {
        return NONE_STL;
    }

    ifstream file;
    file.open(filename.c_str());

    if(file.fail()) {
      cerr << _("Error: Unable to open file - ") << filename << endl;
      return NONE_STL;
    }

    // ASCII files start with "solid [Name_of_file]"
    ustring first_word;
    try {
      file >> first_word;

      // Find bad Solid Works STL header
      // 'solid binary STL from Solid Edge, Unigraphics Solutions Inc.'
      ustring second_word;
      if(first_word == "solid")
	file >> second_word;

      file.close();
      if(first_word == "solid" && second_word != "binary") { // ASCII
	return ASCII_STL;
      } else {
	return BINARY_STL;
      }
    } catch (Glib::ConvertError e) {
      return BINARY_STL; // no keyword -> binary
    }

}


bool File::load_binarySTL(vector<Triangle> &triangles,
			  uint max_triangles, bool readnormals)
{
    ifstream file;
    ustring filename = _file->get_path();
    file.open(filename.c_str(), ifstream::in | ifstream::binary);

    if(file.fail()) {
      cerr << _("Error: Unable to open stl file - ") << filename << endl;
      return false;
    }
    // cerr << "loading bin " << filename << endl;

    /* Binary STL files have a meaningless 80 byte header
     * followed by the number of triangles */
    file.seekg(80, ios_base::beg);
    unsigned int num_triangles;
    unsigned char buffer[4];
    file.read(reinterpret_cast <char *> (buffer), 4);
    // Read platform independent 32-bit little-endian int.
    num_triangles = buffer[0] | buffer[1] << 8 | buffer[2] << 16 | buffer[3] << 24;

    uint step = 1;
    if (max_triangles > 0 && max_triangles < num_triangles) {
      step = ceil(num_triangles/max_triangles);
      triangles.reserve(max_triangles);
    } else
      triangles.reserve(num_triangles);

    uint i = 0;
    for(; i < num_triangles; i+=step)
    {
      if (step>1)
	file.seekg(84 + 50*i, ios_base::beg);

      double a,b,c;
        a = read_double (file);
        b = read_double (file);
        c = read_double (file);
        Vector3d N(a,b,c);
        a = read_double (file);
        b = read_double (file);
        c = read_double (file);
        Vector3d Ax(a,b,c);
        a = read_double (file);
        b = read_double (file);
        c = read_double (file);
        Vector3d Bx(a,b,c);
        a = read_double (file);
        b = read_double (file);
        c = read_double (file);
        Vector3d Cx(a,b,c);

        if (file.eof()) {
            cerr << _("Unexpected EOF reading STL file - ") << filename << endl;
            break;
        }

        /* attribute byte count - sometimes contains face color
            information but is useless for our purposes */
        unsigned short byte_count;
        file.read(reinterpret_cast <char *> (buffer), 2);
	byte_count = buffer[0] | buffer[1] << 8;
	// Repress unused variable warning.
	(void)&byte_count;

	Triangle T = Triangle(Ax,Bx,Cx);
	if (readnormals)
	  if (T.Normal.dot(N) < 0) T.invertNormal();

	// cout << "bin triangle "<< N << ":\n\t" << Ax << "/\n\t"<<Bx << "/\n\t"<<Cx << endl;
        triangles.push_back(T);
    }
    file.close();

    return true;
    // cerr << "Read " << i << " triangles of " << num_triangles << " from file" << endl;
}


bool File::load_asciiSTL(vector< vector<Triangle> > &triangles,
			 vector<ustring> &names,
			 uint max_triangles, bool readnormals)
{
  ustring filename = _file->get_path();
  ifstream file;
  file.open(filename.c_str(), ifstream::in);
  if(file.fail()) {
    cerr << _("Error: Unable to open stl file - ") << filename << endl;
    return false;
  }
  // fileis.imbue(std::locale::classic());

  // get as many shapes as found in file
  while (true) {
    vector<Triangle> tr;
    ustring name;
    if (!File::parseSTLtriangles_ascii(file, max_triangles, readnormals,
				       tr, name))
      break;

    triangles.push_back(tr);
    names.push_back(name);
    // go back to get "solid " keyword again
    streampos where = file.tellg();
    where-=100;
    if (where < 0) break;
    file.seekg(where,ios::beg);
  }

  //     if (ret < 0) {// cannot parse, try binary
  //       cerr << _("Could not read file in ASCII mode, trying Binary: ")<< filename << endl;
  //       file.close();
  //       return loadBinarySTL(filename, max_triangles, readnormals);
  //     }

  file.close();
  return true;
}


bool File::parseSTLtriangles_ascii (istream &text,
				    uint max_triangles, bool readnormals,
				    vector<Triangle> &triangles,
				    ustring &shapename)
{
  //cerr << "loading ascii " << endl;
  //cerr << " locale " << std::locale().name() << endl;

  shapename = _("Unnamed");
    // uint step = 1;
    // if (max_triangles > 0 && max_triangles < num_triangles) {
    //   step = ceil(num_triangles/max_triangles);
    streampos pos = text.tellg();
    text.seekg(0, ios::end);
    streampos fsize = text.tellg();
    text.seekg(pos, ios::beg);

    // a rough estimation
    uint num_triangles = (fsize-pos)/30;

    uint step = 1;
    if (max_triangles > 0 && max_triangles < num_triangles)
      step = ceil(num_triangles/max_triangles);

    //cerr << "step " << step << endl;

    /* ASCII files start with "solid [Name_of_file]"
     * so get rid of them to access the data */
    string solid;
    //getline (text, solid);

    while(!text.eof()) { // Find next solid
      text >> solid;
      if (solid == "solid") {
	string name;
	getline(text,name);
	shapename = (ustring)name;
	break;
      }
    }
    if (solid != "solid") {
      return false;
    }

    // uint itr = 0;
    while(!text.eof()) { // Loop around all triangles
      string facet;
        text >> facet;
	//cerr << text.tellg() << " - " << fsize << " - " <<facet << endl;
	if (step > 1) {
	  for (uint s=0; s < step; s++) {
	    facet = "";
	    while (facet != "facet" && facet != "endsolid")
	      text >> facet;
	    if (facet == "endsolid") {
	      break;
	    }
	  }
	}
        // Parse possible end of text - "endsolid"
        if(facet == "endsolid") {
	  break;
        }
        if(facet != "facet") {
	  cerr << _("Error: Facet keyword not found in STL text!") << endl;
	  return false;
        }

        // Parse Face Normal - "normal %f %f %f"
        string normal;
        Vector3d normal_vec;
        text >> normal;
        if(normal != "normal") {
	  cerr << _("Error: normal keyword not found in STL text!") << endl;
	  return false;
	}

	if (readnormals){
	  text >> normal_vec.x()
	       >> normal_vec.y()
	       >> normal_vec.z();
	}

        // Parse "outer loop" line
        string outer, loop;
	while (outer!="outer" && !(text).eof()) {
	  text >> outer;
	}
	text >> loop;
	if(outer != "outer" || loop != "loop") {
	  cerr << _("Error: Outer/Loop keywords not found!") << endl;
	  return false;
	}

        // Grab the 3 vertices - each one of the form "vertex %f %f %f"
        Vector3d vertices[3];
        for(int i=0; i<3; i++) {
	    string vertex;
            text >> vertex
		 >> vertices[i].x()
		 >> vertices[i].y()
		 >> vertices[i].z();

            if(vertex != "vertex") {
	      cerr << _("Error: Vertex keyword not found") << endl;
	      return false;
            }
        }

        // Parse end of vertices loop - "endloop endfacet"
        string endloop, endfacet;
        text >> endloop >> endfacet;

        if(endloop != "endloop" || endfacet != "endfacet") {
	  cerr << _("Error: Endloop or endfacet keyword not found") << endl;
	  return false;
        }

        // Create triangle object and push onto the vector
        Triangle triangle(vertices[0],
			  vertices[1],
			  vertices[2]);
	if (readnormals){
	  //cerr << "reading normals from file" << endl;
	  if (triangle.Normal.dot(normal_vec) < 0) triangle.invertNormal();
	}

        triangles.push_back(triangle);
    }
    //cerr << "loaded " << filename << endl;
    return true;
}


bool File::load_VRML(vector<Triangle> &triangles, uint max_triangles)

{
  triangles.clear();
  ustring filename = _file->get_path();
    ifstream file;
    file.open(filename.c_str());

    if(file.fail()) {
      cerr << _("Error: Unable to open vrml file - ") << filename << endl;
      return false;
    }
    ustring word;
    std::vector<float> vertices;
    std::vector<int> indices;
    bool finished = false;
    while(!file.eof() && !finished) {
      // while (word!="Shape"  && !file.eof())
      // 	file >> word;
      // while (word!="Appearance" && !file.eof())
      // 	file >> word;
      // while (word!="Coordinate" && !file.eof())
      // 	file >> word;

      while (word!="point" && !file.eof())
	file >> word;
      file >> word;
      if (word=="[") {
	float f;
	while (word!="]" && !file.eof()){
	  file >> word;
	  if (word!="]")
	    if (word.find("#")!=0) {
	      std::istringstream iss(word);
	      iss >> f;
	      vertices.push_back(f);
	      //cerr << f << ", ";
	    }
	}
	//cerr << endl;
      }
      while (word!="coordIndex"  && !file.eof())
	file >> word;
      file >> word;
      if (word=="[") {
	int c;
	while (word!="]" && !file.eof()){
	  file >> word;
	  if (word!="]")
	    if (word.find("#")!=0) {
	      std::istringstream iss(word);
	      iss >> c;
	      indices.push_back(c);
	      //cerr << c << ", ";
	    }
	}
	//cerr << endl;
      }
    }
    file.close();

    if (indices.size()%4!=0) return false;
    if (vertices.size()%3!=0) return false;
    vector<Vector3d> vert;
    for (uint i=0; i<vertices.size();i+=3)
      vert.push_back(Vector3d(vertices[i],
			      vertices[i+1],
			      vertices[i+2]));
    for (uint i=0; i<indices.size();i+=4){
      Triangle T(vert[indices[i]],vert[indices[i+1]],vert[indices[i+2]]);
      triangles.push_back(T);
    }
    return true;
}


bool File::load_AMF(vector<Triangle> &triangles, uint max_triangles)
{
  return false;
}


bool File::saveBinarySTL(ustring filename, const vector<Triangle> &triangles,
			 const Matrix4d &T)
{

  FILE *file  = fopen(filename.c_str(),"wb");

  if (file==0) {
    cerr << _("Error: Unable to open stl file - ") << filename << endl;
    return false;
  }

  int num_tri = (int)triangles.size();

  // Write Header
  string tmp = "solid binary by Repsnapper                                                     ";

  fwrite(tmp.c_str(), 80, 1, file);

  // write number of triangles
  fwrite(&num_tri, 1, sizeof(int), file);

  for(int i=0; i<num_tri; i++){
    Vector3f
      TA = T*triangles[i].A,
      TB = T*triangles[i].B,
      TC = T*triangles[i].C,
      TN = T*triangles[i].Normal; TN.normalize();
    float N[3] = {TN.x(), TN.y(), TN.z()};
    float P[9] = {TA.x(), TA.y(), TA.z(),
		  TB.x(), TB.y(), TB.z(),
		  TC.x(), TC.y(), TC.z()};

    // write the normal, the three coords and a short set to zero
    fwrite(&N,3,sizeof(float),file);
    for(int k=0; k<3; k++) { fwrite(&P[3*k], 3, sizeof(float), file); }
    unsigned short attributes = 0;
    fwrite(&attributes, 1, sizeof(short), file);
  }

  fclose(file);
  return true;

}
