/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010 Kulitorum
    Copyright (C) 2011-12 martin.dieringer@gmx.de

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

#include "poly.h"

#include "layer.h"
#include "shape.h"
#include "clipping.h"
#include "render.h"


#include <poly2tri/poly2tri/poly2tri/poly2tri.h>


Poly::Poly()
{
  closed = true;
  holecalculated = false;
  this->z = -10;
  extrusionfactor = 1.;
}

Poly::Poly(double z, double extrusionfactor)
{
  closed = true;
  this->z = z;
  this->extrusionfactor = extrusionfactor;
  holecalculated = false;
  hole=false;
  //cout << "POLY WITH PLANE"<< endl;
  //plane->printinfo();
  //printinfo();
}

Poly::Poly(const Poly &p, double z_)
{
  closed = p.closed;
  this->z = z_;
  this->extrusionfactor = p.extrusionfactor;
  //uint count = p.vertices.size();
  // vertices.resize(count);
  this->vertices = p.vertices;
  holecalculated = p.holecalculated;
  if (holecalculated) {
    hole = p.hole;
    center = p.center;
  } else
    calcHole();
}

Poly::~Poly()
{
}

void Poly::cleanup(double epsilon)
{
  vertices = simplified(vertices, epsilon);
  if (!closed) return;
  uint n_vert = vertices.size();
  vector<Vector2d> invert;
  invert.insert(invert.end(),vertices.begin()+n_vert/2,vertices.end());
  invert.insert(invert.end(),vertices.begin(),vertices.begin()+n_vert/2);
  vertices = simplified(invert, epsilon);
  //calcHole();
}


/*
 * Iterate the vertices, and calculate whether this
 * shape is a hole or enclosed, based on whether the
 * segment normals point outward (a hole) or inward
 * (enclosing)
 */
void Poly::calcHole() const // hole is mutable
{
  	if(vertices.size() < 3)
	  return;	// hole is undefined
	Vector2d p(-INFTY, -INFTY);
	int v=0;
	center = Vector2d(0,0);
	Vector2d q;
	for(size_t vert=0;vert<vertices.size();vert++)
	{
	  q = vertices[vert];
	  center += q;
	  if(q.x() > p.x())
	    {
	      p = q;
	      v=vert;
	    }
	  else if(q.x() == p.x() && q.y() > p.y())
	    {
	      p.y() = q.y();
	      v=vert;
	    }
	}
	center /= vertices.size();

	// we have the x-most vertex (with the highest y if there was a contest), v
	Vector2d V1 = getVertexCircular(v-1);
	Vector2d V2 = getVertexCircular(v);
	Vector2d V3 = getVertexCircular(v+1);

	// Vector2d Va=V2-V1;
	// Vector2d Vb=V3-V2;
	hole = isleftof(V2, V3, V1); //cross(Vb,Va) > 0;
	holecalculated = true;
}

bool Poly::isHole() const
{
  if (!holecalculated)
    calcHole();
  return hole;
}

Vector2d Poly::getCenter() const
{
  if (!holecalculated) calcHole();
  return center;
}

void Poly::rotate(const Vector2d &rotcenter, double angle)
{
  for (uint i = 0; i < vertices.size();  i++) {
    ::rotate(vertices[i], rotcenter, angle);
  }
}

void Poly::move(const Vector2d &delta)
{
  for (uint i = 0; i < vertices.size();  i++) {
    vertices[i] += delta;
  }
  center+=delta;
}

void Poly::transform(const Matrix4d &T) {
  for (uint i = 0; i < vertices.size();  i++) {
    const Vector3d v = T * getVertexCircular3(i) ;
    vertices[i].set(v.x(),v.y());
  }
  setZ((T * Vector3d(0,0,z)).z());
  calcHole();
}

void Poly::transform(const Matrix3d &T) {
  for (uint i = 0; i < vertices.size();  i++) {
    vertices[i] = T * vertices[i];
  }
  calcHole();
}

void Poly::mirrorX(const Vector3d &center)
{
  uint count = size();
  for (uint i = 0; i < count; i++)
    vertices[i].x() = center.x() - vertices[i].x();
  reverse();
  calcHole();
}


uint Poly::getFarthestIndex(uint &thisindex) const
{
  return getFarthestIndex(vertices[thisindex]);
}

