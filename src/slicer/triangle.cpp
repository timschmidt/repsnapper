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
