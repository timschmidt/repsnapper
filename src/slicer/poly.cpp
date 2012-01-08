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
#include "config.h"
#include "layer.h"
#include "shape.h"
//#include "infill.h"
#include "poly.h"
#include "printlines.h"


long double angleBetween(Vector2d V1, Vector2d V2)
{
  long double result, dotproduct, lengtha, lengthb;

  dotproduct =  (V1.x * V2.x) + (V1.y * V2.y);
  lengtha = V1.length();//sqrt(V1.x * V1.x + V1.y * V1.y);
  lengthb = V2.length();//sqrt(V2.x * V2.x + V2.y * V2.y);
	
	result = acosl( dotproduct / (lengtha * lengthb) );

  if(result < 0)
    result += M_PI;
  else
    result -= M_PI;
  return result;
}

// not correct
long double angleBetweenAtan2(Vector2d V1, Vector2d V2)
{
  long double result;

  Vector2d V1n = V1.getNormalized();
  Vector2d V2n = V2.getNormalized();
	
  long double a2 = atan2l(V2n.y, V2n.x);
  long double a1 = atan2l(V1n.y, V1n.x);
  result = a2 - a1;

  if(result < 0)
    result += 2.*M_PI;
  return result;
}


Poly::Poly(){
  holecalculated = false;
  this->z = -10;
  extrusionfactor = 1.;
}

Poly::Poly(double z, double extrusionfactor){
  this->z = z;
  this->extrusionfactor = extrusionfactor;
  holecalculated = false;
  hole=false;
  //cout << "POLY WITH PLANE"<< endl;
  //plane->printinfo();
  //printinfo();
}

Poly::Poly(const Poly p, double z){
  this->z = z;
  this->extrusionfactor = p.extrusionfactor;
  holecalculated = false;
  hole=false;
  uint count = p.vertices.size();
  vertices.resize(count);
  for (uint i=0; i<count ; i++)
    vertices[i] = p.vertices[i];
}


// Poly Poly::Shrinked(double distance) const{
//   assert(plane!=NULL);
//   size_t count = vertices.size();
//   //cerr << "poly shrink dist=" << distance << endl;
//   //plane->printinfo();
//   Poly offsetPoly(plane);
//   //offsetPoly.printinfo();
//   offsetPoly.vertices.clear();
//   Vector2d Na,Nb,Nc,V1,V2,displace,p;
//   long double s;
//   for(size_t i=0; i < count; i++)
//     {
//       Na = getVertexCircular(i-1);
//       Nb = getVertexCircular(i);
//       Nc = getVertexCircular(i+1);      
//       V1 = (Nb-Na).getNormalized();
//       V2 = (Nc-Nb).getNormalized();
      
//       displace = (V2 - V1).getNormalized();
//       double a = angleBetween(V1,V2);

//       bool convex = V1.cross(V2) < 0;
//       s = sinl(a*0.5);
//       if (convex){
// 	s=-s;
//       }
//       if (s==0){ // hopefully never happens
// 	cerr << "zero angle while shrinking, will have bad shells" << endl;
// 	s=1.; 
//       }
//       p = Nb + displace * distance/s;

//       offsetPoly.vertices.push_back(p);
//     }
//   offsetPoly.calcHole();
//   return offsetPoly;
// }

Poly::~Poly()
{
  //delete clipp;
}

// Remove vertices that are on a straight line
void Poly::cleanup(double maxerror)
{ 
  if (vertices.size()<1) return;
  for (size_t v = 0; v < vertices.size() + 1; )
    {
      Vector2d p1 = getVertexCircular(v-1);
      Vector2d p2 = getVertexCircular(v); 
      Vector2d p3 = getVertexCircular(v+1);

      Vector2d v1 = (p2-p1);
      Vector2d v2 = (p3-p2);

      if (v1.lengthSquared() == 0 || v2.lengthSquared() == 0) 
	{
	  vertices.erase(vertices.begin()+(v % vertices.size()));	  
	  if (vertices.size() < 1) break;
	}
      else
	{
	  v1.normalize();
	  v2.normalize();
	  if ((v1-v2).lengthSquared() < maxerror)
	    {
	      vertices.erase(vertices.begin()+(v % vertices.size()));
	      if (vertices.size() < 1) break;
	    }
	  else
	    v++;
	}
    }
}

