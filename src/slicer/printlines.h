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

#include "../stdafx.h"
#include "../settings.h"
#include "../gcode/command.h"


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
    virtual ~PLine(){}

    PLineArea area = UNDEF;
    vmml::vector<M, double> from, to;

    double speed;

    uint extruder_no;
    double lifted;

    short arc; // -1: ccw arc, 1: cw arc, 0: not an arc
    double angle; // angle of line (in 2d lines), or arc angle
    vmml::vector<M, double> arccenter;

    double absolute_extrusion; // additional absolute extrusion /mm (retract/repush f.e.)

    bool has_absolute_extrusion() const {return (abs(absolute_extrusion)>0.00001);}

    vmml::vector<M, double> dir() const {
        return vmml::vector<M, double>(to) - vmml::vector<M, double>(from);
    }
    vmml::vector<M, double> splitpoint(double at_length) const;

    double time() const;
    double lengthSq() const;
    double length() const;

    bool is_command() const {return (area == COMMAND);}
    virtual bool is_move() const = 0;
    virtual bool is_noop() const {return false;}
    virtual double getFeedratio() {return 0.;}

    void move_to(const vmml::vector<M, double> &from_,
                 const vmml::vector<M, double> &to_);

    vector< PLine<M> *> division(const vmml::vector<M, double> &point) const;
    vector< PLine<M> *> division(
            const vector<vmml::vector<M, double>> &points) const;

protected:
    void calcangle();
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

    Command command;

    double extrusion; // total extrusion in mm of filament

    Vector3d arcIJK() const;  // if is an arc

    int getCommands(Vector3d &lastpos, vector<Command> &commands,
                    const double &minspeed, const double &movespeed,
                    const double &minZspeed, const double &maxZspeed,
                    const double &maxAOspeed, bool useTCommand) const;

    bool is_move() const {return (abs(extrusion) < 0.00001);}
    // // not used
    // string GCode(Vector3d &lastpos, double &lastE, double feedrate,
    // 	       double minspeed, double maxspeed, double movespeed,
    // 	       bool relativeE) const;

    void addAbsoluteExtrusionAmount(double amount, double max_absspeed, double time=0);
    double addMaxAbsoluteExtrusionAmount(double max_absspeed);

    double max_abs_speed(double max_espeed, double max_absspeed) const;

    string info() const;

protected:
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
    PLine2(const PLine2 &rhs);
    PLine2(PLineArea area_, const uint extruder_no,
           const Vector2d &from, const Vector2d &to, double speed,
           double feedratio, double lifted = 0.);
    // arc
    static PLine2 *Arc(PLineArea area_, const uint extruder_no,
                       const Vector2d &from, const Vector2d &to, double speed,
                       double feedratio, const Vector2d &arccenter, bool ccw,
                       double lifted = 0.);

    double angle_to(const PLine2 &rhs) const;

    bool is_move() const {return (abs(feedratio) < 0.00001);}
    bool is_noop() const;

    double getFeedratio() {return feedratio;}

    double distanceFromChord() const;

    string info() const;

protected:
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
    void getLinesTo(vector<PLine<2> *> &lines, ulong startindex,
                    double movespeed) const;

    double getPriority() const {return priority;}
    double getSpeedfactor() const {return speedfactor;}
    ulong   getDisplacedStart(ulong start) const;
    uint   getExtruderNo() const {return  extruder_no;}
    string info() const;
};


