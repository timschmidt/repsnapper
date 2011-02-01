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
#include <vector>
#include <list>
#include "platform.h"

#include "math.h"                                               // Needed for sqrtf

#include <glibmm/keyfile.h>

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <list>

#include <FL/Fl_Gl_Window.H>
#include <FL/gl.h>

#include <vmmlib/vmmlib.h>
#include <polylib/Polygon2f.h>

/*
Vector3f position, normal;
// fill vertices
glNormal3fv( normal.xyz );
glVertex3fv( position.xyz );
*/

using namespace std;
using namespace vmml;
using namespace PolyLib;

typedef vector<Vector2f> outline;

enum AXIS {NEGX, POSX, NEGY, POSY, NEGZ, POSZ, NOT_ALIGNED};

class Triangle
{
public:
	Triangle(const Vector3f &Norml, const Vector3f &Point1,
		 const Vector3f &Point2, const Vector3f &Point3) 
		{ Normal = Norml ; A=Point1;B=Point2;C=Point3;}
	Triangle() {};

	void SetPoints(const Vector3f &P1, const Vector3f &P2, const Vector3f &P3) { A=P1;B=P2;C=P3; }
	void SetNormal(const Vector3f &Norml) { Normal=Norml;}
	float area();

	AXIS axis;			// Used for auto-rotation
	Vector3f A,B,C,Normal;	// p1,p2,p3, Normal
	Vector3f GetMax();
	Vector3f GetMin();
	void AccumulateMinMax(Vector3f &min, Vector3f &max);
	void Translate(const Vector3f &vector);
};


struct InFillHit
{
	Vector2f p;  // The intersection point
	float d;     // Distance from the infill-line start point, used for sorting hits
	float t;     // intersection point on first line
};

class Poly
{
public:
	Poly(){};
	void cleanup();				// Removed vertices that are on a straight line
	void calcHole(vector<Vector2f> &offsetVertices);
	vector<uint> points;			// points, indices into ..... a CuttingPlane or a GCode object
	bool hole;
	Vector2f center;
};

struct locator{
	locator(int polygon, int vertex, float where){p=polygon; v=vertex; t=where;}
	int p;
	int v;
	float t;
};

class CuttingPlaneOptimizer;

/* associates adjacent points with integers */
class PointHash {
	struct Impl;
	Impl *impl;
 public:
	PointHash();
	~PointHash();
	PointHash(const PointHash &copy);
	int  IndexOfPoint (const Vector2f &p);
	void InsertPoint  (uint idx, const Vector2f &p);
	void clear();

        static const float mult;
        static const float float_epsilon;
};

// A (set of) 2D polygon extracted from a 3D model
class CuttingPlane{
public:
	CuttingPlane();
	~CuttingPlane();
	void ShrinkFast(float distance, float optimization, bool DisplayCuttingPlane, bool useFillets, int ShellCount);		// Contracts polygons
	void ShrinkLogick(float distance, float optimization, bool DisplayCuttingPlane, int ShellCount);		// Contracts polygons
	void selfIntersectAndDivide();
	uint selfIntersectAndDivideRecursive(float z, uint startPolygon, uint startVertex, vector<outline> &outlines, const Vector2f endVertex, uint &level);

	void MakeContainedPlane(CuttingPlane& res)
	{
		res = *this;
		res.polygons = res.offsetPolygons;
		res.vertices = res.offsetVertices;
		res.offsetPolygons.clear();
		res.offsetVertices.clear();
	}
	void ClearShrink()
	{
		offsetPolygons.clear();
		offsetVertices.clear();
	}
	void recurseSelfIntersectAndDivide(float z, vector<locator> &EndPointStack, vector<outline> &outlines, vector<locator> &visited);
	void CalcInFill(vector<Vector2f> &infill, uint LayerNr, float InfillDistance, float InfillRotation, float InfillRotationPrLayer, bool DisplayDebuginFill);	// Collide a infill-line with the polygons
	void Draw(bool DrawVertexNumbers, bool DrawLineNumbers, bool DrawOutlineNumbers, bool DrawCPLineNumbers, bool DrawCPVertexNumbers);
	bool LinkSegments(float z, float Optimization);		        // Link Segments to form polygons
	bool CleanupConnectSegments(float z);
	bool CleanupSharedSegments(float z);
	void CleanupPolygons(float Optimization);			// remove redudant points
	void CleanupOffsetPolygons(float Optimization);			// remove redudant points
	void MakeGcode(const std::vector<Vector2f> &infill, GCode &code, float &E, float z, float MinPrintSpeedXY, float MaxPrintSpeedXY, float MinPrintSpeedZ, float MaxPrintSpeedZ, float DistanceToReachFullSpeed, float extrusionFactor, bool UseIncrementalEcode, bool Use3DGcode, bool EnableAcceleration);	// Convert Cuttingplane to GCode
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
		Segment(uint s, uint e) { start = s; end = e; }
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
	void DoMakeOffsetPolygons(Polygon2f* pPoly, vector<Poly>& polys, vector<Vector2f>& vectors);
	void DoRetrieveLines(Polygon2f* pPoly, vector<Vector3f>& lines);
};



class STL
{
public:
	STL();

	bool Read(string filename, bool force_binary = false );
	void GetObjectsFromIvcon();
	void clear() { triangles.clear(); }
	void displayInfillOld(const Glib::KeyFile &settings, CuttingPlane &plane, uint LayerNr, vector<int>& altInfillLayers);
	void draw (RFO &rfo, const Glib::KeyFile &settings, float opacity = 1.0f);
	void CenterAroundXY();
	void CalcCuttingPlane(float where, CuttingPlane &plane, const Matrix4f &T);	// Extract a 2D polygonset from a 3D model
	void OptimizeRotation();			// Auto-Rotate object to have the largest area surface down for printing
	void CalcCenter();
	void RotateObject(Vector3f axis, float angle);  // Rotation for manual rotate and used by OptimizeRotation

	vector<Triangle>  triangles;
	Vector3f Min, Max, Center;
};

