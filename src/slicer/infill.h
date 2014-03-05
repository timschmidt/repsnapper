/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2011-12  martin.dieringer@gmx.de

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

/* #include <glib/gi18n.h> */
#ifdef _OPENMP
#include <omp.h>
#endif

#include "stdafx.h"
#include "clipping.h"


// user selectable have to be first
enum InfillType {ParallelInfill, SmallZigzagInfill, HexInfill, PolyInfill, HilbertInfill,
		 SupportInfill, RaftInfill, BridgeInfill, ZigzagInfill, ThinInfill,
		 INVALIDINFILL};

// these are available for user selection (order must be same as types):
const string InfillNames[] = {_("Parallel"), _("Zigzag"), _("Hexagons"), _("Polygons"), _("Hilbert Curve")};


class Infill
{
  Layer *layer;

  struct pattern
  {
    InfillType type;
    double angle;
    double distance;
    Vector2d Min,Max;
    ClipperLib::Paths cpolys;
  } ;

  static vector<struct pattern> savedPatterns;
#ifdef _OPENMP
  static omp_lock_t save_lock;
#endif

  ClipperLib::Paths makeInfillPattern(InfillType type,
					 const vector<Poly> &tofillpolys,
					 double infillDistance,
					 double offsetDistance,
					 double rotation) ;

  Infill();

  void addInfillPoly(const Poly &p);
  void addInfillPolys(const vector<Poly> &polys);

  vector<Poly> m_tofillpolys;  // the polygons that are being filled

 public:

  Infill(Layer *layer, double extrfactor);
  ~Infill();

  double extrusionfactor;
  string name;
  bool cached; // if this pattern comes from savedPatterns

  void setName(string s){name=s;};
  string getName(){return name;};

  static void clearPatterns();
  InfillType m_type;
  double m_angle;
  double infillDistance;

  vector<Poly> infillpolys;  // for clipper polygon types
  vector<Vector2d> infillvertices; // for lines types

  void addPoly (double z, const Poly &poly, InfillType type, double infillDistance,
	       double offsetDistance, double rotation);
  void addPolys(double z, const vector<Poly> &polys, InfillType type,
		double infillDistance, double offsetDistance, double rotation);
  void addPolys(double z, const vector<Poly> &polys, const vector<Poly> &fillpolys,
		double offsetDistance);
  void addPolys(double z, const vector<Poly> &polys, const ClipperLib::Paths &ifcpolys,
		double offsetDistance);

  void addPoly (double z, const ExPoly &expoly, InfillType type, double infillDistance,
	       double offsetDistance, double rotation);
  void addPolys(double z, const vector<ExPoly> &expolys, InfillType type,
		double infillDistance, double offsetDistance, double rotation);

  void getLines(vector<Vector3d> &lines) const;

  typedef struct { Vector2d from; Vector2d to; } infillline;
  vector<Poly> sortedpolysfromlines(const vector<infillline> &lines, double z);

  void clear();
  uint size() const {return infillpolys.size();};

  vector<Poly> getCachedPattern(double z);

  string info() const;
};


