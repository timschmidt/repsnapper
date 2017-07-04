/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010 Kulitorum
    Copyright (C) 2013 martin.dieringer@gmx.de

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
#include "triangle.h"

// limfit library for arc fitting
#include <lmmin.h>


// #ifdef WIN32
// #  include <GL/glut.h>	// Header GLUT Library
// #endif

#ifdef _MSC_VER
#  pragma warning( disable : 4018 4267)
#endif

/*
// call me before glutting
void checkGlutInit()
{
	static bool inited = false;
	if (inited)
		return;
	inited = true;
	int argc = 1;
	char *argv[] = { (char *) "repsnapper" };
	//glutInit (&argc, argv);
}

void drawString(const Vector3d &pos, const string &text)
{
  //drawString(pos,GLUT_BITMAP_HELVETICA_12,text);
}
void drawString(const Vector3d &pos, void* font, const string &text)
{
        checkGlutInit();
	//glRasterPos3d(pos.x(), pos.y(), pos.z());
	//for (uint c=0;c<text.length();c++)
	  //glutBitmapCharacter(font, text[c]);
}
*/

//////////////////////////// ARC FITTING //////////////////////////


/* function evaluation, determination of residues */
// residue is difference between radius parameter and distance of point to center
void evaluate_arcfit( const double *par, int m_dat, const void *data,
		      double *fvec, int *info )
{
  /* for readability, perform explicit type conversion */
  arc_data_struct *arcdata;
  arcdata = (arc_data_struct*)data;
  const Vector2d center(par[0], par[1]);
  //const double radius_sq = par[2];
  int i;
  for ( i = 0; i < m_dat; i++ ) {
    const Vector2d arcpoint(arcdata->px[i], arcdata->py[i]);
    const double distance_sq = center.squared_distance(arcpoint);
    fvec[i] = abs(par[2] - distance_sq); // sq.radius difference = residue
  }
}

bool fit_arc(const int m_dat, const arc_data_struct data,
	     const int n_par, double *par, double sq_error,
	     Vector2d &result_center, double &result_radiussq)
{
  lm_status_struct status;
  lm_control_struct control = lm_control_double;
  control.ftol = sq_error; // max square error sum

  // printf( "Fitting:\n" );
  lmmin( n_par, par, m_dat, (const void*) &data,
	 evaluate_arcfit, &control, &status);

  result_center.x() = par[0];
  result_center.y() = par[1];
  result_radiussq   = par[2];


  double fvec[m_dat];
  int info;
  evaluate_arcfit(par, m_dat, (const void*) &data, fvec, &info);
  double totres = 0;
  for (int i=0; i<m_dat; ++i ) totres+=fvec[i];

  return  (totres < sq_error);

#if 0
  printf( "\nResults:\n" );
  printf( "status after %d function evaluations:\n  %s\n",
	  status.nfev, lm_infmsg[status.info] );

  printf("obtained parameters:\n");
  int i;
  for ( i=0; i<n_par; ++i )
    printf("  par[%i] = %12g\n", i, par[i]);
  printf("obtained norm:\n  %12g\n", status.fnorm );
#endif

  // printf("fitting data as follows:\n");
  // double ff;
  // for ( i=0; i<m_dat; ++i ){
  //     ff = f(tx[i], tz[i], par);
  //     printf( "  t[%2d]=%12g,%12g y=%12g fit=%12g residue=%12g\n",
  //             i, tx[i], tz[i], y[i], ff, y[i] - ff );
  // }
  // free(data.px);
  // free(data.py);
}

// find center for best fit of arcpoints
bool fit_arc(const vector<Vector2d> &points, double sq_error,
	     Vector2d &result_center, double &result_radiussq)
{
  if (points.size() < 3) return false;
  const int n_par = 3; // center x,y and arc radius_sq
  // start values:
  const Vector2d P = points[0];
  const Vector2d Q = points.back();
  const Vector2d startxy = (P+Q)/2.;
  double par[3] = { startxy.x(), startxy.y(), P.squared_distance(Q) };

  int m_dat = points.size();
  arc_data_struct data;
  data.px = new double[m_dat];
  data.py = new double[m_dat];
  for (int i = 0; i < m_dat; i++) {
    data.px[i] = points[i].x();
    data.py[i] = points[i].y();
  }
  return fit_arc(m_dat, data, n_par, par, sq_error,
		 result_center, result_radiussq);
}


