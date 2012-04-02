/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010 Kulitorum
    Copyright (C) 2012 martin.dieringer@gmx.de

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


#include "geometry.h"
#include "poly.h"
#include "clipping.h"

///////// Transform3D ////////////////////////////////////
Matrix4f Transform3D::getFloatTransform() const 
{
  return (Matrix4f) transform;
}
void Transform3D::setTransform(Matrix4f matr) 
{
  transform = (Matrix4f) matr;
}

Vector3d Transform3D::getTranslation() const
{
  Vector3d p;
  transform.get_translation(p);
  return p;
}


void Transform3D::move(Vector3d delta)
{
  Vector3d trans = getTranslation();
  transform.set_translation(trans + delta * transform[3][3]); // unscale delta
}

void Transform3D::scale(double x)
{
  if (x==0) return;
  transform[3][3] = 1/x;
}

void Transform3D::scale_x(double x)
{
  transform[0][0] = x;
}
  void Transform3D::scale_y(double x)
{
  transform[1][1] = x;
}
void Transform3D::scale_z(double x)
{
  transform[2][2] = x;
}

void Transform3D::rotate(Vector3d axis, double angle)
{
  Matrix4d rot = Matrix4d::IDENTITY;
  Vector3d naxis = axis; naxis.normalize();
  rot.rotate(angle, naxis);
  transform = rot * transform; 
}
void Transform3D::rotate(Vector3d center, double x, double y, double z)
{
  move(-center);
  if (x!=0) transform.rotate_x(x);
  if (y!=0) transform.rotate_y(y);
  if (z!=0) transform.rotate_z(z);
  move(center);
}
///////////////////////////////////////////////////////




/// VMML helpers

void move(Vector3f delta, Matrix4f &mat){
  Vector3f trans;
  mat.get_translation(trans);
  mat.set_translation(trans+delta);
}

Vector3d normalized(const Vector3d v){
  Vector3d n(v); n.normalize(); return n;
}
Vector2d normalized(const Vector2d v){
  Vector2d n(v); n.normalize(); return n;
}

void moveArcballTrans(Matrix4fT &matfT, const Vector3d delta) {
  Matrix4f matf;
  typedef Matrix4f::iterator mIt;
  for (mIt it = matf.begin(); it!=matf.end(); ++it){
    uint i = it - matf.begin();
    *it = matfT.M[i];
  }
  move(delta,matf);  
  for (mIt it = matf.begin(); it!=matf.end(); ++it){
    uint i = it - matf.begin();
    matfT.M[i] = *it;
  }
}
void setArcballTrans(Matrix4fT &matfT, const Vector3d trans) {
  Matrix4f matf;
  typedef Matrix4f::iterator  mIt;
  for (mIt it = matf.begin(); it!=matf.end(); ++it){
    uint i = it - matf.begin();
    *it = matfT.M[i];
  }
  matf.set_translation(trans);
  for (mIt it = matf.begin(); it!=matf.end(); ++it){
    uint i = it - matf.begin();
    matfT.M[i] = *it;
  }
}
void rotArcballTrans(Matrix4fT &matfT,  Vector3d axis, double angle)
{
  Matrix4f rot = Matrix4f::IDENTITY;
  Vector3d naxis = axis; naxis.normalize();
  rot.rotate(angle, naxis);
  Matrix4f matf;
  typedef Matrix4f::iterator  mIt;
  for (mIt it = matf.begin(); it!=matf.end(); ++it){
    uint i = it - matf.begin();
    *it = matfT.M[i];
  }
  matf = rot * matf;
  for (mIt it = matf.begin(); it!=matf.end(); ++it){
    uint i = it - matf.begin();
    matfT.M[i] = *it;
  }
}

// // from V1 to V2
// template< size_t M, typename T >
// long double angleBetween(const vmml::vector< M, T > V1, const vmml::vector< M, T > V2 ) 
// {
//   long double dotproduct =  V1.dot(V2);
//   long double length = V1.length() * V2.length();
//   long double quot = dotproduct / length;
//   if (quot > 1  && quot < 1.0001) quot = 1; // strange case where acos => NaN
//   if (quot < -1 && quot > -1.0001) quot = -1;
//   long double result = acosl( quot ); // 0 .. pi 
//   if (isleftof(T(), V2, V1)) 
//       result = -result;
//   return result;
// }

