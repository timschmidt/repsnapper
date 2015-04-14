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
#include <stdlib.h>


static string numlocale   = "";
static string colllocale  = "";
static string ctypelocale = "";
void save_locales() {
  numlocale   = string(setlocale(LC_NUMERIC, NULL));
  colllocale  = string(setlocale(LC_COLLATE, NULL));
  ctypelocale = string(setlocale(LC_CTYPE,   NULL));
}
void set_locales(const char * loc) {
  setlocale(LC_NUMERIC, loc);
  setlocale(LC_COLLATE, loc);
  setlocale(LC_CTYPE,   loc);
}
void reset_locales() {
  setlocale(LC_NUMERIC, numlocale.c_str());
  setlocale(LC_COLLATE, colllocale.c_str());
  setlocale(LC_CTYPE,   ctypelocale.c_str());
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
  Gio::FileType type = _file->query_file_type();
  if (type != Gio::FILE_TYPE_REGULAR &&
      type != Gio::FILE_TYPE_SYMBOLIC_LINK)
    return;

  ustring name_by_file = _file->get_basename();
  size_t found = name_by_file.find_last_of(".");
  name_by_file = (ustring)name_by_file.substr(0,found);

  set_locales("C");
  if(_type == ASCII_STL) {
    // multiple shapes per file
    load_asciiSTL(triangles, names, max_triangles);
    if (names.size() == 1) // if single shape name by file
      names[0] = name_by_file;
    if (triangles.size() == 0) {// if no success, try binary mode
      _type = BINARY_STL;
      loadTriangles(triangles, names, max_triangles);
      return;
    }
  } else if (_type == AMF) {
    // multiple shapes per file
    load_AMF(triangles, names, max_triangles);
    if (names.size() == 1) // if single shape name by file
      names[0] = name_by_file;
  } else {
    // single shape per file
    triangles.resize(1);
    names.resize(1);
    names[0] = name_by_file;
    if (_type == BINARY_STL) {
      load_binarySTL(triangles[0], max_triangles);
    } else if (_type == VRML) {
      load_VRML(triangles[0], max_triangles);
    } else {
      cerr << _("Unrecognized file - ") << _file->get_parse_name() << endl;
      cerr << _("Known extensions: ") << "STL, WRL, AMF." << endl;
    }
  }
  reset_locales();
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
	    if (text.eof()) break;
	    facet = "";
	    while (facet != "facet" && facet != "endsolid"){
	      if (text.eof()) break;
	      text >> facet;
	    }
	    if (facet == "endsolid") {
	      break;
	    }
	  }
	}
        // Parse possible end of text - "endsolid"
        if(text.eof() || facet == "endsolid" ) {
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
        if(readnormals && normal != "normal") {
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
	while (outer!="outer" && !text.eof()) {
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

    file.imbue(std::locale("C"));
    ustring word;
    std::vector<float> vertices;
    std::vector<int> indices;
    bool finished = false;
    vector<Vector3d> points;
    while(!file.eof() && !finished) {
      // while (word!="Shape"  && !file.eof())
      // 	file >> word;
      // while (word!="Appearance" && !file.eof())
      // 	file >> word;
      // while (word!="Coordinate" && !file.eof())
      // 	file >> word;
      points.clear();
      vertices.clear();
      while (word!="coord" && !file.eof()) // only use coord points
	file >> word;
      while (word!="point" && !file.eof())
	file >> word;
      file >> word;
      if (word=="[") {
	double f;
	while (word!="]" && !file.eof()){
	  file >> word;
          if (word.find(",") == word.length()-1) { // remove comma
            word = word.substr(0,word.length()-1);
          }
	  if (word!="]")
	    if (word.find("#") != 0) { // comment
              f = atof(word.c_str());
	      vertices.push_back(f);
	    } else { // skip rest of line
              file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
	}
        if (vertices.size() % 3 == 0)
          for (uint i = 0; i < vertices.size(); i += 3)
            points.push_back(Vector3d(vertices[i], vertices[i+1], vertices[i+2]));
	//cerr << endl;
      }
      indices.clear();
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
              iss.precision(20);
	      iss >> c;
	      indices.push_back(c);
	      //cerr << word << "=="<< c << ", ";
	    }
	}
        if (indices.size()%4 == 0)
          for (uint i=0; i<indices.size();i+=4) {
            Triangle T(points[indices[i]],points[indices[i+1]],points[indices[i+2]]);
            triangles.push_back(T);
          }
      }
      if (triangles.size()%1000 == 0)
        cerr << "\rread " << triangles.size() << " triangles " ;
    }
    file.close();
    return true;
}


