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

#include "printlines.h"
#include "poly.h"
#include "layer.h"
#include "gcode/gcodestate.h"
#include "ui/progress.h"



//////// Generic PLine functions

template <size_t M>
double PLine<M>::length() const
{
  if (area == COMMAND) return 0;
  if (!arc)
    return from.distance(to);
  else
    return from.distance(arccenter) * abs(angle);
}

template <size_t M>
double PLine<M>::lengthSq() const
{
  if (area == COMMAND) return 0;
  if (!arc)
    return from.squared_distance(to);
  else
    return from.squared_distance(arccenter) * angle*angle;
}

template <size_t M>
double PLine<M>::time() const
{
  if (area == COMMAND) return 0;
  assert(speed >= 0);
  return length()/speed;
}

template <size_t M>
vmml::vector<M, double> PLine<M>::splitpoint(double at_length) const
{
  if (area == COMMAND) return vmml::vector<M, double>::ZERO;
  const double lratio = at_length/length();
  if (!arc)
    return from + dir() * lratio;
  else {
    vmml::vector<M, double> point = from;
    const double rangle = angle * lratio;
    if (M == 3) {
      Vector2d from_rot(from.x(), from.y());
      rotate(from_rot, Vector2d(arccenter.x(), arccenter.y()), rangle, (arc < 0));
      double dz = to.z()-from.z();
      point.set(from_rot.x(), from_rot.y(), from.z() + dz*lratio);
    } else {
      point = rotated(from, Vector2d(arccenter.x(), arccenter.y()), rangle, (arc < 0));
    }
    return point;
  }
}

template <size_t M>
void PLine<M>::move_to(const vmml::vector<M, double> &from_,
		       const vmml::vector<M, double> &to_)
{
  if (area == COMMAND) return;
  from = from_;
  to   = to_;
  calcangle();
}

template <size_t M>
void PLine<M>::calcangle()
{
  if (area == COMMAND) return;
  if (arc == 0){
    angle = planeAngleBetween(Vector2d(1,0), Vector2d(dir()));
  } else {
    angle = Command::calcAngle(from - arccenter, to - arccenter, arc == -1);
  }
}



///////////// PLine3: single 3D printline //////////////////////

PLine3::PLine3(PLineArea area_,  const uint extruder_no_,
	       const Vector3d &from_, const Vector3d &to_,
	       double speed_, double extrusion_)
: extrusion(extrusion_)
{
  area = area_;
  from = from_;
  to = to_;
  speed = speed_;
  arc = 0;
  angle = 0;
  extruder_no = extruder_no_;
  absolute_extrusion = 0;
  lifted = 0;
}

PLine3::PLine3(const PLine3 &rhs)
{
  area = rhs.area;
  command = rhs.command;
  from = rhs.from;
  to = rhs.to;
  speed = rhs.speed;
  arc = rhs.arc;
  arccenter = rhs.arccenter;
  angle = rhs.angle;
  extruder_no = rhs.extruder_no;
  extrusion = rhs.extrusion;
  absolute_extrusion = rhs.absolute_extrusion;
  lifted = rhs.lifted;
}

PLine3::PLine3(const PLine2 &pline, double z, double extrusion_per_mm)
{
  lifted             = pline.lifted;
  area               = pline.area;
  from               = Vector3d(pline.from.x(), pline.from.y(), z);
  to                 = Vector3d(pline.to.x(),   pline.to.y(),   z);
  speed              = pline.speed;
  absolute_extrusion = pline.absolute_extrusion;
  arc                = (pline.is_noop() || pline.is_move() || pline.is_command())
                          ? 0 : pline.arc;
  arccenter          = Vector3d(pline.arccenter.x(), pline.arccenter.y(), z);
  angle              = pline.angle;
  extruder_no        = pline.extruder_no;
  extrusion          = pline.feedratio * extrusion_per_mm * length();
}

// to insert an explicit Command into an array of lines
PLine3::PLine3(const Command &command_)
{
  area = COMMAND;
  command = command_;
  extruder_no = command.extruder_no;
  arc = 0;
}


Vector3d PLine3::arcIJK() const
{
  assert(arc);
  return Vector3d(arccenter.x(), arccenter.y(), to.z()) - from;
}

//nonsense ???
double PLine3::max_abs_speed(double max_Espeed, double max_AOspeed) const
{
  // if have absolute extrusion, allow AOspeed
  double allowedspeed = max_Espeed;
  if (abs(absolute_extrusion) > 0.01) {
    allowedspeed = max_AOspeed;
  }
  double t = time();
  if (t != 0) {
    double tot_speed = abs(extrusion+absolute_extrusion) / t;
    return speed * allowedspeed / tot_speed;
  } else  // halting command
    return allowedspeed;
}

int PLine3::getCommands(Vector3d &lastpos, vector<Command> &commands,
			const double &minspeed, const double &movespeed,
			const double &minZspeed, const double &maxZspeed,
			const double &maxAOspeed,
			bool useTCommand) const
{
  if (area == COMMAND) { // it is an explicit command line
    commands.push_back(command);
    return 1;
  }


  const Vector3d lift (0,0,lifted);
  const Vector3d lifted_from = from + lift;
  const Vector3d lifted_to   = to + lift;

  int command_count = 0;

  // insert move first if necessary
  if (lastpos.squared_distance(lifted_from) > 0.005) {
    uint extruder = extruder_no;
    // keep last extruder for move
    if (commands.size()>0) extruder = commands.back().extruder_no;
    PLine3 move3(area, extruder, lastpos, lifted_from, movespeed, 0);
    // get recursive ...
    vector<Command> movecommands;
    command_count += move3.getCommands(lastpos, movecommands,
				       minspeed, movespeed,
				       minZspeed, maxZspeed,
				       maxAOspeed, useTCommand);
    commands.insert(commands.end(), movecommands.begin(), movecommands.end());
    lastpos = lifted_from;
  }

  // insert extruder change command
  if (useTCommand)
    if (commands.size()>0 &&  commands.back().extruder_no != extruder_no) {
      Command extr_change(SELECTEXTRUDER, extruder_no);
      commands.push_back(extr_change);
      Command extr_reset(RESET_E);
      commands.push_back(extr_reset);
      command_count += 2;
    }


  const double travel_length = length();
  double extrudedMaterial = extrusion;

  double travel_speed = speed;


  if (abs(absolute_extrusion) < 0.00001)
    travel_speed = max(minspeed, speed); // in case speed is too low

  if (!std::isnan(absolute_extrusion))
    // allowed to push/pull at arbitrary speed
    extrudedMaterial += absolute_extrusion;
  else
    cerr << "absolute Extrusion (retract) is NaN!" << endl << info() << endl;

  // slow down if too fast for z axis
  const double dZ = lifted_to.z() - lifted_from.z();
  if (abs(dZ) > 0.00001) {
    const double xyztime = travel_length / travel_speed;
    const double zspeed = abs(dZ / xyztime);
    if ( zspeed > maxZspeed ) {
      travel_speed *= maxZspeed / zspeed;
      // insert feedrate-only command to avoid deceleration on z move
      Command preFeedrate(COORDINATEDMOTION, lifted_from, 0, travel_speed);
      commands.push_back(preFeedrate);
      command_count++;
    }
  }
  travel_speed = max(minZspeed, travel_speed);

  if (abs(travel_length) < 0.00001) // extrusion on halt
    travel_speed = maxAOspeed;

  Command command;
  if (arc)
    {
      GCodes gc = (arc==-1 ? ARC_CCW : ARC_CW);
      command = Command (gc, lifted_to, extrudedMaterial, travel_speed);
      command.arcIJK = arcIJK();
      ostringstream o;
      o << (int)(angle*180/M_PI) << "° ";
      if (arc<0) o << "c";
      o << "cw arc";
      command.comment += o.str();
    }
  else
    {
      command = Command (COORDINATEDMOTION, lifted_to, extrudedMaterial, travel_speed);
    }
  command.extruder_no = extruder_no;
  command.not_layerchange = (lifted != 0);
  command.abs_extr += absolute_extrusion;
  command.travel_length = travel_length;
  if (!command.hasNoEffect(lifted_from, 0, 0,true)) {
    commands.push_back(command);
    command_count++;
  }
  //else cerr << command.info() << endl;

  lastpos = lifted_to;
  return command_count;
}

