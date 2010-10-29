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

#include "GeometryBase.h"

namespace PolyLib
{
	bool IsAngleInBetween(double a12, double a32, double aP2)
	{
		if( aP2 < a12 )
		{
			if( a32 > a12 ) return false;
			return aP2 < a32; 
		}
		else
		{
			if( a32 < a12 ) return true;
			return aP2 < a32; 
		}
	}

	float CalcAngleBetween(Vector2f V1, Vector2f V2)
	{
		float dotproduct, lengtha, lengthb, result;

		dotproduct = (V1.x * V2.x) + (V1.y * V2.y);
		lengtha = sqrt(V1.x * V1.x + V1.y * V1.y);
		lengthb = sqrt(V2.x * V2.x + V2.y * V2.y);

		result = acos( dotproduct / (lengtha * lengthb) );
		if(result > 0)
			result += PI;
		else
			result -= PI;

		return result;
	}

	// Following function licensed as:
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
	int inSegment( const Vector2f &P, const Vector2f &p1, const Vector2f &p2)
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

	// Following function licensed as:
	// Copyright 2001, softSurfer (www.softsurfer.com)
	// This code may be freely used and modified for any purpose
	// providing that this copyright notice is included with it.
	// SoftSurfer makes no warranty for this code, and cannot be held
	// liable for any real or imagined damage resulting from its use.
	// Users of this code must verify correctness for their application.
	//
	// intersect2D_2Segments(): the intersection of 2 finite 2D segments
	//    Input:  two finite segments S1 and S2
	//    Output: *I0 = intersect point (when it exists)
	//            *I1 = endpoint of intersect segment [I0,I1] (when it exists)
	//    Return: 0=disjoint (no intersect)
	//            1=intersect in unique point I0
	//            2=overlap in segment from I0 to I1
	int intersect2D_Segments( const Vector2f &p1, const Vector2f &p2, const Vector2f &p3, const Vector2f &p4, Vector2f &I0, Vector2f &I1, float &t0, float &t1 )
	{
		Vector2f    u = p2 - p1;
		Vector2f    v = p4 - p3;
		Vector2f    w = p1 - p3;
		float    D = perp(u,v);

		// test if they are parallel (includes either being a point)
		if (fabs(D) < SMALL_NUM) {          // S1 and S2 are parallel
			if (perp(u,w) != 0 || perp(v,w) != 0) {
				return 0;                   // they are NOT collinear
			}
			// they are collinear or degenerate
			// check if they are degenerate points
			float du = dot(u,u);
			float dv = dot(v,v);
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
			//float t0, t1;                   // endpoints of S1 in eqn for S2
			Vector2f w2 = p2 - p3;
			if (v.x != 0) {
				t0 = w.x / v.x;
				t1 = w2.x / v.x;
			}
			else {
				t0 = w.y / v.y;
				t1 = w2.y / v.y;
			}
			if (t0 > t1) {                  // must have t0 smaller than t1
				float t=t0; t0=t1; t1=t;    // swap if not
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
		}

		// the segments are skew and may intersect in a point
		// get the intersect parameter for S1
		t0 = perp(v,w) / D;
		if (t0 < 0 || t0 > 1)               // no intersect with S1
			return 0;

		// get the intersect parameter for S2
		t1 = perp(u,w) / D;
		if (t1 < 0 || t1 > 1)               // no intersect with S2
			return 0;

		I0 = p1 + u * t0;               // compute S1 intersect point
		return 1;
	}

    //Compute the distance from segment(l1, l2) to p1
    float linePointDist2D_Segments2(const Vector2f &l1, const Vector2f &l2, const Vector2f &p1)
	{
		float d = dot(l2-l1, p1-l2);
		if( d > 0) return (p1-l2).lengthSquared();

		d = dot(l1-l2, p1-l1);
		if( d > 0) return (p1-l1).lengthSquared();

		return sqr((l2-l1).cross(p1-l1))/(l2-l1).lengthSquared();
    }
}
