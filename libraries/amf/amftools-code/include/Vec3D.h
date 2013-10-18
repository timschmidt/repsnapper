/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef _VEC3D_H
#define _VEC3D_H

//Possible Linux portability issues: min, max

#define __isnand(x) std::isnan(x)


#include <cmath>
#include <float.h>

class CQuat;
class Vec3D {
public:
	//Constructors
	Vec3D() {x = 0; y = 0; z = 0;}
	//Vec3D(const Vec3D& s) {x = s.x; y = s.y; z = s.z;}
	Vec3D(const Vec3D& Vec) {*this = Vec;} //copy constructor

	Vec3D(double dx, double dy, double dz) {x = dx; y = dy; z = dz;}
	inline Vec3D& operator=(const Vec3D& s) {x = s.x; y = s.y; z = s.z; return *this; }; //overload equals


#ifdef WIN32
	bool IsValid() const {return !_isnan(x) && _finite(x) && !_isnan(y) && _finite(y) && !_isnan(z) && _finite(z);} //is a valid vector? (funky windows _ versions...)
#else
	bool IsValid() const {return !__isnand(x) && finite(x) && !__isnand(y) && finite(y) && !__isnand(z) && finite(z);} //is a valid vector?
#endif

	bool IsNear(Vec3D& s, double thresh) {return Dist2(s)<thresh*thresh;}

	//Attributes
	double x, y, z;
	inline double X(void){return x;};
	inline double Y(void){return y;};
	inline double Z(void){return z;};
	inline void setX(double XIn) {x = XIn;};
	inline void setY(double YIn) {y = YIn;};
	inline void setZ(double ZIn) {z = ZIn;};


	//these should all be friends....
	//Operators
	friend Vec3D operator+(const Vec3D& v1, const Vec3D& v2) {return Vec3D(v1.x+v2.x, v1.y+v2.y, v1.z+v2.z);} //Plus
	friend Vec3D operator-(const Vec3D& v1, const Vec3D& v2) {return Vec3D(v1.x-v2.x, v1.y-v2.y, v1.z-v2.z);} //Minus
	friend Vec3D operator*(const Vec3D& v, const double f) {return Vec3D(v.x*f, v.y*f, v.z*f);} //Multiply
	friend Vec3D operator*(const double f, const Vec3D& v) {return v*f;}
	friend Vec3D operator/(const Vec3D& v, const double f) {return Vec3D(v.x/f, v.y/f, v.z/f);} //Divide
	friend Vec3D operator-(const Vec3D& v) {return Vec3D(-v.x, -v.y, -v.z);} //Negative

	friend bool operator==(const Vec3D& v1, const Vec3D& v2) {return (v1.x==v2.x && v1.y==v2.y && v1.z==v2.z);} //Is equal
	friend bool operator!=(const Vec3D& v1, const Vec3D& v2) {return !(v1==v2);} //Is not equal
	Vec3D& operator+=(const Vec3D& s) {x += s.x; y += s.y; z += s.z; return *this;} //add and set
	Vec3D& operator-=(const Vec3D& s) {x -= s.x; y -= s.y; z -= s.z; return *this;} //subract and set
	Vec3D& operator*=(const double f) {x *= f; y *= f; z *= f; return *this;} //multiply and set
	Vec3D& operator/=(const double f) {x /= f; y /= f; z /= f; return *this;} //subtract and set
	
	//Vector operations (change this object)
	inline double Normalize() {double l = sqrt(x*x+y*y+z*z); if (l > 0) {x /= l;y /= l;z /= l;} return l;} //normalizes this vector
	inline Vec3D Rot(Vec3D u, double a) {double ca = cos(a); double sa = sin(a); double t = 1-ca; return Vec3D((u.x*u.x*t + ca) * x + (u.x*u.y*t - u.z*sa) * y + (u.z*u.x*t + u.y*sa) * z, (u.x*u.y*t + u.z*sa) * x + (u.y*u.y*t+ca) * y + (u.y*u.z*t - u.x*sa) * z, (u.z*u.x*t - u.y*sa) * x + (u.y*u.z*t + u.x*sa) * y + (u.z*u.z*t + ca) * z);} //rotates by arbitrary vector arbitrary amount (http://www.cprogramming.com/tutorial/3d/rotation.html (Errors! LH one is actually RH one))
	inline Vec3D Rot(CQuat& Q); //below CQuat for linking sake...