// from V1 to V2
long double angleBetween(Vector3d V1, Vector3d V2)
{
  long double dotproduct =  V1.dot(V2);
  long double length = V1.length() * V2.length();
  if (length==0) return 0;
  long double quot = dotproduct / length;
  if (quot > 1  && quot < 1.0001) quot = 1; // strange case where acos => NaN
  if (quot < -1 && quot > -1.0001) quot = -1;
  long double result = acosl( quot ); // 0 .. pi 
  if (isleftof(Vector3d(0,0,0), V2, V1)) 
      result = -result;
  return result;
}

long double angleBetween(Vector2d V1, Vector2d V2)
{
  long double dotproduct =  V1.dot(V2);
  long double length = V1.length() * V2.length();
  if (length==0) return 0;
  long double quot = dotproduct / length;
  if (quot > 1  && quot < 1.0001) quot = 1;
  if (quot < -1 && quot > -1.0001) quot = -1;
  long double result = acosl( quot ); // 0 .. pi 
  if (isleftof(Vector2d(0,0), V2, V1)) 
      result = -result;
  return result;
}


// is B left of A wrt center?
bool isleftof(Vector2d center, Vector2d A, Vector2d B)
{
  double position = (B.x()-A.x())*(center.y()-A.y()) - (B.y()-A.y())*(center.x()-A.x());
  return (position >= 0);
}
bool isleftof(Vector3d center, Vector3d A, Vector3d B)
{
  return ((B-A).cross(center-A).z() > 0); 
}
// // http://www.cs.uwaterloo.ca/~tmchan/ch3d/ch3dquad.cc
// double turn(Point p, Point q, Point r) {  // <0 iff cw
//   return (q.x()-p.x())*(r.y()-p.y()) - (r.x()-p.x())*(q.y()-p.y());
// }

void center_perpendicular(const Vector2d from, const Vector2d to,
			  Vector2d &p1, Vector2d &p2)
{
  Vector2d center = (from+to)/2.;
  Vector2d dir = Vector2d(from.y()-to.y(), to.x()-from.x());
  p1 = center; 
  p2 = center + dir;
}


double cross(const Vector2d a, const Vector2d b)
{
  return (b.x()*a.y()) - (b.y()*a.x());
}

Vector3d cross2d(Vector2d A, Vector2d B, double z)
{
  Vector3d A3(A.x(),A.y(),z),  B3(B.x(),B.y(),z);
  return A3.cross(B3);
}
Vector2d normalV(const Vector2d a)
{
  return Vector2d(-a.y(),a.x());
}


Vector3d random_displaced(Vector3d v, double delta)
{
  double randdelta = delta * (rand()%1000000)/1000000 - delta/2.;
  return Vector3d(v.x()+randdelta, v.y()+randdelta, v.z()+randdelta);
}
Vector2d random_displaced(Vector2d v, double delta)
{
  double randdelta = delta * (rand()%1000000)/1000000 - delta/2.;
  return Vector2d(v.x()+randdelta, v.y()+randdelta);
}


Vector2d rotated(Vector2d p, Vector2d center, double angle, bool ccw)
{
  Vector3d center3 (center.x(), center.y(), 0);
  Vector3d radius3 (p.x() - center.x(), p.y() - center.y(), 0);
  Vector3d axis(0.,0., ccw?1.:-1.);
  Vector3d rrotated3 = radius3.rotate(angle, axis);
  return center + Vector2d(rrotated3.x(), rrotated3.y());
}

