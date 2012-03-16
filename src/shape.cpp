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
#include "poly.h"
#include "progress.h"

//#include "vrml.h"



void Transform3D::move(Vector3d delta)
{
  Vector3d trans = transform.getTranslation();
  transform.setTranslation(trans + delta);
}

void Transform3D::scale(double x)
{
  transform.m[3][3] = 1/x;
}

void Transform3D::scale_x(double x)
{
  transform.m[0][0] = x;
}
  void Transform3D::scale_y(double x)
{
  transform.m[1][1] = x;
}
void Transform3D::scale_z(double x)
{
  transform.m[2][2] = x;
}

void Transform3D::rotate(Vector3d axis, double angle)
{
  Vector3d naxis=axis;
  naxis.normalise();
  transform.rotate(angle,naxis );
}
void Transform3D::rotate(Vector3d center, double x, double y, double z)
{
  move(-center);
  if (x!=0) transform.rotateX(x);
  if (y!=0) transform.rotateY(y);
  if (z!=0) transform.rotateZ(z);
  move(center);
}


// Constructor
Shape::Shape()
{
	Min.x = Min.y = Min.z = 0.0;
	Max.x = Max.y = Max.z = 200.0;
	scale_factor_x = 1;
	scale_factor_y = 1;
	scale_factor_z = 1;
	scale_factor = 1;
	CalcBBox();
}


Shape::Shape(string filename, istream *text){
  this->filename = filename;
  parseASCIISTL(text);
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

/* Loads an binary STL file by filename
 * Returns 0 on success and -1 on failure */
int Shape::loadBinarySTL(string filename) {

    // if(getFileType(filename) != BINARY_STL) {
    //     return -1;
    // }

    triangles.clear();
    Min.x = Min.y = Min.z = numeric_limits<double>::infinity();
    Max.x = Max.y = Max.z = -numeric_limits<double>::infinity();

    ifstream file;
    file.open(filename.c_str());

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

    for(uint i = 0; i < num_triangles; i++)
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

	// done in Triangle
        /* Recalculate normal vector - can't trust an STL file! */
        // Vector3d AA=Cx-Ax;
        // Vector3d BB=Cx-Bx;
	// N = AA.cross(BB).getNormalized();

        /* attribute byte count - sometimes contains face color
            information but is useless for our purposes */
        unsigned short byte_count;
        file.read(reinterpret_cast <char *> (buffer), 2);
	byte_count = buffer[0] | buffer[1] << 8;
	// Repress unused variable warning.
	(void)&byte_count;

        Triangle T(Ax,Bx,Cx);

	//cout << "bin triangle "<< N << ":\n\t" << Ax << "/\n\t"<<Bx << "/\n\t"<<Cx << endl;

        triangles.push_back(T);
    }
    file.close();
    CenterAroundXY();
    scale_factor = 1.0;
    scale_factor_x=scale_factor_y=scale_factor_z = 1.0;
    cout << _("Shape has volume ") << volume() << " mm^3"<<endl;
    return 0;
}


int Shape::loadASCIIVRML(std::string filename) {
  if(getFileType(filename) != VRML) {
    cerr << _("No VRML file file passed to loadASCIIVRML") << endl;
    return -1;
  }
    triangles.clear();
    Min.x = Min.y = Min.z = numeric_limits<double>::infinity();
    Max.x = Max.y = Max.z = -numeric_limits<double>::infinity();
  
  //#ifndef HAVE_OPENVRML
#if 1
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

    //cerr << vertices.size() << " - " << indices.size() << endl;
    if (indices.size()%4!=0) return -1;
    if (vertices.size()%3!=0) return -1;
    //cerr <<"read ok" << endl;
    vector<Vector3d> vert;
    for (uint i=0; i<vertices.size();i+=3)
      vert.push_back(Vector3d(vertices[i],
			      vertices[i+1],
			      vertices[i+2]));
    for (uint i=0; i<indices.size();i+=4){
      Triangle T(vert[indices[i]],vert[indices[i+1]],vert[indices[i+2]]);
      triangles.push_back(T);
      //triangles.push_back(t);
    }

#else // HAVE_OPENVRML
      // VRMLFile vf(filename);
      // this->triangles = vf.getTriangles();
#endif
    CenterAroundXY();
    scale_factor = 1.0;
    scale_factor_x=scale_factor_y=scale_factor_z = 1.0;
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
    Min.x = Min.y = Min.z = numeric_limits<double>::infinity();
    Max.x = Max.y = Max.z = -numeric_limits<double>::infinity();

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
                 >> vertices[i].x
                 >> vertices[i].y
                 >> vertices[i].z;

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

	// done in Triangle
        /* Recalculate normal vector - can't trust an STL text! */
        // Vector3d AA=vertices[2]-vertices[0];
        // Vector3d BB=vertices[2]-vertices[1];
	// normal_vec = AA.cross(BB).getNormalized();


        // Create triangle object and push onto the vector
        Triangle triangle(vertices[0],
			  vertices[1],
			  vertices[2]);

	//	cout << "txt triangle "<< normal_vec << ":\n\t" << vertices[0] << "/\n\t"<<vertices[1] << "/\n\t"<<vertices[2] << endl;
        triangles.push_back(triangle);
    }
    CenterAroundXY();
    // cerr << triangles.size() << endl;
    // cerr << Center << " - " << Min << " - " << Max <<  endl;
    scale_factor = 1.0;
    scale_factor_x=scale_factor_y=scale_factor_z = 1.0;
    cout << _("Shape has volume ") << volume() << " mm^3"<<endl;
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

    // OptimizeRotation(); // no, I probably have prepared the file
    CenterAroundXY();

    //cout << getSTLsolid(0) << endl;
    scale_factor = 1.0;
    scale_factor_x=scale_factor_y=scale_factor_z = 1.0;
    return 0;
}


