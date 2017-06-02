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
#include "settings.h"
#include "gcode/command.h"


class PLine2; // see below
class ViewProgress;

enum PLineArea { UNDEF, SHELL, SKIN, INFILL, SUPPORT, SKIRT, BRIDGE, COMMAND };
const string AreaNames[] = { _(""), _("Shell"), _("Skin"), _("Infill"),
			     _("Support"),  _("Skirt"), _("Bridge"), _("Command") };


// generic single printline (2d or 3d)
template <size_t M>
class PLine
{
public:
  PLineArea area;
  vmml::vector<M, double> from, to;

  double speed;

  uint extruder_no;
  double lifted;

  short arc; // -1: ccw arc, 1: cw arc, 0: not an arc
  double angle; // angle of line (in 2d lines), or arc angle
  vmml::vector<M, double> arccenter;


  double absolute_extrusion; // additional absolute extrusion /mm (retract/repush f.e.)

  bool has_absolute_extrusion() const {return (abs(absolute_extrusion)>0.00001);}

  // virtual vector< PLine > division(double length) const;

  vmml::vector<M, double> dir() const {
      return vmml::vector<M, double>(to) - vmml::vector<M, double>(from);
  }
  vmml::vector<M, double> splitpoint(double at_length) const;

  virtual double time() const;
  virtual double lengthSq() const;
  virtual double length() const;

  bool is_command() const {return (area == COMMAND);}

  void move_to(const vmml::vector<M, double> &from_,
	       const vmml::vector<M, double> &to_);

protected:
  void calcangle();
  // virtual bool is_move() const;
  // virtual string info() const;
};



// 3D printline for making GCode
class PLine3 : public PLine<3>
{
 public:
  PLine3(PLineArea area_,  const uint extruder_no,
   	 const Vector3d &from_, const Vector3d &to_,
   	 double speed_, double extrusion_);
  PLine3(const PLine3 &rhs);
  PLine3(const PLine2 &pline, double z, double extrusion_per_mm_);
  PLine3(const Command &command);
  ~PLine3(){}

  Command command;

  double extrusion; // total extrusion in mm of filament

  Vector3d arcIJK() const;  // if is an arc

  int getCommands(Vector3d &lastpos, vector<Command> &commands,
		  const double &minspeed, const double &movespeed,
		  const double &minZspeed, const double &maxZspeed,
		  const double &maxAOspeed, bool useTCommand) const;

  // // not used
  // string GCode(Vector3d &lastpos, double &lastE, double feedrate,
  // 	       double minspeed, double maxspeed, double movespeed,
  // 	       bool relativeE) const;

  void addAbsoluteExtrusionAmount(double amount, double max_absspeed, double time=0);
  double addMaxAbsoluteExtrusionAmount(double max_absspeed);

  double max_abs_speed(double max_espeed, double max_absspeed) const;

  vector< PLine3 > division(double length) const;
  vector< PLine3 > division(const Vector3d &point) const;
  vector< PLine3 > division(const vector<Vector3d> &points) const;
  bool is_move() const {return (abs(extrusion) < 0.00001);}

  string info() const;
};


// single 2D printline
class PLine2 : public PLine<2>
{
  friend class PLine3;
  friend class Printlines;

  // non-arc
  double feedratio; // relative extrusion feedrate, normally 1

 public:
  PLine2();
  PLine2(PLineArea area_, const uint extruder_no,
	const Vector2d &from, const Vector2d &to, double speed,
	double feedratio, double lifted = 0.);
  // arc
  PLine2(PLineArea area_, const uint extruder_no,
	const Vector2d &from, const Vector2d &to, double speed,
	double feedratio, const Vector2d &arccenter, bool ccw, double lifted = 0.);

  PLine2(const PLine2 &rhs);

  ~PLine2(){};

  vector< PLine2 > division(double length) const;
  vector< PLine2 > division(const Vector2d &point) const;
  vector< PLine2 > division(const vector<Vector2d> &points) const;

  double angle_to(const PLine2 &rhs) const;
  bool is_noop() const;
  bool is_move() const {return (abs(feedratio) < 0.00001);}
  string info() const;
};



