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

#include "shape.h"
#include "progress.h"
#include "settings.h"
#include "clipping.h"

#ifdef _OPENMP
#include <omp.h>
#endif

// Constructor
Shape::Shape()
  : slow_drawing(false)
{
  Min.set(0,0,0);
  Max.set(200,200,200);
  CalcBBox();
}


Shape::Shape(string filename, istream *text)
  : slow_drawing(false)
{
  this->filename = filename;
  parseASCIISTL(text);
}

void Shape::clear() {
  triangles.clear();
};


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

/* Loads an binary STL file by filename
 * Returns 0 on success and -1 on failure */
int Shape::loadBinarySTL(string filename) 
{
    triangles.clear();

    Min.set(INFTY,INFTY,INFTY);
    Max.set(-INFTY,-INFTY,-INFTY);

    ifstream file;
    file.open(filename.c_str(), ifstream::in | ifstream::binary);

    if(file.fail()) {
      cerr << _("Error: Unable to open stl file - ") << filename << endl;
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

    uint i = 0;
    for(; i < num_triangles; i++)
    {
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

        Triangle T(Ax,Bx,Cx);

	// cout << "bin triangle "<< N << ":\n\t" << Ax << "/\n\t"<<Bx << "/\n\t"<<Cx << endl;

        triangles.push_back(T);
    }
    file.close();

    // cerr << "Read " << i << " triangles of " << num_triangles << " from file" << endl;

    CenterAroundXY();
    double vol = volume();
    if (vol < 0) {
      invertNormals();
      vol = -vol;
    }
    cout << _("Shape has volume ") << vol << _(" mm^3 and ") 
	 << triangles.size() << _(" triangles") << endl;
    return 0;
}

int Shape::saveBinarySTL(string filename) const
{

  FILE *file  = fopen(filename.c_str(),"wb");

  if (file==0) {
    cerr << _("Error: Unable to open stl file - ") << filename << endl;
    return -1;
  }

  int num_tri = (int)triangles.size();

  // Write Header
  string tmp = "solid binary by Repsnapper                                                     "; 

  fwrite(tmp.c_str(), 80, 1, file);

  // write number of triangles
  fwrite(&num_tri, 1, sizeof(int), file);

  Matrix4d T = transform3D.transform;

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
  return 0;
}

int Shape::loadASCIIVRML(std::string filename) 
{
  if(getFileType(filename) != VRML) {
    cerr << _("No VRML file file passed to loadASCIIVRML") << endl;
    return -1;
  }
    triangles.clear();
    Min.set(INFTY,INFTY,INFTY);
    Max.set(-INFTY,-INFTY,-INFTY);
  
    ifstream file;
    file.open(filename.c_str());

    if(file.fail()) {
      cerr << _("Error: Unable to open vrml file - ") << filename << endl;
      return -1;
    }
    string word;
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

    if (indices.size()%4!=0) return -1;
    if (vertices.size()%3!=0) return -1;
    vector<Vector3d> vert;
    for (uint i=0; i<vertices.size();i+=3)
      vert.push_back(Vector3d(vertices[i],
			      vertices[i+1],
			      vertices[i+2]));
    for (uint i=0; i<indices.size();i+=4){
      Triangle T(vert[indices[i]],vert[indices[i+1]],vert[indices[i+2]]);
      triangles.push_back(T);
    }

    CenterAroundXY();
    return 0;
}

/* Loads an ASCII STL file by filename
 * Returns 0 on success and -1 on failure */
int Shape::loadASCIISTL(string filename) {
    // Check filetype
    if(getFileType(filename) != ASCII_STL) {
      cerr << _("None ASCII STL file passed to loadASCIIFile") << endl;
      return -1;
    }
    ifstream file;
    file.open(filename.c_str());

    if(file.fail()) {
      cerr << _("Error: Unable to open stl file - ") << filename << endl;
      return -1;
    }
    int ret = parseASCIISTL(&file);
    if (ret < 0) {// cannot parse, try binary
      cerr << _("Could not read file in ASCII mode, trying Binary: ")<< filename << endl;
      file.close();
      return loadBinarySTL(filename);
    }
    CenterAroundXY();
    this->filename = filename;
    file.close();
    return ret;
} // STL::loadASCIIFile(string filename)


int Shape::parseASCIISTL(istream *text) {

    triangles.clear();
    Min.set(INFTY,INFTY,INFTY);
    Max.set(-INFTY,-INFTY,-INFTY);


    /* ASCII files start with "solid [Name_of_file]"
     * so get rid of them to access the data */
    string solid;
    //getline (text, solid);

    while(!(*text).eof()) { // Find next solid
      *text >> solid;
      if (solid == "solid") {
	getline(*text,filename);
      }
      break;
    }
    if (solid != "solid") {
      return -1;
    }
    while(!(*text).eof()) { // Loop around all triangles
        string facet;
        *text >> facet;

        // Parse possible end of text - "endsolid"
        if(facet == "endsolid") {
	  break;
        }

        if(facet != "facet") {
	  cerr << _("Error: Facet keyword not found in STL text!") << endl;
	  return -1;
        }

        // Parse Face Normal - "normal %f %f %f"
        string normal;
        Vector3d normal_vec;
        *text >> normal;
        if(normal != "normal") {
	  cerr << _("Error: normal keyword not found in STL text!") << endl;
	  return -1;
	}
	
	// forget about normals, we calculate them

        // Parse "outer loop" line
        string outer, loop;
	while (outer!="outer" && !(*text).eof()) {
	  *text >> outer;
	  //cerr << outer<< endl;
	}
	*text >> loop;
	if(outer != "outer" || loop != "loop") {
	  cerr << _("Error: Outer/Loop keywords not found!") << endl;
	  return -1;
	}

        // Grab the 3 vertices - each one of the form "vertex %f %f %f"
        Vector3d vertices[3];
        for(int i=0; i<3; i++) {
            string vertex;
            *text >> vertex
		  >> vertices[i].x()
		  >> vertices[i].y()
		  >> vertices[i].z();

            if(vertex != "vertex") {
	      cerr << _("Error: Vertex keyword not found") << endl;
	      return -1;
            }
        }

        // Parse end of vertices loop - "endloop endfacet"
        string endloop, endfacet;
        *text >> endloop >> endfacet;

        if(endloop != "endloop" || endfacet != "endfacet") {
	  cerr << _("Error: Endloop or endfacet keyword not found") << endl;
	  return -1;
        }

        // Create triangle object and push onto the vector
        Triangle triangle(vertices[0],
			  vertices[1],
			  vertices[2]);
        triangles.push_back(triangle);
    }
    CenterAroundXY();
    cout << _("Shape has volume ") << volume() << " mm^3 and "
	 << triangles.size() << _(" triangles") << endl;
    return 0;
}

/* Returns the filetype of the given file */
filetype_t Shape::getFileType(string filename) {

    // Extract file extension (i.e. "stl")
    string extension = filename.substr(filename.find_last_of(".")+1);

    if(extension == "wrl" || extension == "WRL") {
        return VRML;
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
    string first_word;
    file >> first_word;

    // Find bad Solid Works STL header
    // 'solid binary STL from Solid Edge, Unigraphics Solutions Inc.'
    string second_word;
    if(first_word == "solid")
      file >> second_word;      

    file.close();
    if(first_word == "solid" && second_word != "binary") { // ASCII
      return ASCII_STL;
    } else {
      return BINARY_STL;
    }
}

/* Loads an stl file by filename
 * Returns -1 on error, and 0 on success
 * Error messages placed on stderr */
int Shape::load(string filename)
{
    filetype_t type = getFileType (filename);
    if(type == ASCII_STL) {
        loadASCIISTL(filename);
    } else if (type == BINARY_STL) { // Binary
        loadBinarySTL(filename);
    } else {
      cerr << _("unrecognized file - ") << filename << endl;
	return -1;
    }

    // somehow sort triangles by height to save time when slicing?
    // problem: transform matrix
    //std::sort(triangles.begin(),triangles.end(),Triangle::maxZsort());

    CenterAroundXY();

    return 0;
}


bool Shape::hasAdjacentTriangleTo(const Triangle &triangle, double sqdistance) const
{
  bool haveadj = false;
  int count = (int)triangles.size();
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic) 
#endif
  for (int i = 0; i < count; i++)
    if (!haveadj)
      if (triangle.isConnectedTo(triangles[i],sqdistance)) {
	haveadj = true;
    }
  return haveadj;
}


// recursively build a list of triangles for a shape
void addtoshape(uint i, const vector< vector<uint> > &adj,
		vector<uint> &tr, vector<bool> &done)
{
  if (!done[i]) {
    // add this index to tr
    tr.push_back(i);
    done[i] = true;
    for (uint j = 0; j < adj[i].size(); j++) {
      if (adj[i][j]!=i)
     	addtoshape(adj[i][j], adj, tr, done);
    }
  }
  // we have a complete list of adjacent triangles indices
}

void Shape::splitshapes(vector<Shape*> &shapes, ViewProgress *progress) 
{
  int n_tr = (int)triangles.size();
  if (progress) progress->start(_("Split Shapes"), n_tr);
  int progress_steps = max(1,(int)(n_tr/100));
  vector<bool> done(n_tr);
  bool cont = true;
  // make list of adjacent triangles for each triangle
  vector< vector<uint> > adj(n_tr);
  if (progress) progress->set_label(_("Split: Sorting Triangles ..."));
#ifdef _OPENMP
  omp_lock_t progress_lock;
  omp_init_lock(&progress_lock);
#pragma omp parallel for schedule(dynamic) 
#endif
  for (int i = 0; i < n_tr; i++) {
    if (progress && i%progress_steps==0) {
#ifdef _OPENMP
      omp_set_lock(&progress_lock);
#endif
      cont = progress->update(i);
#ifdef _OPENMP
      omp_unset_lock(&progress_lock);
#endif
    }
    vector<uint> trv;
    for (int j = 0; j < n_tr; j++) {
      if (i!=j) {
	bool add = false;
	if (j<i) // maybe(!) we have it already
	  for (uint k = 0; k<adj[j].size(); k++) {
	    if ((int)adj[j][k] == i) {
	      add = true; break;
	    }
	  }
	add |= (triangles[i].isConnectedTo(triangles[j], 0.01));
	if (add) trv.push_back(j);
      }
    }
    adj[i] = trv;
    if (!cont) i=n_tr;
  }

  if (progress) progress->set_label(_("Split: Building shapes ..."));


  // triangle indices of shapes
  vector< vector<uint> > shape_tri;
  
  for (int i = 0; i < n_tr; i++) done[i] = false;
  for (int i = 0; i < n_tr; i++) {
    if (progress && i%progress_steps==0) 
      cont = progress->update(i);
    if (!done[i]){
      cerr << _("Shape ") << shapes.size()+1 << endl;
      vector<uint> current;
      addtoshape(i, adj, current, done);
      Shape *shape = new Shape();
      shapes.push_back(shape);
      shapes.back()->triangles.resize(current.size());
      for (uint i = 0; i < current.size(); i++)
	shapes.back()->triangles[i] = triangles[current[i]];
      shapes.back()->CalcBBox();
    }
    if (!cont) i=n_tr;
  }

  if (progress) progress->stop("_(Done)");
}

vector<Triangle> cube(Vector3d Min, Vector3d Max)
{
  vector<Triangle> c;
  const Vector3d diag = Max-Min;
  const Vector3d dX(diag.x(),0,0);
  const Vector3d dY(0,diag.y(),0);
  const Vector3d dZ(0,0,diag.z());
  // front
  c.push_back(Triangle(Min, Min+dX, Min+dX+dZ));
  c.push_back(Triangle(Min, Min+dX+dZ, Min+dZ));
  // back
  c.push_back(Triangle(Min+dY, Min+dY+dX+dZ, Min+dY+dX));
  c.push_back(Triangle(Min+dY, Min+dY+dZ, Min+dY+dX+dZ));
  // left
  c.push_back(Triangle(Min, Min+dZ, Min+dY+dZ));
  c.push_back(Triangle(Min, Min+dY+dZ, Min+dY));
  // right
  c.push_back(Triangle(Min+dX, Min+dX+dY+dZ, Min+dX+dZ));
  c.push_back(Triangle(Min+dX, Min+dX+dY, Min+dX+dY+dZ));
  // bottom
  c.push_back(Triangle(Min, Min+dX+dY, Min+dX));
  c.push_back(Triangle(Min, Min+dY, Min+dX+dY));
  // top
  c.push_back(Triangle(Min+dZ, Min+dZ+dX, Min+dZ+dX+dY));
  c.push_back(Triangle(Min+dZ, Min+dZ+dX+dY, Min+dZ+dY));
  return c;
}

void Shape::makeHollow(double wallthickness)
{
  invertNormals();
  const Vector3d wall(wallthickness,wallthickness,wallthickness);
  Matrix4d invT = transform3D.getInverse();
  vector<Triangle> cubet = cube(invT*Min-wall, invT*Max+wall);
  triangles.insert(triangles.end(),cubet.begin(),cubet.end());
  CenterAroundXY();
}

void Shape::invertNormals()
{
  for (uint i = 0; i < triangles.size(); i++)
    triangles[i].invertNormal();
}

void Shape::mirror()
{
  for (uint i = 0; i < triangles.size(); i++)
    triangles[i].mirrorX(Center);
}

double Shape::volume() const
{
  double vol=0;
  for (uint i = 0; i < triangles.size(); i++)
    vol+=triangles[i].projectedvolume(transform3D.transform);
  return vol;
}

string Shape::getSTLsolid() const
{
  stringstream sstr;
  sstr << "solid " << filename <<endl;
  for (uint i = 0; i < triangles.size(); i++)
    sstr << triangles[i].getSTLfacet(transform3D.transform);
  sstr << "endsolid " << filename <<endl;
  return sstr.str();
}

vector<Triangle> Shape::trianglesSteeperThan(double angle) const
{
  vector<Triangle> tr;
  for (uint i = 0; i < triangles.size(); i++) {
    // negative angles are triangles facing downwards
    const double tangle = -triangles[i].slopeAngle(transform3D.transform);
    if (tangle >= angle) 
      tr.push_back(triangles[i]);
  }
  return tr;
}

void Shape::Scale(double in_scale_factor)
{
  transform3D.scale(in_scale_factor);
  CalcBBox();
}

void Shape::ScaleX(double x)
{
  transform3D.scale_x(x);
  CalcBBox();
  return;
}
void Shape::ScaleY(double x)
{
  transform3D.scale_y(x);
  CalcBBox();
  return;
}
void Shape::ScaleZ(double x)
{
  transform3D.scale_z(x);
  CalcBBox();
  PlaceOnPlatform();
  return;
}

void Shape::CalcBBox()
{
  Min.set(INFTY,INFTY,INFTY);
  Max.set(-INFTY,-INFTY,-INFTY);
  for(size_t i = 0; i < triangles.size(); i++) {
    triangles[i].AccumulateMinMax (Min, Max, transform3D.transform);
  }
  Center = (Max + Min) / 2;
}

struct SNorm {
  Vector3d normal;
  double area;
  bool operator<(const SNorm &other) const {return (area<other.area);};
} ;
 
vector<Vector3d> Shape::getMostUsedNormals() const
{
  vector<struct SNorm> normals; 
  // vector<Vector3d> normals;
  // vector<double> area;
  uint ntr = triangles.size();
  vector<bool> done(ntr);
  for(size_t i=0;i<ntr;i++) done[ntr] = false;
  for(size_t i=0;i<ntr;i++)
    {
      bool havenormal = false;
      int numnorm = normals.size();
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic) 
#endif
      for (int n = 0; n < numnorm; n++) {
	if ((normals[n].normal - triangles[i].Normal).squared_length() < 0.000001) {
	  havenormal = true;
	  normals[n].area += triangles[i].area();
	  done[i] = true;
	}
      }
      if (!havenormal){
	SNorm n; n.normal = triangles[i].Normal; n.area = triangles[i].area();
	normals.push_back(n);
	done[i] = true;
      }
    }
  std::sort(normals.rbegin(),normals.rend());
  //cerr << normals.size() << endl;
  vector<Vector3d> nv(normals.size());
  for (uint n=0; n < normals.size(); n++) nv[n] = normals[n].normal;
  return nv;
}