void Poly::calcHole()
{
  	if(vertices.size() == 0)
	  return;	// hole is undefined
	Vector2d p(-6000, -6000);
	int v=0;
	center = Vector2d(0,0);
	Vector2d q;
	for(size_t vert=0;vert<vertices.size();vert++)
	{
	  q = vertices[vert];
	  center += q;
	  if(q.x > p.x)
	    {
	      p = q;
	      v=vert;
	    }
	  else if(q.x == p.x && q.y > p.y)
	    {
	      p.y = q.y;
	      v=vert;
	    }
	}
	center /= vertices.size();

	// we have the x-most vertex (with the highest y if there was a contest), v 
	Vector2d V1 = getVertexCircular(v-1);
	Vector2d V2 = getVertexCircular(v);
	Vector2d V3 = getVertexCircular(v+1);

	Vector2d Va=V2-V1;
	Vector2d Vb=V3-V2;
	hole = Va.cross(Vb) > 0; 
	holecalculated = true;
}

bool Poly::isHole() 
{
  if (!holecalculated)
    calcHole();
  return hole;
}


void Poly::rotate(Vector2d center, double angle) 
{
  double sina = sin(angle);
  double cosa = cos(angle);
  for (uint i = 0; i < vertices.size();  i++) {
    Vector2d mv = vertices[i]-center;
    vertices[i] = Vector2d(mv.x*cosa-mv.y*sina+center.x, 
			   mv.y*cosa+mv.x*sina+center.y);
  }
}

// Find the vertex in the poly closest to point p
uint Poly::nearestDistanceSqTo(const Vector2d p, double &mindist) const
{
  assert(vertices.size() > 0);

  // Start with first vertex as closest
  uint nindex = 0;
  mindist = (vertices[0]-p).lengthSquared();
  // check the rest of the vertices for a closer one.
  for (uint i = 1; i < vertices.size(); i++) {
    double d = (vertices[i]-p).lengthSquared();
    if (d<mindist) {
      mindist= d;
      nindex = i;
    }
  }
  return nindex;
}

bool Poly::vertexInside(const Vector2d p, double maxoffset) const
{
  uint c = 0;
  for (uint i = 0; i < vertices.size()-1;  i++) {
    Vector2d Pi = vertices[i];
    Vector2d Pj = vertices[i+1];
    if ( ((Pi.y>p.y) != (Pj.y>p.y)) &&
	 (p.x < (Pj.x-Pi.x) * (p.y-Pi.y) / (Pj.y-Pj.y) + Pi.x) )
      c = !c;
  }
  return c;
}

bool Poly::polyInside(const Poly * poly, double maxoffset) const
{
  uint i, count=0;
  for (i = 0; i < poly->vertices.size();  i++) {
    Vector2d P = poly->getVertexCircular(i);
    if (vertexInside(P,maxoffset))
      count++;
  }
  return count == poly->vertices.size();
}


void Poly::addVertex(Vector2d v, bool front)
{
  if (front)
    vertices.insert(vertices.begin(),v);
  else
    vertices.push_back(v);
};


Vector2d Poly::getVertexCircular(int index) const
{
  int size = vertices.size();
  index = (index + size) % size;
  //cerr << vertices->size() <<" >  "<< points[pointindex] << endl;
  return vertices[index];
}

Vector3d Poly::getVertexCircular3(int pointindex) const
{
  Vector2d v = getVertexCircular(pointindex);
  return Vector3d(v.x,v.y,z);
}




// ClipperLib::Polygons Poly::getOffsetClipperPolygons(double dist) const 
// {
//   bool reverse = true;
//   ClipperLib::Polygons cpolys;
//   ClipperLib::Polygons offset;
//   while (offset.size()==0){ // try to reverse poly vertices if no result
//     cpolys.push_back(getClipperPolygon(reverse));
//     // offset by half infillDistance
//     ClipperLib::OffsetPolygons(cpolys, offset, 1000.*dist,
// 			       ClipperLib::jtMiter,2);
//     reverse=!reverse;
//     if (reverse) break; 
//   }
//   return offset;
// }




