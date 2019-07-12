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
#include "slicer/geometry.h"


Triangle::Triangle(const Vector3d &Point1,
           const Vector3d &Point2, const Vector3d &Point3)
{ A=Point1;B=Point2;C=Point3;
  calcNormal();
}

void Triangle::calcNormal()
{
  Vector3d AA=C-A;
  Vector3d BB=C-B;
  Normal = normalized(AA.cross(BB));
}

Triangle Triangle::transformed(const Matrix4d &T) const
{
  return Triangle(T*A,T*B,T*C);
}


void Triangle::invertNormal()
{
  Vector3d swap = A;
  A=C; C=swap;
  calcNormal();
}

void Triangle::mirrorX(const Vector3d &center)
{
  for (uint i = 0; i < 3; i++)
    operator[](i).x() = center.x()-operator[](i).x();
  invertNormal();
}

Vector3d &Triangle::operator[] (const uint index)
{
    switch(index) {
        case 0: return A;
        case 1: return B;
        case 2: return C;
    }
    return A;
}
Vector3d const &Triangle::operator[] (const uint index) const
{
    switch(index) {
        case 0: return A;
        case 1: return B;
        case 2: return C;
    }
    return A;
}

// for 2 adjacent triangles test if normals match
bool Triangle::wrongOrientationWith(Triangle const &other, double maxsqerr) const
{
  // find the 2 common vertices
  vector<int> thisv, otherv;
  for (int j = 0; j < 3; j++)  {
    if ( A == other[j] || A.squared_distance(other[j]) < maxsqerr){
      thisv.push_back(0);  otherv.push_back(j);
    }
    if ( B == other[j] || B.squared_distance(other[j]) < maxsqerr) {
      thisv.push_back(1);  otherv.push_back(j);
    }
    if ( C == other[j] || C.squared_distance(other[j]) < maxsqerr) {
      thisv.push_back(2);  otherv.push_back(j);
    }
  }
  if (thisv.size()!=2 || otherv.size()!=2) {
    //cerr << "only " << thisv.size() << " common vertex! " << endl;
    return false; // "ok" because non-adjacent
  }

  int diff = thisv[1]  - thisv[0];
  const bool thisorient =  ( diff == 1 || diff == -2 );
  diff = otherv[1] - otherv[0];
  const bool otherorient = ( diff == 1 || diff == -2 );
  // cerr << "have 2: " << thisorient <<" / " << otherorient << endl;
  return (thisorient == otherorient); // they have different(!) orientation
}

bool Triangle::isConnectedTo(Triangle const &other, double maxsqerr) const
{
  // for (uint i = 0; i < 3; i++) {
  //   Vector3d p = (Vector3d)(operator[](i));
  // first test equal, faster
  for (uint j = 0; j < 3; j++)  {
    if (( A == other[j]))  return true;
    if (( B == other[j]))  return true;
    if (( C == other[j]))  return true;
  }
  if (maxsqerr>0)
    // if not, test distance
    for (uint j = 0; j < 3; j++)  {
      if ( A.squared_distance(other[j]) < maxsqerr)  return true;
      if ( B.squared_distance(other[j]) < maxsqerr)  return true;
      if ( C.squared_distance(other[j]) < maxsqerr)  return true;
    }
  return false;
}

double Triangle::slopeAngle(const Matrix4d &T) const
{
  const double scale = T(3,3);
  Vector3d trans;
  T.get_translation(trans);
  // get scaled translation out of matrix
  const Vector3d n = T * Normal - trans/scale;
  return asin(n.z()/n.length());
}


double Triangle::area() const
{
    return 0.5* ((C-A).cross(B-A)).length() ;
}

// add all these to get shape volume
double Triangle::projectedvolume(const Matrix4d &T) const
{
  if (Normal.z()==0.) return 0;
  Triangle xyproj = Triangle(Vector3d(A.x(),A.y(),0),
                 Vector3d(B.x(),B.y(),0),
                 Vector3d(C.x(),C.y(),0));
  Vector3d min = GetMin(T);
  Vector3d max = GetMax(T);
  double vol =  xyproj.area()*0.5*(max.z()+min.z());
  if (Normal.z()<0) vol=-vol;
  return vol;
}

Vector3d Triangle::GetMax(const Matrix4d &T) const
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