class Printlines;

class PrintPoly
{
  friend class Printlines;

  PrintPoly(const Poly &poly, const Printlines * printlines,
	    double speed, double overhangspeed, double min_time,
	    bool displace_start, PLineArea area);

  Poly *m_poly;
  const Printlines * printlines;
  PLineArea area;
  double speed;
  double min_time;
  bool   displace_start;
  int    overhangingpoints;
  double priority; // distance for next poly search will be divided by this
  double length;
  double speedfactor; // is set after slowdown
  uint   extruder_no;

 public:
  ~PrintPoly();
  void getLinesTo(vector<PLine2> &lines, int startindex,
		  double movespeed) const;

  double getPriority() const {return priority;};
  double getSpeedfactor() const {return speedfactor;};
  uint   getDisplacedStart(uint start) const;
  uint   getExtruderNo() const {return  extruder_no;};
  string info() const;
};


typedef struct {
  uint movestart, moveend, tractstart, pushend;
  void add(uint a) {
    movestart+=a, moveend+=a; tractstart+=a; pushend+=a;
  }
} AORange;


// a bunch of printlines: lines with feedrate
// optimize for corners etc.
class Printlines
{
  friend class PrintPoly;

  vector<PrintPoly *> printpolys;

  double z;
  double Zoffset; // global offset for generated PLine3s, always added at setZ()

  string name;

  /* void addPoly(PLineArea area, vector<PLine2> &lines,  */
  /* 	       const Poly &poly, int startindex=0,  */
  /* 	       double speed=1, double movespeed=1); */
  void addLine(PLineArea area, uint extruder_no, vector<PLine2> &lines,
	       const Vector2d &from, const Vector2d &to,
	       double speed=1, double movespeed=1, double feedratio=1.0) const;


 public:
  Printlines(const Layer * layer, const Settings *settings, double z_offset=0);
  ~Printlines(){ clear(); };

  void clear();

  const Settings *settings;
  const Layer * layer;

  Cairo::RefPtr<Cairo::ImageSurface> overhangs_surface;

  void setName(string s){name=s;};

  Vector2d lastPoint() const;

  void addPolys(PLineArea area,	const vector<Poly> &polys,
		bool displace_start,
		double maxspeed = 0, double min_time = 0);

  double makeLines(Vector2d &startPoint, vector<PLine2> &lines);

#if 0
  void oldMakeLines(PLineArea area,
		    const vector<Poly> &polys,
		    bool displace_startpoint,
		    Vector2d &startPoint,
		    vector<PLine2> &lines,
		    double maxspeed = 0);
#endif

  void optimize(double linewidth,
		double slowdowntime,
		double cornerradius,
		vector<PLine2> &lines);

  uint makeArcs(double linewidth,
		vector<PLine2> &lines) const;
  uint makeIntoArc(guint fromind, guint toind, vector<PLine2> &lines) const;
  uint makeIntoArc(const Vector2d &center, guint fromind, guint toind,
		   vector<PLine2> &lines) const;

  uint roundCorners(double maxdistance, double minarclength, vector<PLine2> &lines) const;
  uint makeCornerArc(double maxdistance, double minarclength,
		     uint ind, vector<PLine2> &lines) const;

  static bool find_nextmoves(double minlength, uint startindex,
			     AORange &range,
			     const vector< PLine3 > &lines);
  static uint makeAntioozeRetract(vector< PLine3 > &lines,
				  const Settings &settings,
				  ViewProgress * progress = NULL);
  static uint insertAntioozeHaltBefore(uint index, double amount, double speed,
				       vector< PLine3 > &lines);

  inline static double length(const vector< PLine3 > &lines, uint from, uint to)
  {
    double totaldistance = 0;
    for (uint j = from; j <= to; j++)
      totaldistance += lines[j].length();
    return totaldistance;
  }

  inline static double time  (const vector< PLine3 > &lines, uint from, uint to)
  {
    double totaltime= 0;
    for (uint j = from; j <= to; j++)
      totaltime += lines[j].time();
    return totaltime;
  }

