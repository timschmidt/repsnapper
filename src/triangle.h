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

#pragma once

#include "stdafx.h"
#include "string.h"
#include "math.h"


enum AXIS {NEGX, POSX, NEGY, POSY, NEGZ, POSZ, NOT_ALIGNED};


class Triangle
{
public:
        Triangle(const Vector3d &Norml, const Vector3d &Point1,
                 const Vector3d &Point2, const Vector3d &Point3)
            : A(Point1), B(Point2), C(Point3), Normal(Norml){}
        Triangle(const Vector3d &Point1,
                 const Vector3d &Point2, const Vector3d &Point3);
        Triangle(){}

        Triangle transformed(const Matrix4d &T) const;

        /* Represent the triangle as an array of length 3 {A, B, C} */
        Vector3d const & operator[](uint index) const;
        Vector3d & operator[](uint index);

        /* void SetPoints(const Vector3d &P1, const Vector3d &P2, const Vector3d &P3) { A=P1;B=P2;C=P3; } */
        /* void SetNormal(const Vector3d &Norml) { Normal=Norml;} */
        void calcNormal();
        void invertNormal();
        void mirrorX(const Vector3d &center);
        double area() const;
        double slopeAngle(const Matrix4d &T=Matrix4d::IDENTITY) const;

        void rotate(const Vector3d &axis, double angle);

        AXIS axis;			// Used for auto-rotation
        Vector3d A,B,C,Normal;	// p1,p2,p3, Normal
        Vector3d GetMax(const Matrix4d &T=Matrix4d::IDENTITY) const;
        Vector3d GetMin(const Matrix4d &T=Matrix4d::IDENTITY) const;

        void AccumulateMinMax(Vector3d &min, Vector3d &max,
                              const Matrix4d &T=Matrix4d::IDENTITY);
        void Translate(const Vector3d &vector);
        int CutWithPlane(double z, const Matrix4d &T,
                         Vector2d &lineStart, Vector2d &lineEnd) const;
        bool isInZrange(double zmin, double zmax, const Matrix4d &T) const;
        int SplitAtPlane(double z,
                         vector<Triangle> &uppertriangles,
                         vector<Triangle> &lowertriangles,
                         const Matrix4d &T=Matrix4d::IDENTITY) const;
        string getSTLfacet(const Matrix4d &T=Matrix4d::IDENTITY) const;
        void draw(GLenum gl_type) const;

        double projectedvolume(const Matrix4d &T=Matrix4d::IDENTITY) const;

        bool isConnectedTo(Triangle const &other, double maxsqerr=0.0001) const;
        bool wrongOrientationWith(Triangle const &other, double maxsqerr) const;

        string info() const;
};