// ??? really have to write all this twice?
vector<PLine2> PLine2::division(const Vector2d &point) const
{
  vector<Vector2d> points;
  if (area != COMMAND)
    points.push_back(point);
  return division(points);
}
vector<PLine3> PLine3::division(const Vector3d &point) const
{
  vector<Vector3d> points;
  if (area != COMMAND)
    points.push_back(point);
  return division(points);
}

vector<PLine2> PLine2::division(const vector<Vector2d> &points) const
{
  uint npoints = points.size();
  vector<PLine2> newlines;
  if (npoints == 0) return newlines;
  PLine2 l(*this);
  newlines.push_back(l);
  newlines.back().to = points[0];
  for (uint i = 0; i < npoints-1; i++) {
    newlines.push_back(l);
    newlines.back().move_to(points[i], points[i+1]);
  }
  newlines.push_back(l);
  newlines.back().from = points[npoints-1];
  double totlength = 0;
  for (uint i = 0; i < newlines.size(); i++){
    totlength += newlines[i].length();
  }
  for (uint i = 0; i < newlines.size(); i++){
    double factor;
    if (totlength>0)
      factor = newlines[i].length() / totlength;
    else
      factor = 1./newlines.size();
    newlines[i].absolute_extrusion *= factor;
    assert(!std::isnan(newlines[i].absolute_extrusion));

  }
  return newlines;
}
vector<PLine3> PLine3::division(const vector<Vector3d> &points) const
{
  uint npoints = points.size();
  vector<PLine3> newlines;
  if (npoints == 0) return newlines;
  PLine3 l(*this);
  double totlength = 0;
  newlines.push_back(l);
  newlines.back().to = points[0];
  totlength += newlines.back().length();
  for (uint i = 0; i < npoints-1; i++) {
    newlines.push_back(l);
    newlines.back().move_to(points[i], points[i+1]);
    totlength += newlines.back().length();
  }
  newlines.push_back(l);
  newlines.back().from = points[npoints-1];
  totlength += newlines.back().length();
  // normal extrusion adjusted to new length, but absolute extrusion is kept
  double newextrusion = extrusion * totlength / length();
  for (uint i = 0; i < newlines.size(); i++){
    double factor;
    if (totlength>0)
      factor = newlines[i].length() / totlength;
    else
      factor = 1./newlines.size();
    newlines[i].absolute_extrusion *= factor;
    newlines[i].extrusion = newextrusion * factor;
  }
  double totext = 0;
  for (uint i = 0; i < newlines.size(); i++)
    totext += newlines[i].extrusion;
  if (abs(totext/extrusion - totlength/length())>0.00000001)
    cerr << totext << " " << extrusion <<  " -- " <<totlength << " "<<length()<< endl;
  return newlines;
}


void PLine3::addAbsoluteExtrusionAmount(double amount, double max_absspeed, double ltime)
{
  if (area == COMMAND) return;
  absolute_extrusion += amount;
  // slowdown if max speed given and not enough time for absolute feed:
  const double feedt = abs(absolute_extrusion/max_absspeed);
  if (ltime == 0) ltime = time();
  if (ltime>0 && feedt > ltime ) // too fast
    speed *= ltime / feedt;
  //cerr << "added abs at speed " << ( absolute_extrusion)/time() << endl;
}
double PLine3::addMaxAbsoluteExtrusionAmount(double max_absspeed)
{
  if (area == COMMAND) return 0;
  const double maxamount = time() * max_absspeed - absolute_extrusion;
  absolute_extrusion += maxamount;
  //cerr << "added max abs at speed " << ( absolute_extrusion)/time() << endl;
  return maxamount;
}

string PLine3::info() const
{
  if (area == COMMAND) {
    return "command-line " + command.info();
  }
  ostringstream ostr;
  ostr << "line "<< AreaNames[area]
       << " " << from;
  if (to!=from) ostr << to;
  ostr << ", speed=" << speed
       << ", extrusion=" << extrusion << "mm, arc=" << arc;
  if (has_absolute_extrusion())
    ostr << ", abs_extr="<<absolute_extrusion;
  if (arc!=0)
    ostr << ", arcIJK=" << arcIJK();
  if (lifted != 0)
    ostr << " lifted=" << lifted;
  if (is_move())
    ostr << " move-only";
  return ostr.str();
}


///////////// PLine: single 2D printline //////////////////////

PLine2::PLine2()
:  feedratio(0)
{
  area = UNDEF;
  speed = 0;
  extruder_no = 0;
  lifted = 0;
  arc = 0;
  angle = 0;
  absolute_extrusion = 0;
}

PLine2::PLine2(PLineArea area_, const uint extruder_no_,
	       const Vector2d &from_, const Vector2d &to_, double speed_,
	       double feedratio_, double lifted_)
:  feedratio(feedratio_)
{
  area = area_;
  from = from_;
  to = to_;
  speed = speed_;
  arc = 0;
  calcangle();
  extruder_no = extruder_no_;
  lifted = lifted_;
  absolute_extrusion = 0;
}


// for arc line
PLine2::PLine2(PLineArea area_, const uint extruder_no_,
	       const Vector2d &from_, const Vector2d &to_, double speed_,
	       double feedratio_, const Vector2d &arccenter_, bool ccw,
	       double lifted_)
: feedratio(feedratio_)
{
  area = area_;
  from = from_;
  to   = to_;
  speed = speed_;
  if (feedratio == 0)
    arc = 0;
  else {
    arccenter = arccenter_;
    arc = ccw ? -1 : 1;
  }
  calcangle();
  absolute_extrusion = 0;
  lifted = lifted_;
  extruder_no = extruder_no_;
}

PLine2::PLine2(const PLine2 &rhs)
{
  area = rhs.area;
  from = rhs.from;
  to = rhs.to;
  speed = rhs.speed;
  arc = rhs.arc;
  arccenter = rhs.arccenter;
  angle = rhs.angle;
  feedratio = rhs.feedratio;
  absolute_extrusion = rhs.absolute_extrusion;
  extruder_no = rhs.extruder_no;
  lifted = rhs.lifted;
}

// double PLine2::calcangle() const
// {
//   assert(!arc);
//   return angleBetween(Vector2d(1,0), to-from);
// }
double PLine2::angle_to(const PLine2 &rhs) const
{
  return planeAngleBetween( dir(), rhs.dir() );
}


bool PLine2::is_noop() const
{
  return (lengthSq() < 0.0001 && feedratio == 0 && absolute_extrusion == 0);
}

string PLine2::info() const
{
  ostringstream ostr;
  ostr << "line "<< AreaNames[area] << " ";
  if (arc!=0) {
    if (arc==-1) ostr << "C";
    ostr << "CW arc ";
  }
  ostr << from;
  if (to!=from) ostr << to;
  ostr << " angle="<< (int)(angle*180/M_PI)<<"°"
       << " length="<< length() << " speed=" << speed
       << " feedratio=" << feedratio;
  if (arc!=0)
    ostr << " center=" << arccenter;
  ostr <<  " feedratio=" << feedratio << " abs.extr="<< absolute_extrusion;
  if (lifted !=0)
    ostr << " lifted=" << lifted;
  return ostr.str();
}