void Shape::OptimizeRotation()
{
  CenterAroundXY();
  vector<Vector3d> normals = getMostUsedNormals();
  // cycle through most-used normals?

  Vector3d N;
  Vector3d Z(0,0,-1);
  double angle=0;
  int count = (int)normals.size();
  for (int n=0; n < count; n++) { 
    //cerr << n << normals[n] << endl;
    N = normals[n];
    angle = acos(N.dot(Z));
    if (angle>0) {
      Vector3d axis = N.cross(Z);
      if (axis.squared_length()>0.1) {
	Rotate(axis,angle);
	break;
      }
    }
  }
  CenterAroundXY();
  PlaceOnPlatform();
}

int Shape::divideAtZ(double z, Shape *upper, Shape *lower, const Matrix4d &T) const
{
  vector<Poly> polys;
  vector<Poly> supportpolys;
  double max_grad;
  bool ok = getPolygonsAtZ(T, z, polys, max_grad, supportpolys, -1);
  if (!ok) return 0;
  vector< vector<Triangle> > surfs;
  triangulate(polys, surfs);

  vector<Triangle> surf;
  for (uint i=0; i<surfs.size(); i++) 
    surf.insert(surf.end(), surfs[i].begin(), surfs[i].end());

  lower->triangles.insert(lower->triangles.end(),surf.begin(),surf.end());
  for (guint i=0; i<surf.size(); i++) surf[i].invertNormal();
  upper->triangles.insert(upper->triangles.end(),surf.begin(),surf.end());
  vector<Triangle> toboth;
  for (guint i=0; i< triangles.size(); i++) {
    Triangle tt = triangles[i].transformed(T*transform3D.transform);
    if (tt.A.z() < z && tt.B.z() < z && tt.C.z() < z )
      lower->triangles.push_back(tt);
    else if (tt.A.z() > z && tt.B.z() > z && tt.C.z() > z )
      upper->triangles.push_back(tt);
    else
      toboth.push_back(tt);
  }
  vector<Triangle> uppersplit,lowersplit;
  for (guint i=0; i< toboth.size(); i++) {
    toboth[i].SplitAtPlane(z, uppersplit, lowersplit);
  }
  upper->triangles.insert(upper->triangles.end(),
			 uppersplit.begin(),uppersplit.end());
  lower->triangles.insert(lower->triangles.end(),
			 lowersplit.begin(),lowersplit.end());
  lower->Rotate(Vector3d(0,1,0),M_PI);
  return 2;
}