typedef struct {
    ulong movestart, moveend, tractstart, pushend;
    void add(ulong a) {
        movestart+=a; moveend+=a; tractstart+=a; pushend+=a;
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
    double lineWidth; //  approx. average linewidth

    string name;

    /* void addPoly(PLineArea area, vector<PLine2> &lines,  */
    /* 	       const Poly &poly, int startindex=0,  */
    /* 	       double speed=1, double movespeed=1); */
    void addLine(PLineArea area, uint extruder_no, vector<PLine<2> *> &lines,
                 const Vector2d &from, const Vector2d &to,
                 double speed=1, double movespeed=1, double feedratio=1.0) const;

public:
    Printlines(const Layer * layer, Settings *settings, double z_offset=0);
    ~Printlines(){ clear(); }

    void clear();

    Settings *settings;
    const Layer * layer;

#ifdef USECAIRO
    Cairo::RefPtr<Cairo::ImageSurface> overhangs_surface;
#endif

    void setName(string s){name=s;}

    Vector2d lastPoint() const;

    void addPolys(PLineArea area,	const vector<Poly> &polys,
                  bool displace_start,
                  double maxspeed, double min_time = 0);

    double makeLines(Vector2d &startPoint, vector<PLine<2> *> &lines) const;

#if 0
    void oldMakeLines(PLineArea area,
                      const vector<Poly> &polys,
                      bool displace_startpoint,
                      Vector2d &startPoint,
                      vector<PLine2> &lines,
                      double maxspeed = 0);
#endif

    void optimize(double slowdowntime,
                  double cornerradius,
                  vector<PLine<2> *> &lines);

    ulong makeArcs(vector<PLine<2> *> &lines) const;
    ulong makeIntoArc(ulong fromind, ulong toind, vector<PLine<2> *> &lines) const;
    ulong makeIntoArc(const Vector2d &center, ulong fromind, ulong toind,
                      vector<PLine<2> *> &lines) const;

    ulong roundCorners(double maxdistance, double minarclength,
                       vector<PLine<2> *> &lines) const;
    ulong makeCornerArc(double maxdistance, double minarclength,
                        ulong ind, vector<PLine<2> *> &lines) const;

    static bool find_nextmoves(double minlength, ulong startindex,
                               AORange &range,
                               const vector<PLine<3> *> &lines);
    static uint makeAntioozeRetract(vector<PLine<3> *> &lines,
                                    Settings *settings,
                                    ViewProgress * progress = NULL);
    static uint insertAntioozeHaltBefore(ulong index, double amount, double speed,
                                         vector<PLine<3> *> &lines);

    static double length(const vector<PLine<3> *> &lines, ulong from, ulong to);

    static double time  (const vector<PLine<3> *> &lines, ulong from, ulong to);

    // slow down to total time needed (cooling)
    double slowdownTo(double totalseconds, vector<PLine<2> *> &lines) ; // returns slowdownfactor
    void setSpeedFactor(double speedfactor, vector<PLine<2> *> &lines) const;

    // keep movements inside polys when possible (against stringing)
    static void clipMovements(const vector<Poly> &polys, vector<PLine<2>*> &lines,
                              bool findnearest, double maxerr=0.0001);

    void getLines(const vector<PLine2> &lines,
                  vector<Vector2d> &linespoints) const;
    void getLines(const vector<PLine2> &lines,
                  vector<Vector3d> &linespoints) const;
    void getLines(const vector<PLine<2> *> &lines,
                  vector<PLine<3> *> &plines, double extrusion_per_mm) const;

    static double totalLength(const vector<PLine2> &lines);
    static double totalLength(const vector<PLine3> &lines);
    static double totalSeconds(const vector<PLine<2>*> &lines);
    static double totalSecondsExtruding(const vector<PLine<2>*> &lines);

    static double total_Extrusion(const vector< PLine3 > &lines);
    static double total_rel_Extrusion(const vector< PLine3 > &lines);
    static double total_abs_Extrusion(const vector< PLine3 > &lines);

    // every added poly will set this
    void setZ(double z) {this->z = z + Zoffset;}
    double getZ() const {return z;}

    double getSlowdownFactor() const {return slowdownfactor;}

    static void toCommands(const vector<PLine<3> *> &plines,
                           Settings *settings,
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


    template <size_t M>
    static void replace(vector< PLine<M> *> &lines,
                        ulong lineindex,
                        const vector< PLine<M> *> &newlines);

    // insert point at length
    template <size_t M>
    static uint divideline(ulong lineindex,
                           const double length,
                           vector< PLine<M> *> &lines);

    // insert single point
    template <size_t M>
    static uint divideline(ulong lineindex,
                           const vmml::vector<M, double> &point,
                           vector< PLine<M> *> &lines);
    // insert points
    template <size_t M>
    static uint divideline(ulong lineindex,
                           const vector< vmml::vector<M, double> > &points,
                           vector< PLine<M> *> &lines);

    static int distribute_AntioozeAmount(double AOamount, double AOspeed,
                                         ulong fromline, ulong &toline,
                                         vector<PLine<3> *> &lines,
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

template<>
uint Printlines::divideline(ulong lineindex,
                            const double length,
                            vector< PLine<3> *> &lines);


template <size_t M>
double PLine<M>::length() const
{
  if (area == COMMAND) return 0;
  if (!arc)
    return from.distance(to);
  else
    return from.distance(arccenter) * abs(angle);
}
