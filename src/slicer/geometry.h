/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010 Kulitorum

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

#pragma once

#include "stdafx.h"
#include "arcball.h"


#define PI 3.141592653589793238462643383279502884197169399375105820974944592308

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


/* void drawString(const Vector3d &pos, void* font, const string &text); */
/* void drawString(const Vector3d &pos, const string &text); */
/* void checkGlutInit(); */


void move(const Vector3f &delta, Matrix4f &mat);
Vector3d normalized(const Vector3d &v);
Vector2d normalized(const Vector2d &v);
Vector2d normalV(const Vector2d &a);
double cross(const Vector2d &a, const Vector2d &b);
void moveArcballTrans(Matrix4fT &matfT, const Vector3d &delta);
void setArcballTrans(Matrix4fT &matfT, const Vector3d &trans);
void rotArcballTrans(Matrix4fT &transform,  const Vector3d &axis, double angle);

bool isleftof(const Vector2d &center, const Vector2d &A, const Vector2d &B);


long double planeAngleBetween(const Vector2d &V1, const Vector2d &V2);

Vector2d angle_bipartition(const Vector2d &center, const Vector2d &A, const Vector2d &B);
void center_perpendicular(const Vector2d &from, const Vector2d &to,
			  Vector2d &p1, Vector2d &p2);

Vector3d cross2d(const Vector2d &A, const Vector2d &B, double z=0);

double point_segment_distance_Sq(const Vector2d &s1, const Vector2d &s2,
				 const Vector2d &p, Vector2d &onseg);

void     rotate (      Vector2d &p, const Vector2d &center, double angle, bool ccw=true);
Vector2d rotated(const Vector2d &p, const Vector2d &center, double angle, bool ccw=true);

struct printline;

struct Intersection
{
	Vector2d p;  // The intersection point
	double d;     // Distance from the start point, used for sorting hits
        bool operator <(const Intersection &p) const { return (d<p.d); } // for std::sort
};


int intersect2D_Segments( const Vector2d &p1, const Vector2d &p2,
			  const Vector2d &p3, const Vector2d &p4,
			  Vector2d &I0, Vector2d &I1,
			  double maxerr=0.0001);
bool IntersectXY(const Vector2d &p1, const Vector2d &p2,
		 const Vector2d &p3, const Vector2d &p4,
		 Intersection &hit, double maxoffset=0.0001);



double dist3D_Segment_to_Segment(const Vector3d &S1P0, const Vector3d &S1P1,
				 const Vector3d &S2P0, const Vector3d &S2P1, double SMALL_NUM);

vector<Vector2d> simplified(const vector<Vector2d> &vert, double epsilon);
int cleandist(vector<Vector2d> &vert, double epsilon);


Poly convexHull2D(const vector<Poly> &polygons);
int delaunayTriang(const vector<Vector2d> &points, vector<Triangle> &triangles,
		   double z);

int triangulate(const vector<Poly> &polys, vector< vector<Triangle> > &triangles,
		double z = 0);

void testangles();


Vector3d random_displaced(const Vector3d &v, double delta=0.05);
Vector2d random_displaced(const Vector2d &v, double delta=0.05);


bool shortestPath(const Vector2d &from, const Vector2d &to,
		  const vector<Poly> &polys, int excludepoly,
		  vector<Vector2d> &path, double maxerr);

vector<Poly> thick_line(const Vector2d &from, const Vector2d &to, double width);
vector<Poly> thick_lines(const vector<Vector2d> &points,  double width);
vector<Poly> dir_thick_line(const Vector2d &from, const Vector2d &to,
			    double fr_width, double to_width);


bool fit_arc(const vector<Vector2d> &points, double sq_error,
	     Vector2d &result_center, double &result_radius);

typedef struct {
  double *px, *py; // arc points
} arc_data_struct;

bool fit_arc(const int m_dat, const arc_data_struct data,
	     const int n_par, double *par, double sq_error,
	     Vector2d &result_center, double &result_radiussq);



bool rasterpolys(const vector<Poly> &polys,
		 const Vector2d &min, const Vector2d &max, double resolution,
		 Cairo::RefPtr<Cairo::ImageSurface> &surface,
		 Cairo::RefPtr<Cairo::Context> &context);

void glDrawPolySurfaceRastered(const vector<Poly> &polys,
			       const Vector2d &min, const Vector2d &max,
			       const double z,
			       const double resolution);

void glDrawCairoSurface(const Cairo::RefPtr<Cairo::ImageSurface> surface,
			const Vector2d &min, const Vector2d &max,
			const double z);

int getCairoSurfaceDatapoint(const Cairo::RefPtr<Cairo::ImageSurface> surface,
			     const Vector2d &min, const Vector2d &max,
			     const Vector2d &p);

