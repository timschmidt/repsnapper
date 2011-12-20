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
#include "types.h"
#pragma once

// for PointHash
#ifdef __GNUC__
#  define _BACKWARD_BACKWARD_WARNING_H 1 // kill annoying warning
#  include <ext/hash_map>
namespace std
{
  using namespace __gnu_cxx;
}
#else
#  include <hash_map>
using namespace stdext;
#endif

#include <vector>
#include <list>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <limits>
#include <algorithm>

#include <vmmlib/vmmlib.h>
#include <polylib/Polygon2d.h>

#include "platform.h"
#include "gcode.h"
#include "poly.h"


#define CUTTING_PLANE_DEBUG 0

using namespace std;
using namespace vmml;
using namespace PolyLib;


typedef vector<Vector2d> outline;

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


struct locator
{
	locator(int polygon, int vertex, double where){
	  p=polygon; v=vertex; t=where;
	}
	int p;
	int v;
	double t;
};



/* associates adjacent points with integers */
class PointHash
{

	struct Impl;
	Impl *impl;
 public:
	PointHash();
	~PointHash();
	PointHash(const PointHash &copy);
	int  IndexOfPoint (const Vector2d &p);
	void InsertPoint  (guint idx, const Vector2d &p);
	void clear();

        static const double mult;
        static const double double_epsilon;
};



// For Logick shrinker ...
class CuttingPlaneOptimizer
{
public:
	double Z;
	//CuttingPlaneOptimizer(double z) { Z = z; };
	CuttingPlaneOptimizer(CuttingPlane* cuttingPlane, double z);
	CuttingPlane* cuttingPlane;
	list<Polygon2d*> positivePolygons;
	void Shrink(double distance, list<Polygon2d*> &resPolygons);
	void Draw();
	void Dispose();
	void MakeOffsetPolygons(vector<Poly>& polys, vector<Vector2d>& vectors);
	void RetrieveLines(vector<Vector3d>& lines);
private:
	void PushPoly(Polygon2d* poly);
	void DoMakeOffsetPolygons(Polygon2d* pPoly, vector<Poly>& polys,
				  vector<Vector2d>& vectors);
	void DoRetrieveLines(Polygon2d* pPoly, vector<Vector3d>& lines);
};





// A (set of) 2D polygon extracted from a 3D model
class CuttingPlane
{
  friend class Poly;
public:
  //CuttingPlane();
	CuttingPlane(int layerno);
	~CuttingPlane();

	// CuttingPlane *previous, *next;

	// Contract polygons:
	void Shrink(int quality, double extrudedWidth, 
		    double optimization, 
		    bool DisplayCuttingPlane, bool useFillets, 
		    int ShellCount);
	void ShrinkFast(double distance, double optimization,
			bool DisplayCuttingPlane,
			bool useFillets, int ShellCount);
	void ShrinkLogick(double distance, double optimization,
			  bool DisplayCuttingPlane, int ShellCount);
	
	// void  selfIntersectAndDivide();
	/* guint selfIntersectAndDivideRecursive(double z, guint startPolygon,  */
	/* 				      guint startVertex, */
	/* 				      vector<outline> &outlines,  */
	/* 				      const Vector2d endVertex, */
	/* 				      guint &level, */
	/* 				      double maxoffset); */
	/* void  recurseSelfIntersectAndDivide  (double z, vector<locator> &EndPointStack, */
	/* 				      vector<outline> &outlines, */
	/* 				      vector<locator> &visited, */
	/* 				      double maxoffset); */

	void ClearShrink()
	{
		offsetPolygons.clear();
		offsetVertices.clear();
	}


	vector<Vector2d> * CalcInFill(double InfillDistance, 
				      double InfillRotation, double InfillRotationPrLayer,
				      bool DisplayDebuginFill);	// Collide an infill-line with the polygons
	void Draw(bool DrawVertexNumbers, bool DrawLineNumbers, bool DrawOutlineNumbers,
		  bool DrawCPLineNumbers, bool DrawCPVertexNumbers) const ;
	bool MakePolygons(double Optimization); // Link Segments to form polygons
	bool CleanupConnectSegments();
	bool CleanupSharedSegments();
	// remove redudant points in given polygons -> done in poly.cpp
	// void CleanupPolygons(vector<Vector2d> vertices,
	// 		     vector<Poly> & polygons,
	// 		     double Optimization);

	void MakeGcode (GCodeState &state,
			const std::vector<Vector2d> *opt_infill,
			double &E, double z,
			const Settings::SlicingSettings &slicing,
			const Settings::HardwareSettings &hardware);
	bool VertexIsOutsideOriginalPolygons( Vector2d point, double z, double maxoffset) const ;

	Vector2d Min, Max;  // Bounding box

	void Clear()
	{
		lines.clear();
		vertices.clear();
		polygons.clear();
		points.clear();
		offsetPolygons.clear();
		offsetVertices.clear();
	}
	void setZ(double value)
	{
		Z = value;
	}
	double getZ() const { return Z; }

	int LayerNo;

	int RegisterPoint(const Vector2d &p);

	struct Segment {
		Segment(guint s, guint e) { start = s; end = e; }
		int start;		// Vertex index of start point
		int end;		// Vertex index of end point
		void Swap() {
			int tmp = start;
			start = end;
			end = tmp;
		}
	};
	void AddSegment(const Segment line);
  void AppendSegments(vector<Segment> segments);

	vector<Poly> GetPolygons() const { return polygons; }
	vector<Vector2d> GetVertices() const { return vertices; }
	vector<Vector2d> GetOffsetVertices() const { return offsetVertices; }

	void printinfo() const ;
private:
	PointHash points;

	vector<CuttingPlaneOptimizer*> optimizers;

	vector<Vector2d> vertices;	// cutpoints with desired z plane
	vector<Segment> lines;		// cutlines, have 2 vertex indices 
	vector<Poly> polygons;		// Closed loops, have indices of vertices
	double Z;

	vector<Poly> offsetPolygons;	// Shrinked closed loops
	vector<Vector2d> offsetVertices;// points for shrinked closed loops
};