uint Poly::getFarthestIndex(const Vector2d &from) const
{
  uint findex = 0;
  double maxdist = 0.;
  for (uint i = 0; i < size(); i++) {
    double d = from.squared_distance(vertices[i]);
    if (d > maxdist) {
      maxdist = d;
      findex = i;
    }
  }
  return findex;
}


// nearest connection point indices of this and other poly
// if poly is not closed, only test first and last point
void Poly::nearestIndices(const Poly &p2, int &thisindex, int &otherindex) const
{
  double mindist = INFTY;
  for (uint i = 0; i < size(); i++) {
    if (!closed && i != 0 && i != size()-1) continue;
    for (uint j = 0; j < p2.size(); j++) {
      if (!p2.closed && j != 0 && j != p2.size()-1) continue;
      double d = vertices[i].squared_distance(p2.vertices[j]);
      if (d < mindist) {
	mindist = d;
	thisindex = i;
	otherindex= j;
      }
    }
  }
}

// Find the vertex in the poly closest to point p
// if not closed, only look for first and last point
uint Poly::nearestDistanceSqTo(const Vector2d &p, double &mindist) const
{
  assert(vertices.size() > 0);
  // Start with first vertex as closest
  uint nindex = 0;
  mindist = (vertices[0]-p).squared_length();
  if (std::isnan(mindist)) { // for infinity point p return point 0 and distance 0
    mindist = 0.;
    return 0;
  }
  // check the rest of the vertices for a closer one.
  for (uint i = 1; i < vertices.size(); i++) {
    if (!closed && i != 0 && i != vertices.size()-1) continue;
    double d = (vertices[i]-p).squared_length();
    if (d<mindist) {
      mindist= d;
      nindex = i;
    }
  }
  return nindex;
}
// returns length and two points
double Poly::shortestConnectionSq(const Poly &p2, Vector2d &start, Vector2d &end) const
{
  double min1 = 100000000, min2 = 100000000;
  int minindex1=0, minindex2=0;
  Vector2d onpoint1, onpoint2;
  // test this vertices
  for (uint i = 0; i < vertices.size(); i++) {
    for (uint j = 0; j < p2.vertices.size(); j++) {
      Vector2d onpoint; // on p2
      // dist from point i to lines on p2
      const double mindist =
	point_segment_distance_Sq(p2.vertices[j], p2.getVertexCircular(j+1),
				  vertices[i], onpoint);
      if (mindist < min1) {
	min1 = mindist; onpoint1 = onpoint; minindex1 = i;
      }
    }
  }
  // test p2 vertices
  for (uint i = 0; i < p2.vertices.size(); i++) {
    for (uint j = 0; j < vertices.size(); j++) {
      Vector2d onpoint; // on this
      // dist from p2 point i to lines on this
      const double mindist =
	point_segment_distance_Sq(vertices[j], getVertexCircular(j+1),
				  p2.vertices[i], onpoint);
      if (mindist < min2) {
	min2 = mindist; onpoint2 = onpoint; minindex2 = i;
      }
    }
  }
  if (min1 < min2) { // this vertex, some point on p2 lines
    start = getVertexCircular(minindex1);
    end = onpoint1;
  } else { // p2 vertex, some point of this lines
    start = p2.getVertexCircular(minindex2);
    end = onpoint2;
  }
  return (end-start).squared_length();
}


double Poly::angleAtVertex(uint i) const
{
  return planeAngleBetween(getVertexCircular(i)-getVertexCircular(i-1),
		      getVertexCircular(i+1)-getVertexCircular(i));
}

bool Poly::vertexInside2(const Vector2d &point, double maxoffset) const
{
  // Shoot a ray along +X and count the number of intersections.
  // If n_intersections is even, return false, else return true
  Vector2d EndP(point.x()+10000, point.y());
  int intersectcount = 1; // we want to test if uneven
  double maxoffsetSq = maxoffset*maxoffset;
  Vector2d dummy;
  for(size_t i=0; i<vertices.size();i++)
    {
      const Vector2d P1 = getVertexCircular(i-1);
      const Vector2d P2 = vertices[i];

      if (point_segment_distance_Sq(point, P1, P2, dummy) <= maxoffsetSq) return true;

      // Skip horizontal lines, we can't intersect with them,
      // because the test line is horizontal
      if(P1.y() == P2.y())
	continue;

      Intersection hit;
      if(IntersectXY(point,EndP,P1,P2,hit,maxoffset))
	intersectcount++;
    }
  return (intersectcount%2==0);
}