///////////// Printlines //////////////////////


Printlines::Printlines(const Layer * layer, const Settings * settings, double z_offset)
  : Zoffset(z_offset), name(""), slowdownfactor(1.)
{
  this->settings = settings;
  this->layer = layer;

  // save overhang polys of layer for point-in-overhang detection
  if (layer!=NULL) {
    Cairo::RefPtr<Cairo::Context>      context;
    vector<Poly> overhangs = layer->getOverhangs();
    rasterpolys(overhangs, layer->getMin(), layer->getMax(), layer->thickness/5,
		overhangs_surface, context);
  }
}

void Printlines::clear()
{
  for (vector<PrintPoly *>::iterator i = printpolys.begin(); i != printpolys.end(); i++)
    delete *i;
  printpolys.clear();
}

void Printlines::addLine(PLineArea area, uint extruder_no, vector<PLine2> &lines,
			 const Vector2d &from, const Vector2d &to,
			 double speed, double movespeed, double feedrate) const
{
  if (to==from) return;
  Vector2d lfrom = from;
  if (lines.size() > 0) {
    const Vector2d lastpos = lines.back().to;
    const bool extruder_change = (lines.back().extruder_no != extruder_no);
    if (extruder_change ||
	lfrom.squared_distance(lastpos) > 0.01) { // add moveline
      // use last extruder for move
      PLine2 move(area, lines.back().extruder_no, lastpos, lfrom, movespeed, 0);
      if (extruder_change || settings->get_boolean("Extruder","ZliftAlways")) {
	move.lifted = settings->get_double("Extruder","AntioozeZlift");
      }
      lines.push_back(move);
    } else {
      lfrom = lastpos;
    }
  }
  lines.push_back(PLine2(area, extruder_no, lfrom, to, speed, feedrate));
}


// // // // // // // // // // // // PrintPoly // // // // // // // // // // // //

PrintPoly::PrintPoly(const Poly &poly,
		     const Printlines * printlines_,
		     double speed_, double overhangspeed,
		     double min_time_,
		     bool displace_start_,
		     PLineArea area_)
  : printlines(printlines_), area(area_),
    speed(speed_), min_time(min_time_),
    displace_start(displace_start_),
    overhangingpoints(0), priority(1.), length(0), speedfactor(1.)
{
  extruder_no = printlines->settings->selectedExtruder;
  // Take a copy of the reference poly
  m_poly = new Poly(poly);
  m_poly->move(Vector2d(-printlines->settings->get_double("Extruder","OffsetX"),
			-printlines->settings->get_double("Extruder","OffsetY")));

  if (area==SHELL || area==SKIN) {
    priority *= 5; // may be 5 times as far away to get preferred as next poly
    for (uint j=0; j<m_poly->size();j++){
      if (getCairoSurfaceDatapoint(printlines->overhangs_surface,
				   printlines->layer->getMin(),
				   printlines->layer->getMax(),
				   m_poly->vertices[j]) != 0) {
	overhangingpoints++;
	priority /= 10; // must be 10 times nearer for each overhang point
      }
    }
  } else if (area==SKIRT) {
    priority *= 1000; // may be 1000 times as far away to get preferred
  } else if (area==SUPPORT) {
    priority *= 100;  // may be 100 times as far away to get preferred
  }
  length = m_poly->totalLineLength();
  if (overhangingpoints>0)
    speed = overhangspeed;
  if (min_time > 0 && speed > 0) {
    const double time = length / speed * 60;  // minutes -> seconds!
    //cerr << AreaNames[area] << ": " << time << " - " << speed ;
    if (time !=0 && time < min_time){
      speedfactor = time / min_time;
      speed *= speedfactor;
    }
    //cerr << " -> "<< speed << " = " << totlength / speed * 60<< endl;
  }
}

PrintPoly::~PrintPoly()
{
  delete m_poly;
}

void PrintPoly::getLinesTo(vector<PLine2> &lines, int startindex,
			   double movespeed) const
{
  vector<Vector2d> pvert;
  m_poly->makeLines(pvert,startindex);
  if (pvert.size() == 0) return;
  assert(pvert.size() % 2 == 0);
  for (uint i=0; i<pvert.size(); i+=2) {
    if (pvert[i+1].squared_distance(pvert[i]) > 0.001)
      printlines->addLine(area, extruder_no, lines, pvert[i], pvert[i+1],
			  speed, movespeed, m_poly->getExtrusionFactor());
  }
}

uint PrintPoly::getDisplacedStart(uint start) const
{
  //cerr << start << " --> ";
  if (displace_start) {
#if 0
    // find next sharp corner (>pi/4)
    uint oldstart = start; // save start position
    start = m_poly->nextVertex(start);
    while (start != oldstart &&
	   abs(m_poly->angleAtVertex(start) < M_PI/4)) {
      start = m_poly->nextVertex(start);
    }
    // no sharp corner found:
    if (start == oldstart)
#endif
    // start = rand()%m_poly->size(); // randomize
    //start = m_poly->getFarthestIndex(start); // take farthest point
  }
  //cerr << start << endl;
  return start;
}

string PrintPoly::info() const
{
  ostringstream ostr;
  ostr << "PrintPoly "
       << AreaNames[area]
       << ", " <<  m_poly->size() <<" vertices"
       << ", prio=" << priority
       << ", speed=" << speed
    ;
  return ostr.str();
}

// // // // // // // // // // // // // // // // // // // // // // // // // // // //



void Printlines::addPolys(PLineArea area,
			  const vector<Poly> &polys,
			  bool displace_start,
			  double maxspeed, double min_time)
{
  if (polys.size() == 0) return;
  if (maxspeed == 0)
    maxspeed = settings->get_double("Extruder","MaxLineSpeed") * 60; // default
  double maxoverhangspeed = settings->get_double("Slicing","MaxOverhangSpeed");
  for(size_t q = 0; q < polys.size(); q++) {
    if (polys[q].size() > 0) {
      PrintPoly *ppoly = new PrintPoly(polys[q], this, /* Takes a copy of the poly */
				       maxspeed, maxoverhangspeed * 60,
				       min_time, displace_start, area);
      printpolys.push_back(ppoly);
      setZ(polys[q].getZ());
    }
  }
}


// bool priority_sort(const PrintPoly &p1, const PrintPoly &p2)
// {
//   return (p1.getPriority() >= p2.getPriority());
// }

