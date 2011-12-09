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
#include <vector>
#include <list>
#include "platform.h"

#include "math.h"                                               // Needed for sqrtf

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <limits>
#include <algorithm>

#include <vmmlib/vmmlib.h>
#include <polylib/Polygon2d.h>

#include "stdafx.h"
#include "string.h"

#include "gcode.h"
#include "math.h"
#include "settings.h"

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

#define ABS(a)	   (((a) < 0) ? -(a) : (a))

/* A number that speaks for itself, every kissable digit.                    */

#define PI 3.141592653589793238462643383279502884197169399375105820974944592308

#define CUTTING_PLANE_DEBUG 0

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define RESOLUTION 4
#define FREE(p)            {if (p) {free(p); (p)= NULL;}}

using namespace std;
using namespace vmml;
using namespace PolyLib;

class RFO;
class GCode;

typedef vector<Vector2d> outline;

enum AXIS {NEGX, POSX, NEGY, POSY, NEGZ, POSZ, NOT_ALIGNED};

enum filetype_t{
    ASCII_STL,
    BINARY_STL,
    NONE_STL
};

class Triangle
{
public:
	Triangle(const Vector3d &Norml, const Vector3d &Point1,
		 const Vector3d &Point2, const Vector3d &Point3) 
		{ Normal = Norml ; A=Point1;B=Point2;C=Point3;}
	Triangle() {};

	/* Represent the triangle as an array of length 3 {A, B, C} */
	Vector3d &operator[](const int index);

	void SetPoints(const Vector3d &P1, const Vector3d &P2, const Vector3d &P3) { A=P1;B=P2;C=P3; }
	void SetNormal(const Vector3d &Norml) { Normal=Norml;}
	double area();

	AXIS axis;			// Used for auto-rotation
	Vector3d A,B,C,Normal;	// p1,p2,p3, Normal
	Vector3d GetMax();
	Vector3d GetMin();
	void AccumulateMinMax(Vector3d &min, Vector3d &max);
	void Translate(const Vector3d &vector);
	int CutWithPlane(float z, const Matrix4d &T, 
			 Vector2d &lineStart, Vector2d &lineEnd);
};


struct InFillHit
{
	Vector2d p;  // The intersection point
	double d;     // Distance from the infill-line start point, used for sorting hits
	double t;     // intersection point on first line
};

bool InFillHitCompareFunc(const InFillHit& d1, const InFillHit& d2);
bool IntersectXY (const Vector2d &p1, const Vector2d &p2,
		  const Vector2d &p3, const Vector2d &p4, InFillHit &hit);

class Poly
{
public:
	Poly(){};
	void cleanup();				// Removed vertices that are on a straight line
	void calcHole(vector<Vector2d> &offsetVertices);
	vector<guint> points;			// points, indices into ..... a CuttingPlane or a GCode object
	bool hole;
	Vector2d center;
};

struct locator
{
	locator(int polygon, int vertex, float where){p=polygon; v=vertex; t=where;}
	int p;
	int v;
	double t;
};

class CuttingPlaneOptimizer;

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

class Command;
struct GCodeStateImpl;
class GCodeState {
  GCodeStateImpl *pImpl;
 public:
  GCodeState(GCode &code);
  ~GCodeState();
  void SetZ (float z);
  void AppendCommand(Command &command);
  void MakeAcceleratedGCodeLine (Vector3d start, Vector3d end,
				 double extrusionFactor,
				 double &E, float z,
				 const Settings::SlicingSettings &slicing,
				 const Settings::HardwareSettings &hardware);
  float GetLastLayerZ(float curZ);
  void  SetLastLayerZ(float z);
  const Vector3d &LastPosition();
  void  SetLastPosition(const Vector3d &v);
  void  ResetLastWhere(Vector3d to);
  double DistanceFromLastTo(Vector3d here);
  double LastCommandF();
};

// A (set of) 2D polygon extracted from a 3D model
class CuttingPlane
{
public:
	CuttingPlane();
	~CuttingPlane();

