/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2011-12 martin.dieringer@gmx.de

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
#include "geometry.h"

class Poly
{
  double z;
  double extrusionfactor;

  //	vector<Poly*> holes;
  bool holecalculated;

public:
        Poly();
	Poly(double z, double extrusionfactor=1.);
        Poly(const Poly, double z);
	/* Poly(double z, */
	/*      const ClipperLib::Polygon cpoly, bool reverse=false); */
        ~Poly();
	
	Vector2d operator[](uint i) const {return vertices[i];};

	Poly Shrinked(double distance) const;
	Poly Shrinked(vector<Vector2d> *vertices, double distance);

	// simplify douglas-peucker
	void cleanup(double maxerror);
	vector<Vector2d> cleaned(const vector<Vector2d> vertices, double maxerror) const;

	void reverse() {std::reverse(vertices.begin(),vertices.end());};

	void clear(){vertices.clear();};


	//vector< vector<Vector2d> > intersect(Poly &poly1, Poly &poly2) const;

	bool vertexInside(const Vector2d point, double maxoffset=0.0001) const;
	bool polyInside(const Poly * poly, double maxoffset=0.0001) const;
	uint nearestDistanceSqTo(const Vector2d p, double &mindist) const;
	void nearestIndices(const Poly p2, int &thisindex, int &otherindex) const;

	void rotate(Vector2d center, double angle);

	void calcHole(); // calc center and whether this is a hole 
	bool isHole();

	vector<Vector2d> getMinMax() const;
	vector<Intersection> lineIntersections(const Vector2d P1, const Vector2d P2,
					       double maxerr=0.0001) const;

	// ClipperLib::Polygons getOffsetClipperPolygons(double dist) const ;
	// ClipperLib::Polygon getClipperPolygon(bool reverse=false) const;
	
	Vector2d getVertexCircular(int pointindex) const;  // 2d point at index 
	Vector3d getVertexCircular3(int pointindex) const; // 3d point at index 
	vector<Vector2d> getVertexRangeCircular(int from, int to) const;

	vector<Vector2d> vertices; // vertices
	void addVertex(Vector2d v, bool front=false);
	bool hole; // this polygon is a hole
	Vector2d center;
	Vector2d getCenter();
	double getZ() const;
	void setZ(double z) {this->z = z;};
	double getExtrusionFactor() const{return extrusionfactor;};
	void setExtrusionFactor(double e){extrusionfactor = e;};
	double getLayerNo() const;

	void draw(int gl_type, bool reverse=false) const; // GL_LINE_LOOP or GL_POINTS
	void draw(int gl_type, double z) const; // draw at given z
	void drawVertexNumbers() const; 
	void drawLineNumbers() const;

	void getLines(vector<Vector2d> &lines, Vector2d &startPoint) const;
	void getLines(vector<Vector3d> &lines, Vector2d &startPoint) const;
	/* void getLines(vector<printline> &plines, Vector2d &startPoint) const; */
	void getLines(vector<Vector2d> &lines,uint startindex=0) const;
	void getLines(vector<Vector3d> &lines,uint startindex=0) const;
	/* void getLines(vector<printline> &plines, uint startindex) const; */
	double getLinelengthSq(uint startindex) const;
	double averageLinelengthSq() const;

	vector<Vector2d> getPathAround(const Vector2d from, const Vector2d to) const;

	int getTriangulation(vector<Triangle> &triangles)  const ;

	uint size() const {return vertices.size(); };
	string info() const;

};