// http://paulbourke.net/geometry/insidepoly/
bool Poly::vertexInside(const Vector2d &p, double maxoffset) const
{
#define POLYINSIDEVERSION 0
#if POLYINSIDEVERSION==0
  // this one works
  uint N = size();
  if (N < 2) return false;
  uint counter = 0;
  uint i;
  double xinters;
  const Vector2d *p1, *p2;
  p1 = &(vertices[0]);
  for (i=1;i<=N;i++) {
    p2 = &(vertices[i % N]);
    if (p.y() > min(p1->y(), p2->y())) {
      if (p.y() <= max(p1->y(), p2->y())) {
        if (p.x() <= max(p1->x(), p2->x())) {
          if (p1->y() != p2->y()) {
            xinters = (p.y()-p1->y())*(p2->x()-p1->x())/(p2->y()-p1->y())+p1->x();
            if (p1->x() == p2->x() || p.x() <= xinters)
              counter++;
          }
        }
      }
    }
    p1 = p2;
  }
  return (counter % 2 != 0);
#else
  // not really working?
  bool c = false;
  //Poly off = Clipping::getOffset(*this,maxoffset).front();
  for (uint i = 0; i < vertices.size();  i++) {
    const Vector2d Pi = vertices[i];
    const Vector2d Pj = getVertexCircular(i+1);
    if ( ((Pi.y() > p.y()) != (Pj.y() > p.y())) &&
	 (abs(p.x() - (Pj.x()-Pi.x())
	      * (p.y()-Pi.y()) / (Pj.y()-Pi.y()) + Pi.x()) > maxoffset) )
      c = !c;
  }
  if (!c)
    for (uint i = 0; i < vertices.size();  i++)
      if ((vertices[i]-p).length() < maxoffset) return true; // on a vertex
  return c;
#endif
}

// this polys completely contained in other
bool Poly::isInside(const Poly &poly, double maxoffset) const
{
  uint i, count=0;
  for (i = 0; i < vertices.size();  i++) {
    if (poly.vertexInside(vertices[i],maxoffset))
      count++;
  }
  return count == vertices.size();
}


void Poly::addVertex(const Vector2d &v, bool front)
{
  if (front)
    vertices.insert(vertices.begin(),v);
  else
    vertices.push_back(v);
  holecalculated=false;
}
void Poly::addVertexUnique(const Vector2d &v, bool front)
{
  for (uint i = 0; i < vertices.size();  i++) {
    if (vertices[i] == v) return;
  }
  addVertex(v,front);
}

void Poly::addVertex(double x, double y, bool front)
{
  addVertex(Vector2d(x,y), front);
}

void Poly::addVertexUnique(double x, double y, bool front)
{
  addVertexUnique(Vector2d(x,y), front);
}


Vector2d const &Poly::getVertexCircular(int index) const
{
  uint size = vertices.size();
  index = (index + size) % size;
  //cerr << vertices->size() <<" >  "<< points[pointindex] << endl;
  return vertices[index];
}

Vector3d Poly::getVertexCircular3(int pointindex) const
{
  Vector2d v = getVertexCircular(pointindex);
  return Vector3d(v.x(),v.y(),z);
}

vector<Vector2d> Poly::getVertexRangeCircular(int from, int to) const
{
  vector<Vector2d> v;
  int size = vertices.size();
  for (int i = from; i<=to; i++)
    v.push_back(vertices[(i+size)%size]);
  return v;
}


vector<Intersection> Poly::lineIntersections(const Vector2d &P1, const Vector2d &P2,
					     double maxerr) const
{
  vector<Intersection> HitsBuffer;
  Vector2d P3,P4;
  for(size_t i = 0; i < vertices.size(); i++)
    {
      P3 = getVertexCircular(i);
      P4 = getVertexCircular(i+1);
      Intersection hit;
      if (IntersectXY(P1,P2,P3,P4,hit,maxerr)) {
	HitsBuffer.push_back(hit);
      }
    }
  // std::sort(HitsBuffer.begin(),HitsBuffer.end());
  // vector<Vector2d> v(HitsBuffer.size());
  // for(size_t i = 0; i < v.size(); i++)
  //   v[i] = HitsBuffer[i].p;
  return HitsBuffer;
}

// double Poly::getLayerNo() const { return plane->LayerNo;}


