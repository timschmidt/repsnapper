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

void Triangle::mirrorX(Vector3d center)
{
  for (uint i = 0; i < 3; i++) 
    operator[](i).x = center.x-operator[](i).x;
  invertNormal();
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

bool Triangle::isConnectedTo(Triangle other, double maxsqerr) 
{
  // for (uint i = 0; i < 3; i++) {
  //   Vector3d p = (Vector3d)(operator[](i));
  for (uint j = 0; j < 3; j++)  {
    if (( A - other[j]).lengthSquared() < maxsqerr)  return true;
    if (( B - other[j]).lengthSquared() < maxsqerr)  return true;
    if (( C - other[j]).lengthSquared() < maxsqerr)  return true;
  }
  return false;
}


double Triangle::area() const
{
	return 0.5* ((C-A).cross(B-A)).length() ;
}

// add all these to get shape volume
double Triangle::projectedvolume(Matrix4d T) const
{
  if (Normal.z==0) return 0;
  Triangle xyproj = Triangle(Vector3d(A.x,A.y,0),
			     Vector3d(B.x,B.y,0),
			     Vector3d(C.x,C.y,0));
  Vector3d min = GetMin(T);
  Vector3d max = GetMax(T);
  double vol =  xyproj.area()*0.5*(max.z+min.z);
  if (Normal.z<0) vol=-vol; 
  return vol;
}

Vector3d Triangle::GetMax(Matrix4d T) const
{
	Vector3d max(-99999999.0, -99999999.0, -99999999.0);
	Vector3d TA=T*A,TB=T*B,TC=T*C;
	for (uint i = 0; i < 3; i++) {
		max[i] = MAX(max[i], TA[i]);
		max[i] = MAX(max[i], TB[i]);
		max[i] = MAX(max[i], TC[i]);
	}
	return max;
}

Vector3d Triangle::GetMin(Matrix4d T) const
{
	Vector3d min(99999999.0, 99999999.0, 99999999.0);
	Vector3d TA=T*A,TB=T*B,TC=T*C;
	for (uint i = 0; i < 3; i++) {
		min[i] = MIN(min[i], TA[i]);
		min[i] = MIN(min[i], TB[i]);
		min[i] = MIN(min[i], TC[i]);
	}
	return min;
}

void Triangle::AccumulateMinMax(Vector3d &min, Vector3d &max, Matrix4d T)
{
	Vector3d tmin = GetMin(T);
	Vector3d tmax = GetMax(T);
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

void Triangle::rotate(const Vector3d axis, double angle) 
{
  //Normal = triangles[i].Normal.rotate(angle, axis.x, axis.y, axis.z);
  A = A.rotate(angle, axis.x, axis.y, axis.z);
  B = B.rotate(angle, axis.x, axis.y, axis.z);
  C = C.rotate(angle, axis.x, axis.y, axis.z);
  calcNormal();
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


string Triangle::getSTLfacet(Matrix4d T) const
{
  Vector3d TA=T*A,TB=T*B,TC=T*C,TN=T*Normal;TN.normalize();
  stringstream sstr;
  sstr << "  facet normal " << TN.x << " " << TN.x << " " << TN.z <<endl;
  sstr << "    outer loop"  << endl;
  sstr << "      vertex "<< scientific << TA.x << " " << TA.y << " " << TA.z <<endl;
  sstr << "      vertex "<< scientific << TB.x << " " << TB.y << " " << TB.z <<endl;
  sstr << "      vertex "<< scientific << TC.x << " " << TC.y << " " << TC.z <<endl;
  sstr << "    endloop" << endl;
  sstr << "  endfacet" << endl;
  return sstr.str();
}
