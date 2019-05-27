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

#include "../stdafx.h"
#include "clipping.h"
#include "infillpattern.h"

#ifdef _OPENMP
#include <omp.h>
#endif


class Infill
{
    //  InfillPattern pattern;

//  static vector<InfillPattern*> savedPatterns;
#ifdef _OPENMP
  static omp_lock_t save_lock;
#endif

  ClipperLib::Paths getInfillPattern(const vector<Poly> &tofillpolys,
                                     int layerNo) ;

  Infill();

  InfillPattern *pattern;

  void addInfillPoly(const Poly &p);
  vector<Poly> getInfillPolys(const vector<Poly> &polys);

public:

  Infill(InfillType type, double infillDistance, double extrfactor,
         double rotation, double rotatePerLayer);
  ~Infill();

  InfillType m_type;
  double infillDistance;
  double extrusionfactor;
  double baseRotation;
  double rotationPerLayer;

  void makeBaseInfillPattern(const Vector2d &min, const Vector2d &max);

  void apply(double z, const Poly &poly, int layerNo, vector<Poly> &toPolys);
  void apply(double z, const vector<Poly> &polys, int layerNo,
             vector<Poly> &toPolys);
  void apply(double z, const ExPoly &expoly, int layerNo,
                     vector<Poly> &toPolys);
  void apply(double z, const vector<ExPoly> &expolys, int layerNo,
             vector<Poly> &toPolys);
  void apply(double z, const vector<Poly> &polys, const vector<Poly> &fillpolys,
             vector<Poly> &toPolys);
  vector<Poly> apply(double z, const vector<Poly> &polys,
                     const ClipperLib::Paths &ifcpolys);

  typedef struct { Vector2d from; Vector2d to; } infillline;
  vector<Poly> sortedpolysfromlines(const vector<infillline> &lines,
                                    const vector<Poly> &tofillpolys, double z);


  string info() const;

  };

class InfillSet
{
public:
    InfillSet(Settings &settings, const Vector2d &min, const Vector2d &max);
    ~InfillSet();

    Infill *normalInfill = nullptr;
    Infill *firstInfill = nullptr;
    Infill *firstFullInfill = nullptr;
    Infill *altInfill = nullptr;
    Infill *fullInfill = nullptr;
    vector<Infill *> skinInfills; // one for each number of skins up to max_skins
    Infill *skirtInfill = nullptr;
    Infill *decorInfill = nullptr;
    Infill *supportInfill = nullptr;
    Infill *thinInfill = nullptr;

};