// length of the line starting at startindex
double Poly::getLinelengthSq(uint startindex) const
{
  const double length = (getVertexCircular(startindex+1) -
			 getVertexCircular(startindex)).squared_length();
  return length;
}

double Poly::averageLinelengthSq() const
{
  double l=0;
  for (uint i = 0; i<vertices.size(); i++){
    l+=getLinelengthSq(i);
  }
  return l/vertices.size();
}

double Poly::totalLineLength() const
{
  if (size() < 2) return 0;
  double l=0;
  for (uint i = 0; i<vertices.size()-1; i++){
    l+=vertices[i].distance(vertices[i+1]);
  }
  if (closed)
    l+=vertices[0].distance(vertices[vertices.size()-1]);
  return l;
}


// add to lines starting with nearest point to startPoint
void Poly::makeLines(vector<Vector3d> &lines, Vector2d &startPoint) const
{
  if (size()<2) return;
  double mindist = INFTY;
  uint index = nearestDistanceSqTo(startPoint, mindist);
  makeLines(lines,index);
  startPoint = Vector2d(lines.back().x(),lines.back().y());
}
void Poly::makeLines(vector<Vector2d> &lines, Vector2d &startPoint) const
{
  if (size()<2) return;
  double mindist = INFTY;
  uint index = nearestDistanceSqTo(startPoint, mindist);
  makeLines(lines,index);
  startPoint = Vector2d(lines.back());
}

// add to lines starting with given index
// closed lines sequence if number of vertices > 2 and poly is closed
void Poly::makeLines(vector<Vector2d> &lines, uint startindex) const
{
  size_t count = vertices.size();
  if (count<2) return; // one point no line
  bool closedlines = closed;
  if (count<3) closedlines = false; // two points one line
  vector<Vector2d> mylines;
  for(size_t i = startindex; i < count+startindex; i++)
    {
      if (!closedlines && i == count-1) continue;
      mylines.push_back(getVertexCircular(i));
      mylines.push_back(getVertexCircular(i+1));
    }
  if (!closedlines && startindex == count-1)
    lines.insert(lines.end(),mylines.rbegin(),mylines.rend());
  else
    lines.insert(lines.end(),mylines.begin(),mylines.end());
}

void Poly::makeLines(vector<Vector3d> &lines, uint startindex) const
{
  vector<Vector3d> mylines;
  size_t count = vertices.size();
  if (count<2) return; // one point no line
  bool closedlines = closed;
  if (count<3) closedlines = false; // two points one line
  for(size_t i = startindex; i < count+startindex; i++)
    {
      if (!closedlines && i == count-1) continue;
      lines.push_back(getVertexCircular3(i));
      lines.push_back(getVertexCircular3(i+1));
    }
  if (!closedlines && startindex == count-1)
    lines.insert(lines.end(),mylines.rbegin(),mylines.rend());
  else
    lines.insert(lines.end(),mylines.begin(),mylines.end());
}

#if 0
vector<Vector2d> Poly::getCenterline() const
{
  vector<Vector2d> line;
  for (uint i=0; i < vertices.size(); i++){
    Vector2d abp = angle_bipartition(vertices[i],
				     getVertexCircular(i-1), getVertexCircular(i+1));

    // int intersect2D_Segments( const Vector2d &p1, const Vector2d &p2,
    // 			      const Vector2d &p3, const Vector2d &p4,
    // 			      Vector2d &I0, Vector2d &I1,
    // 			      double &t0, double &t1,
    // 			      double maxerr)
  }
  return line;
}
#endif