void Shape::PlaceOnPlatform()
{
  CalcBBox();
  transform3D.move(Vector3d(0,0,-Min.z()));
  CalcBBox();
}

// Rotate and adjust for the user - not a pure rotation by any means
void Shape::Rotate(const Vector3d & axis, const double & angle)
{
  CenterAroundXY();
  // transform3D.rotate(axis,angle);
  // return;
  // do a real rotation because matrix transform gives errors when slicing
  int count = (int)triangles.size();
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic) 
#endif
  for (int i=0; i < count ; i++) 
    {
      triangles[i].rotate(axis, angle);
    }
  PlaceOnPlatform();
}

// this is primitive, it just rotates triangle vertices
void Shape::Twist(double angle)
{
  CenterAroundXY();
  double h = Max.z()-Min.z();
  double hangle=0;
  Vector3d axis(0,0,1);
  int count = (int)triangles.size();
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic) 
#endif
  for (int i=0; i<count; i++) {
    for (size_t j=0; j<3; j++) 
      {
	hangle = angle * (triangles[i][j].z() - Min.z()) / h;
	triangles[i][j] = triangles[i][j].rotate(hangle,axis);
      }
    triangles[i].calcNormal();
  }
  CalcBBox();
}

void Shape::CenterAroundXY()
{
  CalcBBox();
  Vector3d displacement = transform3D.getTranslation() - Center;
  int count = (int)triangles.size();
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic) 
#endif
  for(int i=0; i<count ; i++)
    {
      triangles[i].Translate(displacement);
    }
  transform3D.move(-displacement);
  CalcBBox();
}

/*
Poly Shape::getOutline(const Matrix4d &T, double maxlen) const
{
  Matrix4d transform = T * transform3D.transform ;
  vector<Vector2d> points(triangles.size()*3);
  for (uint i = 0; i<triangles.size(); i++) {
    for (uint j = 0; j<3; j++) {
      points[i*3 + j] = Vector2d(triangles[i][j].x(),triangles[i][j].y());
    }
  }
  Poly hull = concaveHull2D(points, maxlen);
  return hull;
}
*/

bool getLineSequences(const vector<Segment> lines, vector< vector<uint> > &connectedlines)
{
  uint nlines = lines.size();
  //cerr << "lines size " << nlines << endl;
  if (nlines==0) return true;
  vector<bool> linedone(nlines);
  for (uint l=0; l < nlines; l++) linedone[l] = false;
  vector<uint> sequence;
  uint donelines = 0;
  vector<uint> connections;
  while (donelines < nlines) {
    connections.clear();
    for (uint l=0; l < nlines; l++) { // add next connecting line
      if ( !linedone[l] && 
	   ( (sequence.size()==0) || 
	     (lines[l].start == lines[sequence.back()].end) ) ) {
	connections.push_back(l);
	//cerr << "found connection" << endl;
      }
    }
    if (connections.size()>0) {
      //cerr << "found " << connections.size() << " connections" << endl;
      sequence.push_back(connections[0]);
      linedone[connections[0]] =true;
      donelines++;
      if (lines[sequence.front()].start == lines[sequence.back()].end) {
	//cerr << "closed sequence" << endl;
        connectedlines.push_back(sequence);
	sequence.clear();
      }
    } else { //   (!foundconnection) {  // new sequence
      //cerr << "sequence size " << sequence.size() << endl;
      connectedlines.push_back(sequence);
      sequence.clear();
      for (uint l=0; l < nlines; l++) { // add next best undone line
	if (!linedone[l]) {
	  sequence.push_back(l);
	  linedone[l] = true;
	  donelines++;
	  break;
	}
      }
    }
  }
  if (sequence.size()>0)
    connectedlines.push_back(sequence);
  //cerr << "found "<< connectedlines.size() << " sequences" << endl;
  return true;
}

