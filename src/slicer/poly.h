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
  mutable bool holecalculated;
  mutable bool hole; // this polygon is a hole
  bool closed;

public:
        Poly();
	Poly(double z, double extrusionfactor=1.);
        Poly(const Poly &p, double z);
	/* Poly(double z, */
	/*      const ClipperLib::Path cpoly, bool reverse=false); */
        ~Poly();

	void setClosed(bool c) { closed = c; };
	bool isClosed() const { return closed; };

	Vector2d operator[](int i) const {
	  if (i >= 0 && i < (int)vertices.size())
	    return vertices[i];
	  else return vertices[(vertices.size()+i)%vertices.size()];
	};

	uint nextVertex(uint i) const {return (i+1)%vertices.size();};

	/* Poly Shrinked(double distance) const; */
	/* Poly Shrinked(vector<Vector2d> *vertices, double distance); */

	// simplify douglas-peucker
	void cleanup(double maxerror);

	void reverse() {std::reverse(vertices.begin(),vertices.end());holecalculated = false;};

	void clear(){vertices.clear(); holecalculated = false;};

	void transform(const Matrix4d &T);
	void transform(const Matrix3d &T);
	void mirrorX(const Vector3d &center);

	//vector< vector<Vector2d> > intersect(Poly &poly1, Poly &poly2) const;

	bool vertexInside(const Vector2d &point, double maxoffset=0.0001) const;
	bool vertexInside2(const Vector2d &point, double maxoffset=0.0001) const;
	bool isInside(const Poly &poly, double maxoffset=0.0001) const;
	uint nearestDistanceSqTo(const Vector2d &p, double &mindist) const;
	void nearestIndices(const Poly &p2, int &thisindex, int &otherindex) const;
	double shortestConnectionSq(const Poly &p2, Vector2d &start, Vector2d &end) const;
	double angleAtVertex(uint i) const;
	vector<Vector2d> getCenterline() const;

	uint getFarthestIndex(uint &thisindex) const;
	uint getFarthestIndex(const Vector2d &from) const;

	void rotate(const Vector2d &center, double angle);
	void move(const Vector2d &delta);

	void calcHole() const; // calc center and whether this is a hole
	bool isHole() const;

	vector<Vector2d> getMinMax() const;
	vector<Intersection> lineIntersections(const Vector2d &P1, const Vector2d &P2,
					       double maxerr=0.0001) const;

	// ClipperLib::Paths getOffsetClipperPolygons(double dist) const ;
	// ClipperLib::Path getClipperPolygon(bool reverse=false) const;

	Vector2d const &getVertexCircular(int pointindex) const;  // 2d point at index
	Vector3d getVertexCircular3(int pointindex) const; // 3d point at index
	vector<Vector2d> getVertexRangeCircular(int from, int to) const;

	vector<Vector2d> vertices; // vertices
	void addVertex(const Vector2d &v, bool front=false);
	void addVertexUnique(const Vector2d &v, bool front=false);
	void addVertex(double x, double y, bool front=false);
	void addVertexUnique(double x, double y, bool front=false);


	mutable Vector2d center;
	Vector2d getCenter() const;
	double getZ() const {return z;}
	void setZ(double z) {this->z = z;};
	double getExtrusionFactor() const{return extrusionfactor;};
	void setExtrusionFactor(double e){extrusionfactor = e;};
	double getLayerNo() const;

	void draw(int gl_type, bool randomized=true) const;
	void draw(int gl_type, double z, bool randomized=true) const; // draw at given z
	void drawVertexNumbers() const;
	void drawVertexAngles() const;
	void drawLineNumbers() const;
	void draw_as_surface() const;

	void makeLines(vector<Vector2d> &lines, Vector2d &startPoint) const;
	void makeLines(vector<Vector3d> &lines, Vector2d &startPoint) const;
	void makeLines(vector<Vector2d> &lines,uint startindex=0) const;
	void makeLines(vector<Vector3d> &lines,uint startindex=0) const;

	double getLinelengthSq(uint startindex) const;
	double averageLinelengthSq() const;
	double totalLineLength() const;

	vector<Vector2d> getPathAround(const Vector2d &from, const Vector2d &to) const;

	int getTriangulation(vector<Triangle> &triangles)  const ;

	uint size() const {return vertices.size(); };
	Vector2d front() {return vertices.front(); };
	Vector2d back()  {return vertices.back(); };
	void push_back (Vector2d v) {
	  vertices.push_back(v); holecalculated = false;};
	void push_front(Vector2d v) {
	  vertices.insert(vertices.begin(),v); holecalculated = false;};

	string info() const;

	string SVGpolygon(string style="fill: black") const;
	string SVGpath(const Vector2d &trans=Vector2d::ZERO) const;

	string gnuplot_path(const Vector2d &trans=Vector2d::ZERO) const; // for debugging


	static void move(vector<Poly> &polys, const Vector2d &trans);

};


////////////////////////////////////////////////////////////////////

class ExPoly
{
 public:
  ExPoly(){};
  ~ExPoly(){};
  Poly outer;
  vector<Poly> holes;

  void clear();
  double getZ() const {return outer.getZ();}

  void draw(int gl_type, bool randomized=false) const;
  void draw(int gl_type, double z, bool randomized=false) const; // draw at given z
  void drawVertexNumbers() const;
  void drawLineNumbers() const;

  void cleanup(double maxerror);

};


////////////////////////////////////////////////////////////////////

void draw_poly (const Poly &poly, int gl_type, int linewidth, int pointsize,
		const float *rgb, float a, bool randomized = false);
void draw_polys(const vector <Poly> &polys, int gl_type, int linewidth, int pointsize,
		const float *rgb, float a, bool randomized = false);
void draw_polys(const vector< vector <Poly> > &polys,
		int gl_type, int linewidth, int pointsize,
		const float *rgb, float a, bool randomized = false);
void draw_poly (const ExPoly &expoly, int gl_type, int linewidth, int pointsize,
		const float *rgb, float a, bool randomized = false);
void draw_polys(const vector <ExPoly> &expolys, int gl_type, int linewidth, int pointsize,
		const float *rgb, float a, bool randomized = false);

void draw_polys_surface(const vector <Poly> &polys,
			const Vector2d &Min, const Vector2d &Max,
			double z,
			double cleandist,
			const float *rgb, float a);
void draw_polys_surface(const vector< vector<Poly> > &polys,
			const Vector2d &Min, const Vector2d &Max,
			double z,
			double cleandist,
			const float *rgb, float a);
void draw_polys_surface(const vector <ExPoly> &expolys,
			const Vector2d &Min, const Vector2d &Max,
			double z,
			double cleandist,
			const float *rgb, float a);

void clearpolys(vector<Poly> &polys);
void clearpolys(vector<ExPoly> &polys);
void clearpolys(vector< vector<Poly> > &polys);
