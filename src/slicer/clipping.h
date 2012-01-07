/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010  Kulitorum
    Copyright (C) 2012  martin.dieringer@gmx.de

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

#include <clipper/clipper/polyclipping/trunk/cpp/clipper.hpp>
// see http://angusj.com/delphi/clipper.php


#include "poly.h"



// Clipper uses non-negative long long integers, so we transform by:
const double CL_FACTOR = 10000; // 1 = 1/10000 mm
const double CL_OFFSET = 10000; // 10 m

namespace CL = ClipperLib;


enum PolyType{subject,clip};
enum JoinType{jsquare,jmiter,jround};


class Clipping
{
  friend class Poly;

  static CL::Clipper clpr;

  double lastZ; // remember last added polygon's Z for output
  
  static CL::JoinType CLType(JoinType type);
  static CL::PolyType CLType(PolyType type);

  static CL::IntPoint ClipperPoint(Vector2d v);
  static Vector2d getPoint(CL::IntPoint p);

  static CL::Polygons CLOffset(CL::Polygons cpolys, int cldist, 
			       CL::JoinType cljtype, double miter_limit=1);

  static CL::Polygons getMerged(CL::Polygons cpolys);

public:
  Clipping(){};
  ~Clipping(){clear();};

  void clear(){clpr.Clear();};

  void addPoly(const Poly poly, PolyType type);
  void addPolys(const vector<Poly> poly, PolyType type);

  vector<Poly> intersect();
  vector<Poly> unite();
  vector<Poly> substract();
  vector<Poly> substractMerged();
  // vector<Poly> xor();

  static vector<Poly> getMerged(vector<Poly> polys);
  
  static Poly getOffset(Poly poly, double distance, JoinType jtype=jmiter, double miterdist=1);
  static vector<Poly> getOffset(vector<Poly> polys, double distance, 
				JoinType jtype=jmiter,double miterdist=1);

  //vector< vector<Vector2d> > intersect(const Poly poly1, const Poly poly2) const;

  static Poly getPoly(const CL::Polygon cpoly, double z);
  static vector <Poly> getPolys(const CL::Polygons cpoly, double z);

  static CL::Polygon  getClipperPolygon (const Poly poly, bool reverse=false);
  static CL::Polygons getClipperPolygons(const vector<Poly> polys, bool reverse=false);

};
