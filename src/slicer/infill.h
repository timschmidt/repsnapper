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

#include <glib/gi18n.h>

#include <omp.h>

#include "layer.h"
#include "clipping.h"


using namespace std; 
using namespace vmml;

class Poly;


// user selectable have to be first
enum InfillType {ParallelInfill, ZigzagLineInfill, PolyInfill, HilbertInfill, 
		 SupportInfill, RaftInfill, BridgeInfill};

// these are available for user selection (order must be same as types):
const string InfillNames[] = {_("Parallel"), _("Zigzag"), _("Polygons"), _("Hilbert Curve")};


class Infill
{
  Layer *layer;

  struct pattern 
  {
    InfillType type;
    double angle;
    double distance;
    Vector2d Min,Max; 
    ClipperLib::Polygons cpolys;
  } ;

  static vector<struct pattern> savedPatterns;
  static omp_lock_t save_lock;

  ClipperLib::Polygons makeInfillPattern(InfillType type, 
					 const vector<Poly> tofillpolys,
					 double infillDistance,
					 double offsetDistance,
					 double rotation) ;

  Infill(); 

  void addInfillPoly(Poly p);
  void addInfillPolys(vector<Poly> polys);


 public:

  Infill(Layer *layer, double extrfactor);
  ~Infill(); 

  double extrusionfactor;
  string name; 

  void setName(string s){name=s;};
  string getName(){return name;};

  static void clearPatterns();
  InfillType type;
  double angle;
  double infillDistance;
  
  vector<Poly> infillpolys;  // for clipper polygon types
  vector<Vector2d> infillvertices; // for lines types
  
  void addInfill(double z, Poly poly, InfillType type, double infillDistance, 
		 double offsetDistance, double rotation);
  void addInfill(double z, vector<Poly> polys, InfillType type, double infillDistance, 
		 double offsetDistance, double rotation);
  void addInfill(double z, const vector<Poly> polys, const vector<Poly> fillpolys,
		 double offsetDistance);
  void addInfill(double z, const vector<Poly> polys, const ClipperLib::Polygons ifcpolys,
		 double offsetDistance);

  void getLines(vector<Vector3d> &lines) const;
  
  void clear();
  uint size() const {return infillpolys.size();};
  string info() const;
};