vector<Vector2d> Poly::getPathAround(const Vector2d &from, const Vector2d &to) const
{
  double dist;
  vector<Vector2d> path1, path2;
  // Poly off = Clipping::getOffset(*this, 0, jround).front();
  //cerr << size()<<  " Off " << off.size()<< endl;
  int nvert = size();
  if (nvert==0) return path1;
  int fromind = (int)nearestDistanceSqTo(from, dist);
  int toind = (int)nearestDistanceSqTo(to, dist);
  if (fromind==toind) {
    path1.push_back(vertices[fromind]);
    return path1;
  }
  //calc both direction paths
  if(fromind < toind) {
    for (int i=fromind; i<=toind; i++)
      path1.push_back(getVertexCircular(i));
    for (int i=fromind+nvert; i>=toind; i--)
      path2.push_back(getVertexCircular(i));
  } else {
    for (int i=fromind; i>=toind; i--)
      path1.push_back(getVertexCircular(i));
    for (int i=fromind; i<=toind+nvert; i++)
      path2.push_back(getVertexCircular(i));
  }
  // find shorter one
  double len1=0,len2=0;
  for (uint i=1; i<path1.size(); i++)
    len1+=(path1[i]-path1[i-1]).squared_length();
  for (uint i=1; i<path2.size(); i++)
    len2+=(path2[i]-path2[i-1]).squared_length();
  if (len1 < len2) {
     // path1.insert(path1.begin(),from);
     // path1.push_back(to);
    return path1;
  }
  else{
     // path2.insert(path2.begin(),from);
     // path2.push_back(to);
    return path2;
  }
}


vector<Vector2d> Poly::getMinMax() const{
  double minx=6000,miny=6000;
  double maxx=-6000,maxy=-6000;
  vector<Vector2d> range;
  range.resize(2);
  Vector2d v;
  for (uint i=0; i < vertices.size();i++){
    v = vertices[i];
    if (v.x()<minx) minx=v.x();
    if (v.x()>maxx) maxx=v.x();
    if (v.y()<miny) miny=v.y();
    if (v.y()>maxy) maxy=v.y();
  }
  range[0] = Vector2d(minx,miny);
  range[1] = Vector2d(maxx,maxy);
  return range;
}

int Poly::getTriangulation(vector<Triangle> &triangles)  const
{
  if(vertices.size()<3) return 0;
  triangles.clear();

  // return delaunayTriang(vertices, triangles, z);

  vector<p2t::Point*> points(vertices.size());
  // add offset because poly2tri crashes on some negative values?
  const double OFF = 0;
  for (guint i=0; i<vertices.size(); i++)  {
    points[i] = new p2t::Point(vertices[i].x()+OFF,
			       vertices[i].y()+OFF);
  }
  p2t::CDT cdt(points);
  cdt.Triangulate();
  vector<p2t::Triangle*> ptriangles = cdt.GetTriangles();
  for (guint i=0; i<ptriangles.size(); i++) {
    const p2t::Point *tp0 = ptriangles[i]->GetPoint(0);
    const p2t::Point *tp1 = ptriangles[i]->GetPoint(1);
    const p2t::Point *tp2 = ptriangles[i]->GetPoint(2);
    Vector3d A((tp0->x-OFF), (tp0->y-OFF), z);
    Vector3d B((tp1->x-OFF), (tp1->y-OFF), z);
    Vector3d C((tp2->x-OFF), (tp2->y-OFF), z);
    triangles.push_back(Triangle(A, B, C));
  }
  return triangles.size();
}



Vector3d rotatedZ(Vector3d v, double angle)
{
  double sina = sin(angle);
  double cosa = cos(angle);
  return Vector3d(v.x()*cosa-v.y()*sina,
		  v.y()*cosa+v.x()*sina, v.z());
}

void Poly::draw_as_surface() const
{
  vector<Triangle> triangles;
  getTriangulation(triangles);
  glBegin(GL_TRIANGLES);
  for(size_t i=0;i<triangles.size();i++)
    {
      glNormal3dv((GLdouble*)&(triangles[i].Normal));
      glVertex3dv((GLdouble*)&(triangles[i].A));
      glVertex3dv((GLdouble*)&(triangles[i].B));
      glVertex3dv((GLdouble*)&(triangles[i].C));
    }
  glEnd();
}

void Poly::draw(int gl_type, double z, bool randomized) const
{
  Vector2d v;
  int count = vertices.size();
  if (!closed && gl_type == GL_LINE_LOOP) {
    gl_type = GL_LINES;
    count--;
  }
  glBegin(gl_type);
  for (int i=0; i < count; i++){
    v = getVertexCircular(i);
    if (randomized) v = random_displaced(v);
    glVertex3f(v.x(),v.y(),z);
    if ( gl_type == GL_LINES ) {
      Vector2d vn = getVertexCircular(i+1);
      if (randomized) vn = random_displaced(vn);
      glVertex3f(vn.x(),vn.y(),z);
    }
  }
  glEnd();
  // if (hole) {
  //   glBegin(GL_LINES);
  //   for (uint i=0; i < count; i++){
  //     glVertex3d(center.x(),center.y(),z);
  //     Vector2d vn = vertices[i];
  //     if (randomized) vn = random_displaced(vn);
  //     glVertex3d(vn.x(),vn.y(),z);
  //   }
  //   glEnd();
  // }
}