bool Shape::getPolygonsAtZ(const Matrix4d &T, double z, 
			   vector<Poly> &polys,
			   double &max_gradient,
			   vector<Poly> &supportpolys,
			   double max_supportangle,
			   double thickness) const
{
  vector<Vector2d> vertices;
  vector<Triangle> support_triangles;
  vector<Segment> lines = getCutlines(T, z, vertices, max_gradient, 
				      support_triangles, max_supportangle, thickness);
  //cerr << vertices.size() << " " << lines.size() << endl;
  if (!CleanupSharedSegments(lines)) return false;
  //cerr << vertices.size() << " " << lines.size() << endl;
  if (!CleanupConnectSegments(vertices,lines,true)) return false;
  //cerr << vertices.size() << " " << lines.size() << endl;
  vector< vector<uint> > connectedlines; // sequence of connected lines indices
  if (!getLineSequences(lines, connectedlines)) return false;
  for (uint i=0; i<connectedlines.size();i++){
    Poly poly(z);
    for (uint j = 0; j < connectedlines[i].size();j++){
      poly.addVertex(vertices[lines[connectedlines[i][j]].start]);
    }
    if (lines[connectedlines[i].back()].end !=
	lines[connectedlines[i].front()].start )
      poly.addVertex(vertices[lines[connectedlines[i].back()].end]);
    //cerr << "poly size " << poly.size() << endl;
    poly.calcHole();
    polys.push_back(poly);
  }

  for (uint i = 0; i < support_triangles.size(); i++) {
    Poly p(z);
    // keep only part of triangle above z
    Vector2d lineStart;
    Vector2d lineEnd;
    // support_triangles are already transformed
    int num_cutpoints = support_triangles[i].CutWithPlane(z, Matrix4d::IDENTITY,
							  lineStart, lineEnd);
    if (num_cutpoints == 0) {
      for (uint j = 0; j < 3; j++) {
	p.addVertex(Vector2d(support_triangles[i][j].x(),
			     support_triangles[i][j].y()));
      }
    }
    else if (num_cutpoints > 1) {
      // add points of triangle above z
      for (uint j = 0; j < 3; j++) {
	if (support_triangles[i][j].z() > z) {
	  p.addVertex(Vector2d(support_triangles[i][j].x(),
			       support_triangles[i][j].y()));
	}
      }
      bool reverse = false;
      // test for order if we get 4 points (2 until now)
      if (p.size() > 1) {
	Vector2d i0,i1;	double t0, t1;
	const int is = intersect2D_Segments(p[1], lineStart, lineEnd, p[0],
					    i0, i1, t0, t1);
	if (is > 0 && is < 3) {
	  reverse = true;
	}
      }
      // add cutline points
      if (reverse) {
	p.addVertex(lineEnd);
	p.addVertex(lineStart);
      } else {
	p.addVertex(lineStart);
	p.addVertex(lineEnd);
      }
    }
    if (p.isHole()) p.reverse();
    supportpolys.push_back(p);
  }

  // remove polygon areas from triangles
  // Clipping clipp;
  // clipp.clear();
  // clipp.addPolys(supportpolys, subject);
  // clipp.addPolys(polys, clip);
  // supportpolys = clipp.subtract(CL::pftPositive,CL::pftPositive);
  
  return true;
}


int find_vertex(const vector<Vector2d> &vertices,  const Vector2d &v, double delta = 0.0001)
{
  for (uint i = 0; i<vertices.size(); i++) {
    if ( (v-vertices[i]).squared_length() < delta ) 
      return i;
  }
  return -1;
}

vector<Segment> Shape::getCutlines(const Matrix4d &T, double z, 
				   vector<Vector2d> &vertices, 
				   double &max_gradient,
				   vector<Triangle> &support_triangles,
				   double supportangle,
				   double thickness) const
{
  Vector2d lineStart;
  Vector2d lineEnd;
  vector<Segment> lines;
  // we know our own tranform:
  Matrix4d transform = T * transform3D.transform ;
  
  int count = (int)triangles.size();
// #ifdef _OPENMP
// #pragma omp parallel for schedule(dynamic) 
// #endif
  for (int i = 0; i < count; i++)
    {
      Segment line(-1,-1);
      int num_cutpoints = triangles[i].CutWithPlane(z, transform, lineStart, lineEnd);
      if (num_cutpoints == 0) {
	if (supportangle >= 0 && thickness > 0) {
	  if (triangles[i].isInZrange(z-thickness, z, transform)) {
	    const double slope = -triangles[i].slopeAngle(transform);
	    if (slope >= supportangle) {
	      support_triangles.push_back(triangles[i].transformed(transform));
	    }
	  }
	}
	continue;
      }
      if (num_cutpoints > 0) {
	int havev = find_vertex(vertices, lineStart);
	if (havev >= 0) 
	  line.start = havev;
	else {
	  line.start = vertices.size();
	  vertices.push_back(lineStart);
	}
	if (abs(triangles[i].Normal.z()) > max_gradient)
	  max_gradient = abs(triangles[i].Normal.z());
	if (supportangle >= 0) {
	  const double slope = -triangles[i].slopeAngle(transform);
	  if (slope >= supportangle)
	    support_triangles.push_back(triangles[i].transformed(transform));
	}
      }
      if (num_cutpoints > 1) {
	int havev = find_vertex(vertices, lineEnd);
	if (havev >= 0) 
	  line.end = havev;
	else {
	  line.end = vertices.size();
	  vertices.push_back(lineEnd);
	}
      }
      // Check segment normal against triangle normal. Flip segment, as needed.
      if (line.start != -1 && line.end != -1 && line.end != line.start)	
	{ // if we found a intersecting triangle
	  Vector3d Norm = triangles[i].Normal;
	  Vector2d triangleNormal = Vector2d(Norm.x(), Norm.y());
	  Vector2d segment = (lineEnd - lineStart);
	  Vector2d segmentNormal(-segment.y(),segment.x()); 
	  triangleNormal.normalize();
	  segmentNormal.normalize();
	  if( (triangleNormal-segmentNormal).squared_length() > 0.2){
	    // if normals do not align, flip the segment
	    int iswap=line.start;line.start=line.end;line.end=iswap;
	  }
	  // cerr << "line "<<line.start << "-"<<line.end << endl;
	  lines.push_back(line);
	}
    }
  return lines;
}




#ifdef WIN32
#  include <GL/glut.h>	// Header GLUT Library
#endif

#ifdef _MSC_VER
#  pragma warning( disable : 4018 4267)
#endif

/* call me before glutting */
void checkGlutInit()
{
	static bool inited = false;
	if (inited)
		return;
	inited = true;
	int argc = 1;
	char *argv[] = { (char *) "repsnapper" };
	glutInit (&argc, argv);
}

void drawString(const Vector3d &pos, const string &text)
{
  drawString(pos,GLUT_BITMAP_HELVETICA_12,text);
}
void drawString(const Vector3d &pos, void* font, const string &text)
{
	checkGlutInit();
	glRasterPos3d(pos.x(), pos.y(), pos.z());
	for (uint c=0;c<text.length();c++)
		glutBitmapCharacter(font, text[c]);
}