// return total speedfactor due to single poly slowdown
double Printlines::makeLines(Vector2d &startPoint,
			     vector<PLine2> &lines)
{
  const uint count = printpolys.size();
  if (count == 0) return 1;

  // // sort into contiguous areas
  // vector<ExPoly> layerexpolys = layer->GetExPolygons();
  // vector< vector<PrintPoly> > towers(layerexpolys.size());

  // for (uint q = 0; q < printpolys.size(); q++) {
  //   bool intower = false;
  //   for (uint i = 0; i < layerexpolys.size(); i++) {
  //     if (layerexpolys[i].vertexInside(printpolys[q]->poly[0])) {
  // 	towers[i].push_back(printpolys[q]);
  // 	intower = true;
  //     }
  //   }
  //   if (!intower) ;
  // }

  //std::sort(printpolys.begin(), printpolys.end(), priority_sort);

  int nvindex=-1;
  int npindex=-1;
  uint nindex;
  vector<bool> done(count); // polys not yet handled
  for(size_t q=0; q < count; q++) done[q]=false;
  uint ndone=0;
  //double nlength;
  double movespeed = settings->get_double("Hardware","MaxMoveSpeedXY") * 60;
  double totallength = 0;
  double totalspeedfactor = 0;
  while (ndone < count)
    {
      double nstdist = INFTY;
      double pdist;
      for(size_t q = 0; q < count; q++) { // find nearest polygon
	if (!done[q])
	  {
	    //cerr << printpolys[q].info() << endl;
	    if (printpolys[q]->m_poly->size() == 0) {done[q] = true; ndone++;}
	    else {
	      pdist = INFTY;
	      nindex = printpolys[q]->m_poly->nearestDistanceSqTo(startPoint, pdist);
              pdist /= printpolys[q]->priority;
	      if (pdist  < nstdist){
		npindex = q;      // index of nearest poly in polysleft
		nstdist = pdist;  // distance of nearest poly
		nvindex = nindex; // nearest point in nearest poly
	      }
	    }
	  }
      }
      if (ndone==0) { // only first in layer
	nvindex = printpolys[npindex]->getDisplacedStart(nvindex);
      }
      if (npindex >= 0 && nvindex >= 0) {
	printpolys[npindex]->getLinesTo(lines, nvindex, movespeed);
	totallength += printpolys[npindex]->length;
	totalspeedfactor += printpolys[npindex]->length * printpolys[npindex]->speedfactor;
	done[npindex]=true;
	ndone++;
      }
      if (lines.size()>0)
	startPoint = lines.back().to;
    }
  if (totallength !=0)
    totalspeedfactor /= totallength;
  else
    totalspeedfactor = 1.;
  return totalspeedfactor;
}

// #if 0
// void Printlines::oldMakeLines(PLineArea area,
// 			      const vector<Poly> &polys,
// 			      bool displace_startpoint,
// 			      Vector2d &startPoint,
// 			      vector<PLine2> &lines,
// 			      double maxspeed)
// {
//   //cerr << "makeLines " << AreaNames[area] <<  " " << layer->info() << endl;
//   // double linewidthratio = hardware.ExtrudedMaterialWidthRatio;
//   //double linewidth = layerthickness/linewidthratio;
//   if ( maxspeed == 0 ) maxspeed = settings->Hardware.MaxPrintSpeedXY * 60;
//   double movespeed = settings->Hardware.MoveSpeed * 60;

//   const uint count = polys.size();
//   if (count == 0) return;
//   int nvindex=-1;
//   int npindex=-1;
//   uint nindex;
//   vector<bool> done(count); // polys not yet handled
//   for(size_t q=0; q < count; q++) done[q]=false;
//   uint ndone=0;
//   //double nlength;
//   while (ndone < count)
//     {
//       double nstdist = INFTY;
//       double pdist;
//       // if (sortbyoverhang) {

//       // }
//       // else
// 	for(size_t q=0; q<count; q++) { // find nearest polygon
// 	  if (!done[q])
// 	    {
// 	      if (polys[q].size() == 0) {done[q] = true; ndone++;}
// 	      else {
// 		pdist = INFTY;
// 		nindex = polys[q].nearestDistanceSqTo(startPoint,pdist);
// 		if (pdist<nstdist){
// 		  npindex = q;      // index of nearest poly in polysleft
// 		  nstdist = pdist;  // distance of nearest poly
// 		  nvindex = nindex; // nearest point in nearest poly
// 		}
// 	      }
// 	    }
// 	}
//       // displace first point to next sharp corner (>pi/4)
//       if (displace_startpoint && ndone==0) {
// 	int oldnvindex = nvindex; // if none found, stay here
// 	nvindex = polys[npindex].nextVertex(nvindex);
// 	while (nvindex != oldnvindex &&
// 	       abs(polys[npindex].angleAtVertex(nvindex) < M_PI/4))
// 	  nvindex = polys[npindex].nextVertex(nvindex);
//       }
//       if (npindex >= 0 && npindex >=0) {
// 	addPoly(area, lines, polys[npindex], nvindex, maxspeed, movespeed);
// 	done[npindex]=true;
// 	ndone++;
//       }
//       if (lines.size()>0)
// 	startPoint = lines.back().to;
//     }
// }
// #endif

void Printlines::optimize(double linewidth,
			  double slowdowntime,
			  double cornerradius,
			  vector<PLine2> &lines)
{
  //optimizeLinedistances(linewidth);
  // double OPTRATIO = 1.5;
  // double optratio = OPTRATIO; //corner cap
  // optimizeCorners(linewidth,linewidthratio,optratio);
  // double E=0;Vector3d start(0,0,0);
  // cout << GCode(start,E,1,1000);
  //cerr << "optimize" << endl;
  makeArcs(linewidth, lines);
  double minarclength = settings->get_double("Slicing","MinArcLength");
  if (!settings->get_boolean("Slicing","UseArcs")) minarclength = cornerradius;
  if (settings->get_boolean("Slicing","RoundCorners"))
    roundCorners(cornerradius, minarclength, lines);
  slowdownTo(slowdowntime, lines);
  //double totext = total_Extrusion(lines);
  //makeAntioozeRetract(lines);
  //double totext2 = total_Extrusion(lines);
  // if (abs(totext-totext2)>0.01)
  //   cerr << "extrusion difference after antiooze " << totext2-totext << endl;
  // else
  //   cerr << " ok" << endl;
}


#define FITARC_FIND 0
#if FITARC_FIND

// find center for best fit of arclines
bool fit_arc(const vector<PLine2> &lines, uint fromind, uint toind,
	     double sq_error,
	     Vector2d &result_center, double &result_radiussq)
{
  if (toind-fromind < 2) return false;
  if (toind > lines.size()) return false;
  const int n_par = 3; // center x,y and arc radius_sq
  // start values:
  const Vector2d &P = lines[fromind].from;
  const Vector2d &Q = lines[toind].to;
  const Vector2d startxy = (P+Q)/2.;
  double par[3] = { startxy.x(), startxy.y(), P.squared_distance(Q) };

  int m_dat = toind-fromind+1;
  arc_data_struct data;
  data.px = new double[m_dat];
  data.py = new double[m_dat];
  data.px[0] = P.x();
  data.py[0] = P.y();
  for (int i = 0; i < m_dat; i++) {
    data.px[i] = lines[fromind+i].to.x();
    data.py[i] = lines[fromind+i].to.y();
  }
  return fit_arc(m_dat, data, n_par, par, sq_error,
		 result_center, result_radiussq);
}


// max offset of the arc from the line
double arc_offset(const Vector2d &center, const PLine2 &line)
{
  const double r = center.distance(line.from);
  const double angle = abs(planeAngleBetween(line.from-center, line.to-center));
  const double off =  r - r*sin(angle/2);
  //cerr << "offset " << off << endl;
  return off;
}

bool continues_arc(const Vector2d &center,
		   uint index, double maxAngle,
		   const vector<PLine2> &lines)
{
  if (index < 2 || index >= lines.size()) return false;
  const PLine2 &l1 = lines[index-2];
  const PLine2 &l2 = lines[index-1];
  const PLine2 &l3 = lines[index];
  const double angle1 = l1.angle_to(l2);
  const double angle2 = l2.angle_to(l3);
  if (abs(angle1) < 0.001) return false;
  if (abs(angle2) < 0.001) return false;
  const double len2 = l2.length();
  const double len3 = l3.length();
  if (abs(len2) < 0.001) return false;
  if (abs(len3) < 0.001) return false;
  return ( angle1 < maxAngle && angle2 < maxAngle
	   && abs(angle1/angle2-1) < 0.3
	   && abs(len2/len3-1) < 0.3 );
}


