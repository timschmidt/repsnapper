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
#include "cuttingplane.h"
#include "slicer.h"
#include "infill.h"
#include "poly.h"

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


// Poly::Poly(){
//   this->plane = NULL;
//   this->vertices = NULL;
// }

Poly::Poly(CuttingPlane *plane, vector<Vector2d> *vertices){
  this->plane = plane;
  this->vertices = vertices;
  //cout << "POLY WITH PLANE"<< endl;
  //plane->printinfo();
  //printinfo();
}


// construct a Poly from a ClipperLib::Poly on given CuttingPlane
Poly::Poly(CuttingPlane *plane, vector<Vector2d> *vertices,
	   const ClipperLib::Polygon cpoly, bool reverse){
  this->plane = plane;
  this->vertices = vertices;
  uint count = cpoly.size();
  for (uint i = 0 ; i<count;  i++) { 
    Vector2d v;
    if (reverse)
      v = Vector2d(double(cpoly[count-1-i].X)/1000.-1000., 
		   double(cpoly[count-1-i].Y)/1000.-1000.);
    else
      v = Vector2d(double(cpoly[i].X)/1000.-1000., double(cpoly[i].Y)/1000.-1000.);
    //cerr << "Poly "<< i <<": "<< v <<  endl;
    points.push_back(vertices->size());
    vertices->push_back(v);
  }
  calcHole();
}


Poly::~Poly()
{
}


Poly Poly::Shrinked(vector<Vector2d> *vertices,
		    double distance) { 
  //this->plane = plane;
  this->vertices = vertices;
  return Shrinked(distance);
}


Poly Poly::Shrinked(double distance) const{
  assert(plane!=NULL);
  size_t count = points.size();
  assert(vertices->size() >= count);
  //cerr << "poly shrink dist=" << distance << endl;
  //plane->printinfo();
  Poly offsetPoly = Poly(plane,&plane->offsetVertices);
  //offsetPoly.printinfo();
  offsetPoly.points.clear();
  Vector2d Na,Nb,Nc,V1,V2,displace,p;
  long double s;
  for(size_t i=0; i < count; i++)
    {
      Na = getVertexCircular(i-1);
      Nb = getVertexCircular(i);
      Nc = getVertexCircular(i+1);      
      V1 = (Nb-Na).getNormalized();
      V2 = (Nc-Nb).getNormalized();
      
      displace = (V2 - V1).getNormalized();
      double a = angleBetween(V1,V2);

      bool convex = V1.cross(V2) < 0;
      s = sinl(a*0.5);
      if (convex){
	s=-s;
      }
      if (s==0){ // hopefully never happens
	cerr << "zero angle while shrinking, will have bad shells" << endl;
	s=1.; 
      }
      p = Nb + displace * distance/s;

      // index is of all offsetVertices in plane
      offsetPoly.points.push_back(offsetPoly.vertices->size()); //vert index of following:
      // add p to plane's offsetVertices == offsetPoly's vertices
      offsetPoly.vertices->push_back(p);
    }
  offsetPoly.calcHole();
  return offsetPoly;
}


// Remove vertices that are on a straight line
void Poly::cleanup(double maxerror)
{
  assert(vertices->size() >= points.size());
  for (size_t v = 0; v < points.size() + 1; )
    {
      Vector2d p1 = getVertexCircular(v-1);
      Vector2d p2 = getVertexCircular(v); 
      Vector2d p3 = getVertexCircular(v+1);

      Vector2d v1 = (p2-p1);
      Vector2d v2 = (p3-p2);

      v1.normalize();
      v2.normalize();

      if ((v1-v2).lengthSquared() < maxerror)
	{
	  points.erase(points.begin()+(v % points.size()));
	}
      else
	v++;
    }
}

