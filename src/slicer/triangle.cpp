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
#include "slicer.h"

Vector3f &Triangle::operator[] (const int index)
{
    switch(index) {
        case 0: return A;
        case 1: return B;
        case 2: return C;
    }
    return A;
}

float Triangle::area()
{
	return ( ((C-A).cross(B-A)).length() );
}

Vector3f Triangle::GetMax()
{
	Vector3f max(-99999999.0f, -99999999.0f, -99999999.0f);
	for (uint i = 0; i < 3; i++) {
		max[i] = MAX(max[i], A[i]);
		max[i] = MAX(max[i], B[i]);
		max[i] = MAX(max[i], C[i]);
	}
	return max;
}

Vector3f Triangle::GetMin()
{
	Vector3f min(99999999.0f, 99999999.0f, 99999999.0f);
	for (uint i = 0; i < 3; i++) {
		min[i] = MIN(min[i], A[i]);
		min[i] = MIN(min[i], B[i]);
		min[i] = MIN(min[i], C[i]);
	}
	return min;
}

void Triangle::AccumulateMinMax(Vector3f &min, Vector3f &max)
{
	Vector3f tmin = GetMin();
	Vector3f tmax = GetMax();
	for (uint i = 0; i < 3; i++) {
		min[i] = MIN(tmin[i], min[i]);
		max[i] = MAX(tmax[i], max[i]);
	}
}

void Triangle::Translate(const Vector3f &vector)
{
	A += vector;
	B += vector;
	C += vector;
}


int Triangle::CutWithPlane(float z, const Matrix4f &T, 
			   Vector2f &lineStart,
			   Vector2f &lineEnd)
{

	Vector3f p;
	float t;

	Vector3f P1 = T*this->A;
	Vector3f P2 = T*this->B;

	int num_cutpoints = 0;
	// Are the points on opposite sides of the plane ?
	if ((z <= P1.z) != (z <= P2.z))
	  {
	    t = (z-P1.z)/(float)(P2.z-P1.z);
	    p = P1+((Vector3f)(P2-P1)*t);
	    lineStart = Vector2f(p.x,p.y);
	    num_cutpoints = 1;
	  }
	
	P1 = T*this->B;
	P2 = T*this->C;
	if ((z <= P1.z) != (z <= P2.z))
	  {
	    t = (z-P1.z)/(float)(P2.z-P1.z);
	    p = P1+((Vector3f)(P2-P1)*t);
	    if(num_cutpoints > 0)
	      {
		lineEnd = Vector2f(p.x,p.y);
		num_cutpoints = 2;
	      }
	    else
	      {
		lineStart = Vector2f(p.x,p.y);
		num_cutpoints = 1;
	      }
	  }
	P1 = T*this->C;
	P2 = T*this->A;
	if ((z <= P1.z) != (z <= P2.z))
	  {
	    t = (z-P1.z)/(float)(P2.z-P1.z);
	    p = P1+((Vector3f)(P2-P1)*t);
	    
	    lineEnd = Vector2f(p.x,p.y);
	    if( lineEnd != lineStart ) num_cutpoints = 2;
	  }
	
	return num_cutpoints;
}
