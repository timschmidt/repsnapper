/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010 Logick

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
#ifdef WIN32
#include <windows.h>											// Header File For Windows
#endif
#include <vmmlib/vmmlib.h>
#include "platform.h"
#include "GeometryBase.h"

#ifdef __GNUC__
#  define _BACKWARD_BACKWARD_WARNING_H 1 // kill annoying warning
#include <ext/hash_map>
namespace std
{
 using namespace __gnu_cxx;
}
#else
#include <hash_map>
using namespace stdext;
#endif
using namespace std;
using namespace vmml;

namespace PolyLib
{
	class Polygon2f
	{
	public:
		Polygon2f(void);
		virtual ~Polygon2f(void);

		vector<Vector2f> vertices;
		bool hole;
		int  index;
		bool shrinked;
		bool expanded;
		list<Polygon2f*> containedHoles;
		list<Polygon2f*> containedSolids;
	
		void Optimize(float minAngle);
		bool ContainsPoint(Vector2f point);
		void PlaceInList(list<Polygon2f*>& dst);
		bool InsertPolygon(Polygon2f* poly);
		void Shrink(float distance, list<Polygon2f*> &parentPolygons, list<Polygon2f*> &polygons);

		static void DisplayPolygons(list<Polygon2f*> &polygons, float z, float r, float g, float b, float a);
		static void DrawRecursive(Polygon2f* poly, float Z, float color);
	private:
		// Helper Functions to Shrink
		void DoShrink(float distance, list<Polygon2f*> &parentPolygons, list<Polygon2f*> &polygons);
		void DoAddPolygonOutline(float distance, vector<Vector2f>& outLine, list<Polygon2f*> &parentPolygons, list<Polygon2f*> &polygons);
		bool TryPlaceInList(list<Polygon2f*>& dst);
	};

	class Line2f
	{
	public:
		Vector2f src;
		list<Vector2f> vertices;
		static void CleanOutLine(list<Line2f>& outline, float minDist2);
	};
}