// squared minimum distance of p to segment s1--s2, onseg = resulting point on segment
// http://stackoverflow.com/a/1501725
double point_segment_distance_Sq(const Vector2d s1, const Vector2d s2, 
				 const Vector2d p, Vector2d &onseg) {
  const double l2 = (s2-s1).squared_length();  // i.e. |w-v|^2 -  avoid a sqrt
  if (l2 == 0.0) { // s1 == s2 case
    onseg = s1;
    return (p-s1).squared_length();   
  } 
  // Consider the line extending the segment, parameterized as s1 + t (s2 - s1).
  // We find projection of point p onto the line. 
  // It falls where t = [(p-s1) . (s2-s1)] / |s2-s1|^2
  const double t = (p-s1).dot(s2-s1) / l2;
  if (t < 0.0) {
    onseg = s1;
    return (p-s1).squared_length();       // Beyond the 's1' end of the segment
  }
  else if (t > 1.0) {
    onseg = s2;
    return (p-s2).squared_length();  // Beyond the 's2' end of the segment
  }
  onseg = s1 + (s2 - s1) * t;  // Projection falls on the segment
  return (onseg-p).squared_length();
}


vector<Poly> thick_line(const Vector2d from, const Vector2d to, double width) 
{
  Poly poly;
  Vector2d dir = (to-from); dir.normalize(); dir *= width/4.;
  Vector2d dirp(-dir.y(),dir.x());
  poly.addVertex(from-dir-dirp);
  poly.addVertex(from-dir+dirp);
  poly.addVertex(to+dir+dirp);
  poly.addVertex(to+dir-dirp);
  vector<Poly> p; p.push_back(poly);
  return Clipping::getOffset(poly, width/4, jmiter, 0);

  // slow:
  // poly.addVertex(from);
  // poly.addVertex(to);
  // return Clipping::getOffset(poly, distance/2, jround, distance/2.);
}

// directed (one end is wider than the other)
vector<Poly> dir_thick_line(const Vector2d from, const Vector2d to, 
			    double fr_width, double to_width) 
{
  Poly poly;
  Vector2d fdir = (to-from); fdir.normalize(); fdir *= fr_width/4.;
  Vector2d tdir = (to-from); tdir.normalize(); tdir *= to_width/4.;
  Vector2d fr_dirp(-fdir.y(), fdir.x());
  Vector2d to_dirp(-tdir.y(), tdir.x());
  poly.addVertex(from-fdir-fr_dirp);
  poly.addVertex(from-fdir+fr_dirp);
  poly.addVertex(to+tdir+to_dirp);
  poly.addVertex(to+tdir-to_dirp);
  vector<Poly> p; p.push_back(poly);
  return p;
  //return Clipping::getOffset(poly, distance/4, jmiter, 0);

  // slow:
  // poly.addVertex(from);
  // poly.addVertex(to);
  // return Clipping::getOffset(poly, distance/2, jround, distance/2.);
}


//////////////////////////////////////////////////////////////
///////////////////////////// LINE INTERSECTION //////////////
//////////////////////////////////////////////////////////////

// Line Segment Intersection
// http://wiki.processing.org/w/Line-Line_intersection
// bool segIntersection(const Vector2d &p1, const Vector2d &p2, 
// 		     const Vector2d &p3, const Vector2d &p4, 
// 		     Intersection &hit,
// 		     double maxoffset) 
// { 
//   double bx = p2.x() - p1.x();
//   double by = p2.y() - p1.y();
//   double dx = p4.x() - p3.x();
//   double dy = p4.y() - p3.y();
//   double b_dot_d_perp = bx * dy - by * dx;
//   if (abs(b_dot_d_perp) < maxoffset) { // parallel
//     return false;
//   }
//   double cx = p3.x() - p1.x();
//   double cy = p3.y() - p1.y();
//   double t = (cx * dy - cy * dx) / b_dot_d_perp;
//   if (t < maxoffset || t > 1-maxoffset) {
//     return false;
//   }
//   double u = (cx * by - cy * bx) / b_dot_d_perp;
//   if (u < maxoffset || u > 1-maxoffset) { 
//     return false;
//   }
//   hit.p = Vector2d(p1.x() + t*bx, p2.y() + t*by);
//   hit.t = t;
//   hit.d = (p1-hit.p).length();
//   return true;
// }




