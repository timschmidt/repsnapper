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
#include "config.h"
#pragma once

#include <vmmlib/vmmlib.h>
#include <polylib/Polygon2d.h>

#include "cuttingplane.h"

using namespace std;
using namespace vmml;
using namespace PolyLib;

class InFillHit;

class Poly
{

  CuttingPlane *plane;
        Poly();
public:
	Poly(CuttingPlane *plane, vector<Vector2d> *vertices);
        ~Poly();
	Poly Shrinked(double distance) const;
	Poly Shrinked(vector<Vector2d> *vertices, double distance);

	// Remove vertices that are on a straight line
	void cleanup(double maxerror);

	vector<Vector2d> intersect(Poly *other, int startVertex, 
				   const Vector2d endVertex,
				   double maxoffset) const;

	bool vertexInside(Vector2d point, double maxoffset) const;

	void calcHole(); // calc center and whether this is a hole 

	vector<Vector2d> getMinMax() const;
	vector<InFillHit>  calcInfill (double InfillDistance,
				       double InfillRotation, 
				       bool DisplayDebuginFill) const;

	Vector2d getVertexCircular(int pointindex) const;  // 2d point at index 
	Vector3d getVertexCircular3(int pointindex) const; // 3d point at index 
	vector<guint> points;			// points, indices into vertices
	vector<Vector2d> *vertices; // pointer to the vertices we have the indices for
	bool hole; // this polygon is a hole
	Vector2d center;

	void draw(int gl_type) const; // GL_LINE_LOOP or GL_POINTS
	void drawVertexNumbers() const; 
	void drawLineNumbers() const;

	void printinfo() const;
};

