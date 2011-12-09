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
  class Polygon2d
  {
  public:
    Polygon2d(void);
    virtual ~Polygon2d(void);

    vector<Vector2d> vertices;
    bool hole;
    int  index;
    bool shrinked;
    bool expanded;
    list<Polygon2d*> containedHoles;
    list<Polygon2d*> containedSolids;
	
    void Optimize(double minAngle);
    bool ContainsPoint(Vector2d point);
    void PlaceInList(list<Polygon2d*>& dst);
    bool InsertPolygon(Polygon2d* poly);
    void Shrink(double distance, list<Polygon2d*> &parentPolygons, list<Polygon2d*> &polygons);

    static void DisplayPolygons(list<Polygon2d*> &polygons, double z, double r, double g, double b, double a);
    static void DrawRecursive(Polygon2d* poly, double Z, double color);
  private:
    // Helper Functions to Shrink
    void DoShrink(double distance, list<Polygon2d*> &parentPolygons, list<Polygon2d*> &polygons);
    void DoAddPolygonOutline(double distance, vector<Vector2d>& outLine, list<Polygon2d*> &parentPolygons, list<Polygon2d*> &polygons);
    bool TryPlaceInList(list<Polygon2d*>& dst);
  };

  class Line2d
  {
  public:
    Vector2d src;
    list<Vector2d> vertices;
    static void CleanOutLine(list<Line2d>& outline, double minDist2);
  };
}