// calculates intersection and checks for parallel lines.
// also checks that the intersection point is actually on
// the line segment p1-p2
bool IntersectXY(const Vector2d p1, const Vector2d p2, 
		 const Vector2d p3, const Vector2d p4, 
		 Intersection &hit, double maxoffset)
{
  // // BBOX test
  // if(MIN(p1.x(),p2.x()) > MAX(p3.x(),p4.x()))
  //   return false;
  // if(MAX(p1.x(),p2.x()) < MIN(p3.x(),p4.x()))
  //   return false;
  // if(MIN(p1.y(),p2.y()) > MAX(p3.y(),p4.y()))
  //   return false;
  // if(MAX(p1.y(),p2.y()) < MIN(p3.y(),p4.y()))
  //   return false;


  // if(ABS(p1.x()-p3.x()) < maxoffset && ABS(p1.y() - p3.y()) < maxoffset)
  //   {
  //     hit.p = p1;
  //     hit.d = (p1-hit.p).length();
  //     hit.t = 0.0;
  //     return true;
  //   }
  // if(ABS(p2.x()-p3.x()) < maxoffset && ABS(p2.y() - p3.y()) < maxoffset)
  //   {
  //     hit.p = p2;
  //     hit.d = (p1-hit.p).length();
  //     hit.t = 1.0;
  //     return true;
  //   }
  // if(ABS(p1.x()-p4.x()) < maxoffset && ABS(p1.y() - p4.y()) < maxoffset)
  //   {
  //     hit.p = p1;
  //     hit.d = (p1-hit.p).length();
  //     hit.t = 0.0;
  //     return true;
  //   }
  // if(ABS(p2.x()-p4.x()) < maxoffset && ABS(p2.y() - p4.y()) < maxoffset)
  //   {
  //     hit.p = p2;
  //     hit.d = (p1-hit.p).length();
  //     hit.t = 1.0;
  //     return true;
  //   }

  Vector2d inter2;
  double t0, t1;
  int is = intersect2D_Segments(p1,p2,p3,p4, hit.p, inter2, t0, t1);
  if (is > 0 && is < 3)
    {
      hit.d = (p1-hit.p).length();
      hit.t = t0;
      return true;
    }
  return false;
}


// Following functions licensed as:
// Copyright 2001, softSurfer (www.softsurfer.com)
// This code may be freely used and modified for any purpose
// providing that this copyright notice is included with it.
// SoftSurfer makes no warranty for this code, and cannot be held
// liable for any real or imagined damage resulting from its use.
// Users of this code must verify correctness for their application.
//
// inSegment(): determine if a point is inside a segment
//    Input:  a point P, and a collinear segment p1--p2
//    Return: true  = P is inside p1--p2
//            fasle = P is not inside p1--p2
bool inSegment( const Vector2d P, const Vector2d p1, const Vector2d &p2)
{
  if (p1.x() != p2.x()) {    // S is not vertical
    if (p1.x() <= P.x() && P.x() <= p2.x())
      return true;
    if (p1.x() >= P.x() && P.x() >= p2.x())
      return true;
  }
  else {    // S is vertical, so test y coordinate
    if (p1.y() <= P.y() && P.y() <= p2.y())
      return true;
    if (p1.y() >= P.y() && P.y() >= p2.y())
      return true;
  }
  return false;
}