// called from Model::draw
void Shape::draw(const Settings &settings, bool highlight) 
{
  //cerr << "Shape::draw" <<  endl;
	// polygons
	glEnable(GL_LIGHTING);

	float no_mat[] = {0.0f, 0.0f, 0.0f, 1.0f};
//	float mat_ambient[] = {0.7f, 0.7f, 0.7f, 1.0f};
//	float mat_ambient_color[] = {0.8f, 0.8f, 0.2f, 1.0f};
	float mat_diffuse[] = {0.1f, 0.5f, 0.8f, 1.0f};
	float mat_specular[] = {1.0f, 1.0f, 1.0f, 1.0f};
//	float no_shininess = 0.0f;
//	float low_shininess = 5.0f;
	float high_shininess = 100.0f;
//	float mat_emission[] = {0.3f, 0.2f, 0.2f, 0.0f};
        int i;


        for (i = 0; i < 4; i++) {
	  mat_diffuse[i] = settings.Display.PolygonRGBA[i];
	}

	if (highlight)
	  //for (i = 0; i < 4; i++)
	  mat_diffuse[3] += 0.3*(1.-mat_diffuse[3]);
	  
	mat_specular[0] = mat_specular[1] = mat_specular[2] = settings.Display.Highlight;

	/* draw sphere in first row, first column
	* diffuse reflection only; no ambient or specular
	*/
	glMaterialfv(GL_FRONT, GL_AMBIENT, no_mat);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialf(GL_FRONT, GL_SHININESS, high_shininess);
	glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);

	// glEnable (GL_POLYGON_OFFSET_FILL);

	if(settings.Display.DisplayPolygons)
	{
		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
//		glDepthMask(GL_TRUE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  //define blending factors
                draw_geometry();
	}

	glDisable (GL_POLYGON_OFFSET_FILL);

	// WireFrame
	if(settings.Display.DisplayWireframe)
	{
		if(!settings.Display.DisplayWireframeShaded)
			glDisable(GL_LIGHTING);


                for (i = 0; i < 4; i++)
                        mat_diffuse[i] = settings.Display.WireframeRGBA[i];
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);

		glColor4fv(settings.Display.WireframeRGBA);
		for(size_t i=0;i<triangles.size();i++)
		{
			glBegin(GL_LINE_LOOP);
			glLineWidth(1);
			glNormal3dv((GLdouble*)&(triangles[i].Normal));
			glVertex3dv((GLdouble*)&(triangles[i].A));
			glVertex3dv((GLdouble*)&(triangles[i].B));
			glVertex3dv((GLdouble*)&(triangles[i].C));
			glEnd();
		}
	}

	glDisable(GL_LIGHTING);

	// normals
	if(settings.Display.DisplayNormals)
	{
		glColor4fv(settings.Display.NormalsRGBA);
		glBegin(GL_LINES);
		for(size_t i=0;i<triangles.size();i++)
		{
			Vector3d center = (triangles[i].A+triangles[i].B+triangles[i].C)/3.0;
			glVertex3dv((GLdouble*)&center);
			Vector3d N = center + (triangles[i].Normal*settings.Display.NormalsLength);
			glVertex3dv((GLdouble*)&N);
		}
		glEnd();
	}

	// Endpoints
	if(settings.Display.DisplayEndpoints)
	{
		glColor4fv(settings.Display.EndpointsRGBA);
		glPointSize(settings.Display.EndPointSize);
		glBegin(GL_POINTS);
		for(size_t i=0;i<triangles.size();i++)
		{
		  glVertex3dv((GLdouble*)&(triangles[i].A));
		  glVertex3dv((GLdouble*)&(triangles[i].B));
		  glVertex3dv((GLdouble*)&(triangles[i].C));
		}
		glEnd();
	}
	glDisable(GL_DEPTH_TEST);
}

// the bounding box is in real coordinates (not transformed)
void Shape::drawBBox() const 
{
		// Draw bbox
		glColor3f(1,0.2,0.2);
		glLineWidth(1);
		glBegin(GL_LINE_LOOP);
		glVertex3f(Min.x(), Min.y(), Min.z());
		glVertex3f(Min.x(), Max.y(), Min.z());
		glVertex3f(Max.x(), Max.y(), Min.z());
		glVertex3f(Max.x(), Min.y(), Min.z());
		glEnd();
		glBegin(GL_LINE_LOOP);
		glVertex3f(Min.x(), Min.y(), Max.z());
		glVertex3f(Min.x(), Max.y(), Max.z());
		glVertex3f(Max.x(), Max.y(), Max.z());
		glVertex3f(Max.x(), Min.y(), Max.z());
		glEnd();
		glBegin(GL_LINES);
		glVertex3f(Min.x(), Min.y(), Min.z());
		glVertex3f(Min.x(), Min.y(), Max.z());
		glVertex3f(Min.x(), Max.y(), Min.z());
		glVertex3f(Min.x(), Max.y(), Max.z());
		glVertex3f(Max.x(), Max.y(), Min.z());
		glVertex3f(Max.x(), Max.y(), Max.z());
		glVertex3f(Max.x(), Min.y(), Min.z());
		glVertex3f(Max.x(), Min.y(), Max.z());
		glEnd();

  glColor3f(1,0.6,0.6);
  ostringstream val;
  val.precision(1);
  Vector3d pos;
  val << fixed << (Max.x()-Min.x());
  pos = Vector3d((Max.x()+Min.x())/2.,Min.y(),Max.z());
  drawString(pos,val.str());
  val.str("");
  val << fixed << (Max.y()-Min.y());
  pos = Vector3d(Min.x(),(Max.y()+Min.y())/2.,Max.z());
  drawString(pos,val.str());
  val.str("");
  val << fixed << (Max.z()-Min.z());
  pos = Vector3d(Min.x(),Min.y(),(Max.z()+Min.z())/2.);
  drawString(pos,val.str());
}


void Shape::draw_geometry() 
{
	Glib::TimeVal starttime;
	starttime.assign_current_time();

	glBegin(GL_TRIANGLES);
	for(size_t i=0;i<triangles.size();i++)
	{
#if 0
		switch(triangles[i].axis)
			{
			case NEGX:	glColor4f(1,0,0,opacity); break;
			case POSX:	glColor4f(0.5f,0,0,opacity); break;
			case NEGY:	glColor4f(0,1,0,opacity); break;
			case POSY:	glColor4f(0,0.5f,0,opacity); break;
			case NEGZ:	glColor4f(0,0,1,opacity); break;
			case POSZ:	glColor4f(0,0,0.3f,opacity); break;
			default: glColor4f(0.2f,0.2f,0.2f,opacity); break;
			}
#endif
		glNormal3dv((GLdouble*)&(triangles[i].Normal));
		glVertex3dv((GLdouble*)&(triangles[i].A));
		glVertex3dv((GLdouble*)&(triangles[i].B));
		glVertex3dv((GLdouble*)&(triangles[i].C));
	}
	glEnd();

	Glib::TimeVal endtime;
	endtime.assign_current_time();
	Glib::TimeVal usedtime = endtime-starttime;
	if (usedtime.as_double() > 0.3) slow_drawing = true;
}

/*
 * sometimes we find adjacent polygons with shared boundary
 * points and lines; these cause grief and slowness in
 * LinkSegments, so try to identify and join those polygons
 * now.
 */
// ??? as only coincident lines are removed, couldn't this be
// done easier by just running through all lines and finding them?
bool CleanupSharedSegments(vector<Segment> &lines)
{
#if 1 // just remove coincident lines
  vector<int> lines_to_delete;

  for (uint j = 0; j < lines.size(); j++) {
    const Segment &jr = lines[j];
    for (uint k = j + 1; k < lines.size(); k++)
      {
	const Segment &kr = lines[k];
	if ((jr.start == kr.start && jr.end == kr.end) ||
	    (jr.end == kr.start && jr.start == kr.end))
	  {
	    lines_to_delete.push_back (j);  // remove both???
	    //lines_to_delete.push_back (k);
	    break;
	  }
      }
  }
  // we need to remove from the end first to avoid disturbing
  // the order of removed items
  std::sort(lines_to_delete.begin(), lines_to_delete.end());
  for (int r = lines_to_delete.size() - 1; r >= 0; r--)
    {
      lines.erase(lines.begin() + lines_to_delete[r]);
    }
  return true;

#endif


#if 0
  vector<int> vertex_counts; // how many lines have each point
  vertex_counts.resize (vertices.size());
  
  for (uint i = 0; i < lines.size(); i++)
    {
      vertex_counts[lines[i].start]++; 
      vertex_counts[lines[i].end]++;
    }
  
  // ideally all points have an entrance and
  // an exit, if we have co-incident lines, then
  // we have more than one; do we ?
  std::vector<int> duplicate_points;
  for (uint i = 0; i < vertex_counts.size(); i++)
    if (vertex_counts[i] > 2) // no more than 2 lines should share a point
      duplicate_points.push_back (i);
  
  if (duplicate_points.size() == 0)
    return true; // all sane
  
  for (uint i = 0; i < duplicate_points.size(); i++)
    {
      std::vector<int> dup_lines;
      
      // find all line segments with this point in use 
      for (uint j = 0; j < lines.size(); j++)
	{
	  if (lines[j].start == duplicate_points[i] ||
	      lines[j].end == duplicate_points[i])
	    dup_lines.push_back (j);
	}
      
      // identify and eliminate identical line segments
      // NB. hopefully by here dup_lines.size is small.
      std::vector<int> lines_to_delete;
      for (uint j = 0; j < dup_lines.size(); j++)
	{
	  const Segment &jr = lines[dup_lines[j]];
	  for (uint k = j + 1; k < dup_lines.size(); k++)
	    {
	      const Segment &kr = lines[dup_lines[k]];
	      if ((jr.start == kr.start && jr.end == kr.end) ||
		  (jr.end == kr.start && jr.start == kr.end))
		{
		  lines_to_delete.push_back (dup_lines[j]);
		  lines_to_delete.push_back (dup_lines[k]);
		}
	    }
	}
      // we need to remove from the end first to avoid disturbing
      // the order of removed items
      std::sort(lines_to_delete.begin(), lines_to_delete.end());
      for (int r = lines_to_delete.size() - 1; r >= 0; r--)
	{
	  lines.erase(lines.begin() + lines_to_delete[r]);
	}
    }
  return true;
#endif
}