	void RotZ(double a) {double xt =  x*cos(a) - y*sin(a); double yt = x*sin(a) + y*cos(a); x = xt; y = yt;} //rotates about Z axis "a" radians
	void RotY(double a) {double xt =  x*cos(a) + z*sin(a); double zt = -x*sin(a) + z*cos(a); x = xt; z = zt;} //rotates about Y axis "a" radians
	void RotX(double a) {double yt =  y*cos(a) + z*sin(a); double zt = -y*sin(a) + z*cos(a); y = yt; z = zt;} //rotates about X axis "a" radians

	//Vector operations (don't change this object!)
	inline Vec3D Cross(Vec3D& v) {return Vec3D(y*v.z-z*v.y,z*v.x-x*v.z,x*v.y-y*v.x);} //Cross product
	inline double Dot(Vec3D& v) {return (x * v.x + y * v.y + z * v.z);} //Dot product
	inline Vec3D Abs() {return Vec3D(fabs(x),fabs(y),fabs(z));} //Absolute value
	inline Vec3D Normalized() {	double l = sqrt(x*x+y*y+z*z); return (l>0?(*this)/l:(*this));} //returns normalized vec
	inline Vec3D ProjXY() {	return Vec3D(x,y,0);} //projection into the xy plane
	inline double Length() {return sqrt(x*x+y*y+z*z);} //length of the vector
	inline double Length2() {return (x*x+y*y+z*z);} //length squared of the vector
	inline Vec3D Min(const Vec3D& s) {return Vec3D(x<s.x ? x:s.x, y<s.y ? y:s.y, z<s.z ? z:s.z);} //min vector of the two
	inline Vec3D Max(const Vec3D& s) {return Vec3D(x>s.x ? x:s.x, y>s.y ? y:s.y, z>s.z ? z:s.z);} //max vector of the two
	inline double Min() {double Min1 = (x<y ? x:y); return (z<Min1 ? z:Min1);} //minimum element of this vector
	inline double Max() {double Max1 = (x>y ? x:y); return (z>Max1 ? z:Max1);} //max element of this vector
	inline Vec3D Scale(const Vec3D& v) {return Vec3D(x*v.x, y*v.y, z*v.z);} //scales by another vector
	inline Vec3D ScaleInv(const Vec3D& v) {return Vec3D(x/v.x, y/v.y, z/v.z);} //scales by inverse of another vector
	inline double Dist(const Vec3D& v) {return sqrt(Dist2(v));} //distance from another vector
	inline double Dist2(const Vec3D& v) {return (v.x-x)*(v.x-x)+(v.y-y)*(v.y-y)+(v.z-z)*(v.z-z);} //distance from another vector
	inline double AlignWith(Vec3D target, Vec3D& rotax) {Vec3D thisvec = Normalized(); Vec3D targvec = target.Normalized(); Vec3D rotaxis = thisvec.Cross(targvec); if (rotaxis.Length2() == 0) {rotaxis=target.ArbitraryNormal();} rotax = rotaxis.Normalized(); return acos(thisvec.Dot(targvec));} //returns vector (rotax) and amount (return val) to align this vector with target vector
	inline Vec3D ArbitraryNormal() {Vec3D n = Normalized(); if (fabs(n.x) <= fabs(n.y) && fabs(n.x) <= fabs(n.z)){n.x = 1;} else if (fabs(n.y) <= fabs(n.x) && fabs(n.y) <= fabs(n.z)){n.y = 1;}	else {n.z = 1;}	return Cross(n).Normalized();} //generate arbitrary normal
};


class CQuat // : public Vec3D //extending Vec3D saves us reimplementing some stuff, I think? this is not a comprehensive quaternion class at this point...
{
public:
	CQuat(void) {Clear();}
	~CQuat(void){};
	
	CQuat(double dw, double dx, double dy, double dz) {w=dw; x=dx; y=dy; z=dz;} //constructor
	CQuat(const CQuat& Quat) {*this = Quat;} //copy constructor
	CQuat(const Vec3D& VecIn) {w = 0; x = VecIn.x; y = VecIn.y; z = VecIn.z;}
	CQuat(double angle, const Vec3D &axis){Clear(); const double a = angle * 0.5f; const double s = sin(a); const double c = cos(a); w = c; x = axis.x * s; y = axis.y * s; z = axis.z * s;};
	inline CQuat& operator=(const CQuat& s) {w = s.w; x = s.x; y = s.y; z = s.z; return *this; }; //overload equals
	inline void Clear() {w=1; x=0; y=0; z=0;} // for (int i=0; i<3; i++){for (int j=0; j<3; j++){M[i][j] = 0;iM[i][j] = 0;}}};

	inline Vec3D ToVec(void) {return Vec3D(x, y, z);} //shouldnt be necessaty... should be able to just set equal...

