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
#include "triangle.h"



Triangle::Triangle(const Vector3d &Point1,
		   const Vector3d &Point2, const Vector3d &Point3)
{ A=Point1;B=Point2;C=Point3;
  calcNormal();
}


void Triangle::calcNormal()
{
  Vector3d AA=C-A;
  Vector3d BB=C-B;
  Normal = AA.cross(BB).getNormalized();
}


void Triangle::invertNormal()
{
  Vector3d swap = A;
  A=C; C=swap;
  calcNormal();
}

Vector3d &Triangle::operator[] (const int index)
{
    switch(index) {
        case 0: return A;
        case 1: return B;
        case 2: return C;
    }
    return A;
}



double Triangle::area()
{
	return ( ((C-A).cross(B-A)).length() );
}

Vector3d Triangle::GetMax()
{
	Vector3d max(-99999999.0, -99999999.0, -99999999.0);
	for (uint i = 0; i < 3; i++) {
		max[i] = MAX(max[i], A[i]);
		max[i] = MAX(max[i], B[i]);
		max[i] = MAX(max[i], C[i]);
	}
	return max;
}

Vector3d Triangle::GetMin()
{
	Vector3d min(99999999.0, 99999999.0, 99999999.0);
	for (uint i = 0; i < 3; i++) {
		min[i] = MIN(min[i], A[i]);
		min[i] = MIN(min[i], B[i]);
		min[i] = MIN(min[i], C[i]);
	}
	return min;
}

void Triangle::AccumulateMinMax(Vector3d &min, Vector3d &max)
{
	Vector3d tmin = GetMin();
	Vector3d tmax = GetMax();
	for (uint i = 0; i < 3; i++) {
		min[i] = MIN(tmin[i], min[i]);
		max[i] = MAX(tmax[i], max[i]);
	}
}

void Triangle::Translate(const Vector3d &vector)
{
	A += vector;
	B += vector;
	C += vector;
}


int Triangle::CutWithPlane(double z, const Matrix4d &T, 
			   Vector2d &lineStart,
			   Vector2d &lineEnd) const
{
	Vector3d p;
	double t;

	Vector3d P1 = T*this->A;
	Vector3d P2 = T*this->B;

	int num_cutpoints = 0;
	// Are the points on opposite sides of the plane?
	if ((z <= P1.z) != (z <= P2.z))
	  {
	    t = (z-P1.z)/(P2.z-P1.z);
	    p = P1+((Vector3d)(P2-P1)*t);
	    lineStart = Vector2d(p.x,p.y);
	    num_cutpoints = 1;
	  }
	
	P1 = T*this->B;
	P2 = T*this->C;
	if ((z <= P1.z) != (z <= P2.z))
	  {
	    t = (z-P1.z)/(P2.z-P1.z);
	    p = P1+((Vector3d)(P2-P1)*t);
	    if(num_cutpoints > 0)
	      {
		lineEnd = Vector2d(p.x,p.y);
		num_cutpoints = 2;
	      }
	    else
	      {
		lineStart = Vector2d(p.x,p.y);
		num_cutpoints = 1;
	      }
	  }
	P1 = T*this->C;
	P2 = T*this->A;
	if ((z <= P1.z) != (z <= P2.z))
	  {
	    t = (z-P1.z)/(P2.z-P1.z);
	    p = P1+((Vector3d)(P2-P1)*t);	    
	    lineEnd = Vector2d(p.x,p.y);
	    if( lineEnd != lineStart ) num_cutpoints = 2;
	  }
	
	return num_cutpoints;
}


string Triangle::getSTLfacet() const
{
  stringstream sstr;
  sstr << "  facet normal " << Normal.x << " " << Normal.x << " " << Normal.z <<endl;
  sstr << "    outer loop"  << endl;
  sstr << "      vertex "<< scientific << A.x << " " << A.y << " " << A.z <<endl;
  sstr << "      vertex "<< scientific << B.x << " " << B.y << " " << B.z <<endl;
  sstr << "      vertex "<< scientific << C.x << " " << C.y << " " << C.z <<endl;
  sstr << "    endloop" << endl;
  sstr << "  endfacet" << endl;
  return sstr.str();
}