vector<InFillHit> Poly::lineIntersections(const Vector2d P1, const Vector2d P2,
					  double maxerr) const
{
  vector<InFillHit> HitsBuffer;
  Vector2d P3,P4;
  for(size_t i = 0; i < vertices.size(); i++)
    {  
      P3 = getVertexCircular(i);
      P4 = getVertexCircular(i+1);
      InFillHit hit;
      if (IntersectXY (P1,P2,P3,P4,hit,maxerr))
	HitsBuffer.push_back(hit);
    }
  return HitsBuffer;
}

double Poly::getZ() const {return z;} 
// double Poly::getLayerNo() const { return plane->LayerNo;}


// vector<Vector2d> Poly::getInfillVertices () const {
//   cerr << "get infill vertices"<<endl;
//   printinfo();
//   cerr << "get " << infill.infill.size() << " infill vertices "<< endl;
//   return infill.infill;
// };

// void Poly::calcInfill (double InfillDistance,
// 		       double InfillRotation, // radians!
// 		       bool DisplayDebuginFill)
// {
//   ParallelInfill infill;
//   cerr << " Poly parinfill: " << endl;
//   if (!hole)
//     infill.calcInfill(this,InfillRotation,InfillDistance); 
//   infill.printinfo();
//   this->infill = infill;
//   printinfo();
// }


// length of the line starting at startindex
double Poly::getLinelengthSq(uint startindex) const
{
  double length = (getVertexCircular(startindex+1) - 
		   getVertexCircular(startindex)).lengthSquared();
  return length;
}

// add to lines starting with nearest point to startPoint
void Poly::getLines(vector<printline> &plines, Vector2d &startPoint) const
{
  if (size()<2) return;
  double mindist = 1000000;
  uint index = nearestDistanceSqTo(startPoint, mindist);
  getLines(plines,index);
  startPoint = Vector2d(plines.back().to.x,plines.back().to.y);
}


void Poly::getLines(vector<Vector3d> &lines, Vector2d &startPoint) const
{
  if (size()<2) return;
  double mindist = 1000000;
  uint index = nearestDistanceSqTo(startPoint, mindist);
  getLines(lines,index);
  startPoint = Vector2d(lines.back().x,lines.back().y);
}
void Poly::getLines(vector<Vector2d> &lines, Vector2d &startPoint) const
{
  if (size()<2) return;
  double mindist = 1000000;
  uint index = nearestDistanceSqTo(startPoint, mindist);
  getLines(lines,index);
  startPoint = Vector2d(lines.back().x,lines.back().y);
}

// add to lines starting with given index
// closed lines sequence if number of vertices > 2
void Poly::getLines(vector<Vector2d> &lines, uint startindex) const
{
  size_t count = vertices.size();
  if (count<2) return; // one point no line
  if (count<3) count--; // two points one line
  for(size_t i=0;i<count;i++)
    {
      lines.push_back(getVertexCircular(i+startindex));
      lines.push_back(getVertexCircular(i+startindex+1));
    }
}
// add to lines starting with given index
// closed lines sequence
void Poly::getLines(vector<printline> &plines, uint startindex) const
{
  size_t count = vertices.size();
  if (count<2) return; // one point no line
  if (count<3) count--; // two points one line
  for(size_t i=0;i<count;i++)
    {
      struct printline pline;
      pline.from = getVertexCircular3(i+startindex);
      pline.to = getVertexCircular3(i+startindex+1);
      pline.extrusionfactor = extrusionfactor ;
      plines.push_back(pline);
    }
}
void Poly::getLines(vector<Vector3d> &lines, uint startindex) const
{
  size_t count = vertices.size();
  if (count<2) return; // one point no line
  if (count<3) count--; // two points one line
  for(size_t i=0;i<count;i++)
    {
      lines.push_back(getVertexCircular3(i+startindex));
      lines.push_back(getVertexCircular3(i+startindex+1));
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
    //cerr << "vert " << i << ": " <<v << endl ;
    if (v.x<minx) minx=v.x;
    if (v.x>maxx) maxx=v.x;
    if (v.y<miny) miny=v.y;
    if (v.y>maxy) maxy=v.y;
  }
  //cerr<< "minmax at Z=" << plane->getZ() << ": "<<minx <<"/"<<miny << " -- "<<maxx<<"/"<<maxy<<endl;
  range[0] = Vector2d(minx,miny);
  range[1] = Vector2d(maxx,maxy);
  return range;
}

