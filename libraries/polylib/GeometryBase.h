/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010 Logick

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

#include <vmmlib/vmmlib.h>

using namespace vmml;

namespace PolyLib
{
	#define PI 3.141592653589793238462643383279502884197169399375105820974944592308
#ifndef sqr
	#define sqr(x)				((x)*(x))
#endif
#ifndef MIN
	#define MIN(A,B)			((A)<(B)? (A):(B))
#endif
#ifndef MAX
	#define MAX(A,B)			((A)>(B)? (A):(B))
#endif
#ifndef ABS
	#define ABS(a)				(((a) < 0) ? -(a) : (a))
#endif

	// dot product (2D) which allows vector operations in arguments
	#define dot(u,v)   ((u).x * (v).x + (u).y * (v).y)
	#define perp(u,v)  ((u).x * (v).y - (u).y * (v).x)  // perp product (2D)

	#define SMALL_NUM  0.00000001 // anything that avoids division overflow

	bool IsAngleInBetween(double a12, double a32, double aP2);
	double CalcAngleBetween(Vector2d V1, Vector2d V2);
	int intersect2D_Segments( const Vector2d &p1, const Vector2d &p2, const Vector2d &p3, const Vector2d &p4, Vector2d &I0, Vector2d &I1, double &t0, double &t1 );
	double linePointDist2D_Segments2(const Vector2d &l1, const Vector2d &l2, const Vector2d &p1);

}