uint Printlines::makeArcs(double linewidth,
			  vector<PLine2> &lines) const
{
  if (!settings->get_boolean("Slicing","UseArcs")) return 0;
  if (lines.size() < 3) return 0;
  const double maxAngle = settings->get_double("Slicing","ArcsMaxAngle") * M_PI/180;
  const double linewidth_sq = linewidth*linewidth;
  if (maxAngle <= 0) return 0;
  double arcRadiusSq = 0;
  Vector2d arccenter(1000000,1000000);
  guint arcstart = 0;
  Vector2d newcenter;
  double newradiusSq = 0;
  uint i = arcstart;
  uint arcend = i;
  while (arcstart < lines.size()-4) {
    i = arcstart+2;
    arcend = arcstart;
    while ( continues_arc(arccenter, i, maxAngle, lines) ) {
      if ( fit_arc (lines, arcstart, i, linewidth_sq, newcenter, newradiusSq)
	   //&&  arc_offset(newcenter, lines[i]) < 5*linewidth
	   ) {
	arccenter = newcenter;
	arcend = i;
      }
      i++;
    }
    if (arcend > arcstart + 2) {
      cerr << "found arc from " << arcstart << " to " << arcend << endl;
      i -= makeIntoArc(arccenter, arcstart, arcend, lines);
    }
    arcstart = i+1;
  }
}

#else

// gets center of common arc of 2 lines if radii match inside maxSqerr range
Vector2d Printlines::arcCenter(const PLine2 &l1, const PLine2 &l2,
			       double maxSqerr) const
{
  Vector2d l1p1,l1p2;
  center_perpendicular(l1.from, l1.to, l1p1, l1p2);
  Vector2d l2p1,l2p2;
  center_perpendicular(l2.from, l2.to, l2p1, l2p2);
  Vector2d center, ip;
  int is = intersect2D_Segments(l1p1, l1p2, l2p1, l2p2,
 				center, ip);
  if (is > 0) {
    // radii match?
    if (abs(l1p1.squared_distance(center) -
	    l2p1.squared_distance(center)) < maxSqerr)
      return center;
  }
  return Vector2d(10000000,10000000);
}

uint Printlines::makeArcs(double linewidth,
			  vector<PLine2> &lines) const
{
  if (!settings->get_boolean("Slicing","UseArcs")) return 0;
  if (lines.size() < 2) return 0;
  double maxAngle = settings->get_double("Slicing","ArcsMaxAngle") * M_PI/180;
  if (maxAngle < 0) return 0;
  double arcRadiusSq = 0;
  Vector2d arccenter(1000000,1000000);
  guint arcstart = 0;
  for (uint i=1; i < lines.size(); i++) {
    const PLine2 &l1 = lines[i-1];
    const PLine2 &l2 = lines[i];
    if (l1.arc) { arcstart = i+1; cerr << "1 arc" << endl;continue; }
    if (l2.arc) { i++; arcstart = i+1;cerr << "2 arc" << endl; continue; }
    double dangle         = l2.angle_to(l1);
    double feedratechange = l2.feedratio - l1.feedratio;
    Vector2d nextcenter   = arcCenter(l2, l1, 0.05*arcRadiusSq);
    double radiusSq       = nextcenter.squared_distance(l2.from);
    // test if NOT continue arc:
    if (l2.from.squared_distance(l1.to) > 0.001 // not adjacent
	|| abs(feedratechange) > 0.1            // different feedrate
	|| abs(dangle) < 0.0001                 // straight continuation
	|| abs(dangle) > maxAngle               // too big angle
	|| ( i>1 && arccenter.squared_distance(nextcenter) > 0.05*radiusSq ) // center displacement
	)
      {
	arccenter   = nextcenter;
	arcRadiusSq = radiusSq;
	// this one doesn't fit, so i-1 is last line of the arc
	if (arcstart+2 < i-1) // at least three lines to make an arc
	  i -= makeIntoArc(arcstart, i-1, lines);
	// set start for potential next arc
	arcstart = i;
      }
  }
  // remaining
  if (arcstart+2 < lines.size()-1)
    makeIntoArc(arcstart, lines.size()-1, lines);
  return 0;
}
#endif


uint Printlines::makeIntoArc(const Vector2d &center,
			     guint fromind, guint toind,
			     vector<PLine2> &lines) const
{
  if (toind < fromind+1 || toind+1 > lines.size()) return 0;
  bool ccw = isleftof(center, lines[fromind].from, lines[fromind].to);
  //cerr << "makeIntoArc ccw " << ccw << endl;
  lines[fromind] = PLine2(lines[fromind].area, lines[fromind].extruder_no,
        lines[fromind].from, lines[toind].to,
        lines[fromind].speed, lines[fromind].feedratio,
        center, ccw, lines[fromind].lifted );
  lines.erase(lines.begin()+fromind+1, lines.begin()+toind+1);
  return toind-fromind;
}

