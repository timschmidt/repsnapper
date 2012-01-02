/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2011  martin.dieringer@gmx.de

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
#include <vmmlib/vmmlib.h> 

#include "cuttingplane.h"


using namespace std; 
using namespace vmml;

class Poly;



enum InfillType {ParallelInfill,LinesInfill,SupportInfill};


class Infill
{
  CuttingPlane *plane;

  struct pattern 
  {
    InfillType type;
    double angle;
    double distance;
    ClipperLib::Polygons cpolys;
  } ;

  static vector<struct pattern> savedPatterns;

  ClipperLib::Polygons makeInfillPattern(InfillType type, double infillDistance,
					 double offsetDistance,
					 double rotation) ;

  Infill(){}; 

 public:

  Infill(CuttingPlane *plane);
  ~Infill(){}; 

  static void clearPatterns();
  InfillType type;
  double angle;

  Vector2d center,Min,Max;  
  vector<Poly> infillpolys;  // for clipper polygon types
  vector<Vector2d> infillvertices; // for lines types
  
  void calcInfill(vector<Poly> poly, InfillType type, double infillDistance, 
		  double offsetDistance, double rotation);
  void calcInfill(const vector<Poly> polys, const vector<Poly> fillpolys,
		  double offsetDistance);
  void calcInfill(const vector<Poly> polys, const ClipperLib::Polygons ifcpolys,
		  double offsetDistance);

  void addInfillPoly(const Poly p);

  void getLines(vector<Vector3d> &lines) const;
  
  void clear(){infillpolys.clear();};
  uint size() const {return infillpolys.size();};
  void printinfo() const;
};



// for historic reasons:
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