/*
 * Unfortunately, finding connections via co-incident points detected by
 * the PointHash is not perfect. For reasons unknown (probably rounding
 * errors), this is often not enough. We fall-back to finding a nearest
 * match from any detached points and joining them, with new synthetic
 * segments.
 */
bool CleanupConnectSegments(const vector<Vector2d> &vertices, vector<Segment> &lines, bool connect_all)
{
	vector<int> vertex_types;
	vertex_types.resize (vertices.size());
	// vector<int> vertex_counts;
	// vertex_counts.resize (vertices.size());

	// which vertices are referred to, and how much:
	for (uint i = 0; i < lines.size(); i++)
	{
		vertex_types[lines[i].start]++;
		vertex_types[lines[i].end]--;
		// vertex_counts[lines[i].start]++;
		// vertex_counts[lines[i].end]++;
	}

	// the vertex_types count should be zero for all connected lines,
	// positive for those ending no-where, and negative for
	// those starting no-where.
	std::vector<int> detached_points; // points with only one line
	for (uint i = 0; i < vertex_types.size(); i++)
	{
		if (vertex_types[i])
		{
#if CUTTING_PLANE_DEBUG
			cout << "detached point " << i << "\t" << vertex_types[i] << " refs at " << vertices[i].x() << "\t" << vertices[i].y() << "\n";
#endif
			detached_points.push_back (i); // i = vertex index
		}
	}

	// Lets hope we have an even number of detached points
	if (detached_points.size() % 2) {
		cout << "oh dear - an odd number of detached points => an un-pairable impossibility\n";
		return false;
	}

	// pair them nicely to their matching type
	for (uint i = 0; i < detached_points.size(); i++)
	{
		double nearest_dist_sq = (std::numeric_limits<double>::max)();
		uint   nearest = 0;
		int   n = detached_points[i]; // vertex index of detached point i
		if (n < 0) // handled already
		  continue;

		const Vector2d &p = vertices[n]; // the real detached point
		// find nearest other detached point to the detached point n:
		for (uint j = i + 1; j < detached_points.size(); j++)
		{
		        int pt = detached_points[j];
			if (pt < 0) 
			  continue; // already connected

			// don't connect a start to a start, or end to end
			if (vertex_types[n] == vertex_types[pt])
			        continue; 

			const Vector2d &q = vertices[pt];  // the real other point
			double dist_sq = (p-q).squared_length(); //pow (p.x() - q.x(), 2) + pow (p.y() - q.y(), 2);
			if (dist_sq < nearest_dist_sq)
			{
				nearest_dist_sq = dist_sq;
				nearest = j;
			}
		}
		//assert (nearest != 0);
		if (nearest == 0) continue;

		// allow points 10mm apart to be joined, not more
		if (!connect_all && nearest_dist_sq > 100.0) {
			cout << "oh dear - the nearest connecting point is " << sqrt (nearest_dist_sq) << "mm away - not connecting\n";
			continue; //return false;
		}
		// warning above 1mm apart 
		if (!connect_all && nearest_dist_sq > 1.0) {
			cout << "warning - the nearest connecting point is " << sqrt (nearest_dist_sq) << "mm away - connecting anyway\n";
		}

#if CUTTING_PLANE_DEBUG
		cout << "add join of length " << sqrt (nearest_dist_sq) << "\n" ;
#endif
		Segment seg(n, detached_points[nearest]);
		if (vertex_types[n] > 0) // already had start but no end at this point
			seg.Swap();
		lines.push_back(seg);
		detached_points[nearest] = -1;
	}

	return true;
}


string Shape::info() const
{
  ostringstream ostr;
  ostr <<"Shape with "<<triangles.size() << " triangles "
       << "min/max/center: "<<Min<<Max <<Center ;
  return ostr.str();
}






////////////////////////////// FlatShape /////////////////////////

FlatShape::FlatShape()
{
  slow_drawing = false;
  Min.set(0,0,0);
  Max.set(200,200,0);
  CalcBBox();
}


FlatShape::FlatShape(string filename) 
{
  slow_drawing = false;
  this->filename = filename;
  loadSVG(filename);
}
  
// FlatShape::FlatShape(const FlatShape &rhs) 
// {
//   slow_drawing = false;
//   polygons = rhs.polygons;
//   scale_factor_x = rhs.scale_factor_x;
//   scale_factor_y = rhs.scale_factor_y;
//   scale_factor = rhs.scale_factor;
//   Min = rhs.Min;
//   Max = rhs.Max;
//   Center = rhs.Center;
// }

vector<string> REMatches(const Glib::RefPtr<Glib::Regex> &RE,
			 const string &input,
			 const string &name)
{
  Glib::MatchInfo match_info;
  vector<string> matches;
  if (RE->match(input, match_info)) {
    matches.push_back(match_info.fetch_named(name).c_str());
    while (match_info.next()){
      matches.push_back(match_info.fetch_named(name).c_str());
    }
  }
  return matches;
}
vector<string> REMatches(const string &regex, 
			 const string &input,
			 const string &name)
{
  Glib::RefPtr<Glib::Regex> RE = Glib::Regex::create(regex);
  return REMatches(RE, input,name);
}

// int loadSVGold(string filename)
// {
//   Min=Vector3d(1000000,1000000,0);
//   Max=Vector3d(-1000000,-1000000,0);

//   polygons.clear();
  
//   ifstream file;
//   file.open(filename.c_str());
    
  
//   string lines;  
//   string line;
//   if (file.is_open()) {
//     while ( file.good() ) {
//       getline (file,line);
//       lines += line;
//     }
//     file.close();

//     Glib::RefPtr<Glib::Regex> polyregex = 
//       Glib::Regex::create ("(?ims)<path.*?stroke\\:none.*?\\sd\\=\"(?<POLY>.*?(Z|\"/\\>))");
//     Glib::RefPtr<Glib::Regex> strokeregex = 
//       Glib::Regex::create ("(?ims)<path.*?(?<STROKE>fill\\:none.*?\\sd\\=\".*?\"/\\>)");
//     Glib::RefPtr<Glib::Regex> strwidthregex =
//       Glib::Regex::create ("stroke\\-width\\:(?<STRWIDTH>[\\-\\.\\d]+)");
//     Glib::RefPtr<Glib::Regex> pointregex =
//       Glib::Regex::create ("(?<POINT>[LM](\\s+[\\-\\.\\d]+){2})");
//     Glib::RefPtr<Glib::Regex> transregex =
//       Glib::Regex::create ("transform=\"(?<TRANS>.*?)\"");
//     Glib::RefPtr<Glib::Regex> matrixregex =
//       Glib::Regex::create ("matrix\\((?<MATR>([\\-\\.\\d]*?(\\,|\\))){6})");