// intersect2D_2Segments(): the intersection of 2 finite 2D segments
//    Input:  two finite segments p1-p2 and p3-p4
//    Output: *I0 = intersect point (when it exists)
//            *I1 = endpoint of intersect segment [I0,I1] (when it exists)
//    Return: 0=disjoint (no intersect)
//            1=intersect in unique point I0
//            2=overlap in segment from I0 to I1
//            3=intersect outside 
#define perp(u,v)  ((u).x() * (v).y() - (u).y() * (v).x())  // perp product (2D)
int intersect2D_Segments( const Vector2d p1, const Vector2d p2, 
			  const Vector2d p3, const Vector2d p4, 
			  Vector2d &I0, Vector2d &I1, 
			  double &t0, double &t1,
			  double maxerr)
{
  Vector2d    u = p2 - p1;
  Vector2d    v = p4 - p3;
  Vector2d    w = p1 - p3;
  double    D = perp(u,v);

  // test if they are parallel (includes either being a point)
  if (abs(D) < maxerr) {          // S1 and S2 are parallel
    if (perp(u,w) != 0 || perp(v,w) != 0) {
      return 0;                   // they are NOT collinear
    }
    // they are collinear or degenerate
    // check if they are degenerate points
    double du = u.dot(u);
    double dv = v.dot(v);
    if (du==0 && dv==0) {           // both segments are points
      if (p1 != p3)         // they are distinct points
	return 0;
      I0 = p1;                // they are the same point
      return 1;
    }
    if (du==0) {                    // S1 is a single point
      if (!inSegment(p1, p3, p4))  // but is not in S2
	return 0;
      I0 = p1;
      return 1;
    }
    if (dv==0) {                    // S2 a single point
      if (!inSegment(p3, p1,p2))  // but is not in S1
	return 0;
      I0 = p3;
      return 1;
    }
    // they are collinear segments - get overlap (or not)
    //double t0, t1;                   // endpoints of S1 in eqn for S2
    Vector2d w2 = p2 - p3;
    if (v.x() != 0) {
      t0 = w.x() / v.x();
      t1 = w2.x() / v.x();
    }
    else {
      t0 = w.y() / v.y();
      t1 = w2.y() / v.y();
    }
    if (t0 > t1) {                  // must have t0 smaller than t1
      double t=t0; t0=t1; t1=t;    // swap if not
    }
    if (t0 > 1 || t1 < 0) {
      return 0;     // NO overlap
    }
    t0 = t0<0? 0 : t0;              // clip to min 0
    t1 = t1>1? 1 : t1;              // clip to max 1
    if (t0 == t1) {                 // intersect is a point
      I0 = p3 + v*t0;
      return 1;
    }

    // they overlap in a valid subsegment
    I0 = p3 + v*t0;
    I1 = p3 + v*t1;
    return 2;
  } // end parallel

  bool outside = false;
  // the segments are skew and may intersect in a point
  // get the intersect parameter for S1
  t0 = perp(v,w) / D;
  if (t0 < 0 || t0 > 1)               // no intersect in S1
    outside = true;
  //return 0;

  // get the intersect parameter for S2
  t1 = perp(u,w) / D;
  if (t1 < 0 || t1 > 1)               // no intersect in S2
    outside = true;
  //    return 0;

  I0 = p1 + u * t0;               // compute S1 intersect point
  if (outside) return 3;
  return 1;
}


// dist3D_Segment_to_Segment():
//    Input:  two 3D line segments S1 and S2
//    Return: the shortest distance between S1 and S2
double dist3D_Segment_to_Segment(Vector3d S1P0, Vector3d S1P1, 
				 Vector3d S2P0, Vector3d S2P1, double SMALL_NUM)
{
     Vector3d   u = S1P1 - S1P0;
     Vector3d   v = S2P1 - S2P0;
     Vector3d   w = S1P0 - S2P0;
     double    a = u.dot(u);        // always >= 0
     double    b = u.dot(v);
     double    c = v.dot(v);        // always >= 0
     double    d = u.dot(w);
     double    e = v.dot(w);
     double    D = a*c - b*b;       // always >= 0
     double    sc, sN, sD = D;      // sc = sN / sD, default sD = D >= 0
     double    tc, tN, tD = D;      // tc = tN / tD, default tD = D >= 0

     // compute the line parameters of the two closest points
     if (D < SMALL_NUM) { // the lines are almost parallel
         sN = 0.0;        // force using point P0 on segment S1
         sD = 1.0;        // to prevent possible division by 0.0 later
         tN = e;
         tD = c;
     }
     else {                // get the closest points on the infinite lines
         sN = (b*e - c*d);
         tN = (a*e - b*d);
         if (sN < 0.0) {       // sc < 0 => the s=0 edge is visible
             sN = 0.0;
             tN = e;
             tD = c;
         }
         else if (sN > sD) {  // sc > 1 => the s=1 edge is visible
             sN = sD;
             tN = e + b;
             tD = c;
         }
     }

     if (tN < 0.0) {           // tc < 0 => the t=0 edge is visible
         tN = 0.0;
         // recompute sc for this edge
         if (-d < 0.0)
             sN = 0.0;
         else if (-d > a)
             sN = sD;
         else {
             sN = -d;
             sD = a;
         }
     }
     else if (tN > tD) {      // tc > 1 => the t=1 edge is visible
         tN = tD;
         // recompute sc for this edge
         if ((-d + b) < 0.0)
             sN = 0;
         else if ((-d + b) > a)
             sN = sD;
         else {
             sN = (-d + b);
             sD = a;
         }
     }
     // finally do the division to get sc and tc
     sc = (abs(sN) < SMALL_NUM ? 0.0 : sN / sD);
     tc = (abs(tN) < SMALL_NUM ? 0.0 : tN / tD);

     // get the difference of the two closest points
     Vector3d dP = w + (u * sc ) - (v * tc);  // = S1(sc) - S2(tc)

     return dP.length();   // return the closest distance
 }

