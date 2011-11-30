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
#include <polylib/Polygon2f.h>

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

typedef vector<Vector2f> outline;

enum AXIS {NEGX, POSX, NEGY, POSY, NEGZ, POSZ, NOT_ALIGNED};

enum filetype_t{
    ASCII_STL,
    BINARY_STL,
    NONE_STL
};

class Triangle
{
public:
	Triangle(const Vector3f &Norml, const Vector3f &Point1,
		 const Vector3f &Point2, const Vector3f &Point3) 
		{ Normal = Norml ; A=Point1;B=Point2;C=Point3;}
	Triangle() {};

	/* Represent the triangle as an array of length 3 {A, B, C} */
	Vector3f &operator[](const int index);

	void SetPoints(const Vector3f &P1, const Vector3f &P2, const Vector3f &P3) { A=P1;B=P2;C=P3; }
	void SetNormal(const Vector3f &Norml) { Normal=Norml;}
	float area();

	AXIS axis;			// Used for auto-rotation
	Vector3f A,B,C,Normal;	// p1,p2,p3, Normal
	Vector3f GetMax();
	Vector3f GetMin();
	void AccumulateMinMax(Vector3f &min, Vector3f &max);
	void Translate(const Vector3f &vector);
	int CutWithPlane(float z, const Matrix4f &T, 
			 Vector2f &lineStart, Vector2f &lineEnd);
};


struct InFillHit
{
	Vector2f p;  // The intersection point
	float d;     // Distance from the infill-line start point, used for sorting hits
	float t;     // intersection point on first line
};

bool InFillHitCompareFunc(const InFillHit& d1, const InFillHit& d2);
bool IntersectXY (const Vector2f &p1, const Vector2f &p2,
		  const Vector2f &p3, const Vector2f &p4, InFillHit &hit);

class Poly
{
public:
	Poly(){};
	void cleanup();				// Removed vertices that are on a straight line
	void calcHole(vector<Vector2f> &offsetVertices);
	vector<guint> points;			// points, indices into ..... a CuttingPlane or a GCode object
	bool hole;
	Vector2f center;
};

struct locator
{
	locator(int polygon, int vertex, float where){p=polygon; v=vertex; t=where;}
	int p;
	int v;
	float t;
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
	int  IndexOfPoint (const Vector2f &p);
	void InsertPoint  (guint idx, const Vector2f &p);
	void clear();

        static const float mult;
        static const float float_epsilon;
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
  void MakeAcceleratedGCodeLine (Vector3f start, Vector3f end,
				 float extrusionFactor,
				 float &E, float z,
				 const Settings::SlicingSettings &slicing,
				 const Settings::HardwareSettings &hardware);
  float GetLastLayerZ(float curZ);
  void  SetLastLayerZ(float z);
  const Vector3f &LastPosition();
  void  SetLastPosition(const Vector3f &v);
  void  ResetLastWhere(Vector3f to);
  float DistanceFromLastTo(Vector3f here);
  float LastCommandF();
};

// A (set of) 2D polygon extracted from a 3D model
class CuttingPlane
{
public:
	CuttingPlane();
	~CuttingPlane();

	// Contract polygons:
	void ShrinkFast(float distance, float optimization, bool DisplayCuttingPlane,
			bool useFillets, int ShellCount);
	void ShrinkLogick(float distance, float optimization, bool DisplayCuttingPlane,
			  int ShellCount);

	void  selfIntersectAndDivide();
	guint selfIntersectAndDivideRecursive(float z, guint startPolygon, 
					      guint startVertex,
					      vector<outline> &outlines, 
					      const Vector2f endVertex,
					      guint &level);
	void  recurseSelfIntersectAndDivide  (float z, vector<locator> &EndPointStack,
					      vector<outline> &outlines,
					      vector<locator> &visited);

	void ClearShrink()
	{
		offsetPolygons.clear();
		offsetVertices.clear();
	}
	vector<Vector2f> * CalcInFill(guint LayerNr, float InfillDistance, 
				      float InfillRotation, float InfillRotationPrLayer,
				      bool DisplayDebuginFill);	// Collide an infill-line with the polygons
	void Draw(bool DrawVertexNumbers, bool DrawLineNumbers, bool DrawOutlineNumbers,
		  bool DrawCPLineNumbers, bool DrawCPVertexNumbers);
	bool LinkSegments(float z, float Optimization);		        // Link Segments to form polygons
	bool CleanupConnectSegments(float z);
	bool CleanupSharedSegments(float z);
	void CleanupPolygons(float Optimization);			// remove redudant points
	void CleanupOffsetPolygons(float Optimization);			// remove redudant points
	void MakeGcode (GCodeState &state,
			const std::vector<Vector2f> *opt_infill,
			float &E, float z,
			const Settings::SlicingSettings &slicing,
			const Settings::HardwareSettings &hardware);
	bool VertexIsOutsideOriginalPolygon( Vector2f point, float z);

	Vector2f Min, Max;  // Bounding box

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

	int RegisterPoint(const Vector2f &p);

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
	vector<Vector2f>& GetVertices() { return vertices; }

private:
	PointHash points;

	vector<CuttingPlaneOptimizer*> optimizers;

	vector<Segment> lines;		// Segments - 2 points pr. line-segment
	vector<Poly> polygons;		// Closed loops
	vector<Vector2f> vertices;	// points
	float Z;

	vector<Poly> offsetPolygons;	// Shrinked closed loops
	vector<Vector2f> offsetVertices;// points for shrinked closed loops
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
	void CalcCuttingPlane(float where, CuttingPlane &plane, const Matrix4f &T);
	// Auto-Rotate object to have the largest area surface down for printing:
	void OptimizeRotation(); 
	void CalcCenter();
	// Rotation for manual rotate and used by OptimizeRotation:
	void RotateObject(Vector3f axis, float angle);  
    void Scale(float scale_factor);
    float getScaleFactor(){ return scale_factor; };

	vector<Triangle>  triangles;
	Vector3f Min, Max, Center;

private:
    float scale_factor;
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
	list<Polygon2f*> positivePolygons;
	void Shrink(float distance, list<Polygon2f*> &resPolygons);
	void Draw();
	void Dispose();
	void MakeOffsetPolygons(vector<Poly>& polys, vector<Vector2f>& vectors);
	void RetrieveLines(vector<Vector3f>& lines);
private:
	void PushPoly(Polygon2f* poly);
	void DoMakeOffsetPolygons(Polygon2f* pPoly, vector<Poly>& polys,
				  vector<Vector2f>& vectors);
	void DoRetrieveLines(Polygon2f* pPoly, vector<Vector3f>& lines);
};

#include "rfo.h"