bool Shape::hasAdjacentTriangleTo(Triangle triangle, double sqdistance) const
{
  for (uint i = 0; i < triangles.size(); i++)
    if (triangle.isConnectedTo(triangles[i],sqdistance)) return true;
  return false;
}
void Shape::splitshapes(vector<Shape> &shapes, ViewProgress *progress) 
{
  uint n_tr = triangles.size();
  vector<bool> done(n_tr);
  for (uint i = 0; i < n_tr; i++) done[i] = false;
  uint donetriangles = 0;
  if (progress) progress->start(_("Split Shapes"), n_tr);
  int progress_steps = (int)(n_tr/100);
  while (donetriangles < n_tr) 
    {
      bool foundadj = false;
      for (uint s = 0; s < shapes.size(); s++){
	for (uint i = 0; i < n_tr; i++){
	  if (!done[i]) {
	    if (//shapes[s].triangles.size() == 0 ||
		shapes[s].hasAdjacentTriangleTo(triangles[i])) {
	      shapes[s].triangles.push_back(triangles[i]);
	      done[i] = true;
	      donetriangles++;
	      foundadj = true;
	    }
	  }
	}
      } 
      //  have no shape any triangle attaches to
      if (!foundadj) {
	Shape shape;
	// take next undone triangle for new shape
	for (uint i = 0; i < n_tr; i++) 
	  if (!done[i]) {
	    shapes.push_back(shape);
	    shapes.back().triangles.push_back(triangles[i]);
	    cerr << _("shape ") << shapes.size() << endl;
	    done[i] = true;
	    donetriangles++;
	    break;
	  }
      }
      if (progress && donetriangles%progress_steps==0) 
	progress->update(donetriangles);
   }
   if (progress) progress->stop("_(Done)");
   cerr << shapes.size() << _(" shapes ") << endl;
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


void Shape::Scale(double in_scale_factor)
{
  transform3D.scale(in_scale_factor);
  scale_factor = in_scale_factor;
  CalcBBox();
}

void Shape::ScaleX(double x)
{
  transform3D.scale_x(x);
  scale_factor_x = x;
  CalcBBox();
  return;
}
void Shape::ScaleY(double x)
{
  transform3D.scale_y(x);
  scale_factor_y = x;
  CalcBBox();
  return;
}
void Shape::ScaleZ(double x)
{
  transform3D.scale_z(x);
  scale_factor_z = x;
  CalcBBox();
  return;
}
#if 0
    for(size_t i = 0; i < triangles.size(); i++)
    {
        for(int j = 0; j < 3; j++)
        {
            /* Translate to origin */
            triangles[i][j] = triangles[i][j] - Min;
            triangles[i][j].scale(in_scale_factor/scale_factor);
            triangles[i][j] = triangles[i][j] + Min;
        }
    }

    //Min = Min - Center;
    //Min.scale(in_scale_factor/scale_factor);
    //Min = Min + Center;

    Max -= Min;
    Max.scale(in_scale_factor/scale_factor);
    Max += Min;
    
    CenterAroundXY();

    /* Save current scale_factor */
    scale_factor = in_scale_factor;
#endif


void Shape::CalcBBox()
{
  Min.x = Min.y = Min.z = numeric_limits<double>::infinity();
  Max.x = Max.y = Max.z = -numeric_limits<double>::infinity();
  for(size_t i = 0; i < triangles.size(); i++)
    triangles[i].AccumulateMinMax (Min, Max, transform3D.transform);
  Center = (Max + Min )/2;
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
      for (uint n=0; n < normals.size(); n++) {
	if ((normals[n].normal - triangles[i].Normal).lengthSquared() < 0.000001) {
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
  for (uint n=0; n < normals.size(); n++) { 
    //cerr << n << normals[n] << endl;
    N = normals[n];
    angle = acos(N.dot(Z));
    if (angle>0) {
      Vector3d axis = N.cross(Z);
      if (axis.lengthSquared()>0.1) {
	Rotate(axis,angle);
	break;
      }
    }
  }


#if 0  // this only does 90° rotations 

	// Find the axis that has the largest surface
	// Rotate to point this face towards -Z

	// if dist center <|> 0.1 && Normal points towards, add area

	Vector3d AXIS_VECTORS[3];
	AXIS_VECTORS[0] = Vector3d(1,0,0);
	AXIS_VECTORS[1] = Vector3d(0,1,0);
	AXIS_VECTORS[2] = Vector3d(0,0,1);

	double area[6];
	for(uint i=0;i<6;i++)
		area[i] = 0.0;

	for(size_t i=0;i<triangles.size();i++)
	{
		triangles[i].axis = NOT_ALIGNED;
		for(size_t triangleAxis=0;triangleAxis<3;triangleAxis++)
		{
			if (triangles[i].Normal.cross(AXIS_VECTORS[triangleAxis]).length() < 0.1)
			{
				int positive=0;
				if(triangles[i].Normal[triangleAxis] > 0)// positive
					positive=1;
				AXIS axisNr = (AXIS)(triangleAxis*2+positive);
				triangles[i].axis = axisNr;
				if( ! (ABS(Min[triangleAxis]-triangles[i].A[triangleAxis]) < 1.1 
				       || ABS(Max[triangleAxis]-triangles[i].A[triangleAxis]) < 1.1) )	// not close to boundingbox edges?
				{
					triangles[i].axis = NOT_ALIGNED;	// Not close to bounding box
					break;
				}
				area[axisNr] += triangles[i].area();
				break;
			}
		}
	}


	AXIS down = NOT_ALIGNED;
	double LargestArea = 0;
	for(uint i=0;i<6;i++)
	{
		if(area[i] > LargestArea)
		{
			LargestArea = area[i];
			down = (AXIS)i;
		}
	}

	switch(down)
	{
	case NEGX: Rotate(Vector3d(0,-1,0), M_PI/2.0); break;
	case POSX: Rotate(Vector3d(0,1,0), M_PI/2.0); break;
	case NEGY: Rotate(Vector3d(1,0,0), M_PI/2.0); break;
	case POSY: Rotate(Vector3d(-1,0,0), M_PI/2.0); break;
	case POSZ: Rotate(Vector3d(1,0,0), M_PI); break;
	default: // unhandled
	  break;
	}
#endif
	CenterAroundXY();
	PlaceOnPlatform();
}

int Shape::divideAtZ(double z, Shape &upper, Shape &lower, const Matrix4d &T) const
{
  vector<Poly> polys;
  double max_grad;
  bool ok = getPolygonsAtZ(T, z, polys, max_grad);
  if (!ok) return 0;
  vector<Triangle> surf;
  for (guint i=0; i< polys.size(); i++) {
    polys[i].getTriangulation(surf);
  }
  lower.triangles.insert(lower.triangles.end(),surf.begin(),surf.end());
  for (guint i=0; i<surf.size(); i++) surf[i].invertNormal();
  upper.triangles.insert(upper.triangles.end(),surf.begin(),surf.end());
  vector<Triangle> toboth;
  for (guint i=0; i< triangles.size(); i++) {
    Triangle tt = triangles[i].transformed(T*transform3D.transform);
    if (tt.A.z < z && tt.B.z < z && tt.C.z < z )
      lower.triangles.push_back(tt);
    else if (tt.A.z > z && tt.B.z > z && tt.C.z > z )
      upper.triangles.push_back(tt);
    else
      toboth.push_back(tt);
  }
  vector<Triangle> uppersplit,lowersplit;
  for (guint i=0; i< toboth.size(); i++) {
    toboth[i].SplitAtPlane(z, uppersplit, lowersplit);
  }
  upper.triangles.insert(upper.triangles.end(),
			 uppersplit.begin(),uppersplit.end());
  lower.triangles.insert(lower.triangles.end(),
			 lowersplit.begin(),lowersplit.end());
  lower.Rotate(Vector3d(0,1,0),M_PI);
  return 2;
}



void Shape::PlaceOnPlatform()
{
  CalcBBox();
  transform3D.move(Vector3d(0,0,-Min.z));
  CalcBBox();
}

// Rotate and adjust for the user - not a pure rotation by any means
void Shape::Rotate(Vector3d axis, double angle)
{
  CenterAroundXY();
  // Vector3d CenterXY(Center.x, Center.y, 0);
  // do a real rotation because matrix transform gives errors when slicing
  for (size_t i=0; i<triangles.size(); i++)
    {
      triangles[i].rotate(axis, angle);
    }
  PlaceOnPlatform();
}

// this is primitive, it just rotates triangle vertices
void Shape::Twist(double angle)
{
  CenterAroundXY();
  CalcBBox();
  double h = Max.z-Min.z;
  double hangle=0;
  for (size_t i=0; i<triangles.size(); i++) {
    for (size_t j=0; j<3; j++) 
      {
	hangle = angle * (triangles[i][j].z - Min.z) / h;
	triangles[i][j] = triangles[i][j].rotate(hangle,0,0,1);
      }
    triangles[i].calcNormal();
  }
  CalcBBox();
}

void Shape::CenterAroundXY()
{
  CalcBBox();
  Vector3d displacement = transform3D.transform.getTranslation() - Center;
  for(size_t i=0; i<triangles.size() ; i++)
    {
      triangles[i].A = triangles[i].A + displacement;
      triangles[i].B = triangles[i].B + displacement;
      triangles[i].C = triangles[i].C + displacement;
    }
  transform3D.move(-displacement);
  CalcBBox();
}

// // every 2 points denote a line
// vector<Vector2d> Shape::getCutlines(const Matrix4d &T, double z) const
// {
//   Vector2d lineStart;
//   Vector2d lineEnd;
//   int num_cutpoints;
//   vector<Vector2d> lines; 
//   for (size_t i = 0; i < triangles.size(); i++)
//     {
//       num_cutpoints = triangles[i].CutWithPlane(z, T, lineStart, lineEnd);
//       if (num_cutpoints>1)
// 	{
// 	  Vector3d Norm3 = triangles[i].Normal;
// 	  Vector2d triangleNormal = Vector2d(Norm3.x, Norm3.y);
// 	  Vector2d segmentNormal = (lineEnd - lineStart).normal();
// 	  triangleNormal.normalise();
// 	  segmentNormal.normalise();
// 	  if( (triangleNormal-segmentNormal).lengthSquared() > 0.2)  {
// 	    // if normals do not align, flip the segment
// 	    lines.push_back(lineEnd); 
// 	    lines.push_back(lineStart);
// 	  } else {
// 	    lines.push_back(lineStart);
// 	    lines.push_back(lineEnd); 
// 	  }
// 	}
//     }
//   return lines;
// }


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
			   vector<Poly> &polys, double &max_grad) const
{
  vector<Vector2d> vertices;
  vector<Segment> lines = getCutlines(T, z, vertices, max_grad);
  //cerr << vertices.size() << " " << lines.size() << endl;
  if (!CleanupSharedSegments(lines)) return false;
  //cerr << vertices.size() << " " << lines.size() << endl;
  if (!CleanupConnectSegments(vertices,lines,true)) return false;
  //cerr << vertices.size() << " " << lines.size() << endl;
  vector< vector<uint> > connectedlines; // sequence of connected lines indices
  if (!getLineSequences(lines, connectedlines)) return false;
  for (uint i=0; i<connectedlines.size();i++){
    Poly poly(z);
    for (uint j=0; j<connectedlines[i].size();j++){
      poly.addVertex(vertices[lines[connectedlines[i][j]].start]);
    }
    if (lines[connectedlines[i].back()].end !=
	lines[connectedlines[i].front()].start )
      poly.addVertex(vertices[lines[connectedlines[i].back()].end]);
    //cerr << "poly size " << poly.size() << endl;
    polys.push_back(poly);
  }
  //cerr << "found " << polys.size() << " polys" << endl;
  return true;
}

#if 0
// given the overall translation matrix, not our own!
bool Shape::getPolygonsAtZ(const Matrix4d &T, double z, 
			   vector<Poly> &polys, double &max_grad) const
{
  vector<Vector2d> vertices;
  vector<Segment> lines = getCutlines(T, z, vertices, max_grad);
  
  if (z < Min.z || z > Max.z) return true; // no polys, but all ok

  if (vertices.size()==0) return false; 
  //cerr << "vertices ok" <<endl;
  if (!CleanupSharedSegments(lines)) return false; 
  //cerr << "clean shared ok" <<endl;
  if (!CleanupConnectSegments(vertices,lines,true)) return false;
  //cerr << "clean connect ok" <<endl;

	vector< vector<int> > planepoints;
	planepoints.resize(vertices.size());

	// all line numbers starting from every point
	for (uint i = 0; i < lines.size(); i++)
		planepoints[lines[i].start].push_back(i); 

	// Build polygons
	vector<bool> used;
	used.resize(lines.size());
	for (uint i=0;i>used.size();i++)
		used[i] = false;
	
	//polys.clear();

	for (uint current = 0; current < lines.size(); current++)
	{
	  //cerr << used[current]<<"linksegments " << current << " of " << lines.size()<< endl;
		if (used[current])
			continue; // already used
		used[current] = true;

		int startPoint = lines[current].start;
		int endPoint = lines[current].end;

		Poly poly = Poly(z);
		poly.addVertex(vertices[endPoint]);
		//poly.printinfo();
		int count = lines.size()+100;
		while (endPoint != startPoint && count != 0)	// While not closed
		  {
		        // lines starting at endPoint
			const vector<int> &pathsfromhere = planepoints[endPoint];

			// Find the next line.
			if (pathsfromhere.size() == 0) // no where to go ...
			{
				// lets get to the bottom of this data set:
				cout.precision (8);
				cout.width (12);
				cout << "\r\npolygon was cut at z " << z << " LinkSegments at vertex " << endPoint;
				cout << "\n " << vertices.size() << " vertices:\nvtx\tpos x\tpos y\trefs\n";
				for (int i = 0; i < (int)vertices.size(); i++)
				{
					int refs = 0, pol = 0;
					for (int j = 0; j < (int)lines.size(); j++)
					{
						if (lines[j].start == i) { refs++; pol++; }
						if (lines[j].end == i) { refs++; pol--; }
					}
					cout << i << "\t" << vertices[i].x << "\t" << vertices[i].y << "\t" << refs << " pol " << pol;
					if (refs % 2) // oh dear, a dangling vertex
						cout << " odd, dangling vertex\n";
					cout << "\n";
				}
				cout << "\n " << lines.size() << " lines:\nline\tstart vtx\tend vtx\n";
				for (int i = 0; i < (int)lines.size(); i++)
				{
					if (i == endPoint)
						cout << "problem line:\n";
					cout << i << "\t" << lines[i].start << "\t" << lines[i].end << "\n";
				}

				cout << "\n " << vertices.size() << " vertices:\nvtx\tpos x\tpos y\tlinked to\n";
				for (int i = 0; i < (int)planepoints.size(); i++)
				{
					if (i == endPoint)
						cout << "problem vertex:\n";
					cout << i << "\t" << vertices[i].x << "\t" << vertices[i].y << "\t";
					int j;
					switch (planepoints[i].size()) {
					case 0:
						cout << "nothing - error !\n";
						break;
					case 1:
						cout << "neighbour: " << planepoints[i][0];
						break;
					default:
						cout << "Unusual - multiple: \n";
						for (j = 0; j < (int)planepoints[i].size(); j++)
							cout << planepoints[i][j] << " ";
						cout << " ( " << j << " other vertices )";
						break;
					}

					cout << "\n";
				}
				// model failure - we will get called recursivelly
				// for a different z and different cutting plane.
				return false;
			} // endif nowhere to go
			if (pathsfromhere.size() != 1)
				cout << "Risky co-incident node \n";

			// TODO: we need to do better here, some idas:
			//       a) calculate the shortest path back to our start node, and
			//          choose that and/or
			//       b) identify all 2+ nodes and if they share start/end
			//          directions eliminate them to join the polygons.

			uint i;
			// i = first unused path:
			for (i = 0; i < pathsfromhere.size() && used[pathsfromhere[i]]; i++); 
			if (i >= pathsfromhere.size())
			{
				cout << "no-where unused to go";
				return false;
			}
			used[pathsfromhere[i]] = true;

			const Segment &nextsegment = lines[pathsfromhere[i]];
			assert( nextsegment.start == endPoint );
			endPoint = nextsegment.end;

			poly.addVertex(vertices[endPoint]);
			count--;
		}

		// Check if loop is complete
		if (count != 0) {
		  //poly.cleanup(Optimization);
		  // cout << "## POLY add ";
		  // poly.printinfo();
		  polys.push_back(poly);		// This is good
		} else {
		  // We will be called for a slightly different z
		  cout << "\r\nentered loop at shape polyons Z=" << z;
		  return false;
		}
	}

  return true;
}
#endif

int find_vertex(const vector<Vector2d> vertices,  const Vector2d v, double delta = 0.0001)
{
  for (uint i = 0; i<vertices.size(); i++) {
    if ( (v-vertices[i]).lengthSquared() < delta ) 
      return i;
  }
  return -1;
}

vector<Segment> Shape::getCutlines(const Matrix4d &T, double z, 
				   vector<Vector2d> &vertices, double &max_gradient) const
{
  // Vector3d min = T*Min;
  // Vector3d max = T*Max;
  Vector2d lineStart;
  Vector2d lineEnd;
  vector<Segment> lines;
  int num_cutpoints;
  // we know our own tranform:
  Matrix4d transform = T * transform3D.transform ;

  for (uint i = 0; i < triangles.size(); i++)
    {
      Segment line(-1,-1);
      num_cutpoints = triangles[i].CutWithPlane(z, transform, lineStart, lineEnd);
      if (num_cutpoints>0) {
	int havev = find_vertex(vertices, lineStart);
	if (havev >= 0) 
	  line.start = havev;
	else {
	  line.start = vertices.size();
	  vertices.push_back(lineStart);
	}
	if (abs(triangles[i].Normal.z) > max_gradient)
	  max_gradient = abs(triangles[i].Normal.z);
      }
      if (num_cutpoints>1) {
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
	  Vector2d triangleNormal = Vector2d(Norm.x, Norm.y);
	  Vector2d segmentNormal = (lineEnd - lineStart).normal();
	  triangleNormal.normalise();
	  segmentNormal.normalise();
	  if( (triangleNormal-segmentNormal).lengthSquared() > 0.2){
	    // if normals do not align, flip the segment
	    int iswap=line.start;line.start=line.end;line.end=iswap;
	  }
	  // cerr << "line "<<line.start << "-"<<line.end << endl;
	  lines.push_back(line);
	}
    }
  return lines;
}


// intersect lines with plane and link segments
// add result to given CuttingPlane
// void Shape::CalcCuttingPlane(const Matrix4d &T, CuttingPlane *plane) const
// {
//   double z = plane->getZ();
// #if CUTTING_PLANE_DEBUG
// 	cout << "intersect with z " << z << "\n";
// #endif

// 	Vector3d min = T*Min;
// 	Vector3d max = T*Max;

// 	plane->setBBox(min,max);

// 	Vector2d lineStart;
// 	Vector2d lineEnd;

// 	int num_cutpoints;
// 	for (size_t i = 0; i < triangles.size(); i++)
// 	  {
// 	    CuttingPlane::Segment line(-1,-1);
// 	    num_cutpoints = triangles[i].CutWithPlane(z, T, lineStart, lineEnd);
// 	    if (num_cutpoints>0)
// 	      line.start = plane->RegisterPoint(lineStart);
// 	    if (num_cutpoints>1)
// 	      line.end = plane->RegisterPoint(lineEnd);
	      
// 	    // Check segment normal against triangle normal. Flip segment, as needed.
// 	    if (line.start != -1 && line.end != -1 && line.end != line.start)	
// 	      // if we found a intersecting triangle
// 	      {
// 		Vector3d Norm = triangles[i].Normal;
// 		Vector2d triangleNormal = Vector2d(Norm.x, Norm.y);
// 		Vector2d segmentNormal = (lineEnd - lineStart).normal();
// 		triangleNormal.normalise();
// 		segmentNormal.normalise();
// 		if( (triangleNormal-segmentNormal).lengthSquared() > 0.2)
// 		  // if normals does not align, flip the segment
// 		  line.Swap();
// 		// cerr << "line "<<line.start << "-"<<line.end << endl;
// 		// cerr << " -> "<< plane->GetVertices()[line.start] << "-"<<plane->GetVertices()[line.end] << endl;
// 		plane->AddSegment(line);
// 	      }
// 	  }
// 	return ;
// }



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

void drawString(Vector3d pos, string text)
{
  drawString(pos,GLUT_BITMAP_HELVETICA_12,text);
}
void drawString(Vector3d pos, void* font, string text)
{
	checkGlutInit();
	glRasterPos3d(pos.x, pos.y, pos.z);
	for (uint c=0;c<text.length();c++)
		glutBitmapCharacter(font, text[c]);
}


// called from Model::draw
void Shape::draw(const Model *model, const Settings &settings, bool highlight) const 
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

        for (i = 0; i < 4; i++)
            mat_diffuse[i] = settings.Display.PolygonRGBA.rgba[i];

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
                        mat_diffuse[i] = settings.Display.WireframeRGBA.rgba[i];
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);

		glColor4fv(settings.Display.WireframeRGBA.rgba);
		for(size_t i=0;i<triangles.size();i++)
		{
			glBegin(GL_LINE_LOOP);
			glLineWidth(1);
			glNormal3dv((GLdouble*)&triangles[i].Normal);
			glVertex3dv((GLdouble*)&triangles[i].A);
			glVertex3dv((GLdouble*)&triangles[i].B);
			glVertex3dv((GLdouble*)&triangles[i].C);
			glEnd();
		}
	}

	glDisable(GL_LIGHTING);

	// normals
	if(settings.Display.DisplayNormals)
	{
		glColor4fv(settings.Display.NormalsRGBA.rgba);
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
		glColor4fv(settings.Display.EndpointsRGBA.rgba);
		glPointSize(settings.Display.EndPointSize);
		glBegin(GL_POINTS);
		for(size_t i=0;i<triangles.size();i++)
		{
			glVertex3f(triangles[i].A.x, triangles[i].A.y, triangles[i].A.z);
			glVertex3f(triangles[i].B.x, triangles[i].B.y, triangles[i].B.z);
			glVertex3f(triangles[i].C.x, triangles[i].C.y, triangles[i].C.z);
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
		glVertex3f(Min.x, Min.y, Min.z);
		glVertex3f(Min.x, Max.y, Min.z);
		glVertex3f(Max.x, Max.y, Min.z);
		glVertex3f(Max.x, Min.y, Min.z);
		glEnd();
		glBegin(GL_LINE_LOOP);
		glVertex3f(Min.x, Min.y, Max.z);
		glVertex3f(Min.x, Max.y, Max.z);
		glVertex3f(Max.x, Max.y, Max.z);
		glVertex3f(Max.x, Min.y, Max.z);
		glEnd();
		glBegin(GL_LINES);
		glVertex3f(Min.x, Min.y, Min.z);
		glVertex3f(Min.x, Min.y, Max.z);
		glVertex3f(Min.x, Max.y, Min.z);
		glVertex3f(Min.x, Max.y, Max.z);
		glVertex3f(Max.x, Max.y, Min.z);
		glVertex3f(Max.x, Max.y, Max.z);
		glVertex3f(Max.x, Min.y, Min.z);
		glVertex3f(Max.x, Min.y, Max.z);
		glEnd();

  glColor3f(1,0.6,0.6);
  ostringstream val;
  val.precision(1);
  Vector3d pos;
  val << fixed << (Max.x-Min.x);
  pos = Vector3d((Max.x+Min.x)/2.,Min.y,Max.z);
  drawString(pos,val.str());
  val.str("");
  val << fixed << (Max.y-Min.y);
  pos = Vector3d(Min.x,(Max.y+Min.y)/2.,Max.z);
  drawString(pos,val.str());
  val.str("");
  val << fixed << (Max.z-Min.z);
  pos = Vector3d(Min.x,Min.y,(Max.z+Min.z)/2.);
  drawString(pos,val.str());
}

// void Shape::displayInfillOld(const Settings &settings, CuttingPlane &plane,
// 			      uint LayerNr, vector<int>& altInfillLayers)
// {
//   double fullInfillDistance = settings.Hardware.ExtrudedMaterialWidth
//     * settings.Hardware.ExtrusionFactor;  

//   //cerr << "Shape::displayInfillOld" <<  endl;
//   if (false)//settings.Display.DisplayinFill)
//     {
//       vector<Vector2d> *infill = NULL;
      
//       CuttingPlane infillCuttingPlane = plane;
//       if (settings.Slicing.ShellOnly == false)
// 	{
// 	  switch (settings.Slicing.ShrinkQuality)
// 	    {
// 	    case SHRINK_FAST:
// 	      //infillCuttingPlane.ClearShrink();
// 	      infillCuttingPlane.MakeShells(settings.Slicing.ShellCount,
// 					    settings.Hardware.ExtrudedMaterialWidth,
// 					    settings.Slicing.Optimization,
// 					    //settings.Display.DisplayCuttingPlane,
// 					    false);
// 	      break;
// 	    case SHRINK_LOGICK:
// 	      break;
// 	    }
	  
// 	  // check if this if a layer we should use the alternate infill distance on
// 	  double infillDistance = settings.Slicing.InfillDistance;
// 	  if (std::find(altInfillLayers.begin(), altInfillLayers.end(), LayerNr) != altInfillLayers.end()) {
// 	    infillDistance = settings.Slicing.AltInfillDistance;
// 	  }
	  
// 	  infillCuttingPlane.CalcInfill(infillDistance,
// 					fullInfillDistance,
// 					settings.Slicing.InfillRotation,
// 					settings.Slicing.InfillRotationPrLayer,
// 					settings.Display.DisplayDebuginFill);
// 	  //infill = infillCuttingPlane.getInfillVertices();
// 	}
//       glColor4f(1,1,0,1);
//       glPointSize(5);
//       glBegin(GL_LINES);
//       for (size_t i = 0; i < infill->size(); i += 2)
// 	{
// 	  if (infill->size() > i+1)
// 	    {
// 	      glVertex3d ((*infill)[i  ].x, (*infill)[i  ].y, plane.getZ());
// 	      glVertex3d ((*infill)[i+1].x, (*infill)[i+1].y, plane.getZ());
// 	    }
// 	}
//       glEnd();
//       delete infill;
//     }
// }

void Shape::draw_geometry() const
{
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
		glNormal3dv((GLdouble*)&triangles[i].Normal);
		glVertex3dv((GLdouble*)&triangles[i].A);
		glVertex3dv((GLdouble*)&triangles[i].B);
		glVertex3dv((GLdouble*)&triangles[i].C);
	}
	glEnd();
}