//     vector<string> strokes = REMatches(strokeregex, lines, "STROKE");
//     for (uint i = 0; i < strokes.size(); i++) {
//       cerr << i << ": "<<strokes[i] << endl;
//       vector<string> strwidth = REMatches(strwidthregex,strokes[i],"STRWIDTH");
//       for (uint j = 0; j < strwidth.size(); j++) {
// 	cerr << "STRW " << strwidth[j] << endl;
//       }
//       vector<string> points = REMatches(pointregex,strokes[i],"POINT");
//       for (uint j = 0; j < points.size(); j++) {
// 	cerr << j << ":: "<<points[j]<< endl;
//       }
//       vector<string> trans = REMatches(transregex,strokes[i],"TRANS");
//       for (uint j = 0; j < trans.size(); j++) {
// 	cerr << j << "trans: "<<trans[j]<< endl;
// 	vector<string> matrix = REMatches(matrixregex,trans[j],"MATR");
// 	for (uint k = 0; k < matrix.size(); k++) 
// 	  cerr << k << " matr: "<<matrix[k]<< endl;	  
//       }
//     }
//     vector<string> polys = REMatches(polyregex, lines, "POLY");
//     for (uint i = 0; i < polys.size(); i++) {
//       vector<string> points = REMatches(pointregex, polys[i], "POINT");
//       Poly poly;
//       //cout << i << ": ";
//       //cout << polys[i] << endl;
//       for (uint j = 0; j < points.size(); j++) {
// 	//cout << j << " - " << points[j]  << endl ;
// 	istringstream is(points[j]);
// 	string type;
// 	is >> type;
// 	//cerr << type<< endl;
// 	if (type=="M" || type == "L") {
// 	  double x,y;
// 	  is >> x;
// 	  is >> y;
// 	  //cout << x << "," << y << endl;
// 	  poly.addVertex(x,y);
// 	  if (x<Min.x()) Min.x() = x;
// 	  if (y<Min.y()) Min.y() = y;
// 	  if (x>Max.x()) Max.x() = x;
// 	  if (y>Max.y()) Max.y() = y;
// 	}
//       }
//       if (poly.size()>0) {
// 	poly.setZ(0);
// 	//cerr << poly.info() << endl;
// 	polygons.push_back(poly);
//       }
//       cout << endl;
//     }
//   }
//   else cerr << _("Error: Unable to open SVG file - ") << filename << endl;
//   Center = (Min+Max)/2;
// }

bool FlatShape::getPolygonsAtZ(const Matrix4d &T, double z, 
			       vector<Poly> &polys, double &max_grad) const
{
  max_grad = 0;
  polys = polygons;
  const Matrix4d trans = T * transform3D.transform;
  //cerr << " get polys at "<< z  << " with " << endl<<trans << endl;
  for (uint i = 0; i < polys.size(); i++) {
    polys[i].setZ(0);
    polys[i].transform(trans);
  }
  return true;
}

void FlatShape::clear()
{
  polygons.clear();
}

void FlatShape::draw_geometry() {
  const Matrix4d invT = transform3D.getInverse();
  const Vector3d minT = invT*Min;
  const Vector3d maxT = invT*Max;
  const Vector2d min2d(minT.x(), minT.y());
  const Vector2d max2d(maxT.x(), maxT.y());
  glDrawPolySurfaceRastered(polygons, min2d, max2d, 0, 0.1);
  for (uint i = 0; i < polygons.size(); i++) {
    polygons[i].draw(GL_LINE_LOOP,false);
    // Poly p;
    // p.vertices = simplified(polygons[i].vertices, 0.2);
    // cleandist(p.vertices, 0.2);
    // p.draw_as_surface(); 

    //polygons[i].draw_as_surface();
  }
}

void FlatShape::CalcBBox()
{
  Min.set(INFTY,INFTY,0);
  Max.set(-INFTY,-INFTY,0);
  for(size_t i = 0; i < polygons.size(); i++)
    for(size_t j = 0; j < polygons[i].size(); j++){
      if ( polygons[i][j].x() < Min.x() ) Min.x() = polygons[i][j].x();
      if ( polygons[i][j].y() < Min.y() ) Min.y() = polygons[i][j].y();
      if ( polygons[i][j].x() > Max.x() ) Max.x() = polygons[i][j].x();
      if ( polygons[i][j].y() > Max.y() ) Max.y() = polygons[i][j].y();
    }
  Min = transform3D.transform*Min;
  Max = transform3D.transform*Max;
  Center = (Max + Min )/2;
}


void FlatShape::invertNormals()
{
  for (uint i = 0; i < polygons.size(); i++)
    polygons[i].reverse();
}

void FlatShape::mirror()
{
  for (uint i = 0; i < polygons.size(); i++)
    polygons[i].mirrorX(Center);
}

// Rotate and adjust for the user - not a pure rotation by any means
void FlatShape::Rotate(const Vector3d & axis, const double & angle)
{
  CalcBBox();
  if (axis.z()==0) return; // try to only 2D-rotate
  Vector2d center(Center.x(),Center.y());
  for (size_t i=0; i<polygons.size(); i++)
    {
      polygons[i].rotate(center, angle);
    }
  PlaceOnPlatform();
}

void FlatShape::splitshapes(vector<Shape*> &shapes, ViewProgress *progress) 
{
  uint count = polygons.size();
  if (progress) progress->start(_("Split Polygons"), count);
  int progress_steps = max(1,(int)(count/100));

  for (uint i = 0; i < count; i++) {
    FlatShape *fs  = new FlatShape();
    fs->polygons.push_back(polygons[i]);
    if (progress && i%progress_steps==0 && !progress->update(count)) break;
    shapes.push_back(fs);
  }
  progress->stop("_(Done)");
}


string FlatShape::info() const
{
  ostringstream ostr;
  ostr <<"FlatShape with "<<polygons.size() << " polygons "
       << "min/max/center: "<<Min<<Max <<Center ;
  return ostr.str();
}

////////////////////////////// XML //////////////////////////////////////////

inline double ToDouble(string s)
{
	std::istringstream i(s);
	double x;
	if (!(i >> x))
		return -1;
	return x;
}

const Glib::RefPtr<Glib::Regex> polyregex = 
    Glib::Regex::create ("(?ims)<path.*?stroke\\:none.*?\\sd\\=\"(?<POLY>.*?(Z|\"/\\>))");
const Glib::RefPtr<Glib::Regex> strokeregex = 
    Glib::Regex::create ("(?ims)<path.*?(?<STROKE>fill\\:none.*?\\sd\\=\".*?\"/\\>)");
const Glib::RefPtr<Glib::Regex> strwidthregex =
    Glib::Regex::create ("stroke\\-width\\:(?<STRWIDTH>[\\-\\.\\d]+)");
const Glib::RefPtr<Glib::Regex> pointregex =
    Glib::Regex::create ("(?<POINT>[LM](\\s+[\\-\\.\\d]+){2})");
const Glib::RefPtr<Glib::Regex> transregex =
    Glib::Regex::create ("transform=\"(?<TRANS>.*?)\"");
const Glib::RefPtr<Glib::Regex> matrixregex =
    Glib::Regex::create ("matrix\\((?<MATR>([\\-\\.\\d]*?(\\,|\\))){6})");


Matrix3d svg_trans(const string &line)
{
  Matrix3d mat;
  vector<string> val = REMatches(matrixregex, line, "MATR");
  if (val.size()>0) {
    vector<string> vals = Glib::Regex::split_simple("[\\,\\)]",val[0]);
    if (vals.size()>5) {
      mat.set_row(0,Vector3d(ToDouble(vals[0]),ToDouble(vals[2]),ToDouble(vals[4])));
      mat.set_row(1,Vector3d(ToDouble(vals[1]),ToDouble(vals[3]),ToDouble(vals[5])));
      mat.set_row(2,Vector3d(0,0,1));
    }
  }
  return mat;
}

string get_attr(const string &line, const string &attrname)
{
  vector<string> parts = Glib::Regex::split_simple(";",line);
  for (uint p = 0; p < parts.size(); p++) {
    vector<string> lr = Glib::Regex::split_simple(":",parts[p]);
    if (lr.size()==2){
      if (lr[0] == attrname) return lr[1];
    } else return "";
  }
  return "";
}

