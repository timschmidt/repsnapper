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


// #include "infill.h" 

//class Infill;

using namespace std;
using namespace vmml;
using namespace PolyLib;


struct InFillHit
{
	Vector2d p;  // The intersection point
	double d;     // Distance from the infill-line start point, used for sorting hits
	double t;     // intersection point on first line
};

bool InFillHitCompareFunc(const InFillHit& d1, const InFillHit& d2);
bool IntersectXY (const Vector2d &p1, const Vector2d &p2,
		  const Vector2d &p3, const Vector2d &p4, 
		  InFillHit &hit, double maxoffset);


class Poly
{

  double z;
  //	vector<Poly*> holes;
	bool holecalculated;
	//Infill infill;

	//Clipping clipp;
public:
        Poly();
	Poly(double z);
	/* Poly(double z, */
	/*      const ClipperLib::Polygon cpoly, bool reverse=false); */
        ~Poly();

	Poly Shrinked(double distance) const;
	Poly Shrinked(vector<Vector2d> *vertices, double distance);

	// Remove vertices that are on a straight line
	void cleanup(double maxerror=0.05);

	void clear(){vertices.clear();};


	//vector< vector<Vector2d> > intersect(Poly &poly1, Poly &poly2) const;

	bool vertexInside(const Vector2d point, double maxoffset) const;
	bool polyInside(const Poly * poly, double maxoffset) const;
	uint nearestDistanceSqTo(const Vector2d p, double &mindist) const;

	void rotate(Vector2d center, double angle);

	void calcHole(); // calc center and whether this is a hole 
	bool isHole();

	vector<Vector2d> getMinMax() const;
	vector<InFillHit> lineIntersections(const Vector2d P1, const Vector2d P2,
					    double maxerr) const;

	// ClipperLib::Polygons getOffsetClipperPolygons(double dist) const ;
	// ClipperLib::Polygon getClipperPolygon(bool reverse=false) const;
	
	Vector2d getVertexCircular(int pointindex) const;  // 2d point at index 
	Vector3d getVertexCircular3(int pointindex) const; // 3d point at index 
	//	vector<guint> points;			// points, indices into vertices
	vector<Vector2d> vertices; // vertices
	void addVertex(Vector2d v, bool front=false);
	bool hole; // this polygon is a hole
	Vector2d center;
	double getZ() const;
	void setZ(double z) {this->z = z;};
	double getLayerNo() const;

	void draw(int gl_type, bool reverse=false) const; // GL_LINE_LOOP or GL_POINTS
	void draw(int gl_type, double z) const; // draw at given z
	void drawVertexNumbers() const; 
	void drawLineNumbers() const;

	void getLines(vector<Vector2d> &lines, Vector2d &startPoint) const;
	void getLines(vector<Vector3d> &lines, Vector2d &startPoint) const;
	void getLines(vector<Vector2d> &lines,uint startindex=0) const;
	void getLines(vector<Vector3d> &lines,uint startindex=0) const;
	double getLinelengthSq(uint startindex) const;

	uint size() const {return vertices.size(); };
	void printinfo() const;

};