Vector3d Triangle::GetMin(const Matrix4d &T) const
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

void Triangle::AccumulateMinMax(Vector3d &min, Vector3d &max, const Matrix4d &T)
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

void Triangle::rotate(const Vector3d &axis, double angle)
{
  A = A.rotate(angle, axis);
  B = B.rotate(angle, axis);
  C = C.rotate(angle, axis);
  calcNormal();
}

void triangulateQuadrilateral(vector<Vector3d> fourpoints, vector<Triangle> &triangles)
{
  assert(fourpoints.size()==4);
  vector<Triangle> tr(2);
  double SMALL = 0.01;
  // find diagonals
  double dist = dist3D_Segment_to_Segment(fourpoints[0],fourpoints[2],
                      fourpoints[1],fourpoints[3],
                      SMALL*SMALL);
  if (dist < SMALL)
    { // found -> divide at shorter diagonal
      if ((fourpoints[0]-fourpoints[2]).squared_length()
      < (fourpoints[1]-fourpoints[3]).squared_length()) {
      tr[0] = Triangle(fourpoints[0],fourpoints[1],fourpoints[2]);
      tr[1] = Triangle(fourpoints[2],fourpoints[3],fourpoints[0]);
      } else {
    tr[0] = Triangle(fourpoints[0],fourpoints[1],fourpoints[3]);
    tr[1] = Triangle(fourpoints[1],fourpoints[2],fourpoints[3]);
      }
    }
  else
    { // take other 2
      double dist = dist3D_Segment_to_Segment(fourpoints[1],fourpoints[2],
                          fourpoints[0],fourpoints[3],
                          SMALL*SMALL);
      if (dist < SMALL)
    {
      if ((fourpoints[1]-fourpoints[2]).squared_length()
          < (fourpoints[0]-fourpoints[3]).squared_length()) {
        tr[0] = Triangle(fourpoints[1],fourpoints[2],fourpoints[3]);
        tr[1] = Triangle(fourpoints[0],fourpoints[1],fourpoints[2]);
      } else {
        tr[0] = Triangle(fourpoints[1],fourpoints[0],fourpoints[3]);
        tr[1] = Triangle(fourpoints[0],fourpoints[2],fourpoints[3]);
      }
    }
      else
    { // take 3rd possibility, not the case here, because 2-3 is cut line
      double dist = dist3D_Segment_to_Segment(fourpoints[0],fourpoints[1],
                          fourpoints[2],fourpoints[3],
                          SMALL*SMALL);
      if (dist < SMALL)
        {
          tr[0] = Triangle(fourpoints[0],fourpoints[2],fourpoints[3]);
          tr[1] = Triangle(fourpoints[2],fourpoints[1],fourpoints[3]);
        }
      else  {
        //cerr << dist << " cannot find diagonals" << endl;
        return;
      }
    }
    }
  triangles.insert(triangles.end(), tr.begin(), tr.end());
}

int Triangle::SplitAtPlane(double z,
               vector<Triangle> &uppertriangles,
               vector<Triangle> &lowertriangles,
               const Matrix4d &T) const
{
  vector<Vector3d> upper, lower;
  if  ((T*A).z()>z) upper.push_back(T*A); else lower.push_back(T*A);
  if  ((T*B).z()>z) upper.push_back(T*B); else lower.push_back(T*B);
  if  ((T*C).z()>z) upper.push_back(T*C); else lower.push_back(T*C);
  Vector2d lstart,lend;
  int cut = CutWithPlane(z,T,lstart,lend);
  if (cut==0) return 0;
  else if (cut==1) { // cut at a triangle point
    if (upper.size()>lower.size())
      upper.push_back(Vector3d(lstart.x(),lstart.y(),z));
    else
      lower.push_back(Vector3d(lstart.x(),lstart.y(),z));
  }
  else if (cut==2) {
    upper.push_back(Vector3d(lstart.x(),lstart.y(),z));
    upper.push_back(Vector3d(lend.x(),lend.y(),z));
    lower.push_back(Vector3d(lstart.x(),lstart.y(),z));
    lower.push_back(Vector3d(lend.x(),lend.y(),z));
  }
  vector<Triangle> uppertr,lowertr;
  if (upper.size()==3) { // half of triangle with 1 triangle point
    uppertr.push_back(Triangle(upper[0],upper[1],upper[2]));
  }
  else if (upper.size()==4) { // 0 and 1 are triangle points, 2 and 3 are cutline
    triangulateQuadrilateral(upper, uppertr);
  }
  else cerr << "upper size " << upper.size() << endl;
  if (lower.size()==3) {
    lowertr.push_back(Triangle(lower[0],lower[1],lower[2]));
  }
  else if (lower.size()==4) {
    triangulateQuadrilateral(lower, lowertr);
  }
  else cerr << "lower size " << lower.size() << endl;
  Vector3d TN = T*Normal; TN.normalize();
  for (guint i=0; i < uppertr.size(); i++)
    if ((uppertr[i].Normal + TN).length()<0.1) uppertr[i].invertNormal();
  for (guint i=0; i < lowertr.size(); i++)
    if ((lowertr[i].Normal + TN).length()<0.1) lowertr[i].invertNormal();
  uppertriangles.insert(uppertriangles.end(),uppertr.begin(),uppertr.end());
  lowertriangles.insert(lowertriangles.end(),lowertr.begin(),lowertr.end());
  return cut;
}

