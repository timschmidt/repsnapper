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

class PLine; // see below



// 3D printline for making GCode
class PLine3
{
 public: 
  PLine3(PLine pline, double z);
  ~PLine3(){};

  Vector3d from, to;
  double speed;
  double extrusionfactor; 
  double absolute_extrusion; // additional absolute extrusion /mm (retract/repush f.e.)
  Vector3d arcIJK;  // if is an arc
  short arc; // -1: ccw arc, 1: cw arc, 0: not an arc
  double arcangle;

  vector<Command> getCommands(Vector3d &lastpos, double extrusion,
			      double minspeed, double maxspeed, double movespeed) const;
  // not used
  string GCode(Vector3d &lastpos, double &lastE, double feedrate, 
	       double minspeed, double maxspeed, double movespeed, 
	       bool relativeE) const;
  double length() const;
  string info() const;
};


// single 2D printline
class PLine
{
  friend class PLine3;
  friend class Printlines;
  PLine(const Vector2d from, const Vector2d to, double speed, 
	double feedrate);
  PLine(const Vector2d from, const Vector2d to, double speed, 
	double feedrate, short arc, Vector2d arccenter, double angle);
  Vector2d from, to;
  double speed; // mm/min (!)
  double feedrate; // relative extrusion feedrate 
  double absolute_feed; // additional absolute feed /mm (retract/repush f.e.)
  double angle; // angle of line
  Vector2d arccenter;
  short arc;  // -1: ccw arc, 1: cw arc, 0: not an arcx

  void addAbsoluteExtrusionAmount(double amount);
  double calcangle() const;
  double calcangle(const PLine rhs) const;
  double lengthSq() const;
  double length() const;
  double time() const;  // time in minutes
  PLine3 getPrintline(double z) const;
  bool is_noop() const;
  string info() const;
 public: 
  ~PLine(){};
};


// a bunch of printlines: lines with feedrate
// optimize for corners etc.
class Printlines
{
    
  vector<PLine> lines;

  double z;
  double Zoffset; // global offset for generated PLine3s, always added at setZ()

  string name;

  void addPoly(const Poly poly, int startindex=0, 
	       double speed=1, double movespeed=1);
  void addLine(Vector2d from, Vector2d to, 
	       double speed=1, double movespeed=1, double feedrate=1.0);

 public:
  Printlines(double z_offset=0);
  ~Printlines(){clear();};
  
  void clear(){lines.clear();};
  void setName(string s){name=s;};

  Vector2d lastPoint() const;

  void makeLines(const vector<Poly> polys,
		 vector<Poly> *clippolys,
		 Vector2d &startPoint, 
		 bool displace_startpoint, 
		 double minspeed, double maxspeed, double movespeed,
		 double linewidth, double linewidthratio, double optratio,
		 double maxArcAngle, bool linelengthsort,
		 double AOmindistance, double AOspeed,
		 double AOamount, double AOrepushratio);
    
  void optimize(double minspeed, double maxspeed, double movespeed,
		double linewidth, double linewidthratio, double optratio,
		double maxArcAngle,
		double AOmindistance, double AOspeed,
		double AOamount, double AOrepushratio);

  guint makeArcs(double maxAngle);
  guint makeIntoArc(guint fromind, guint toind);


  // slow down to total time needed (cooling)
  double slowdownTo(double totalseconds); // returns speedfactor
  void setSpeedFactor(double speedfactor);

  // keep movements inside polys when possible (against stringing)
  void clipMovements(vector<Poly> *polys, double maxerr=0.01);

  void getLines(vector<Vector2d> &linespoints) const;
  void getLines(vector<Vector3d> &linespoints) const;
  void getLines(vector<PLine3> &plines) const;

  uint size() const {return lines.size(); };

  double totalLength() const;
  double totalSeconds() const;
  double totalSecondsExtruding() const;

  // every added poly will set this
  void setZ(double z) {this->z = z + Zoffset;};
  double getZ() const {return z;};
  
  /* string GCode(Vector3d &lastpos, double &E, double feedrate,  */
  /* 	       double minspeed, double maxspeed, double movespeed,  */
  /* 	       bool relativeE) const; */
  string info() const;

 private:
  void optimizeLinedistances(double maxdist);
  void mergelines(PLine &l1, PLine &l2, double maxdist);
  double distance(const Vector2d p, const PLine l2) const;
  void optimizeCorners(double linewidth, double linewidthratio, double optratio);
  bool capCorner(PLine &l1, PLine &l2, double linewidth, double linewidthratio, 
		 double optratio);

  uint makeAntioozeRetraction(double AOmindistance, double AOspeed,
			      double AOamount, double AOrepushratio);

  uint divideline(uint lineindex, const vector<Vector2d> points);
  uint divideline(uint lineindex, const double t);


  Vector2d arcCenter(const PLine l1, const PLine l2, 
		     double maxerr) const;
  
  /* string GCode(PLine l, Vector3d &lastpos, double &E, double feedrate,  */
  /* 	       double minspeed, double maxspeed, double movespeed,  */
  /* 	       bool relativeE) const; */

  typedef vector<PLine>::const_iterator lineCIt ;
  typedef vector<PLine>::iterator lineIt ;
  //list<line>::iterator lIt;
};