// we dont want this
// bool CleanupStraightLines(const vector<Vector2d> vertices, vector<Segment> &lines)
// {
//   vector<int> lines_to_delete;
//   for (uint j = 0; j < lines.size(); j++) {
//     for (uint k = j + 1; k < lines.size(); k++) {
//       if ( lines[j].end == lines[k].start ) {
// 	Vector2d dirj = vertices[lines[j].end]-vertices[lines[j].start];
// 	Vector2d dirk = vertices[lines[k].end]-vertices[lines[k].start];
// 	dirj.normalize();
// 	dirk.normalize();
// 	if ((dirk-dirj).lengthSquared() < 0.1) {
// 	  cerr << "equal direction" << endl;
// 	  lines[j].end = lines[k].end;
// 	  lines_to_delete.push_back(k);
// 	}
//       }    
//     }
//   }
//   std::sort(lines_to_delete.begin(), lines_to_delete.end());
//   for (int r = lines_to_delete.size() - 1; r >= 0; r--)
//     lines.erase(lines.begin() + lines_to_delete[r]);
//   return true;
// }




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
#if 1
  vector<int> lines_to_delete;
  for (uint j = 0; j < lines.size(); j++) {
    const Segment &jr = lines[j];
    for (uint k = j + 1; k < lines.size(); k++)
      {
	const Segment &kr = lines[k];
	if ((jr.start == kr.start && jr.end == kr.end) ||
	    (jr.end == kr.start && jr.start == kr.end))
	  {
	    lines_to_delete.push_back (j);
	    lines_to_delete.push_back (k); // remove both???
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
bool CleanupConnectSegments(const vector<Vector2d> vertices, vector<Segment> &lines, bool connect_all)
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
			cout << "detached point " << i << "\t" << vertex_types[i] << " refs at " << vertices[i].x << "\t" << vertices[i].y << "\n";
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
			double dist_sq = (p-q).lengthSquared(); //pow (p.x - q.x, 2) + pow (p.y - q.y, 2);
			if (dist_sq < nearest_dist_sq)
			{
				nearest_dist_sq = dist_sq;
				nearest = j;
			}
		}
		assert (nearest != 0);

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