void Poly::draw(int gl_type, bool randomized) const
{
  draw(gl_type, getZ(), randomized);
}

void Poly::drawVertexNumbers() const
{
  Vector3d v;
  for (uint i=0;i < vertices.size();i++){
    v = getVertexCircular3(i);
    glVertex3f(v.x(),v.y(),v.z());
    ostringstream oss;
    oss << i;
    Render::draw_string(v, oss.str());
  }
}

void Poly::drawVertexAngles() const
{
  Vector3d v;
  for (uint i=0;i < vertices.size();i++){
    v = getVertexCircular3(i);
    glVertex3f(v.x(),v.y(),v.z());
    double angle = angleAtVertex(i);
    ostringstream oss;
    oss << (int)(angle*180/M_PI);
    Render::draw_string(v, oss.str());
  }
}

void Poly::drawLineNumbers() const
{
  Vector3d v,v2;
  for (uint i=0;i < vertices.size();i++){
    v = getVertexCircular3(i);
    v2 = getVertexCircular3(i+1);
    ostringstream oss;
    oss << i;
    Render::draw_string((v+v2)/2., oss.str());
  }
}


string Poly::info() const
{
  ostringstream ostr;
  ostr <<"Poly at Z="<<z;
  ostr <<", "<< vertices.size() <<" vertices";
  ostr <<", extrf="<< extrusionfactor;
  return ostr.str();
}


string Poly::SVGpolygon(string style) const
{
  ostringstream ostr;
  ostr << "  <polygon points=\"";
  ostr.precision(5);
  for (uint i=0; i<size(); i++) {
    ostr  << fixed << vertices[i].x() << "," << vertices[i].y();
    if (i < size()-1) ostr << " ";
  }
  ostr << "\" style=\"" << style <<"\">";
  return ostr.str();
}

string Poly::SVGpath(const Vector2d &trans) const
{
  if (size()==0) return "";
  ostringstream ostr;
  Poly transpoly(*this,0);
  transpoly.move(trans);
  ostr.precision(5);
  if (closed)
    if (hole)
      ostr << "<polygon fill=\"white\" stroke=\"black\" stroke-width=\"0px\"";
    else
      ostr << "<polygon fill=\"black\" stroke=\"black\" stroke-width=\"0px\"";
  else
    ostr << "<polyline fill=\"white\" stroke=\"black\" stroke-width=\"1px\"";
  ostr << " points=\"";
  for (uint i=0; i < transpoly.size(); i++) {
    ostr  << fixed << transpoly[i].x() << " " << transpoly[i].y();
    if (i < size()-1) ostr << ", ";
  }
  //<< "fill-rule=\"evenodd\" ";
  ostr << "\" />";
  return ostr.str();
}

string Poly::gnuplot_path(const Vector2d &trans) const
{
  ostringstream ostr;
  ostr.precision(5);
  Poly transpoly(*this,0);
  transpoly.move(trans);
  ostr << "# " << size() << endl;
  for (uint i=0; i<size(); i++) {
    CL::IntPoint cp = Clipping::ClipperPoint(transpoly[i]);
    ostr  << fixed << cp.X << ", " << cp.Y;
    //if (i < size()-1) ostr << "," ;
    ostr << endl;
  }
  ostr << endl;
  return ostr.str();
}


void Poly::move(vector<Poly> &polys, const Vector2d &trans)
{
  for (uint i=0; i<polys.size(); i++)
    polys[i].move(trans);
}



///////////////////////////  ExPoly  /////////////////////////////////////////////

void ExPoly::clear()
{
  outer.clear();
  for (uint i=0; i < holes.size(); i++)
    holes[i].clear();
}

void ExPoly::draw(int gl_type, double z, bool randomized) const
{
  outer.draw(gl_type, z, randomized);
  for (uint i=0; i < holes.size(); i++)
    holes[i].draw(gl_type, z, randomized);
}

void ExPoly::draw(int gl_type, bool randomized) const
{
  outer.draw(gl_type, randomized);
  for (uint i=0; i < holes.size(); i++)
    holes[i].draw(gl_type, randomized);
}