	// Contract polygons:
	void ShrinkFast(double distance, double optimization, bool DisplayCuttingPlane,
			bool useFillets, int ShellCount);
	void ShrinkLogick(double distance, double optimization, bool DisplayCuttingPlane,
			  int ShellCount);

	void  selfIntersectAndDivide();
	guint selfIntersectAndDivideRecursive(float z, guint startPolygon, 
					      guint startVertex,
					      vector<outline> &outlines, 
					      const Vector2d endVertex,
					      guint &level);
	void  recurseSelfIntersectAndDivide  (float z, vector<locator> &EndPointStack,
					      vector<outline> &outlines,
					      vector<locator> &visited);

	void ClearShrink()
	{
		offsetPolygons.clear();
		offsetVertices.clear();
	}
	vector<Vector2d> * CalcInFill(guint LayerNr, double InfillDistance, 
				      double InfillRotation, double InfillRotationPrLayer,
				      bool DisplayDebuginFill);	// Collide an infill-line with the polygons
	void Draw(bool DrawVertexNumbers, bool DrawLineNumbers, bool DrawOutlineNumbers,
		  bool DrawCPLineNumbers, bool DrawCPVertexNumbers);
	bool LinkSegments(float z, double Optimization);		        // Link Segments to form polygons
	bool CleanupConnectSegments(float z);
	bool CleanupSharedSegments(float z);
	void CleanupPolygons(double Optimization);			// remove redudant points
	void CleanupOffsetPolygons(double Optimization);			// remove redudant points
	void MakeGcode (GCodeState &state,
			const std::vector<Vector2d> *opt_infill,
			double &E, float z,
			const Settings::SlicingSettings &slicing,
			const Settings::HardwareSettings &hardware);
	bool VertexIsOutsideOriginalPolygon( Vector2d point, float z);

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
	void SetZ(float value)
	{
		Z = value;
	}
	float getZ() { return Z; }

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
	void AddLine(const Segment &line);

	vector<Poly>& GetPolygons() { return polygons; }
	vector<Vector2d>& GetVertices() { return vertices; }

private:
	PointHash points;

	vector<CuttingPlaneOptimizer*> optimizers;

	vector<Segment> lines;		// Segments - 2 points pr. line-segment
	vector<Poly> polygons;		// Closed loops
	vector<Vector2d> vertices;	// points
	float Z;

	vector<Poly> offsetPolygons;	// Shrinked closed loops
	vector<Vector2d> offsetVertices;// points for shrinked closed loops
};


#define sqr(x) ((x)*(x))

class Slicer
{
public:
	Slicer();

    int load(std::string filename);

	void clear() { triangles.clear(); }
	void displayInfillOld(const Settings &settings, CuttingPlane &plane, 
			      guint LayerNr, vector<int>& altInfillLayers);
	void draw (RFO &rfo, const Settings &settings);
	void draw_geometry ();
	void CenterAroundXY();
	// Extract a 2D polygonset from a 3D model:
	void CalcCuttingPlane(float where, CuttingPlane &plane, const Matrix4d &T);
	// Auto-Rotate object to have the largest area surface down for printing:
	void OptimizeRotation(); 
	void CalcCenter();
	// Rotation for manual rotate and used by OptimizeRotation:
	void RotateObject(Vector3d axis, double angle);  
    void Scale(double scale_factor);
    double getScaleFactor(){ return scale_factor; };

	vector<Triangle>  triangles;
	Vector3d Min, Max, Center;

private:
    double scale_factor;
    int loadASCIISTL(std::string filename);
    int loadBinarySTL(std::string filename);
    filetype_t getFileType(std::string filename);
};

// For Logick shrinker ...
class CuttingPlaneOptimizer
{
public:
	float Z;
	CuttingPlaneOptimizer(float z) { Z = z; }
	CuttingPlaneOptimizer(CuttingPlane* cuttingPlane, float z);
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

#include "rfo.h"