Vector3d rotatedZ(Vector3d v, double angle) 
{
  double sina = sin(angle);
  double cosa = cos(angle);
  return Vector3d(v.x*cosa-v.y*sina,
		  v.y*cosa+v.x*sina, v.z);
}

void Poly::draw(int gl_type, double z) const
{
  Vector2d v;
  uint count = vertices.size();
  glBegin(gl_type);	  
  for (uint i=0;i < count;i++){
    v = getVertexCircular(i);
    glVertex3f(v.x,v.y,z);
  }
  glEnd();
}

void Poly::draw(int gl_type, bool reverse) const
{
  Vector3d v;//,vn,m,dir;
  uint count = vertices.size();
  glBegin(gl_type);	  
  for (uint i=0;i < count;i++){
    if (reverse){
      v = getVertexCircular3(-i);
      // vn = getVertexCircular3(-i-1);
    } else {
      v = getVertexCircular3(i);
      // vn = getVertexCircular3(i+1);
    }
    glVertex3f(v.x,v.y,v.z);
    // if (gl_type==GL_LINE_LOOP){
    //   m = (v+vn)/2;
    //   dir = m + rotatedZ((vn-v)/10,M_PI/2);
    //   glVertex3f(dir.x,dir.y,z);
    // }
  }
  glEnd();
}

void Poly::drawVertexNumbers() const
{
  Vector3d v;
  for (uint i=0;i < vertices.size();i++){
    v = getVertexCircular3(i);
    glVertex3f(v.x,v.y,v.z);
    ostringstream oss;
    oss << i;
    renderBitmapString(v, GLUT_BITMAP_8_BY_13 , oss.str());
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
    renderBitmapString((v+v2)/2., GLUT_BITMAP_8_BY_13 , oss.str());
  }
}


void Poly::printinfo() const
{
  cout <<"Poly at Z="<<z;
  cout <<", "<< vertices.size();
  cout <<", extrf="<< extrusionfactor;
  cout <<" vertices";//, infill: "<< infill->getSize();
  cout << endl;
}








bool InFillHitCompareFunc(const InFillHit& d1, const InFillHit& d2)
{
	return d1.d < d2.d;
}

// calculates intersection and checks for parallel lines.
// also checks that the intersection point is actually on
// the line segment p1-p2
bool IntersectXY(const Vector2d &p1, const Vector2d &p2, 
		 const Vector2d &p3, const Vector2d &p4, InFillHit &hit,
		 double maxoffset)
{
	// BBOX test
	if(MIN(p1.x,p2.x) > MAX(p3.x,p4.x))
		return false;
	if(MAX(p1.x,p2.x) < MIN(p3.x,p4.x))
		return false;
	if(MIN(p1.y,p2.y) > MAX(p3.y,p4.y))
		return false;
	if(MAX(p1.y,p2.y) < MIN(p3.y,p4.y))
		return false;


	if(ABS(p1.x-p3.x) < maxoffset && ABS(p1.y - p3.y) < maxoffset)
	{
		hit.p = p1;
		hit.d = (p1-hit.p).length();
		hit.t = 0.0;
		return true;
	}
	if(ABS(p2.x-p3.x) < maxoffset && ABS(p2.y - p3.y) < maxoffset)
	{
		hit.p = p2;
		hit.d = (p1-hit.p).length();
		hit.t = 1.0;
		return true;
	}
	if(ABS(p1.x-p4.x) < maxoffset && ABS(p1.y - p4.y) < maxoffset)
	{
		hit.p = p1;
		hit.d = (p1-hit.p).length();
		hit.t = 0.0;
		return true;
	}
	if(ABS(p2.x-p4.x) < maxoffset && ABS(p2.y - p4.y) < maxoffset)
	{
		hit.p = p2;
		hit.d = (p1-hit.p).length();
		hit.t = 1.0;
		return true;
	}

	InFillHit hit2;
	double t0,t1;
	if(intersect2D_Segments(p1,p2,p3,p4,hit.p, hit2.p, t0,t1) == 1)
	{
	  hit.d = (p1-hit.p).length();
	  hit.t = t0;
	  return true;
	}
	return false;
}