void ExPoly::cleanup(double epsilon)
{
  outer.vertices = simplified(outer.vertices, epsilon);
  for (uint i=0; i < holes.size(); i++)
    holes[i].vertices = simplified(holes[i].vertices, epsilon);
}

void ExPoly::drawVertexNumbers() const
{
  outer.drawVertexNumbers();
  for (uint i=0; i < holes.size(); i++)
    holes[i].drawVertexNumbers();
}

void ExPoly::drawLineNumbers() const
{
  outer.drawLineNumbers();
  for (uint i=0; i < holes.size(); i++)
    holes[i].drawLineNumbers();
}


///////////////////////// poly drawing  /////////////////////////////////////////


void draw_poly(const Poly &poly, int gl_type, int linewidth, int pointsize,
	       const float *rgb, float a, bool randomized)
{
  glColor4f(rgb[0],rgb[1],rgb[2],a);
  glLineWidth(linewidth);
  glPointSize(pointsize);
  poly.draw(gl_type, randomized);
}

void draw_polys(const vector <Poly> &polys, int gl_type, int linewidth, int pointsize,
		const float *rgb, float a, bool randomized)
{
  glColor4f(rgb[0],rgb[1],rgb[2], a);
  glLineWidth(linewidth);
  glPointSize(pointsize);
  for(size_t p=0; p<polys.size();p++) {
    polys[p].draw(gl_type, randomized);
  }
}

void draw_polys(const vector< vector <Poly> > &polys,
		int gl_type, int linewidth, int pointsize,
		const float *rgb, float a, bool randomized)
{
  for(size_t p=0; p<polys.size();p++)
    draw_polys(polys[p], gl_type, linewidth, pointsize, rgb, a, randomized);
}

void draw_poly (const ExPoly &expoly, int gl_type, int linewidth, int pointsize,
		const float *rgb, float a, bool randomized)
{
  draw_poly(expoly.outer,  gl_type, linewidth, pointsize, rgb, a, randomized);
  for(size_t p=0; p < expoly.holes.size();p++)
    draw_poly(expoly.holes[p],  gl_type, linewidth, pointsize, rgb, a, randomized);
}

void draw_polys(const vector <ExPoly> &expolys, int gl_type, int linewidth, int pointsize,
		const float *rgb, float a, bool randomized)
{
  for(size_t p=0; p < expolys.size();p++) {
    draw_poly(expolys[p],  gl_type, linewidth, pointsize, rgb, a, randomized);
  }
}


void draw_polys_surface(const vector <Poly> &polys,
			const Vector2d &Min, const Vector2d &Max,
			double z,
			double cleandist,
			const float *rgb, float a)
{
  glColor4f(rgb[0],rgb[1],rgb[2], a);
  glDrawPolySurfaceRastered(polys, Min, Max, z, cleandist);

  // glColor4f(rgb[0],rgb[1],rgb[2], a);
  // for(size_t p=0; p<polys.size();p++) {
  //   polys[p].cleanup(cleandist);
  //   ::cleandist(polys[p].vertices, cleandist);
  //   polys[p].draw_as_surface();
  // }
}
void draw_polys_surface(const vector< vector<Poly> > &polys,
			const Vector2d &Min, const Vector2d &Max,
			double z,
			double cleandist,
			const float *rgb, float a)
{
  for(size_t p=0; p < polys.size();p++)
    draw_polys_surface(polys[p], Min, Max, z, cleandist, rgb, a);
}

void draw_polys_surface(const vector< ExPoly > &expolys,
			const Vector2d &Min, const Vector2d &Max,
			double z,
			double cleandist,
			const float *rgb, float a)
{
  for(size_t p=0; p < expolys.size();p++)
    draw_polys_surface(Clipping::getPolys(expolys[p]), Min, Max, z, cleandist, rgb, a);
}



void clearpolys(vector<Poly> &polys){
  for (uint i=0; i<polys.size();i++)
    polys[i].clear();
  polys.clear();
}
void clearpolys(vector<ExPoly> &polys){
  for (uint i=0; i<polys.size();i++) {
    polys[i].outer.clear();
    for (uint j=0; j<polys[i].holes.size();j++) {
      polys[i].holes[j].clear();
    }
  }
  polys.clear();
}
void clearpolys(vector< vector<Poly> > &polys){
  for (uint i=0; i<polys.size();i++)
    clearpolys(polys[i]);
  polys.clear();
}