bool Triangle::isInZrange(double zmin, double zmax, const Matrix4d &T) const
{
  const Vector3d TA = T * A;
  if (TA.z() < zmin || TA.z() > zmax) return false;
  const Vector3d TB = T * B;
  if (TB.z() < zmin || TB.z() > zmax) return false;
  const Vector3d TC = T * C;
  if (TC.z() < zmin || TC.z() > zmax) return false;
  return true;
}

int Triangle::CutWithPlane(double z, const Matrix4d &T,
               Vector2d &lineStart,
               Vector2d &lineEnd) const
{
    Vector3d p;
    double t;

    const Vector3d TA = T * A;
    const Vector3d TB = T * B;
    const Vector3d TC = T * C;

    int num_cutpoints = 0;
    // Are the points on opposite sides of the plane?
    if ((z <= TA.z()) != (z <= TB.z()))
      {
        t = (z - TA.z())/(TB.z()-TA.z());
        p = TA + (TB - TA) * t;
        lineStart = Vector2d(p.x(),p.y());
        num_cutpoints = 1;
      }
    if ((z <= TB.z()) != (z <= TC.z()))
      {
        t = (z - TB.z())/(TC.z() - TB.z());
        p = TB + (TC - TB) * t;
        if(num_cutpoints > 0)
          {
        lineEnd = Vector2d(p.x(),p.y());
        num_cutpoints = 2;
          }
        else
          {
        lineStart = Vector2d(p.x(),p.y());
        num_cutpoints = 1;
          }
      }
    if ((z <= TC.z()) != (z <= TA.z()))
      {
        t = (z - TC.z())/(TA.z() - TC.z());
        p = TC + (TA - TC) * t;
        lineEnd = Vector2d(p.x(),p.y());
        if( lineEnd != lineStart ) num_cutpoints = 2;
      }

    return num_cutpoints;
}

void Triangle::draw(GLenum gl_type) const
{
  glBegin(gl_type);
  glVertex3dv(A);
  glVertex3dv(B);
  glVertex3dv(C);
  glEnd();
}


string Triangle::getSTLfacet(const Matrix4d &T) const
{
  Vector3d TA=T*A,TB=T*B,TC=T*C,TN=T*Normal;TN.normalize();
  stringstream sstr;
  sstr << "  facet normal " << TN.x() << " " << TN.x() << " " << TN.z() <<endl;
  sstr << "    outer loop"  << endl;
  sstr << "      vertex "<< scientific << TA.x() << " " << TA.y() << " " << TA.z() <<endl;
  sstr << "      vertex "<< scientific << TB.x() << " " << TB.y() << " " << TB.z() <<endl;
  sstr << "      vertex "<< scientific << TC.x() << " " << TC.y() << " " << TC.z() <<endl;
  sstr << "    endloop" << endl;
  sstr << "  endfacet" << endl;
  return sstr.str();
}

string Triangle::info() const
{
  ostringstream ostr;
  ostr <<"Triangle A="<< A
       <<", B="<< B
       <<", C="<< C
       <<", N="<< Normal ;
  return ostr.str();
}