void testangles(){
  Vector2d C(0,0);
  Vector2d P(-1,1);
  for (double a = 0; a < 2*M_PI; a+=0.3){
    Vector2d Q(cos(a), sin(a));
    double an = angleBetween(P,Q);
    double bn = angleBetween(Q,P);
    //if(an<0) an+=2*M_PI;
    //if(bn<0) bn+=2*M_PI;
    cerr << a*180/M_PI << " - " << an*180/M_PI << " - " << bn*180/M_PI << endl;
  }
}




/////////////////// PATH IN POLYGON //////////////////////

bool pointInPolys(Vector2d point,  vector<Poly> polys)
{
  for (uint i=0; i< polys.size(); i++)
    if (polys[i].vertexInside(point)) return true;
  return false;
}

//  Public-domain code by Darel Rex Finley, 2006.
// http://alienryderflex.com/shortest_path/

// will return false
// if the line cuts any of the given polygons except excluded one
bool lineInPolys(Vector2d from, Vector2d to, vector<Poly> polys, 
		 int excludepoly, double maxerr)
{
  uint ninter = 0;
  for (int i=0; i< (int)polys.size(); i++) {
    if (i != excludepoly){
      if (polys[i].vertexInside(from)) ninter++;
      if (polys[i].vertexInside(to)) ninter++;
      vector<Intersection> inter = polys[i].lineIntersections(from,to,maxerr);
      ninter += inter.size();
    }
  }
  return (ninter != 0);
}
      
//  Finds the shortest path from from to to that stays within the polygon set.
//
//  Note:  To be safe, the solutionX and solutionY arrays should be large enough
//         to accommodate all the corners of your polygon set (although it is
//         unlikely that anywhere near that many elements will ever be needed).
//
//  Returns true if the optimal solution was found, or false if there is no solution.
//  If a solution was found, the solution vector will contain the coordinates
//  of the intermediate nodes of the path, in order.  (The startpoint and endpoint
//  are assumed, and will not be included in the solution.)

