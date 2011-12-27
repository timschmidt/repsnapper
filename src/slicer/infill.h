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

using namespace std; 
using namespace vmml;

class Poly;

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


class Infill
{

 public:
  Infill();
  /* ~Infill(){}; */
  
  vector<Vector2d> infill;
  
  virtual void calcInfill(Poly *poly);

  uint getSize() const {return infill.size();};
  void printinfo() const;
};


class ZigZagInfill : public Infill
{
 public:
  ZigZagInfill() {Infill();};
  ZigZagInfill(double fillRatio){ZigZagInfill(); this->fillRatio = fillRatio;};
  ~ZigZagInfill(){};

  double fillRatio;
  void calcInfill(Poly *poly);
  void calcInfill(Poly *poly, const Vector2d startpoint, 
		  double startAngle, const double fillRatio);

};


class ParallelInfill : public Infill
{
 public:
  ParallelInfill(){Infill();};
  ParallelInfill(double fillRatio){ParallelInfill(); this->fillRatio = fillRatio;};
  ~ParallelInfill(){};

  double fillRatio;
  void calcInfill(Poly *poly);
  void calcInfill(Poly *poly, 
		  double startAngle, const double fillRatio);

};