void Poly::calcHole()
{
  	if(points.size() == 0)
		return;	// hole is undefined
	Vector2d p(-6000, -6000);
	int v=0;
	center = Vector2d(0,0);
	Vector2d q;
	for(size_t vert=0;vert<points.size();vert++)
	{
	  q = getVertexCircular(vert);
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
	center /= points.size();

	// we have the x-most vertex (with the highest y if there was a contest), v 
	Vector2d V1 = getVertexCircular(v-1);
	Vector2d V2 = getVertexCircular(v);
	Vector2d V3 = getVertexCircular(v+1);

	Vector2d Va=V2-V1;
	Vector2d Vb=V3-V2;
	hole = Va.cross(Vb) > 0; 
}

bool Poly::vertexInside(const Vector2d p, double maxoffset) const
{
  uint c = 0;
  for (uint i = 0; i < points.size()-1;  i++) {
    Vector2d Pi = getVertexCircular(i);
    Vector2d Pj = getVertexCircular(i+1);
    if ( ((Pi.y>p.y) != (Pj.y>p.y)) &&
	 (p.x < (Pj.x-Pi.x) * (p.y-Pi.y) / (Pj.y-Pj.y) + Pi.x) )
      c = !c;
  }
  return c;
}

bool Poly::polyInside(const Poly * poly, double maxoffset) const
{
  uint i, count=0;
  for (i = 0; i < poly->points.size();  i++) {
    Vector2d P = poly->getVertexCircular(i);
    if (vertexInside(P,maxoffset))
      count++;
  }
  return count == poly->points.size();
}


// bool Poly::vertexInsideOld(const Vector2d point, double maxoffset) const
// {
//   // Shoot a ray along +X and count the number of intersections.
//   // If n_intersections is even, return false, else return true
//   Vector2d EndP(point.x+10000, point.y);
//   int intersectcount = 1; // we want to test if uneven

//   for(size_t i=0; i<points.size();i++)
//     {
//       Vector2d P1 = getVertexCircular(i-1);
//       Vector2d P2 = getVertexCircular(i);
		    
//       // Skip horizontal lines, we can't intersect with them, 
//       // because the test line is horizontal
//       if(P1.y == P2.y)	
// 	continue;
      
//       InFillHit hit;
//       if(IntersectXY(point,EndP,P1,P2,hit,maxoffset))
// 	intersectcount++;
//     }
//   return intersectcount%2;
// }


Vector2d Poly::getVertexCircular(int pointindex) const
{
  uint size = points.size();
  pointindex = (pointindex + size) % size;
  //cerr << vertices->size() <<" >  "<< points[pointindex] << endl;
  assert (vertices->size() > points[pointindex]);
  return (*vertices)[points[pointindex]];
}

Vector3d Poly::getVertexCircular3(int pointindex) const
{
  Vector2d v = getVertexCircular(pointindex);
  return Vector3d(v.x,v.y,plane->Z);
}


// return intersection polys
vector< vector<Vector2d> > Poly::intersect(Poly &poly1, Poly &poly2) const
{
  ClipperLib::Polygon cpoly1 = poly1.getClipperPolygon(),
    cpoly2 =  poly2.getClipperPolygon();
  ClipperLib::Clipper clppr;
  ClipperLib::Polygons sol;
  clppr.AddPolygon(cpoly1,ClipperLib::ptSubject);
  clppr.AddPolygon(cpoly2,ClipperLib::ptClip);
  clppr.Execute(ClipperLib::ctIntersection, sol, 
		  ClipperLib::pftEvenOdd, ClipperLib::pftEvenOdd);

  vector< vector<Vector2d> > result;
  for(size_t i=0; i<sol.size();i++)
    {
      vector<Vector2d> polypoints;
      ClipperLib::Polygon cpoly = sol[i];
      for(size_t j=0; j<cpoly.size();j++){
	polypoints.push_back(Vector2d(cpoly[j].X,cpoly[j].Y));
      }
      result.push_back(polypoints);
    }
    return result;
}

ClipperLib::Polygon Poly::getClipperPolygon(bool reverse) const 
{
  ClipperLib::Polygon cpoly;
  for(size_t i=0; i<points.size();i++){
    Vector2d P1;
    if (reverse)
      P1 = getVertexCircular(-i); // normally have to reverse, why?
    else 
      P1 = getVertexCircular(i); 
    cpoly.push_back(ClipperLib::IntPoint(1000*(P1.x+1000.),
					 1000*(P1.y+1000.)));
  }
  return cpoly;
}


// intersection points of all lines (from start) with all lines of other poly
// does it work???
vector<Vector2d> Poly::intersect(Poly *other, int startVertex, 
				 const Vector2d endVertex, // use of this?
				 double maxoffset) const
{
  size_t count = points.size();
  size_t count2 = other->points.size();
  vector<Vector2d> result;

  //cerr << "poly::intersect" << endl;
  for(size_t v=startVertex; v<count;v++)
    {
      Vector2d P1 = getVertexCircular(v);
      Vector2d P2 = getVertexCircular(v+1);
      result.push_back(P1);
      for(size_t v2 = 0; v2 < count2; v2++)
	{
	  Vector2d P3 = other->getVertexCircular(v2);
	  Vector2d P4 = other->getVertexCircular(v2+1);
	  InFillHit hit;
	  if(P1 != P3 && P2 != P3 && P1 != P4 && P2 != P4)
	    if(IntersectXY(P1,P2,P3,P4,hit,maxoffset))
	      {
		// if( (hit.p-endVertex).length() < maxoffset)
		//   {
		//     //	outlines.push_back(result);
		//     //	return (v+1)%count;
		//   }
		result.push_back(hit.p);
		//  v=selfIntersectAndDivideRecursive(z, p2, (v2+1)%count2, outlines, hit.p, level);
		//  outlines.push_back(result);
		//  return;
	      }
	}
    }
  //cerr << "intersection points: " << result.size()<< endl;
  return result;
}


vector<InFillHit> Poly::lineIntersections(const Vector2d P1, const Vector2d P2,
					  double maxerr) const
{
  vector<InFillHit> HitsBuffer;
  Vector2d P3,P4;
  for(size_t i = 0; i < points.size(); i++)
    {  
      P3 = getVertexCircular(i);
      P4 = getVertexCircular(i+1);
      InFillHit hit;
      if (IntersectXY (P1,P2,P3,P4,hit,maxerr))
	HitsBuffer.push_back(hit);
    }
  return HitsBuffer;
}

double Poly::getZ() const {return plane->getZ();} 
double Poly::getLayerNo() const { return plane->LayerNo;}


vector<Vector2d> Poly::getInfillVertices () const {
  cerr << "get infill vertices"<<endl;
  printinfo();
  cerr << "get " << infill.infill.size() << " infill vertices "<< endl;
  return infill.infill;
};

void Poly::calcInfill (double InfillDistance,
		       double InfillRotation, // radians!
		       bool DisplayDebuginFill)
{
  ParallelInfill infill;
  cerr << " Poly parinfill: " << endl;
  if (!hole)
    infill.calcInfill(this,InfillRotation,InfillDistance); 
  infill.printinfo();
  this->infill = infill;
  printinfo();
}

vector<Vector2d> Poly::getMinMax() const{
  double minx=6000,miny=6000;
  double maxx=-6000,maxy=-6000;
  vector<Vector2d> range;
  Vector2d v;
  for (uint i=0; i < points.size();i++){
    v = getVertexCircular(i);
    //cerr << "vert " << i << ": " <<v << endl ;
    if (v.x<minx) minx=v.x;
    if (v.x>maxx) maxx=v.x;
    if (v.y<miny) miny=v.y;
    if (v.y>maxy) maxy=v.y;
  }
  //cerr<< "minmax at Z=" << plane->getZ() << ": "<<minx <<"/"<<miny << " -- "<<maxx<<"/"<<maxy<<endl;
  range.push_back(Vector2d(minx,miny));
  range.push_back(Vector2d(maxx,maxy));
  return range;
}


void Poly::draw(int gl_type) const
{
  Vector3d v;
  glBegin(gl_type);	  
  for (uint i=0;i < points.size();i++){
    v = getVertexCircular3(i);
    glVertex3f(v.x,v.y,v.z);
  }
  glEnd();
}

void Poly::drawVertexNumbers() const
{
  Vector3d v;
  for (uint i=0;i < points.size();i++){
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
  for (uint i=0;i < points.size();i++){
    v = getVertexCircular3(i);
    v2 = getVertexCircular3(i+1);
    ostringstream oss;
    oss << i;
    renderBitmapString((v+v2)/2., GLUT_BITMAP_8_BY_13 , oss.str());
  }
}


void Poly::printinfo() const
{
  cout <<"Poly at Z="<<plane->getZ()<<", layer "<<plane->LayerNo ;
  cout <<", "<< points.size();
  cout <<" points of "<< vertices->size();
  cout <<"vertices, infill: "<< infill.getSize();
  cout << endl;
}