vector<Vector2d> ToVertices(const string &line)
{
  vector<string> points = REMatches(pointregex, line, "POINT");
  vector<Vector2d> v;
  for (uint j = 0; j < points.size(); j++) {
    //cout << j << " - " << points[j]  << endl ;
    istringstream is(points[j]);
    string type;
    is >> type;
    //cerr << type<< endl;
    if (type=="M" || type == "L") {
      double x,y;
      if (is >> x && is >> y) 
	//cout << x << "," << y << endl;
	v.push_back(Vector2d(x,y));
    }
  }
  return v;
}

int FlatShape::svg_addPolygon()
{ 
  
  vector<Poly> polys;
  if (svg_cur_style.find("stroke:none") != string::npos) { // polygon
    Poly poly;
    poly.vertices = ToVertices(svg_cur_path);
    poly.setZ(0);
    poly.reverse();
    polys.push_back(poly);
  }
  else if (svg_cur_style.find("fill:none") != string::npos) { // stroke
    // cerr << "stroke " << svg_cur_path << endl;
    // cerr << "\t" << svg_cur_style << endl;
    string wstr = get_attr(svg_cur_style,"stroke-width");
    double width = ToDouble(wstr);
    // cerr << "\t width " << wstr << " = " << width << endl;
    vector<Vector2d> vertices = ToVertices(svg_cur_path);
    polys = thick_lines(vertices, width);
    // cerr <<"thick "<< polys.size()<<" of " << vertices.size() << endl;
  }
  else if (svg_cur_style!="") { 
    cerr << "unknown " << svg_cur_path << " in " << svg_cur_name << endl;
    cerr << "\t style: " << svg_cur_style << endl;
  }

  if (polys.size()>0) {
    if (svg_cur_trans!="") {
      // cerr << svg_cur_trans << endl;
      Matrix3d  mat = svg_trans(svg_cur_trans);
      // cerr << mat << endl;
      for (uint i=0; i < polys.size(); i++) {
	polys[i].setZ(0);
	polys[i].transform(mat);
      }
    }
    polygons.insert(polygons.begin(),polys.begin(),polys.end());
  }
  return polys.size();
} 


void FlatShape::xml_handle_node(const xmlpp::Node* node)
{
  //std::cout << std::endl; //Separate nodes by an empty line.
  
  const xmlpp::ContentNode* nodeContent = dynamic_cast<const xmlpp::ContentNode*>(node);
  const xmlpp::TextNode*       nodeText = dynamic_cast<const xmlpp::TextNode*>(node);
  const xmlpp::CommentNode* nodeComment = dynamic_cast<const xmlpp::CommentNode*>(node);

  // if(nodeText && nodeText->is_white_space()) //Let's ignore the indenting - you don't always want to do this.
  //   return;
    
  const Glib::ustring nodename = node->get_name();

  if(!nodeText && !nodeComment && !nodename.empty()) //Let's not say "name: text".
  {
    const Glib::ustring namespace_prefix = node->get_namespace_prefix();
    if (svg_cur_name != "") svg_addPolygon();
    svg_cur_name = nodename;
    svg_cur_path = "";
    svg_cur_trans = "";
    svg_cur_style = "";
    // if(namespace_prefix.empty()) 
    //   std::cout << "Node name = " << nodename << std::endl;
    // else
    //   std::cout << "Node name = " << namespace_prefix << ":" << nodename << std::endl;
  }
  // else if(nodeText) //Let's say when it's text. - e.g. let's say what that white space is.
  // {
  //   std::cout << "Text Node " << nodename << nodeText->get_content()<< std::endl;
  // }

  //Treat the various node types differently: 
  if(nodeText)
  {
    ; //std::cout << "text = \"" << nodeText->get_content() << "\"" << std::endl;
  }
  else if(nodeComment)
  {
    ; //std::cout << "comment = " << nodeComment->get_content() << std::endl;
  }
  else if(nodeContent)
  {
    ; //std::cout << "content = " << nodeContent->get_content() << std::endl;
  } else
  if (const xmlpp::Element* nodeElement = dynamic_cast<const xmlpp::Element*>(node)) {
    //A normal Element node:
    
    //line() works only for ElementNodes.
    // std::cout << "     line = " << node->get_line() << std::endl;

    //attributes:
    const xmlpp::Element::AttributeList& attributes = nodeElement->get_attributes();

    for(xmlpp::Element::AttributeList::const_iterator iter = attributes.begin(); 
	iter != attributes.end(); ++iter) {
      const xmlpp::Attribute* attribute = *iter;

      const Glib::ustring namespace_prefix = attribute->get_namespace_prefix();
      string attr = attribute->get_name();
      if (attr=="d")
	svg_cur_path  = attribute->get_value();
      else if (attr == "style")
	svg_cur_style = attribute->get_value();
      else if (attr == "transform")
	svg_cur_trans = attribute->get_value();
      else if (svg_cur_name == "svg" ){
	if (attr == "width" || attr == "height") {
	  string val = attribute->get_value();
	  if (val.find("pt") != string::npos)
	    svg_prescale = 0.3527;
	}	  
      }
      else if (attr=="id") {
      }
      else 
	cerr << "unknown Attribute in " << svg_cur_name << " : " << attr << " = " <<attribute->get_value() << endl;
      // if(namespace_prefix.empty()) 
      //   std::cout << "  Attribute " << attribute->get_name() << " = " <<  << std::endl; 
      // else
      //   std::cout << "  Attribute " << namespace_prefix  << ":" << attribute->get_name() << " = " << attribute->get_value() << std::endl;
    }

    const xmlpp::Attribute* attribute = nodeElement->get_attribute("title");
    if(attribute) {
      std::cout << "title found: =" << attribute->get_value() << std::endl;
    }
  }
  
  if(!nodeContent)
  {
    //Recurse through child nodes:
    xmlpp::Node::NodeList list = node->get_children();
    for(xmlpp::Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter)
      {
	xml_handle_node(*iter); //recursive
      }
    // get last bit (?)
    if (svg_cur_name!="") svg_addPolygon();
  }
}


int FlatShape::loadSVG(string filename) 
{  // Set the global C++ locale to the user-configured locale,
  // so we can use std::cout with UTF-8, via Glib::ustring, without exceptions.

  #ifdef LIBXMLCPP_EXCEPTIONS_ENABLED
  try
  {
  #endif //LIBXMLCPP_EXCEPTIONS_ENABLED 
    xmlpp::DomParser parser;
    //parser.set_validate();
    parser.set_substitute_entities(); //We just want the text to be resolved/unescaped automatically.
    parser.parse_file(filename);
    if(parser)
    {
      polygons.clear();
      svg_cur_name = "";
      svg_cur_path = "";
      svg_cur_trans = "";
      svg_cur_style = "";
      svg_prescale = 1.;
      //Walk the tree:
      const xmlpp::Node* pNode = parser.get_document()->get_root_node(); //deleted by DomParser.
      xml_handle_node(pNode);
    }

    if (svg_prescale!=1)
      for (uint i= 0; i<polygons.size(); i++) 
	for (uint j= 0; j<polygons[i].size(); j++) 
	  polygons[i].vertices[j] *= svg_prescale;
    Clipping clipp;
    clipp.addPolys(polygons, subject);
    polygons = clipp.unite(CL::pftNonZero,CL::pftNegative);
    CalcBBox();
    Vector2d center2(Center.x(),Center.y());
    for (uint i= 0; i<polygons.size(); i++) {
      polygons[i].mirrorX(Center);
      polygons[i].rotate(center2, M_PI);
    }
    CalcBBox();
    
  #ifdef LIBXMLCPP_EXCEPTIONS_ENABLED
  }
  catch(const std::exception& ex)
  {
    std::cerr << "Exception caught: " << ex.what() << std::endl;
  }
  #endif //LIBXMLCPP_EXCEPTIONS_ENABLED 

  return 0;
}


