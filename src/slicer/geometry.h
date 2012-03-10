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


using namespace std;
using namespace vmml;


/* template < typename T >  */
/* long double angleBetween(T V1, T V2); */
long double angleBetween(Vector3d V1, Vector3d V2);
long double angleBetween(Vector2d V1, Vector2d V2);
void center_perpendicular(const Vector2d from, const Vector2d to,
			  Vector2d &p1, Vector2d &p2);

bool isleftof(Vector2d center, Vector2d A, Vector2d B);
bool isleftof(Vector3d center, Vector3d A, Vector3d B);

Vector3d cross2d(Vector2d A, Vector2d B, double z=0);

double minimum_distance_Sq(const Vector2d s1, const Vector2d s2, 
			   const Vector2d p, Vector2d &onseg);


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



double dist3D_Segment_to_Segment(Vector3d S1P0, Vector3d S1P1, 
				 Vector3d S2P0, Vector3d S2P1, double SMALL_NUM);


void testangles();


Vector3d random_displace(Vector3d v, double delta=0.05);
Vector2d random_displace(Vector2d v, double delta=0.05);
