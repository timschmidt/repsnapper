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
#include "stdafx.h"
#include "config.h"
#pragma once

#include <vmmlib/vmmlib.h>
#include <polylib/Polygon2d.h>

#include <clipper/clipper/polyclipping/trunk/cpp/clipper.hpp>

/* #include "cuttingplane.h" */
#include "infill.h" 

struct InFillHit;
class CuttingPlane;
//class Infill;

using namespace std;
using namespace vmml;
using namespace PolyLib;



class Poly
{

  CuttingPlane *plane;
        Poly();
	vector<Poly*> holes;
	Infill infill;
public:
	Poly(CuttingPlane *plane, vector<Vector2d> *vertices);
	Poly(CuttingPlane *plane, vector<Vector2d> *vertices,
	     const ClipperLib::Polygon cpoly,bool reverse=true);
        ~Poly();
	Poly Shrinked(double distance) const;
	Poly Shrinked(vector<Vector2d> *vertices, double distance);

	// Remove vertices that are on a straight line
	void cleanup(double maxerror);

	vector<Vector2d> intersect(Poly *other, int startVertex, 
				   const Vector2d endVertex,
				   double maxoffset) const;

	vector< vector<Vector2d> > intersect(Poly &poly1, Poly &poly2) const;

	bool vertexInside(const Vector2d point, double maxoffset) const;
	bool polyInside(const Poly * poly, double maxoffset) const;

	void calcHole(); // calc center and whether this is a hole 

	vector<Vector2d> getMinMax() const;
	vector<InFillHit> lineIntersections(const Vector2d P1, const Vector2d P2,
					    double maxerr) const;
	void calcInfill (double InfillDistance,
			 double InfillRotation, 
			 bool DisplayDebuginFill);
	vector<Vector2d> getInfillVertices() const;

	ClipperLib::Polygon getClipperPolygon(bool reverse=true) const;

	Vector2d getVertexCircular(int pointindex) const;  // 2d point at index 
	Vector3d getVertexCircular3(int pointindex) const; // 3d point at index 
	vector<guint> points;			// points, indices into vertices
	vector<Vector2d> *vertices; // pointer to the vertices we have the indices for
	bool hole; // this polygon is a hole
	Vector2d center;
	double getZ() const;
	double getLayerNo() const;

	void draw(int gl_type) const; // GL_LINE_LOOP or GL_POINTS
	void drawVertexNumbers() const; 
	void drawLineNumbers() const;

	void printinfo() const;
};

