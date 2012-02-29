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

// template < typename T > 
// long double angleBetween(T V1, T V2) 
// {
//   long double dotproduct =  V1.dot(V2);
//   long double length = V1.length() * V2.length();
//   long double result = acosl( dotproduct / length ); // 0 .. pi 
//   if (isleftof(T(), V2, V1)) 
//       result = -result;
//   return result;
// }

// from V1 to V2
long double angleBetween(Vector3d V1, Vector3d V2)
{
  long double dotproduct =  V1.dot(V2);
  long double length = V1.length() * V2.length();
  long double result = acosl( dotproduct / length ); // 0 .. pi 
  if (isleftof(Vector3d(0,0,0), V2, V1)) 
      result = -result;
  return result;
}

long double angleBetween(Vector2d V1, Vector2d V2)
{
  long double result, dotproduct, length;
  dotproduct =  V1.dot(V2);
  length = V1.length() * V2.length();
  result = acosl( dotproduct / length ); // 0..pi
  if (isleftof(Vector2d(0,0), V2, V1)) 
      result = -result;
  return result;
}


// is B left of A wrt center?
bool isleftof(Vector2d center, Vector2d A, Vector2d B)
{
  double position = (B.x-A.x)*(center.y-A.y) - (B.y-A.y)*(center.x-A.x);
  return (position > 0);
}
bool isleftof(Vector3d center, Vector3d A, Vector3d B)
{
  return ((B-A).cross(center-A).z > 0); 
}
// // http://www.cs.uwaterloo.ca/~tmchan/ch3d/ch3dquad.cc
// double turn(Point p, Point q, Point r) {  // <0 iff cw
//   return (q.x-p.x)*(r.y-p.y) - (r.x-p.x)*(q.y-p.y);
// }

void center_perpendicular(const Vector2d from, const Vector2d to,
			  Vector2d &p1, Vector2d &p2)
{
  Vector2d center = (from+to)/2.;
  Vector2d dir = Vector2d(from.y-to.y, to.x-from.x);
  p1 = center; 
  p2 = center + dir;
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
//   double bx = p2.x - p1.x;
//   double by = p2.y - p1.y;
//   double dx = p4.x - p3.x;
//   double dy = p4.y - p3.y;
//   double b_dot_d_perp = bx * dy - by * dx;
//   if (abs(b_dot_d_perp) < maxoffset) { // parallel
//     return false;
//   }
//   double cx = p3.x - p1.x;
//   double cy = p3.y - p1.y;
//   double t = (cx * dy - cy * dx) / b_dot_d_perp;
//   if (t < maxoffset || t > 1-maxoffset) {
//     return false;
//   }
//   double u = (cx * by - cy * bx) / b_dot_d_perp;
//   if (u < maxoffset || u > 1-maxoffset) { 
//     return false;
//   }
//   hit.p = Vector2d(p1.x + t*bx, p2.y + t*by);
//   hit.t = t;
//   hit.d = (p1-hit.p).length();
//   return true;
// }




// calculates intersection and checks for parallel lines.
// also checks that the intersection point is actually on
// the line segment p1-p2
bool IntersectXY(const Vector2d &p1, const Vector2d &p2, 
		 const Vector2d &p3, const Vector2d &p4, Intersection &hit,
		 double maxoffset)
{
  // // BBOX test
  // if(MIN(p1.x,p2.x) > MAX(p3.x,p4.x))
  //   return false;
  // if(MAX(p1.x,p2.x) < MIN(p3.x,p4.x))
  //   return false;
  // if(MIN(p1.y,p2.y) > MAX(p3.y,p4.y))
  //   return false;
  // if(MAX(p1.y,p2.y) < MIN(p3.y,p4.y))
  //   return false;


  // if(ABS(p1.x-p3.x) < maxoffset && ABS(p1.y - p3.y) < maxoffset)
  //   {
  //     hit.p = p1;
  //     hit.d = (p1-hit.p).length();
  //     hit.t = 0.0;
  //     return true;
  //   }
  // if(ABS(p2.x-p3.x) < maxoffset && ABS(p2.y - p3.y) < maxoffset)
  //   {
  //     hit.p = p2;
  //     hit.d = (p1-hit.p).length();
  //     hit.t = 1.0;
  //     return true;
  //   }
  // if(ABS(p1.x-p4.x) < maxoffset && ABS(p1.y - p4.y) < maxoffset)
  //   {
  //     hit.p = p1;
  //     hit.d = (p1-hit.p).length();
  //     hit.t = 0.0;
  //     return true;
  //   }
  // if(ABS(p2.x-p4.x) < maxoffset && ABS(p2.y - p4.y) < maxoffset)
  //   {
  //     hit.p = p2;
  //     hit.d = (p1-hit.p).length();
  //     hit.t = 1.0;
  //     return true;
  //   }

  Intersection hit2;
  double t0,t1;
  int is = intersect2D_Segments(p1,p2,p3,p4,hit.p, hit2.p, t0,t1);
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
//    Input:  a point P, and a collinear segment S
//    Return: 1 = P is inside S
//            0 = P is not inside S
int inSegment( const Vector2d &P, const Vector2d &p1, const Vector2d &p2)
{
  if (p1.x != p2.x) {    // S is not vertical
    if (p1.x <= P.x && P.x <= p2.x)
      return 1;
    if (p1.x >= P.x && P.x >= p2.x)
      return 1;
  }
  else {    // S is vertical, so test y coordinate
    if (p1.y <= P.y && P.y <= p2.y)
      return 1;
    if (p1.y >= P.y && P.y >= p2.y)
      return 1;
  }
  return 0;
}

// intersect2D_2Segments(): the intersection of 2 finite 2D segments
//    Input:  two finite segments S1 and S2
//    Output: *I0 = intersect point (when it exists)
//            *I1 = endpoint of intersect segment [I0,I1] (when it exists)
//    Return: 0=disjoint (no intersect)
//            1=intersect in unique point I0
//            2=overlap in segment from I0 to I1
//            3=intersect outside 
#define perp(u,v)  ((u).x * (v).y - (u).y * (v).x)  // perp product (2D)
int intersect2D_Segments( const Vector2d &p1, const Vector2d &p2, 
			  const Vector2d &p3, const Vector2d &p4, 
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
      if (inSegment(p1, p3, p4) == 0)  // but is not in S2
	return 0;
      I0 = p1;
      return 1;
    }
    if (dv==0) {                    // S2 a single point
      if (inSegment(p3, p1,p2) == 0)  // but is not in S1
	return 0;
      I0 = p3;
      return 1;
    }
    // they are collinear segments - get overlap (or not)
    //double t0, t1;                   // endpoints of S1 in eqn for S2
    Vector2d w2 = p2 - p3;
    if (v.x != 0) {
      t0 = w.x / v.x;
      t1 = w2.x / v.x;
    }
    else {
      t0 = w.y / v.y;
      t1 = w2.y / v.y;
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
