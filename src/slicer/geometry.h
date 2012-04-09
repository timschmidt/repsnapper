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

class Transform3D
{
public:
	Transform3D(){identity();}
	void identity(){transform=Matrix4d::IDENTITY;}
	Matrix4d transform;
	Matrix4f getFloatTransform() const;
	Vector3d getTranslation() const;
	void setTransform(Matrix4f matrf); 
	void scale(double x);
	void scale_x(double x);
	void scale_y(double x);
	void scale_z(double x);
	void move(Vector3d delta);
	void rotate(Vector3d center, double x, double y, double z);
	void rotate(Vector3d axis, double angle);
};


void move(const Vector3f &delta, Matrix4f &mat);
Vector3d normalized(const Vector3d &v);
Vector2d normalized(const Vector2d &v);
Vector2d normalV(const Vector2d &a);
double cross(const Vector2d &a, const Vector2d &b);
void moveArcballTrans(Matrix4fT &matfT, const Vector3d &delta);
void setArcballTrans(Matrix4fT &matfT, const Vector3d &trans);
void rotArcballTrans(Matrix4fT &transform,  const Vector3d &axis, double angle);

/* template< size_t M, typename T > */
/* long double angleBetween(const vmml::vector< M, T > V1, const vmml::vector< M, T > V2 ); */
long double angleBetween(const Vector3d &V1, const Vector3d &V2);
long double angleBetween(const Vector2d &V1, const Vector2d &V2);
void center_perpendicular(const Vector2d &from, const Vector2d &to,
			  Vector2d &p1, Vector2d &p2);

bool isleftof(const Vector2d &center, const Vector2d &A, const Vector2d &B);
bool isleftof(const Vector3d &center, const Vector3d &A, const Vector3d &B);

Vector3d cross2d(const Vector2d &A, const Vector2d &B, double z=0);

double point_segment_distance_Sq(const Vector2d &s1, const Vector2d &s2, 
				 const Vector2d &p, Vector2d &onseg);

Vector2d rotated(const Vector2d &p, const Vector2d &center, double angle, bool ccw=true);

struct printline;

struct Intersection
{
	Vector2d p;  // The intersection point
	double d;     // Distance from the start point, used for sorting hits
	double t;     // intersection point on first line
  bool operator <(const Intersection &p) const { return (d<p.d); } // for std::sort
};


int intersect2D_Segments( const Vector2d &p1, const Vector2d &p2, 
			  const Vector2d &p3, const Vector2d &p4, 
			  Vector2d &I0, Vector2d &I1, 
			  double &t0, double &t1,
			  double maxerr=0.0001);
bool IntersectXY(const Vector2d &p1, const Vector2d &p2,
		 const Vector2d &p3, const Vector2d &p4, 
		 Intersection &hit, double maxoffset=0.0001);



double dist3D_Segment_to_Segment(const Vector3d &S1P0, const Vector3d &S1P1, 
				 const Vector3d &S2P0, const Vector3d &S2P1, double SMALL_NUM);


Poly convexHull2D(const vector<Poly> &polygons);


void testangles();


Vector3d random_displaced(const Vector3d &v, double delta=0.05);
Vector2d random_displaced(const Vector2d &v, double delta=0.05);


bool shortestPath(const Vector2d &from, const Vector2d &to, 
		  const vector<Poly> &polys, int excludepoly, 
		  vector<Vector2d> &path, double maxerr);

vector<Poly> thick_line(const Vector2d &from, const Vector2d &to, double width);
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
