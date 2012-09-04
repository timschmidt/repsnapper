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
#include "files.h"
#include "progress.h"
#include "settings.h"
#include "clipping.h"

#ifdef _OPENMP
#include <omp.h>
#endif

// Constructor
Shape::Shape()
  : slow_drawing(false), gl_List(-1)
{
  Min.set(0,0,0);
  Max.set(200,200,200);
  CalcBBox();
}

void Shape::clear() {
  triangles.clear();
  gl_List = -1;
};

void Shape::setTriangles(const vector<Triangle> &triangles_)
{
  triangles = triangles_;

  CenterAroundXY();
  double vol = volume();
  if (vol < 0) {
    invertNormals();
    vol = -vol;
  }

  PlaceOnPlatform();
  cerr << _("Shape has volume ") << volume() << _(" mm^3 and ")
       << triangles.size() << _(" triangles") << endl;
}


int Shape::saveBinarySTL(Glib::ustring filename) const
{

  if (!File::saveBinarySTL(filename, triangles, transform3D.transform))
    return -1;
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
      shapes.back()->CenterAroundXY();
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

// doesn't work
void Shape::repairNormals(double sqdistance)
{
  for (uint i = 0; i < triangles.size(); i++) {
    vector<uint> adjacent;
    uint numadj=0, numwrong=0;
    for (uint j = i+1; j < triangles.size(); j++) {
      if (i!=j) {
	if (triangles[i].isConnectedTo(triangles[j], sqdistance)) {
	  numadj++;
	  if (triangles[i].wrongOrientationWith(triangles[j], sqdistance)) {
	    numwrong++;
	    triangles[j].invertNormal();
	  }
	}
      }
    }
    //cerr << i<< ": " << numadj << " - " << numwrong  << endl;
    //if (numwrong > numadj/2) triangles[i].invertNormal();
  }
}

void Shape::mirror()
{
  const Vector3d mCenter = transform3D.getInverse() * Center;
  for (uint i = 0; i < triangles.size(); i++)
    triangles[i].mirrorX(mCenter);
  CalcBBox();
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

void Shape::addTriangles(const vector<Triangle> &tr)
{
  triangles.insert(triangles.end(), tr.begin(), tr.end());
  CalcBBox();
}

vector<Triangle> Shape::getTriangles(const Matrix4d &T) const
{
  vector<Triangle> tr(triangles.size());
  for (uint i = 0; i < triangles.size(); i++) {
    tr[i] = triangles[i].transformed(T*transform3D.transform);
  }
  return tr;
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


void Shape::FitToVolume(const Vector3d &vol)
{
  Vector3d diag = Max-Min;
  const double sc_x = diag.x() / vol.x();
  const double sc_y = diag.y() / vol.y();
  const double sc_z = diag.z() / vol.z();
  double max_sc = max(max(sc_x, sc_y),sc_z);
  if (max_sc > 1.)
    Scale(1./max_sc, true);
}

void Shape::Scale(double in_scale_factor, bool calcbbox)
{
  transform3D.move(-Center);
  transform3D.scale(in_scale_factor);
  transform3D.move(Center);
  if (calcbbox)
    CalcBBox();
}

void Shape::ScaleX(double x)
{
  transform3D.move(-Center);
  transform3D.scale_x(x);
  transform3D.move(Center);
}
void Shape::ScaleY(double x)
{
  transform3D.move(-Center);
  transform3D.scale_y(x);
  transform3D.move(Center);
}
void Shape::ScaleZ(double x)
{
  transform3D.move(-Center);
  transform3D.scale_z(x);
  transform3D.move(Center);
}

void Shape::CalcBBox()
{
  Min.set(INFTY,INFTY,INFTY);
  Max.set(-INFTY,-INFTY,-INFTY);
  for(size_t i = 0; i < triangles.size(); i++) {
    triangles[i].AccumulateMinMax (Min, Max, transform3D.transform);
  }
  Center = (Max + Min) / 2;
  gl_List = -1;
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
	if ( (normals[n].normal -
	      triangles[i].transformed(transform3D.transform).Normal)
	     .squared_length() < 0.000001) {
	  havenormal = true;
	  normals[n].area += triangles[i].area();
	  done[i] = true;
	}
      }
      if (!havenormal){
	SNorm n;
	n.normal = triangles[i].transformed(transform3D.transform).Normal;
	n.area = triangles[i].area();
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
  // CenterAroundXY();
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
  upper->CenterAroundXY();
  lower->CenterAroundXY();
  lower->Rotate(Vector3d(0,1,0),M_PI);
  upper->move(Vector3d(10+Max.x()-Min.x(),0,0));
  lower->move(Vector3d(2*(10+Max.x()-Min.x()),0,0));
  upper->PlaceOnPlatform();
  lower->PlaceOnPlatform();
  return 2;
}

void Shape::PlaceOnPlatform()
{
  transform3D.move(Vector3d(0,0,-Min.z()));
}

// Rotate and adjust for the user - not a pure rotation by any means
void Shape::Rotate(const Vector3d & axis, const double & angle)
{
  transform3D.rotate(axis,angle);
  return;
//   CenterAroundXY();
//   // do a real rotation because matrix transform gives errors when slicing
//   int count = (int)triangles.size();
// #ifdef _OPENMP
// #pragma omp parallel for schedule(dynamic)
// #endif
//   for (int i=0; i < count ; i++)
//     {
//       triangles[i].rotate(axis, angle);
//     }
//   PlaceOnPlatform();
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
  //cerr << "DISPL " << displacement << endl;
  //CalcBBox();
  // Min    -= displacement;
  // Max    -= displacement;
  // Center -= displacement;
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
	Vector2d i0, i1;
	const int is = intersect2D_Segments(p[1], lineStart, lineEnd, p[0],
					    i0, i1);
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


int find_vertex(const vector<Vector2d> &vertices,
		const Vector2d &v, double delta = 0.0001)
{
  int found = -1;
  int count = (int)vertices.size();
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic)
#endif
  for (int i=0; i<count; i++) {
    if (found != -1) continue;
    if ( (v-vertices[i]).squared_length() < delta ) {
      found = i;
#ifndef _OPENMP
      break;
#endif
    }
  }
  return found;
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
	  Vector3d Norm = triangles[i].transformed(transform).Normal;
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
void Shape::draw(const Settings &settings, bool highlight, uint max_triangles)
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
	  mat_diffuse[3] += 0.3*(1.-mat_diffuse[3]);

	// invert colours if partial draw (preview mode)
	if (max_triangles > 0) {
	  for (uint i = 0; i < 3; i++)
	    mat_diffuse[i] = 1.-mat_diffuse[i];
	  mat_diffuse[3] = 0.9;
	}


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
                draw_geometry(max_triangles);
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


void Shape::draw_geometry(uint max_triangles)
{

  bool listDraw = (max_triangles == 0); // not in preview mode
  bool haveList = gl_List >= 0;

  if (listDraw && !haveList) {
    gl_List = glGenLists(1);
    glNewList(gl_List, GL_COMPILE);
  }
  if (!listDraw || !haveList) {
	uint step = 1;
	if (max_triangles>0) step = floor(triangles.size()/max_triangles);
	step = max((uint)1,step);

	glBegin(GL_TRIANGLES);
	for(size_t i=0;i<triangles.size();i+=step)
	{
		glNormal3dv((GLdouble*)&(triangles[i].Normal));
		glVertex3dv((GLdouble*)&(triangles[i].A));
		glVertex3dv((GLdouble*)&(triangles[i].B));
		glVertex3dv((GLdouble*)&(triangles[i].C));
	}
	glEnd();
  }
  if (listDraw && !haveList) {
    glEndList();
  }


  if (listDraw && gl_List >= 0) { // have stored list
    Glib::TimeVal starttime, endtime;
    if (!slow_drawing) {
      starttime.assign_current_time();
    }
    glCallList(gl_List);
    if (!slow_drawing) {
      endtime.assign_current_time();
      Glib::TimeVal usedtime = endtime-starttime;
      if (usedtime.as_double() > 0.2) slow_drawing = true;
    }
    return;
  }



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
  int count = (int)lines.size();
#ifdef _OPENMP
  //#pragma omp parallel for schedule(dynamic)
#endif
  for (int j = 0; j < count; j++) {
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
	int count = lines.size();
	for (int i = 0; i < count; i++)
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
	count = vertex_types.size();
// #ifdef _OPENMP
// #pragma omp parallel for schedule(dynamic)
// #endif
	for (int i = 0; i < count; i++)
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
	count = detached_points.size();
// #ifdef _OPENMP
// #pragma omp parallel for schedule(dynamic)
// #endif
	for (int i = 0; i < count; i++)
	{
		double nearest_dist_sq = (std::numeric_limits<double>::max)();
		uint   nearest = 0;
		int   n = detached_points[i]; // vertex index of detached point i
		if (n < 0) // handled already
		  continue;

		const Vector2d &p = vertices[n]; // the real detached point
		// find nearest other detached point to the detached point n:
		for (int j = i + 1; j < count; j++)
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