	friend CQuat operator+(const CQuat& s1, const CQuat& s2) {return CQuat(s1.w+s2.w, s1.x+s2.x, s1.y+s2.y, s1.z+s2.z);} //Plus
	CQuat& operator+=(const CQuat& s) {w += s.w; x += s.x; y += s.y; z += s.z; return *this;} //add and set
	friend CQuat operator-(const CQuat& s1, const CQuat& s2) {return CQuat(s1.w-s2.w, s1.x-s2.x, s1.y-s2.y, s1.z-s2.z);} //Minus
	CQuat& operator-=(const CQuat& s) {w -= s.w; x -= s.x; y -= s.y; z -= s.z; return *this;} //subract and set

	inline CQuat operator*(double f) {return CQuat(w*f, x*f, y*f, z*f);}; //scalar multiplication
	friend CQuat operator*(double f, CQuat v) {return CQuat(v.w*f, v.x*f, v.y*f, v.z*f);};
	inline CQuat operator*(const CQuat& f) {return CQuat(w*f.w - x*f.x - y*f.y - z*f.z, w*f.x + x*f.w + y*f.z - z*f.y, w*f.y - x*f.z + y*f.w + z*f.x, w*f.z + x*f.y - y*f.x + z*f.w);}; //overload Quaternion multiplication!
	operator Vec3D() {return Vec3D(x, y, z);};

	inline double Length() {return sqrt(Length2());} //length of the vector
	inline double Length2() {return (w*w+x*x+y*y+z*z);} //length squared of the vector
	double Normalize(void) {double l = Length(); if (l == 0){w = 1; x = 0; y = 0; z = 0;} else if (l > 0) {w /= l; x /= l; y /= l; z /= l;} return l;};
	CQuat Inverse(void) {double n = w*w + x*x + y*y + z*z; return CQuat(w/n, -x/n, -y/n, -z/n); };
	CQuat Conjugate(void) {return CQuat(w, -x, -y, -z);};

	double w, x, y, z;

	double M [3][3];
	double iM [3][3];
	void CalcMatrix(void) {M[0][0] = 1.0f-2.0f*(y*y+z*z); M[0][1] = 2.0f*(y*x-z*w); M[0][2] = 2.0f*(z*x+y*w); M[1][1] = 1.0f-2.0f*(x*x+z*z); M[1][2] = 2.0f*(z*y-x*w); M[2][2] = 1.0f-2.0f*(x*x+y*y); M[1][0] = M[0][1]; M[2][1] = M[1][2]; M[2][0] = M[0][2];};
	void CalciMatrix(void) {double determinant = -M[0][2]*M[1][1]*M[2][0] + M[0][1]*M[1][2]*M[2][0] + M[0][2]*M[1][0]*M[2][1] - M[0][0]*M[1][2]*M[2][1] - M[0][1]*M[1][0]*M[2][2] + M[0][0]*M[1][1]*M[2][2]; double k = 1.0 / determinant; iM[0][0] = (M[1][1]*M[2][2] - M[2][1]*M[1][2])*k; iM[0][1] = (M[2][1]*M[0][2] - M[0][1]*M[2][2])*k; iM[0][2] = (M[0][1]*M[1][2] - M[1][1]*M[0][2])*k; iM[1][0] = (M[1][2]*M[2][0] - M[2][2]*M[1][0])*k; iM[1][1] = (M[2][2]*M[0][0] - M[0][2]*M[2][0])*k; iM[1][2] = (M[0][2]*M[1][0] - M[1][2]*M[0][0])*k; iM[2][0] = (M[1][0]*M[2][1] - M[2][0]*M[1][1])*k; iM[2][1] = (M[2][0]*M[0][1] - M[0][0]*M[2][1])*k; iM[2][2] = (M[0][0]*M[1][1] - M[1][0]*M[0][1])*k;};
	void AngleAxis(double &angle, Vec3D &axis) {if (w>1.0) w = 1.0; double squareLength = x*x + y*y + z*z; if (squareLength>0.0000000000001){angle = 2.0f * (double) acos(w); double inverseLength = 1.0f / (double) pow(squareLength, 0.5); axis.x = x * inverseLength; axis.y = y * inverseLength; axis.z = z * inverseLength;} else{angle = 0.0f; axis.x = 1.0f; axis.y = 0.0f; axis.z = 0.0f;}};
};


inline Vec3D Vec3D::Rot(CQuat& Q){return (Q*CQuat(*this)*Q.Conjugate()).ToVec();}


#endif //_VEC3D_H