/////////////////////////////////////////////////////////////////////////////




/// VMML helpers

void move(const Vector3f &delta, Matrix4f &mat){
  Vector3f trans;
  mat.get_translation(trans);
  mat.set_translation(trans+delta);
}

Vector3d normalized(const Vector3d &v){
  Vector3d n(v); n.normalize(); return n;
}
Vector2d normalized(const Vector2d &v){
  Vector2d n(v); n.normalize(); return n;
}

void moveArcballTrans(Matrix4fT &matfT, const Vector3d &delta) {
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
void setArcballTrans(Matrix4fT &matfT, const Vector3d &trans) {
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
void rotArcballTrans(Matrix4fT &matfT,  const Vector3d &axis, double angle)
{
  Matrix4f rot = Matrix4f::IDENTITY;
  Vector3d naxis(axis); naxis.normalize();
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


// return A halfway rotated around center in direction of B
Vector2d angle_bipartition(const Vector2d &center, const Vector2d &A, const Vector2d &B)
{
  double angle = planeAngleBetween(center-A, B-center) / 2;
  return rotated(A, center, angle);
}

long double planeAngleBetween(const Vector2d &V1, const Vector2d &V2)
{
    long double dotproduct =  V1.dot(V2);
    long double length = V1.length() * V2.length();
    long double quot = dotproduct / length;
    if (quot > 1  && quot < 1.0001) quot = 1; // strange case where acos => NaN
    if (quot < -1 && quot > -1.0001) quot = -1;
    long double result = acosl( quot ); // 0 .. pi
    if (isleftof(Vector2d(0,0), V2, V1))
        result = -result;
    return result;
}


// is B left of A wrt center?
bool isleftof(const Vector2d &center, const Vector2d &A, const Vector2d &B)
{
  double position = (B.x()-A.x())*(center.y()-A.y()) - (B.y()-A.y())*(center.x()-A.x());
  return (position >= 0);
}
// // http://www.cs.uwaterloo.ca/~tmchan/ch3d/ch3dquad.cc
// double turn(Point p, Point q, Point r) {  // <0 iff cw
//   return (q.x()-p.x())*(r.y()-p.y()) - (r.x()-p.x())*(q.y()-p.y());
// }

void center_perpendicular(const Vector2d &from, const Vector2d &to,
			  Vector2d &p1, Vector2d &p2)
{
  Vector2d center = (from+to)/2.;
  Vector2d dir = Vector2d(from.y()-to.y(), to.x()-from.x());
  p1 = center;
  p2 = center + dir;
}


double cross(const Vector2d &a, const Vector2d &b)
{
  return (b.x()*a.y()) - (b.y()*a.x());
}

Vector3d cross2d(const Vector2d &A, const Vector2d &B, double z)
{
  Vector3d A3(A.x(),A.y(),z),  B3(B.x(),B.y(),z);
  return A3.cross(B3);
}
Vector2d normalV(const Vector2d &a)
{
  return Vector2d(-a.y(),a.x());
}


Vector3d random_displaced(const Vector3d &v, double delta)
{
  double randdelta = delta * (rand()%1000000)/1000000 - delta/2.;
  return Vector3d(v.x()+randdelta, v.y()+randdelta, v.z()+randdelta);
}
Vector2d random_displaced(const Vector2d &v, double delta)
{
  double randdelta = delta * (rand()%1000000)/1000000 - delta/2.;
  return Vector2d(v.x()+randdelta, v.y()+randdelta);
}

void rotate(Vector2d &p, const Vector2d &center, double angle, bool ccw)
{
  if (p==center) return ;
  if (!ccw) angle = -angle;
  double cosa = cos(angle);
  double sina = sin(angle);
  Vector2d r(p - center);
  p.x() = center.x() + r.x() * cosa - r.y() * sina;
  p.y() = center.y() + r.x() * sina + r.y() * cosa;
}

Vector2d rotated(const Vector2d &p, const Vector2d &center, double angle, bool ccw)
{
  Vector2d r(p);
  rotate(r,center,angle,ccw);
  return r;
  // Vector3d center3 (center.x(), center.y(), 0);
  // Vector3d radius3 (p.x() - center.x(), p.y() - center.y(), 0);
  // Vector3d axis(0.,0., ccw?1.:-1.);
  // Vector3d rrotated3 = radius3.rotate(angle, axis);
  // return center + Vector2d(rrotated3.x(), rrotated3.y());
}

// squared minimum distance of p to segment s1--s2, onseg = resulting point on segment
// http://stackoverflow.com/a/1501725
double point_segment_distance_Sq(const Vector2d &s1, const Vector2d &s2,
				 const Vector2d &p, Vector2d &onseg) {
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

vector<Poly> thick_lines(const vector<Vector2d> &points,  double width)
{
  vector<Poly> poly;
  for (uint i = 0; i<points.size()-1; i++) {
    vector<Poly> thick_l = thick_line(points[i],points[i+1],width);
    poly.insert(poly.end(),thick_l.begin(),thick_l.end());
  }
  return poly;
}

vector<Poly> thick_line(const Vector2d &from, const Vector2d &to, double width)
{
  vector<Poly> p;
  if (width < 0.001) return p;
  if (to.squared_distance(from) < 0.001) return p;
  Poly poly;
  Vector2d dir = (to-from); dir.normalize(); dir *= width/4.;
  Vector2d dirp(-dir.y(),dir.x());
  poly.addVertex(from-dir-dirp);
  poly.addVertex(from-dir+dirp);
  poly.addVertex(to+dir+dirp);
  poly.addVertex(to+dir-dirp);
  p.push_back(poly);
  return Clipping::getOffset(poly, width/4, jmiter, 0);

  // slow:
  // poly.addVertex(from);
  // poly.addVertex(to);
  // return Clipping::getOffset(poly, distance/2, jround, distance/2.);
}

// directed (one end is wider than the other)
vector<Poly> dir_thick_line(const Vector2d &from, const Vector2d &to,
			    double fr_width, double to_width)
{
  vector<Poly> p;
  if (fr_width < 0.001 || to_width < 0.001) return p;
  if (to.squared_distance(from) < 0.001) return p;
  if ((fr_width < 0) != (to_width < 0)) return p;
  Poly poly;
  Vector2d fdir = (to-from); fdir.normalize();
  Vector2d tdir = fdir;
  fdir *= fr_width/4.;
  tdir *= to_width/4.;
  Vector2d fr_dirp(-fdir.y(), fdir.x());
  Vector2d to_dirp(-tdir.y(), tdir.x());
  poly.addVertex(from-fdir-fr_dirp);
  poly.addVertex(from-fdir+fr_dirp);
  poly.addVertex(to+tdir+to_dirp);
  poly.addVertex(to+tdir-to_dirp);
  p.push_back(poly);
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




// Check the segments p1-p2 and p3-p4 for intersection,
// If the segments intersection, the intersection
// also checks that the intersection point is actually on
// the line segment p1-p2
bool IntersectXY(const Vector2d &p1, const Vector2d &p2,
		 const Vector2d &p3, const Vector2d &p4,
		 Intersection &hit, double maxoffset)
{
  Vector2d inter2;
  int is = intersect2D_Segments(p1,p2,p3,p4, hit.p, inter2);
  if (is > 0 && is < 3)
    {
      hit.d = (p1-hit.p).length();
      return true;
    }
  return false;
}


// The following functions
// (inSegment, intersect2D_Segments and dist3D_Segment_to_Segment)
// are licensed as:
//
// Copyright 2001 softSurfer, 2012-13 Dan Sunday
// This code may be freely used, distributed and modified for any
// purpose providing that this copyright notice is included with it.
// SoftSurfer makes no warranty for this code, and cannot be held
// liable for any real or imagined damage resulting from its use.
// Users of this code must verify correctness for their application.
//
// inSegment(): determine if a point is inside a segment
//    Input:  a point P, and a collinear segment p1--p2
//    Return: true  = P is inside p1--p2
//            fasle = P is not inside p1--p2
bool inSegment( const Vector2d &P, const Vector2d &p1, const Vector2d &p2)
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
//            3=lines intersect outside the segments
#define perp(u,v)  ((u).x() * (v).y() - (u).y() * (v).x())  // perp product (2D)
int intersect2D_Segments( const Vector2d &p1, const Vector2d &p2,
			  const Vector2d &p3, const Vector2d &p4,
			  Vector2d &I0, Vector2d &I1,
			  double maxerr)
{
  Vector2d    u = p2 - p1;
  Vector2d    v = p4 - p3;
  Vector2d    w = p1 - p3;
  double    D = perp(u,v);
  double t0, t1; // Temp vars for parametric checks

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
double dist3D_Segment_to_Segment(const Vector3d &S1P0, const Vector3d &S1P1,
				 const Vector3d &S2P0, const Vector3d &S2P1, double SMALL_NUM)
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

/////////////////// PATH IN POLYGON //////////////////////

bool pointInPolys(const Vector2d &point,  const vector<Poly> &polys)
{
  for (uint i=0; i< polys.size(); i++)
    if (polys[i].vertexInside(point)) return true;
  return false;
}

//  Public-domain code by Darel Rex Finley, 2006.
// http://alienryderflex.com/shortest_path/

// will return false
// if the line cuts any of the given polygons except excluded one
bool lineInPolys(const Vector2d &from, const Vector2d &to, const vector<Poly> &polys,
		 uint excludepoly, double maxerr)
{
  uint ninter = 0;
  for (uint i=0; i< polys.size(); i++) {
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
bool shortestPath(const Vector2d &from, const Vector2d &to,
		  const vector<Poly> &polys, int excludepoly,
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
  // for (uint i = 0; i < treeCount; i++) {
  //   cerr << i << " PL " << pointList[i].prev << endl;
  // }
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
Poly convexHull2D(const vector<Poly> &polygons)
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

///////////////////// CLEANUP/SIMPILFY //////////////////////

// make vertices at least epsilon apart
int cleandist(vector<Vector2d> &vert, double epsilon)
{
  uint n_vert = vert.size();
  double sqeps = epsilon * epsilon;
  uint n_moved = 0;
  if (vert[0].squared_distance(vert[n_vert-1]) < sqeps){
    const Vector2d center = (vert[0]+vert[n_vert-1])/2;
    Vector2d dir = vert[0]-center;
    dir.normalize();
    vert[0] = center + dir*epsilon;
    n_moved++;
  }
  for (uint i = 1; i < n_vert ; i++) {
    if (vert[i].squared_distance(vert[i-1]) < sqeps){
      const Vector2d center = (vert[i]+vert[i-1])/2;
      Vector2d dir = vert[i]-center;
      dir.normalize();
      vert[i] = center + dir*epsilon;
      n_moved++;
    }
  }
  return n_moved;
}


// Douglas-Peucker algorithm
vector<Vector2d> simplified(const vector<Vector2d> &vert, double epsilon)
{
  if (epsilon == 0) return vert;
  uint n_vert = vert.size();
  if (n_vert<3) return vert;
  double dmax = 0;
  //Find the point with the maximum distance from line start-end
  uint index = 0;
  Vector2d normal = normalV(vert.back()-vert.front());
  normal.normalize();
  if( (normal.length()==0) || ((abs(normal.length())-1)>epsilon) ) return vert;
  for (uint i = 1; i < n_vert-1 ; i++)
    {
      double dist = abs((vert[i]-vert.front()).dot(normal));
      if (dist >= epsilon && dist > dmax) {
	index = i;
	dmax = dist;
      }
    }
  vector<Vector2d> newvert;
  if (index > 0) // there is a point > epsilon
    {
      // divide at max dist point and cleanup both parts recursively
      vector<Vector2d> part1;
      part1.insert(part1.end(), vert.begin(), vert.begin()+index+1);
      vector<Vector2d> c1 = simplified(part1, epsilon);
      vector<Vector2d> part2;
      part2.insert(part2.end(), vert.begin()+index, vert.end());
      vector<Vector2d> c2 = simplified(part2, epsilon);
      newvert.insert(newvert.end(), c1.begin(), c1.end()-1);
      newvert.insert(newvert.end(), c2.begin(), c2.end());
    }
  else
    { // all points are nearer than espilon
      newvert.push_back(vert.front());
      newvert.push_back(vert.back());
    }
  return newvert;
}

///////////////////// POLY2TRI TRIANG ////////////////////

#include <poly2tri/poly2tri/poly2tri/poly2tri.h>

vector<p2t::Point*> getP2Tpoints(const Poly &poly)
{
  vector<p2t::Point*> points(poly.size());
  for (uint i=0; i<poly.size(); i++)  {
    points[i] = new p2t::Point(poly.vertices[i].x(),
			       poly.vertices[i].y());
  }
  return points;
}

Triangle getTriangle(p2t::Triangle *cdttriangle, double z)
{
  const p2t::Point *tp0 = cdttriangle->GetPoint(0);
  const p2t::Point *tp1 = cdttriangle->GetPoint(1);
  const p2t::Point *tp2 = cdttriangle->GetPoint(2);
  const Vector3d A((tp0->x), (tp0->y), z);
  const Vector3d B((tp1->x), (tp1->y), z);
  const Vector3d C((tp2->x), (tp2->y), z);
  return Triangle(A, B, C);
}

vector<Triangle> getTriangles(p2t::CDT &cdt, double z)
{
  vector<p2t::Triangle*> ptriangles = cdt.GetTriangles();
  vector<Triangle> triangles(ptriangles.size());
  for (guint i=0; i < ptriangles.size(); i++) {
    triangles[i] = getTriangle(ptriangles[i], z);
  }
  return triangles;
}

int triangulate(const vector<Poly> &polys, vector< vector<Triangle> > &triangles,
		double z)
{
  vector<ExPoly> expolys = Clipping::getExPolys(polys, z, 0);
  cerr << expolys.size() << endl;
  for (uint i = 0; i<expolys.size(); i++) {
    vector<p2t::Point*> outerpoints = getP2Tpoints(expolys[i].outer);
    if(outerpoints.size() > 0) {
      p2t::CDT cdt(outerpoints);
      for (uint h = 0; h < expolys[i].holes.size(); h++) {
        vector<p2t::Point*> holespoints = getP2Tpoints(expolys[i].holes[h]);
        cdt.AddHole(holespoints);
      }
      cdt.Triangulate();
      triangles.push_back(getTriangles(cdt, z));
    }
  }
  return triangles.size();
}


///////////////////// DELAUNAY TRIANG ////////////////////

#define REAL double
//#include "triangle/triangle/triangle.h"

//#include "triangle/PolygonTriangulator.h"

int delaunayTriang(const vector<Vector2d> &points,
		   vector<Triangle> &triangles,
		   double z)
{
#define PTRIANGULATOR 0
#if PTRIANGULATOR
  vector<Vector2d> spoints = simplified(points, 1);
  uint npoints = spoints.size();
  vector<double> xpoints(npoints);
  vector<double> ypoints(npoints);
  for (uint i = 0; i < npoints; i++) {
    xpoints[i] = spoints[npoints-i-1].x();
    ypoints[i] = spoints[npoints-i-1].y();
  }

  polytri::PolygonTriangulator pt(xpoints, ypoints);

  const polytri::PolygonTriangulator::Triangles * tr = pt.triangles();

  uint ntr = tr->size();

  cerr << npoints << " -> " << ntr << endl;

  triangles.resize(ntr);

  uint itri=0;
  for (polytri::PolygonTriangulator::Triangles::const_iterator itr = tr->begin();
       itr != tr->end(); ++itr) {

    const polytri::PolygonTriangulator::Triangle t = *itr;

    triangles[itri] = Triangle(Vector3d(xpoints[t[0]], ypoints[t[0]], z),
			       Vector3d(xpoints[t[1]], ypoints[t[1]], z),
			       Vector3d(xpoints[t[2]], ypoints[t[2]], z));
    itri++;
  }

  return ntr;
#endif

  // struct triangulateio in;

  // in.numberofpoints = npoints;
  // in.numberofpointattributes = (int)0;
  // in.pointlist =
  // in.pointattributelist = NULL;
  // in.pointmarkerlist = (int *) NULL;
  // in.numberofsegments = 0;
  // in.numberofholes = 0;
  // in.numberofregions = 0;
  // in.regionlist = (REAL *) NULL;

  // delclass = new piyush;
  // piyush *pdelclass = (piyush *)delclass;
  // triswitches.push_back('\0');
  // char *ptris = &triswitches[0];


  // pmesh = new piyush::__pmesh;
  // pbehavior = new piyush::__pbehavior;

  // piyush::__pmesh     * tpmesh     = (piyush::__pmesh *)     pmesh;
  // piyush::__pbehavior * tpbehavior = (piyush::__pbehavior *) pbehavior;

  // pdelclass->triangleinit(tpmesh);
  // pdelclass->parsecommandline(1, &ptris, tpbehavior);

  // pdelclass->transfernodes(tpmesh, tpbehavior, in.pointlist,
  // 			   in.pointattributelist,
  // 			   in.pointmarkerlist, in.numberofpoints,
  // 			   in.numberofpointattributes);
  // tpmesh->hullsize = pdelclass->delaunay(tpmesh, tpbehavior);

  // /* Ensure that no vertex can be mistaken for a triangular bounding */
  // /*   box vertex in insertvertex().                                 */
  // tpmesh->infvertex1 = (piyush::vertex) NULL;
  // tpmesh->infvertex2 = (piyush::vertex) NULL;
  // tpmesh->infvertex3 = (piyush::vertex) NULL;

  // 	/* Calculate the number of edges. */
  // tpmesh->edges = (3l * tpmesh->triangles.items + tpmesh->hullsize) / 2l;
  // pdelclass->numbernodes(tpmesh, tpbehavior);



  /////////////////////////////////////////////// triangle++ crap


  // typedef reviver::dpoint <double, 2> Point;
  // vector<Point> p(points.size());
  // for (uint i = 0; i < p.size(); i++) {
  //   p[i] = Point(points[i].x(),points[i].y());
  // }
  // tpp::Delaunay del(p);
  // /*
  //   -p  Triangulates a Planar Straight Line Graph (.poly file).
  //   -r  Refines a previously generated mesh.
  //   -q  Quality mesh generation.  A minimum angle may be specified.
  //   -a  Applies a maximum triangle area constraint.
  //   -u  Applies a user-defined triangle constraint.
  //   -A  Applies attributes to identify triangles in certain regions.
  //   -c  Encloses the convex hull with segments.
  //   -D  Conforming Delaunay:  all triangles are truly Delaunay.
  //   -j  Jettison unused vertices from output .node file.
  //   -e  Generates an edge list.
  //   -v  Generates a Voronoi diagram.
  //   -n  Generates a list of triangle neighbors.
  //   -g  Generates an .off file for Geomview.
  //   -B  Suppresses output of boundary information.
  //   -P  Suppresses output of .poly file.
  //   -N  Suppresses output of .node file.
  //   -E  Suppresses output of .ele file.
  //   -I  Suppresses mesh iteration numbers.
  //   -O  Ignores holes in .poly file.
  //   -X  Suppresses use of exact arithmetic.
  //   -z  Numbers all items starting from zero (rather than one).
  //   -o2 Generates second-order subparametric elements.
  //   -Y  Suppresses boundary segment splitting.
  //   -S  Specifies maximum number of added Steiner points.
  //   -i  Uses incremental method, rather than divide-and-conquer.
  //   -F  Uses Fortune's sweepline algorithm, rather than d-and-c.
  //   -l  Uses vertical cuts only, rather than alternating cuts.
  //   -s  Force segments into mesh by splitting (instead of using CDT).
  //   -C  Check consistency of final mesh.
  //   -Q  Quiet:  No terminal output except errors.
  // */
  // string switches = "pq0DzQ";
  // del.Triangulate(switches);
  // int ntri = del.ntriangles();
  // if (ntri>0) {
  //   triangles.resize(ntri);
  //   uint itri=0;
  //   for (tpp::Delaunay::fIterator dit = del.fbegin(); dit != del.fend(); ++dit) {
  //     Point pA = del.point_at_vertex_id(del.Org (dit));
  //     Point pB = del.point_at_vertex_id(del.Dest(dit));
  //     Point pC = del.point_at_vertex_id(del.Apex(dit));
  //     triangles[itri] = Triangle(Vector3d(pA[0],pA[1],z),
  //     				 Vector3d(pB[0],pB[1],z),
  //     				 Vector3d(pC[0],pC[1],z));
  //     // Vector2d vA = points[del.Org (dit)];
  //     // Vector2d vB = points[del.Dest(dit)];
  //     // Vector2d vC = points[del.Apex(dit)];
  //     // triangles[itri] = Triangle(Vector3d(vA.x(),vA.y(),z),
  //     // 				 Vector3d(vB.x(),vB.y(),z),
  //     // 				 Vector3d(vC.x(),vC.y(),z));
  //     itri++;
  //   }
  // }

  // return ntri;
  return 0;
}


// typedef struct {
//   Vector2d a,b,c;
// } triangle2;


// bool pointInCircumcircle(const triangle2 &t, const Vector2d &p)
// {
//   Vector2d l1p1,l1p2;
//   center_perpendicular(t.a, t.b, l1p1, l1p2);
//   Vector2d l2p1,l2p2;
//   center_perpendicular(t.a, t.c, l2p1, l2p2);
//   Vector2d center, ip;
//   double t0, t1;
//   int is = intersect2D_Segments(l1p1, l1p2, l2p1, l2p2,
//    				center, ip, t0,t1);

//   if (is > 0) {
//     if (t.a.squared_distance(center) > p.squared_distance(center))
//       return true;
//   }
//   return false;
// }

// int delaunayTriang(const vector<Vector2d> &points, vector<Triangle> &triangles,
// 		   double z)
// {
//   if (points.size() < 3) return 0;

//   triangles.clear();

//   uint nump = points.size();

//   vector<int> seq(nump,1); // 1,1,1...
//   partial_sum(seq.begin(), seq.end(), seq.begin()); // 1,2,3,...,20
//   srand(time(NULL));
//   random_shuffle(seq.begin(), seq.end()); // shuffle

//   vector<triangle2> d_triangles;

//   triangle2 t = { points[seq[0]-1], points[seq[1]-1], points[seq[2]-1] };
//   d_triangles.push_back(t);

//   for (uint i = 3; i < nump; i+=3) {
//     //triangle2 t = { points[seq[i]-1], points[seq[i+1]-1], points[seq[i+2]-1] };
//     bool deleted = false;
//     for (int ti = d_triangles.size()-1; ti >= 0 ; ti--) {
//       if (pointInCircumcircle( d_triangles[ti], points[seq[i]-1] )) {
// 	d_triangles.erase(d_triangles.begin()+ti);
// 	deleted = true;
//       }
//     }
//     if (deleted) {
//       // make new triangles in hole
//     }
//     else {
//       // make new triangle from next points and new point
//     }

//   }

//   return triangles.size();
//   // double xy[point_num*2];

//   // for (uint i=0; i < point_num; i+=2)  {
//   //   xy[i]   = poly.vertices[i].x();
//   //   xy[i+1] = poly.vertices[i].y();
//   // }
//   // int tri_num;
//   // int tri_vert[2*point_num*3];
//   // int tri_nabe[2*point_num*3];

//   // int result = dtris2(point_num, xy, &tri_num, tri_vert, tri_nabe);

//   // if (result != 0)
//   //   return triangles;

//   // double z = poly.getZ();
//   // for (int i=0; i<tri_num; i+=3) {
//   //   Vector3d A(poly[tri_vert[i]].x(),   poly[tri_vert[i]].y(),   z);
//   //   Vector3d B(poly[tri_vert[i+1]].x(), poly[tri_vert[i+1]].y(), z);
//   //   Vector3d C(poly[tri_vert[i+2]].x(), poly[tri_vert[i+2]].y(), z);
//   //   triangles.push_back(Triangle(A, B, C));
//   // }
// }



////////////////////////////// RASTER /////////////////////////////////////



#include <cairomm/cairomm.h>


Matrix3d getMatrix(const Cairo::RefPtr<Cairo::Context> context)
{
  /* A #cairo_matrix_t holds an affine transformation, such as a scale,
   * rotation, shear, or a combination of those. The transformation of
   * a point (x, y) is given by:
   * <programlisting>
   *     x_new = xx * x + xy * y + x0;
   *     y_new = yx * x + yy * y + y0;
   * </programlisting>
   **/
  //   typedef struct _cairo_matrix {
  //     double xx; double yx;
  //     double xy; double yy;
  //     double x0; double y0;
  //   } cairo_matrix_t;
  // In a cairo.matrix(1,2,3,4,5,6), 1 is a11, 2 is a21, 3 is a12, 4 is a22, 5 is a13 and 6 is a23. a31 and a32 are 0, a33 is 1.
  Cairo::Matrix cm;
  context->get_matrix(cm);
  Matrix3d m;
  m[0][0] = cm.xx; m[0][1] = cm.xy; m[0][2] = cm.x0;
  m[1][0] = cm.yx; m[1][1] = cm.yy; m[1][2] = cm.y0;
  m[2][0] = 0;     m[2][1] = 0;     m[2][2] = 1;
  return m;
}


bool rasterpolys(const vector<Poly> &polys,
		 const Vector2d &min, const Vector2d &max, double resolution,
		 Cairo::RefPtr<Cairo::ImageSurface> &surface,
		 Cairo::RefPtr<Cairo::Context> &context)
{
  Vector2d diag = max - min;
  int width  = (int)ceil(diag.x()/resolution);
  int height = (int)ceil(diag.y()/resolution);
  if (height <= 0 || width <= 0)
    return false;
  surface = Cairo::ImageSurface::create(Cairo::FORMAT_A8, width, height);
  //surface->set_device_offset(-min.x()/resolution, -min.y()/resolution);
  context = Cairo::Context::create (surface);
  context->set_fill_rule(Cairo::FILL_RULE_WINDING);
  context->set_antialias(Cairo::ANTIALIAS_DEFAULT);
  context->scale(1/resolution, 1/resolution);
  context->translate(-min.x(), -min.y());
  context->set_source_rgb (0,0,0);
  //cerr << min << endl <<getMatrix(context) << endl;

  // draw boundary
  // context->begin_new_path();
  // context->set_line_width(0.5);
  // context->move_to(min.x(),min.y());
  // context->line_to(max.x(),min.y());
  // context->line_to(max.x(),max.y());
  // context->line_to(min.x(),max.y());
  // // context->move_to(0,0);
  // // context->line_to(diag.x(),0);
  // // context->line_to(diag.x(),diag.y());
  // // context->line_to(0,diag.y());
  // context->close_path();
  // context->stroke();

  context->begin_new_path();
  context->set_line_width(0);
  for (uint i=0; i<polys.size(); i++) {
    Vector2d v = polys[i][0];
    context->move_to(v.x(),v.y());
    for (uint j=0; j<polys[i].size(); j++) {
      Vector2d v = polys[i][j];
      context->line_to(v.x(),v.y());
    }
  }
  context->fill();
  return true;
}


void glDrawPolySurfaceRastered(const vector<Poly> &polys,
			       const Vector2d &min, const Vector2d &max,
			       const double z,
			       const double resolution)
{
  Cairo::RefPtr<Cairo::ImageSurface> surface;
  Cairo::RefPtr<Cairo::Context> context;
  if (!rasterpolys(polys, min, max, resolution, surface, context)) return;
  glDrawCairoSurface(surface, min, max,z);
}


void glDrawCairoSurface(const Cairo::RefPtr<Cairo::ImageSurface> surface,
			const Vector2d &min, const Vector2d &max,
			const double z)
{
  if (!surface) return;
  int w = surface->get_width();
  int h = surface->get_height();
  unsigned char * data = surface->get_data();

  GLuint texture; glGenTextures( 1, &texture );
  glBindTexture( GL_TEXTURE_2D, texture );

  // http://www.nullterminator.net/gltexture.html
  // select modulate to mix texture with color for shading
  glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
  // when texture area is small, bilinear filter the closest mipmap
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		   GL_LINEAR_MIPMAP_NEAREST );
  // when texture area is large, bilinear filter the original
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

  // build our texture mipmaps
  gluBuild2DMipmaps( GL_TEXTURE_2D, GL_ALPHA, w, h,
		     GL_ALPHA, GL_UNSIGNED_BYTE, data );

  glEnable(GL_TEXTURE_2D);
  glBegin(GL_QUADS);
  glTexCoord2d(0.0,0.0); glVertex3d(min.x(),min.y(),z);
  glTexCoord2d(1.0,0.0); glVertex3d(max.x(),min.y(),z);
  glTexCoord2d(1.0,1.0); glVertex3d(max.x(),max.y(),z);
  glTexCoord2d(0.0,1.0); glVertex3d(min.x(),max.y(),z);
  glEnd();
  glDisable(GL_TEXTURE_2D);
  glDeleteTextures( 1, &texture );
}

int getCairoSurfaceDatapoint(const Cairo::RefPtr<Cairo::ImageSurface> surface,
			     const Vector2d &min, const Vector2d &max,
			     const Vector2d &p)
{
  if (!surface) return 0;
  const int w = surface->get_stride();
  const int h = surface->get_height();
  const unsigned char * data = surface->get_data();
  const Vector2d diag = max - min;
  const Vector2d relp = p - min;
  int ipx = (int)(relp.x() / diag.x() * (double)w);
  int ipy = (int)(relp.y() / diag.y() * (double)h);
  if (ipx < 0) ipx = 0;
  if (ipy < 0) ipy = 0;
  if (ipx >= w) ipx = w-1;
  if (ipy >= h) ipy = h-1;
  int value = data[ipy*w + ipx];
  return value;
}