struct pathpoint {
  Vector2d v;
  double totalDist;
  int prev;
};
// excludepoly: poly not to test, contains from and to vectors.
bool shortestPath(Vector2d from, Vector2d to, vector<Poly> polys, int excludepoly, 
		  vector<Vector2d> &path, double maxerr)
{
  //  Fail if either the startpoint or endpoint is outside the polygon set.
  if (!pointInPolys(from, polys)
      ||  !pointInPolys(to, polys))
    return false; 
  
  //  If there is a straight-line solution, no path vertices added
  if (lineInPolys(from, to, polys, excludepoly, maxerr)) 
    return true; 

  const double INF = 9999999.;     //  (larger than total solution dist could ever be)
  double  bestDist, newDist ;
  // uint numpoints=0;
  // for (uint i=0; i<polys.size(); i++) numpoints += polys[i].size();
  vector<struct pathpoint> pointList;
  //  Build a point list that refers to the corners of the
  //  polygons, as well as to the startpoint and endpoint.
  pointList.push_back((pathpoint){from, 0, 0});
  for (uint i=0; i<polys.size(); i++)
    for (uint j=0; j<polys[i].size(); j++)
      pointList.push_back((pathpoint){polys[i].vertices[j], 0, 0});
  pointList.push_back((pathpoint){to, 0, 0});
  uint pointCount = pointList.size();

  //  Initialize the shortest-path tree to include just the startpoint.
  uint treeCount=1; 

  //  Iteratively grow the shortest-path tree until it reaches the endpoint
  //  -- or until it becomes unable to grow, in which case exit with failure.
  uint bestI = 0, bestJ = 0;
  while (bestJ < pointCount-1) { // until the to-point is arrived
    bestDist = INF;
    // for every path point so far find another with minimal total path length 
    for (uint i = 0; i < treeCount; i++) {
      for (uint j = treeCount; j < pointCount; j++) {
	if (lineInPolys(pointList[i].v, pointList[j].v, 
			polys, -1, maxerr)) { // line does not intersect
	  // take point into account
	  newDist = pointList[i].totalDist + 
	    (pointList[i].v - pointList[j].v).length();
	  if (newDist < bestDist) { // if shortest total path 
	    //cerr << i << " found " << j << " - " << newDist << " - " <<pointList[j].v <<endl;
	    bestDist = newDist;
	    bestI = i;
	    bestJ = j;
	  }
	}
      }
    }
    if (bestDist==INF) return false;   //  (no solution)

    pointList[bestJ].prev      = bestI   ;
    pointList[bestJ].totalDist = bestDist;
    //cerr << bestI << " PRE "<<pointList[bestJ].prev << endl;
    // store all tree points below treeCount
    // struct pathpoint SWAP = pointList[treeCount];
    // pointList[treeCount] = pointList[bestJ];
    // pointList[bestJ] = SWAP;
    swap(pointList[bestJ], pointList[treeCount]);
    treeCount++; 
  }
  for (uint i = 0; i < treeCount; i++) {
    cerr << i << " PL " << pointList[i].prev << endl;
  }
  int numsolutions = 0;
  int i = treeCount -1;
  while (i > 0) {
    path.push_back(pointList[i].v);
    i = pointList[i].prev;
    numsolutions++;
    cerr << treeCount << " - " << numsolutions << " - " << i << endl;
  }
  return true;
  
  int j = numsolutions-1;
  if (j > 0) {
    int psize = path.size();
    path.resize(psize + j);
    i = treeCount-1;
    while (j >= 0) {
      i = pointList[i].prev;
      path[psize + j] = pointList[i].v;
      j--;
    }
  }
  //  Success.
  return true; 
}
  


/////////////////////////// CONVEX HULL /////////////////////////

// http://en.wikibooks.org/wiki/Algorithm_Implementation/Geometry/Convex_hull/Monotone_chain


// 2D cross product of OA and OB vectors, i.e. z-component of their 3D cross product.
// Returns a positive value, if OAB makes a counter-clockwise turn,
// negative for clockwise turn, and zero if the points are collinear.
double cross_2(const Vector2d &O, const Vector2d &A, const Vector2d &B)
{
   return (A.x() - O.x()) * (B.y() - O.y()) - (A.y() - O.y()) * (B.x() - O.x());
}

struct sortable_point {
  Vector2d v;
  bool operator <(const sortable_point &p) const {
    return (v.x() < p.v.x()) || ((v.x() == p.v.x()) && (v.y() < p.v.y()));
  }
};

// calc convex hull and Min and Max of layer
// Monotone chain algo
Poly convexHull2D(const vector<Poly> polygons) 
{
  Poly hullPolygon;
  vector<struct sortable_point> P;
  for (uint i = 0; i<polygons.size(); i++) 
    for (uint j = 0; j<polygons[i].size(); j++) {
      struct sortable_point point;
      point.v = polygons[i].vertices[j];
      P.push_back(point);
    }
  
  int n = P.size();
  vector<Vector2d> H(2*n);

  if (n<2) return hullPolygon;
  if (n<4) {
    for (int i = 0; i < n; i++) 
      hullPolygon.addVertex(P[i].v);
    return hullPolygon;
  }
  sort(P.begin(), P.end());

  // Build lower hull
  int k=0;
  for (int i = 0; i < n; i++) {
    while (k >= 2 && cross_2(H[k-2], H[k-1], P[i].v) <= 0) k--;
    H[k++] = P[i].v;
  }
 
  // Build upper hull
  for (int i = n-2, t = k+1; i >= 0; i--) {
    while (k >= t && cross_2(H[k-2], H[k-1], P[i].v) <= 0) k--;
    H[k++] = P[i].v;
  }
  H.resize(k);
  hullPolygon.vertices = H;
  hullPolygon.reverse();
  return hullPolygon;
}