  // slow down to total time needed (cooling)
  double slowdownTo(double totalseconds, vector<PLine2> &lines) ; // returns slowdownfactor
  void setSpeedFactor(double speedfactor, vector<PLine2> &lines) const;

  // keep movements inside polys when possible (against stringing)
  void clipMovements(const vector<Poly> &polys, vector<PLine2> &lines,
		     bool findnearest, double maxerr=0.0001) const;

  void getLines(const vector<PLine2> &lines,
		vector<Vector2d> &linespoints) const;
  void getLines(const vector<PLine2> &lines,
		vector<Vector3d> &linespoints) const;
  void getLines(const vector<PLine2> &lines,
		vector<PLine3> &plines, double extrusion_per_mm) const;

  static double totalLength(const vector<PLine2> &lines);
  static double totalLength(const vector<PLine3> &lines);
  static double totalSeconds(const vector<PLine2> &lines);
  static double totalSecondsExtruding(const vector<PLine2> &lines);

  static double total_Extrusion(const vector< PLine3 > &lines);
  static double total_rel_Extrusion(const vector< PLine3 > &lines);
  static double total_abs_Extrusion(const vector< PLine3 > &lines);

  // every added poly will set this
  void setZ(double z) {this->z = z + Zoffset;};
  double getZ() const {return z;};

  double getSlowdownFactor() const {return slowdownfactor;};

  static void getCommands(const vector<PLine3> &plines,
			  const Settings &settings,
			  GCodeState &state,
			  ViewProgress * progress = NULL);

  string info() const;


 private:
  void optimizeLinedistances(double maxdist, vector<PLine2> &lines) const;
  void mergelines(PLine2 &l1, PLine2 &l2, double maxdist) const;
  double distance(const Vector2d &p, const PLine2 &l2) const;
  void optimizeCorners(double linewidth, double linewidthratio, double optratio,
		       vector<PLine2> &lines) const;
  bool capCorner(PLine2 &l1, PLine2 &l2, double linewidth, double linewidthratio,
		 double optratio) const;


  static void replace(vector< PLine2 > &lines,
		      uint lineindex,
		      const vector< PLine2 > &newlines){
    lines[lineindex] = newlines[0];
    if (newlines.size() > 1)
      lines.insert(lines.begin() + lineindex+1, newlines.begin()+1, newlines.end());
  }
  static void replace(vector< PLine3 > &lines,
		      uint lineindex,
		      const vector< PLine3 > &newlines){
    lines[lineindex] = newlines[0];
    if (newlines.size() > 1)
      lines.insert(lines.begin() + lineindex+1, newlines.begin()+1, newlines.end());
  }

  // insert single point
  static uint divideline(uint lineindex,
			 const Vector2d &point,
			 vector< PLine2 > &lines);
  static uint divideline(uint lineindex,
			 const Vector3d &point,
			 vector< PLine3 > &lines);
  // insert points
  static uint divideline(uint lineindex,
			 const vector< Vector2d > &points,
			 vector< PLine2 > &lines);
  static uint divideline(uint lineindex,
			 const vector< Vector3d > &points,
			 vector< PLine3 > &lines);
  // insert point at length
  static uint divideline(uint lineindex,
			 const double length,
			 vector< PLine3 > &lines);

  static int distribute_AntioozeAmount(double AOamount, double AOspeed,
				       uint fromline, uint &toline,
				       vector< PLine3 > &lines,
				       double &havedistributed);

  Vector2d arcCenter(const PLine2 &l1, const PLine2 &l2,
		     double maxerr) const;

  double slowdownfactor; // result of slowdown/setspeedfactor. not used here.

  /* string GCode(PLine2 l, Vector3d &lastpos, double &E, double feedrate,  */
  /* 	       double minspeed, double maxspeed, double movespeed,  */
  /* 	       bool relativeE) const; */

  typedef vector<PLine3>::const_iterator line3CIt ;
  typedef vector<PLine3>::iterator line3It ;
  typedef vector<PLine2>::const_iterator lineCIt ;
  typedef vector<PLine2>::iterator lineIt ;
  //list<line>::iterator lIt;

};