// does not cross-compile ....
#ifndef WIN32
#define ENABLE_AMF 1
#else
#define ENABLE_AMF 0
#endif

#if ENABLE_AMF
#include "amf/amftools-code/include/AMF_File.h"
 class AMFLoader : public AmfFile
 {
   double _scale;
 public:
   AMFLoader() : _scale(1.) {};
   ~AMFLoader(){};
   bool open(ustring path) {
     bool ok = Load(path);
     if (!ok)
       cerr << "AMF Error: " << GetLastErrorMsg() << endl;
     return ok;
   }
   Vector3d getVertex(const nMesh &mesh, uint i) const
   {
     nCoordinates c = mesh.Vertices.VertexList[i].Coordinates;
     return Vector3d(_scale * c.X, _scale * c.Y, _scale * c.Z);
   }
   Triangle getTriangle(const nMesh &mesh, const nTriangle &t)
   {
     return Triangle(getVertex(mesh, t.v1),
		     getVertex(mesh, t.v2),
		     getVertex(mesh, t.v3));
   }

   bool getObjectTriangles(uint onum, vector<Triangle> &triangles)
   {
     nObject* object = GetObject(onum);
     uint nmeshes = object->Meshes.size();
     for (uint m = 0; m < nmeshes; m++) {
       const nMesh mesh = object->Meshes[m];
       //cerr << "Units "<<  GetUnitsString() << endl;
       switch (aUnit) {
       case UNIT_M:  _scale = 1000.; break;
       case UNIT_IN: _scale = 25.4;  break;
       case UNIT_FT: _scale = 304.8; break;
       case UNIT_UM: _scale = 0.001; break;
       case UNIT_MM:
       default: _scale = 1.; break;
       }
       uint nvolumes = mesh.Volumes.size();
       for (uint v = 0; v < nvolumes; v++) {
	 uint ntria = mesh.Volumes[v].Triangles.size();
	 for (uint t = 0; t < ntria; t++) {
	   triangles.push_back( getTriangle(mesh, mesh.Volumes[v].Triangles[t]) );
	 }
       }
     }
     return true;
   }

 };
 class AMFWriter : public AmfFile
 {
 public:
   AMFWriter() {};
   ~AMFWriter() {};

   nVertex vertex(const Vector3d &v) const {
     return nVertex(v.x(), v.y(), v.z());
   }

   bool AddObject(const vector<Triangle> &triangles,
		  const ustring name)
   {
     int num = AmfFile::AddObject(string(name));
     if (num<0) return false;
     nObject* object = GetObject(num, true);
     if (!object) return false;
     nMesh mesh;
     for (uint t = 0; t <  triangles.size(); t++) {
       nVertex A = vertex(triangles[t].A);
       mesh.AddVertex(A);
       nVertex B = vertex(triangles[t].B);
       mesh.AddVertex(B);
       nVertex C = vertex(triangles[t].C);
       mesh.AddVertex(C);
     }
     nVolume* vol = mesh.NewVolume(name);
     for (uint t = 0; t <  triangles.size(); t++) {
       nTriangle tr(3*t, 3*t+1, 3*t+2);
       vol->AddTriangle(tr);
     }
     object->Meshes.push_back(mesh);
     return true;
   }

 };
#endif

bool File::load_AMF(vector< vector<Triangle> > &triangles,
		    vector<ustring> &names,
		    uint max_triangles)
{
#if ENABLE_AMF
  AMFLoader amf;
  if (!amf.open(_file->get_path()))
    return false;
  uint nobjs = amf.GetObjectCount();
  //cerr << nobjs << " objs" << endl;
  for (uint o = 0; o < nobjs; o++) {
    vector<Triangle> otr;
    amf.getObjectTriangles(o,otr);
    triangles.push_back(otr);
    names.push_back(ustring(amf.GetObjectName(o)));
  }
  return true;
#else
  return false;
#endif
}

bool File::save_AMF (ustring filename,
		     const vector< vector<Triangle> > &triangles,
		     const vector<ustring> &names,
		     bool compressed)
{
#if ENABLE_AMF
  AMFWriter amf;
  amf.SetUnits(UNIT_MM);
  uint nobjs = triangles.size();
  for (uint o = 0; o < nobjs; o++) {
    bool ok = amf.AddObject(triangles[o],names[o]);
    if (!ok) return false;
  }
  amf.Save(filename, compressed);
  return true;
#else
  return false;
#endif
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