// return how many lines are removed
uint Printlines::makeIntoArc(guint fromind, guint toind,
			     vector<PLine2> &lines) const
{
  if (toind < fromind+1 || toind+1 > lines.size()) return 0;
  //cerr<< "arcstart = " << fromind << endl;
  const Vector2d &P = lines[fromind].from;

#define FITARC 0
#if FITARC

  Vector2d center; double fitradius_sq;
  vector<Vector2d> arcpoints;
  arcpoints.push_back(P);
  for (uint i = fromind; i <= toind; i++)
    arcpoints.push_back(lines[i].to);

  if (  fit_arc(arcpoints, 0.1, center, fitradius_sq) ) {
    cerr << " found center " << center << " radius="<< sqrt(fitradius_sq) << endl;
#else
  const Vector2d &Q = lines[toind].to;

  //bool fullcircle = (P==Q);
  // get center: intersection of center perpendiculars of 2 chords
  // center perp of start -- endpoint:
  guint end1ind = toind;
  //if (fullcircle) { // take one-third for first center_perp
    end1ind = fromind + (toind-fromind)/2;
    //}
  Vector2d chord1p1, chord1p2;
  center_perpendicular(P, lines[end1ind].to, chord1p1, chord1p2);
  // center perp of midpoint -- endpoint:
  guint start2ind =  fromind + (toind-fromind)/2;
  Vector2d chord2p1, chord2p2;
  center_perpendicular(lines[start2ind].from, Q, chord2p1, chord2p2);
  // intersection = center
  Vector2d center, ip;
  int is = intersect2D_Segments(chord1p1, chord1p2, chord2p1, chord2p2,
   				center, ip);
  if (is > 0) {
#endif
    return makeIntoArc(center, fromind, toind, lines);
  } // else cerr << "arc not possible" << endl;
  return 0;
}

uint Printlines::roundCorners(double maxdistance, double minarclength,
			      vector<PLine2> &lines) const
{
  if (lines.size() < 2) return 0;
  uint num = 0;
  for (uint i=0; i < lines.size()-1; i++) {
    uint n = makeCornerArc(maxdistance, minarclength, i, lines);
    i+=n;
    if (n>0) i--;
    num+=n;
  }
  return num;
}

// make corner of lines[ind], lines[ind+1] into arc
// or rounded sequence of lines
// maxdistance is distance of arc begin from corner
uint Printlines::makeCornerArc(double maxdistance, double minarclength,
			       uint ind,
			       vector<PLine2> &lines) const
{
  if (ind > lines.size()-2) return 0;
  if (lines[ind].arc != 0 || lines[ind+1].arc != 0) return 0;
  // movement in between?
  if ((lines[ind].to - lines[ind+1].from).squared_length() > 0.01) return 0;
  // if ((lines[ind].from - lines[ind+1].to).squared_length()
  //     < maxdistance*maxdistance) return 0;
  const double len1 = lines[ind].length();
  const double len2 = lines[ind+1].length();
  maxdistance   = min(maxdistance, len1); // ok to eat up line 1
  maxdistance   = min(maxdistance, len2 / 2.1); // only eat up less than half of second line
  const  Vector2d dir1 = lines[ind].dir();
  const  Vector2d dir2 = lines[ind+1].dir();
  //double lenbefore = 0;
  //for (uint i = 0; i<=ind + 1; i++){
  //    lenbefore += lines[i].length();
  //}
  double angle  = planeAngleBetween(dir1, dir2);
  // arc start and end point:
  const Vector2d p1   = lines[ind].to     - normalized(dir1)*maxdistance;
  const Vector2d p2   = lines[ind+1].from + normalized(dir2)*maxdistance;
  // intersect perpendiculars at arc start/end
  Vector2d center, I1;
  int is = intersect2D_Segments(p1, p1 + Vector2d(-dir1.y(),dir1.x()),
				p2, p2 + Vector2d(-dir2.y(),dir2.x()),
   				center, I1);
  if (is==0) return 0;
  const double radius = center.distance(p1);
  if (radius > 10*maxdistance) return 0; // calc error(?)
  const bool ccw = isleftof(center, p1, p2);
  if (!ccw) angle = -angle;
  if (angle <= 0) angle += 2*M_PI;
  // need 2 half arcs?
  const bool split =
    (lines[ind].feedratio != lines[ind+1].feedratio)
    || (lines[ind].extruder_no != lines[ind+1].extruder_no);
  const double arc_len = abs(radius * angle);
  // too small for arc, replace by 2 straight lines
  const bool not_arc =
    !settings->get_boolean("Slicing","UseArcs")
    || (arc_len < (split?minarclength:(minarclength*2)));
  // too small to make 2 lines, just make 1 line
  const bool toosmallfortwo  =
    (arc_len < (split?(minarclength/2):minarclength));
  // if (toosmallfortwo) return 0;

  vector<PLine2> newlines; // all lines replacing lines[ind] and lines[ind+1]
  uint insertbefore = ind+1;
  if (p2 != lines[ind+1].to) { // straight line 2
    lines[ind+1].move_to(p2, lines[ind+1].to);
  } else
      lines.erase(lines.begin() + ind+1);
  if (p1 != lines[ind].from) { // straight line 1
    lines[ind].move_to(lines[ind].from, p1);
  } else {
      lines.erase(lines.begin() + ind);
      insertbefore--;
  }
  if (p2 != p1)  {
    if (toosmallfortwo) { // 1 line
      const double feedr = ( lines[ind].feedratio + lines[ind+1].feedratio ) / 2;
      newlines.push_back(PLine2(lines[ind].area, lines[ind].extruder_no,
				p1, p2, lines[ind].speed, feedr,
				(feedr!=0)?lines[ind].lifted:0));
    }
    else if (split || not_arc) { // calc arc midpoint
      const Vector2d splitp = rotated(p1, center, angle/2, ccw);
      if (not_arc) { // 2 straight lines
        newlines.push_back(lines[ind]);
	newlines.back().move_to(p1, splitp);
        newlines.push_back(lines[ind+1]);
	newlines.back().move_to(splitp, p2);
      } else if (split) { // 2 arcs
	newlines.push_back(PLine2(lines[ind].area, lines[ind].extruder_no,  p1, splitp,
				 lines[ind].speed, lines[ind].feedratio, center, ccw, lines[ind].lifted));
	newlines.push_back(PLine2(lines[ind+1].area, lines[ind+1].extruder_no, splitp, p2,
				 lines[ind+1].speed, lines[ind+1].feedratio, center, ccw, lines[ind+1].lifted));
      }
    } else { // 1 arc
      newlines.push_back(PLine2(lines[ind].area, lines[ind].extruder_no, p1, p2,
			       lines[ind].speed, lines[ind].feedratio, center, ccw, lines[ind].lifted));
    }
  }

  const uint numnew = newlines.size();
  lines.insert(lines.begin() + insertbefore, newlines.begin(), newlines.end());
  //double lenafter = 0;
  //for (uint i = 0; i<=insertbefore + numnew; i++){
  //    lenafter += lines[i].length();
  //}
  //cerr << " len "<< lenbefore << " -> " << lenafter << endl;
  return max(0, (int)numnew);
}



// split line at given length


uint Printlines::divideline(uint lineindex,
			    const double length,
			    vector< PLine3 > &lines)
{
  typedef Vector3d vec;
  PLine3 *l = &lines[lineindex];
  double linelen = l->length();
  if (length > linelen) return 0;
  vec splitp = l->splitpoint(length);
  if ( !l->arc ) {
    uint nlines = divideline(lineindex, splitp, lines);
    return nlines;
  } else {
    const double angle = l->angle * length/linelen;
    PLine3 line1(*l);
    line1.to = splitp;
    line1.angle = angle;
    PLine3 line2(*l);
    line2.from = splitp;
    line2.angle = l->angle - angle;
    if (l->absolute_extrusion != 0) { // distribute absolute extrusion
      const double totlength = line1.length() + line2.length();
      line1.absolute_extrusion = l->absolute_extrusion * line1.length()/totlength;
      line2.absolute_extrusion = l->absolute_extrusion * line2.length()/totlength;
    }
    line1.extrusion *= line1.angle/l->angle;
    line2.extrusion *= line2.angle/l->angle;
    lines[lineindex] = line1;
    lines.insert(lines.begin() + lineindex + 1, line2);
    return 1;
  }
  return 0;
}


uint Printlines::divideline(uint lineindex,
			    const Vector2d &point,
			    vector< PLine2 > &lines)
{
  vector< PLine2 > newlines = lines[lineindex].division(point);
  replace(lines, lineindex, newlines);
  return newlines.size()-1;
}
uint Printlines::divideline(uint lineindex,
			    const vector<Vector2d> &points,
			    vector< PLine2 > &lines)
{
  vector< PLine2 > newlines = lines[lineindex].division(points);
  replace(lines, lineindex, newlines);
  return newlines.size()-1;
}

uint Printlines::divideline(uint lineindex,
			    const Vector3d &point,
			    vector< PLine3 > &lines)
{
  vector< PLine3 > newlines = lines[lineindex].division(point);
  replace(lines, lineindex, newlines);
  return newlines.size()-1;
}
uint Printlines::divideline(uint lineindex,
			    const vector< Vector3d > &points,
			    vector< PLine3 > &lines)
{
  vector<PLine3> newlines = lines[lineindex].division(points);
  replace(lines, lineindex, newlines);
  return newlines.size();
}


// walk around holes
#define NEWCLIP 1
#if NEWCLIP
// polys are clippolys (shells)
void Printlines::clipMovements(const vector<Poly> &polys, vector<PLine2> &lines,
			       bool findnearest, double maxerr) const
{
  if (polys.size()==0 || lines.size()==0) return;
  vector<PLine2> newlines;
  for (guint i=0; i < lines.size(); i++) {
    if (lines[i].is_move()) {
      // // don't clip a lifted line
      // if (lines[i].lifted > 0) continue;
      int frompoly=-1, topoly=-1;
      // get start and end poly of move
      for (uint p = 0; p < polys.size(); p++) {
	if ((frompoly==-1) && polys[p].vertexInside(lines[i].from, maxerr))
	  frompoly=(int)p;
	if ((topoly==-1)   && polys[p].vertexInside(lines[i].to,   maxerr))
	  topoly=(int)p;
      }
      int div = 0;
      //cerr << frompoly << " --> "<< topoly << endl;
      if (frompoly >=0 && topoly >=0) {
	if (findnearest && frompoly != topoly) {
	  int fromind, toind;
	  polys[frompoly].nearestIndices(polys[topoly], fromind, toind);
	  vector<Vector2d> path(2);
	  path[0] = polys[frompoly].vertices[fromind];
	  path[1] = polys[topoly].  vertices[toind];
	  // for (uint pi=0; pi < path.size(); pi++)
	  //   cerr << path[pi] << endl;
	  div += divideline(i, path, lines);
	  // cerr << i << " _ "<<  frompoly<<":"<<fromind << " ==> "<< topoly<<":"<<toind
	  //      << " - " << div<< endl;
	  //i++;
	  // continue;
	}
      }
      //continue;
#define FASTPATH 0
#if FASTPATH // find shortest path through polygon
      // faster to print but slow to calculate
      if (frompoly >=0 && topoly >=0) {
	// vector<Poly> allpolys;
	// allpolys.push_back(polys[frompoly]);
	// // add holes
	// for (int p = 0; p < polys.size(); p++) {
	//   if (p != frompoly && polys[p].isInside(polys[frompoly]))
	//     allpolys.push_back(polys[p]);
	// }
	// cerr << allpolys.size() << " holes" << endl;
	vector<Vector2d> path;
	bool ispath = shortestPath(lines[i].from, lines[i].to,
				   polys, frompoly, path, maxerr);
	//cerr << path.size() << " path points" << endl;

	if (ispath) {
	  div += (divideline(i,path,lines));
	  if (divisions>0)
	    cerr << divisions << " div in poly " << topoly << " - " << ispath << " path " << path.size()<<endl;
	}
      }
#else // walk along perimeters
      // intersections with all polys
      for (uint p = 0; p < polys.size(); p++) {
	vector<Intersection> pinter =
	  polys[p].lineIntersections(lines[i].from,lines[i].to, maxerr);
	if (pinter.size() > 0) {
	  // if (pinter.size()%2 == 0) {
	    vector<Vector2d> path =
	      polys[p].getPathAround(lines[i].from, lines[i].to);
	    // after divide, skip number of added lines -> test remaining line later
	    div += (divideline(i, path, lines));
	    //continue;
	  // }
	}
      }

#endif
      i += div;
    }
  }
}
#else  // old clip
void Printlines::clipMovements(const vector<Poly> &polys, vector<PLine2> &lines,
			       double maxerr) const
{
  if (polys.size()==0 || lines.size()==0) return;
  vector<PLine2> newlines;
  for (guint i=0; i < lines.size(); i++) {
    if (lines[i].is_move()) {
      int frompoly=-1, topoly=-1;
      for (uint p = 0; p < polys.size(); p++) {
	if ((frompoly==-1) && polys[p].vertexInside(lines[i].from, maxerr))
	  frompoly=(int)p;
	if ((topoly==-1) && polys[p].vertexInside(lines[i].to, maxerr))
	  topoly=(int)p;
      }
      if ((frompoly == -1) || (topoly == -1)) {
	//cerr <<frompoly << " -- " << topoly << endl;
	continue;
      }
      // if (frompoly != topoly && polys[frompoly].hole) { // walk out of hole
      // 	double dist;
      // 	uint nearest = polys[frompoly].nearestDistanceSqTo(lines[i].from, dist);
      // 	i += divideline(i,polys[frompoly][nearest],lines);
      // 	continue;
      // }
      // line outside polys - ?
      // line inside same poly, find path:
      if (topoly == frompoly) {
#define FASTPATH 0
#if FASTPATH // find shortest path through polygon
	vector<Poly> holes;
	//holes.push_back((*polys)[frompoly]);
	for (uint p = 0; p < polys->size(); p++) {
	  //   //if ((*polys)[p].isHole())
	  if (polys[frompoly].polyInside(polys[p]))
	    holes.push_back(polys[p]);
	}
	vector<Vector2d> path;
	bool ispath = shortestPath(lines[i].from,lines[i].to,
				   holes, frompoly, path, maxerr);
	if (ispath) {
	  int divisions = (divideline(i,path,lines));
	  i += divisions;
	  if (divisions>0)
	    cerr << divisions << " div in poly " << topoly << " - " << ispath << " path " << path.size()<<endl;
	}
	continue; // next line
#else // walk along perimeters
	for (uint p = 0; p < polys.size(); p++) {
 	  vector<Intersection> pinter =
	    polys[p].lineIntersections(lines[i].from,lines[i].to, maxerr);
	  if (pinter.size() > 0) {
	    if (pinter.size()%2 == 0) { // holes
	      std::sort(pinter.begin(), pinter.end());
	      vector<Vector2d> path =
		polys[p].getPathAround(pinter.front().p, pinter.back().p);
	      // after divide, skip number of added lines -> test remaining line later
	      i += (divideline(i, path, lines));
	    }
	  }
	}
#endif
      }
      else if (frompoly != -1 && topoly != -1 && frompoly != topoly) {
	cerr << i << " : "<<frompoly << " p>> " << topoly << endl;
	// vector<Intersection> frominter =
	//   polys[frompoly].lineIntersections(lines[i].from,lines[i].to, maxerr);
	// vector<Intersection> tointer =
	//   polys[topoly].lineIntersections(lines[i].from,lines[i].to, maxerr);
	// cerr << frominter.size() << " -- " << tointer.size() << endl;
	// vector<Vector2d> frompath =
	//   polys[frompoly].getPathAround(lines[i].from, lines[i].to);
	// vector<Vector2d> topath =
	//   polys[topoly].getPathAround(lines[i].from, lines[i].to);
	// cerr << frompath.size() << " -- " << topath.size() << endl;
	int fromind, toind;
	polys[frompoly].nearestIndices(polys[topoly], fromind, toind);
	vector<Vector2d> path;
	//path.push_back(lines[i].from);
	path.push_back(polys[frompoly].vertices[fromind]);
	path.push_back(polys[topoly].vertices[toind]);
	//path.push_back(lines[i].to);
	for (uint pi=0; pi < path.size(); pi++)
	  cerr << path[pi] << endl;
	int div=(divideline(i, path, lines));
	cerr << fromind << "--" << toind << "  - " << div <<endl;
	i+=div;
      }
    }
  }
}
#endif // NEWCLIP=0


void Printlines::setSpeedFactor(double speedfactor, vector<PLine2> &lines) const
{
  if (speedfactor == 1) return;
  for (uint i=0; i < lines.size(); i++){
    if (!lines[i].is_move())
      lines[i].speed *= speedfactor;
  }
}
double Printlines::slowdownTo(double totalseconds, vector<PLine2> &lines)
{
  double totalnow = totalSecondsExtruding(lines);
  if (totalseconds == 0 || totalnow == 0) return 1;
  double speedfactor = totalnow / totalseconds;
  if (speedfactor < 1.){
    setSpeedFactor(speedfactor,lines);
    slowdownfactor *= speedfactor;
  }
  return slowdownfactor;
}

// merge too near parallel lines
void Printlines::mergelines(PLine2 &l1, PLine2 &l2, double maxdist) const
{
  Vector2d d2 = l2.to - l2.from;
  double len2 = d2.length();
  if (len2==0) return;
  double dist2 = abs(cross2d(d2,l1.from-l2.from).length()) / len2 ; // by area of parallelogram
  //cerr << len2 << " - " << len2b << endl;
  // Vector2d dp = l1.to - l2.from;
  // double l = dot(dp,d2);
  // Vector2d pproj = l2.from + d2 * l/len2;  // proj. of l1.to on l2
  // double d = (l1.to - pproj).length();  // = distance of parallel lines
  // cerr << "dist " << d<< " - " << dist2 <<endl;
  if (dist2<4*maxdist) {
    // l1.from=(l1.from+l2.from)*0.5; // test
    // l1.to=(l1.to+l2.to)*0.5; // test
    l1.from=l1.to;
    l2.from=l2.to;
  }
}


void Printlines::optimizeLinedistances(double maxdist, vector<PLine2> &lines) const
{
 uint count = lines.size();
 for (uint i=0; i<count; i++){
   for (uint j=i+1; i<count; i++){
     if (abs(lines[i].angle_to(lines[j]))<0.1){ // parallel
	 mergelines(lines[i],lines[j],maxdist);

       // if (distance(lines[i].from, lines[j]) < maxdist){
       // 	 lines[i].to = lines[i].from;
       // 	 cerr<< "removed "<<i<< endl;
	 // delete line?
     }
   }
 }
}

bool Printlines::capCorner(PLine2 &l1, PLine2 &l2,
			   double linewidth, double linewidthratio,
			   double optratio) const
{
  double MINLEN = 4; // minimum line length to handle
  bool done = false;
  if (l1.to!=l2.from) return done;  // only handle adjacent lines
  //if ((l1.to-l2.from).length() > linewidth ) return done;
  double da = l1.angle_to(l2);
  while (da>=2*M_PI) da-=2*M_PI;
  while (da<=-2*M_PI)da+=2*M_PI;
  double tana = abs(tan((da)/2.));
  // new endpoints should have this distance:
  double dist = linewidth * linewidthratio ; //dafactor * linewidth * linewidthratio * optratio ;
  // cut until endpoints have the distance
  double cutlength = dist*tana/2. ; // cut both lines by this length
  cerr << "da=" << da<< " -> "<< cutlength<<endl;
  //cerr  << dist << " : " << da << endl;
  Vector2d d1 = l1.to-l1.from;
  double d1l = d1.length();
  if (d1l > MINLEN && d1l > cutlength){
    double newlenfact1 = 1-cutlength/d1l;
    l1.to = l1.from + d1*newlenfact1;
    done = true;
  } // else l1.to=l1.from; // delete line if too short
  Vector2d d2 = l2.from-l2.to;
  double d2l = d2.length();
  if (d2l > MINLEN && d2l > cutlength){
    double lenfact2 = 1-cutlength/d2l;
    l2.from = l2.to + d2*lenfact2;
    done = true;
  } //else l2.to=l2.from; // delete line if too short
  return done;
}

void Printlines::optimizeCorners(double linewidth, double linewidthratio, double optratio,
				 vector<PLine2> &lines) const
{
  //cout << "optimizecorners " ; printinfo();
 uint count = lines.size();
 uint j;
 uint done = 1;
 while (done>0) {
   done=0;
   for (uint i=0; i<count; i++){
     j = i+1 % count;
     if (capCorner(lines[i], lines[j], linewidth, linewidthratio,optratio))
       done++;
   }
   //done=0;
 }
}


// Vector2d Printlines::lastPoint() const
// {
//   return lines.back().to;
// }


void Printlines::getLines(const vector<PLine2> &lines,
			  vector<Vector2d> &olinespoints) const
{
  for (lineCIt lIt = lines.begin(); lIt!=lines.end(); ++lIt){
    if (lIt->is_noop()) continue;
    olinespoints.push_back(lIt->from);
    olinespoints.push_back(lIt->to);
  }
}
void Printlines::getLines(const vector<PLine2> &lines,
			  vector<Vector3d> &olinespoints) const
{
  for (lineCIt lIt = lines.begin(); lIt!=lines.end(); ++lIt){
    if (lIt->is_noop()) continue;
    olinespoints.push_back(Vector3d(lIt->from.x(),lIt->from.y(),z));
    olinespoints.push_back(Vector3d(lIt->to.x(),lIt->to.y(),z));
  }
}

void Printlines::getLines(const vector<PLine2> &lines,
			  vector<PLine3> &plines,
			  double extrusion_per_mm) const
{
  for (lineCIt lIt = lines.begin(); lIt!=lines.end(); ++lIt){
    if (lIt->is_noop()) continue;
    plines.push_back( PLine3(*lIt, z, extrusion_per_mm) );
  }
}

double Printlines::totalLength(const vector<PLine2> &lines)
{
  double l = 0;
  for (lineCIt lIt = lines.begin(); lIt!=lines.end();++lIt){
    l += lIt->length();
  }
  return l;
}
double Printlines::totalLength(const vector<PLine3> &lines)
{
  double l = 0;
  for (line3CIt lIt = lines.begin(); lIt!=lines.end();++lIt){
    l += lIt->length();
  }
  return l;
}


double Printlines::total_rel_Extrusion(const vector< PLine3 > &lines)
{
  double l = 0;
  for (line3CIt lIt = lines.begin(); lIt!=lines.end();++lIt){
    l += lIt->extrusion;
  }
  return l;
}

double Printlines::total_abs_Extrusion(const vector< PLine3 > &lines)
{
  double l = 0;
  for (line3CIt lIt = lines.begin(); lIt!=lines.end();++lIt){
    l +=  lIt->absolute_extrusion;
  }
  return l;
}

double Printlines::total_Extrusion(const vector< PLine3 > &lines)
{
  double l = 0;
  for (line3CIt lIt = lines.begin(); lIt!=lines.end();++lIt){
    l +=  lIt->extrusion + lIt->absolute_extrusion;
  }
  return l;
}

double Printlines::totalSeconds(const vector<PLine2> &lines)
{
  double t = 0;
  for (lineCIt lIt = lines.begin(); lIt!=lines.end();++lIt){
    t += lIt->time() ;
  }
  return t * 60;
}
double Printlines::totalSecondsExtruding(const vector<PLine2> &lines)
{
  double t = 0;
  for (lineCIt lIt = lines.begin(); lIt!=lines.end();++lIt){
    if (!lIt->is_move() || lIt->absolute_extrusion!=0)
      t += lIt->time() ;
  }
  return t * 60;
}


void Printlines::getCommands(const vector<PLine3> &plines,
			     const Settings & settings,
			     GCodeState &gc_state,
			     ViewProgress * progress)
{
  // push all lines to commands
  PLineArea lastArea = UNDEF;
  uint count = plines.size();
  if (count==0) return;
  if (progress) progress->restart (_("Making GCode"), count);
  Vector3d lastPos = plines[0].from;
  int progress_steps = max(1,(int)(count/100));
  bool cont = true;
  vector<Command> commands;
  const double
    minspeed   = settings.get_double("Hardware","MinMoveSpeedXY") * 60,
    movespeed  = settings.get_double("Hardware","MaxMoveSpeedXY") * 60,
    //maxspeed   = min(movespeed, (double)settings.Extruder.MaxLineSpeed * 60),
    minZspeed  = settings.get_double("Hardware","MinMoveSpeedZ") * 60,
    maxZspeed  = settings.get_double("Hardware","MaxMoveSpeedZ") * 60,
    //maxEspeed  = settings.Extruder.EMaxSpeed * 60,
    maxAOspeed = settings.get_double("Extruder","AntioozeSpeed") * 60;
  const bool useTCommand = settings.get_boolean("Slicing","UseTCommand");
  for (uint i = 0; i < plines.size(); i++) {
    if (progress && i%progress_steps==0){
      cont = (progress->update(i)) ;
      if (!cont)
          break;
    }
    if (plines[i].area != COMMAND && plines[i].area != lastArea) {
      lastArea = plines[i].area;
      commands.push_back(Command(AreaNames[lastArea]));
    }
    plines[i].getCommands(lastPos, commands,
			  minspeed, movespeed, minZspeed, maxZspeed,
			  maxAOspeed, useTCommand);
  }
  gc_state.AppendCommands(commands, settings.get_boolean("Slicing","RelativeEcode"));
}



string Printlines::info() const
{
  ostringstream ostr;
  ostr << "Printlines "<<name<<" at z=" <<z;
  return ostr.str();
}
