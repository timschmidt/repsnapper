/*
    This file is a part of the RepSnapper project.
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

#pragma once
#include "config.h"
#include "types.h"

// ClipperLib: see http://angusj.com/delphi/clipper.php
#if HAVE_CLIPPERLIB==1
  #include <polyclipping/clipper.hpp>
#else
  #include <clipper/clipper/polyclipping-code/cpp/clipper.hpp>
#endif

#include "poly.h"


// Clipper uses non-negative long long integers, so we transform by:
const double CL_FACTOR = 10000; // 1 = 1/10000 mm
const double CL_OFFSET = 10000; // 10 meters

namespace CL = ClipperLib;


enum PolyType{subject,clip};
enum JoinType{jsquare,jmiter,jround};


class Clipping
{
  friend class Poly;

  CL::Clipper clpr;

  double lastZ;     // remember last added polygon's Z for output
  double lastExtrF; // remember last added polygon's extrusion factor for output

  static CL::JoinType CLType(JoinType type);
  static CL::PolyType CLType(PolyType type);

  static CL::IntPoint ClipperPoint(const Vector2d &v);
  static Vector2d getPoint(const CL::IntPoint &p);

  static CL::Paths CLOffset(const CL::Paths &cpolys, int cldist,
			    CL::JoinType cljtype, double miter_limit=1,
			    bool reverse=false);

  bool debug;
  vector<CL::Paths> subjpolygons; // for debugging
  vector<CL::Paths> clippolygons;

public:
  Clipping(bool debugclipper=false){debug = debugclipper;};
  ~Clipping(){clear();};

  void clear();

  void addPoly    (const Poly &poly, PolyType type);
  void addPolys   (const vector<Poly> &poly, PolyType type);
  void addPolys   (const vector<ExPoly> &expolys, PolyType type);
  void addPolys   (const ExPoly &poly, PolyType type);
  void addPolygons(const CL::Paths &cp, PolyType type);

  // do after addPoly... and before clipping/results
  void setZ(double z) {lastZ = z;};
  double getZ() const {return lastZ;};
  void setExtrusionFactor(double e) {lastExtrF = e;};

  vector<Poly>   intersect      (CL::PolyFillType sft=CL::pftEvenOdd,
				 CL::PolyFillType cft=CL::pftEvenOdd);
  vector<Poly>   unite          (CL::PolyFillType sft=CL::pftEvenOdd,
				 CL::PolyFillType cft=CL::pftEvenOdd);
  vector<Poly>   subtract       (CL::PolyFillType sft=CL::pftEvenOdd,
				 CL::PolyFillType cft=CL::pftEvenOdd);
  vector<Poly>   subtractMerged (double overlap=0.001,
				 CL::PolyFillType sft=CL::pftEvenOdd,
				 CL::PolyFillType cft=CL::pftEvenOdd);
  vector<Poly>   Xor            (CL::PolyFillType sft=CL::pftEvenOdd,
				 CL::PolyFillType cft=CL::pftEvenOdd);
  vector<ExPoly> ext_intersect  (CL::PolyFillType sft=CL::pftEvenOdd,
				 CL::PolyFillType cft=CL::pftEvenOdd);
  vector<ExPoly> ext_unite      (CL::PolyFillType sft=CL::pftEvenOdd,
				 CL::PolyFillType cft=CL::pftEvenOdd);
  vector<ExPoly> ext_subtract   (CL::PolyFillType sft=CL::pftEvenOdd,
				 CL::PolyFillType cft=CL::pftEvenOdd);

  static vector<Poly> getMerged(const vector<Poly> &polys, double overlap=0.001);
  static CL::Paths    getMerged(const CL::Paths &cpolys, int overlap=3);

  static vector<Poly> getOffset(const Poly &poly, double distance,
				JoinType jtype=jmiter, double miterdist=1);
  static vector<Poly> getOffset(const vector<Poly> &polys, double distance,
				JoinType jtype=jmiter,double miterdist=1);
  static vector<Poly> getOffset(const ExPoly &expoly, double distance,
				JoinType jtype=jmiter, double miterdist=1);
  static vector<Poly> getOffset(const vector<ExPoly> &expolys, double distance,
				JoinType jtype=jmiter, double miterdist=1);

  static vector<Poly> getShrinkedCapped(const vector<Poly> &polys, double distance,
					JoinType jtype=jmiter,double miterdist=1);

  //vector< vector<Vector2d> > intersect(const Poly poly1, const Poly poly2) const;

  static Poly           getPoly(const CL::Path &cpoly, double z, double extrusionfactor);
  static vector<Poly>   getPolys(const ExPoly &expoly);
  static vector<Poly>   getPolys(const vector<ExPoly> &expolys);
  static vector<Poly>   getPolys(const CL::Paths &cpoly, double z, double extrusionfactor);
  static vector<ExPoly> getExPolys(const CL::PolyTree &ctree, double z,
				   double extrusionfactor);

  static vector<ExPoly> getExPolys(const vector<Poly> &polys,
				   double z, double extrusionfactor);
  static vector<ExPoly> getExPolys(const vector<Poly> &polys);
  static CL::PolyTree   getClipperTree(const vector<Poly> &polys);

  static CL::Path    getClipperPolygon (const Poly &poly);
  static CL::Paths   getClipperPolygons(const vector<Poly> &polys);
  static CL::Paths   getClipperPolygons(const ExPoly &expoly);
  //static CL::PolyTree   getClipperTree(const vector<ExPoly> &expolys);

  static double Area(const Poly &poly);
  static double Area(const vector<Poly> &polys);
  static double Area(const ExPoly &expoly);
  static double Area(const vector<ExPoly> &expolys);

  static void ReversePoints(vector<Poly> &polys);

 protected:
  // old API compatibility
  // polytree to expolygons
  struct ExPolygon {
    CL::Path outer;
    CL::Paths holes;
  };
  typedef std::vector< ExPolygon > ExPolygons;
  static void AddOuterPolyNodeToExPolygons(const CL::PolyNode *polynode,
					   ExPolygons& expolygons);
  static void PolyTreeToExPolygons(const CL::PolyTree * polytree, ExPolygons& expolygons);

};
