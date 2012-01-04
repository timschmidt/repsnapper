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
#include <vector>


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

#include <vmmlib/vmmlib.h>
#include <polylib/Polygon2d.h>


#include "cuttingplane.h"



using namespace vmml;
using namespace PolyLib;


#define RESOLUTION 4
#define FREE(p)            {if (p) {free(p); (p)= NULL;}}

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
	void InsertPoint  (uint idx, const Vector2d &p);
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
	void MakeOffsetPolygons(vector<Poly>& polys);
	void RetrieveLines(vector<Vector3d>& lines);
private:
	void PushPoly(Polygon2d* poly);
	void DoMakeOffsetPolygons(Polygon2d* pPoly, vector<Poly>& polys);
	void DoRetrieveLines(Polygon2d* pPoly, vector<Vector3d>& lines);
};

