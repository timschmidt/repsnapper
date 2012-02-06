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

#include <vector>
//#include <list>

#include "stdafx.h"


struct printline
{
  Vector3d from, to;
  double speed;
  double extrusionfactor; 
};


// a bunch of printlines: lines with feedrate
// optimize for corners etc.
class Printlines
{

  struct line
  {
    Vector2d from, to;
    double speed; // mm/min(!)
    double feedrate; // relative extrusion feedrate 
    double angle; // angle of line
  };

  vector<struct line> lines;

  double z;

  void addPoly(const Poly poly, int startindex=0, double speed=1);
  void addLine(Vector2d from, Vector2d to, double speed=1, double feedrate=1.0);

 public:
  Printlines(){};
  ~Printlines(){clear();};
  
  void clear(){lines.clear();};

  Vector2d lastPoint() const;

  void makeLines(const vector<Poly> polys, 
		 Vector2d &startPoint, 
		 bool displace_startpoint, 
		 double minspeed, double maxspeed,
		 double linewidth, double linewidthratio, double optratio,
		 bool linelengthsort = false);
    
  void optimize(double minspeed, double maxspeed,
		double linewidth, double linewidthratio, double optratio);

  // slow down to total time needed (cooling)
  void slowdownTo(double totalseconds);

  // keep movements inside polys when possible (against stringing)
  void clipMovements(const vector<Poly> polys, double maxerr=0.0001);

  void getLines(vector<Vector2d> &lines) const;
  void getLines(vector<Vector3d> &lines) const;
  void getLines(vector<printline> &plines) const;

  uint size() const {return lines.size(); };

  double totalLength() const;
  double totalSeconds() const;
  double totalSecondsExtruding() const;

  string info() const;
  void setZ(double z) {this->z=z;};
  double getZ() const {return z;};
  

  string GCode(Vector3d &lastpos, double &E, double feedrate, double speed) const;

 private:
  void optimizeLinedistances(double maxdist);
  void mergelines(line &l1, line &l2, double maxdist);
  double distance(const Vector2d p, const line l2) const;
  void optimizeCorners(double linewidth, double linewidthratio, double optratio);
  bool capCorner(line &l1, line &l2, double linewidth, double linewidthratio, 
		 double optratio);

  uint divideline(uint lineindex, const vector<Vector2d> points);
  uint divideline(uint lineindex, const double t);

  double angle(const Vector2d p) const;
  double angle(const line l) const;
  double angle(const line l1, const line l2) const;
  double lengthSq(const line l) const;
  double length(const line l) const;

  string GCode(line l, Vector3d &lastpos, double &E, double feedrate, double speed) const;

  typedef vector<line>::const_iterator lineCIt ;
  typedef vector<line>::iterator lineIt ;
  //list<line>::iterator lIt;
};
