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
#include "gcodestate.h"

///////////// PLine3: single 3D printline //////////////////////

PLine3::PLine3(PLineArea area_, const Vector3d &from_, const Vector3d &to_, 
	       double speed_, double extrusionfactor_)
: area(area_), from(from_), to(to_), speed(speed_), 
  extrusionfactor(extrusionfactor_), absolute_extrusion(0),
  arc(0), arcangle(0)
{
}

PLine3::PLine3(const PLine &pline, double z)
{ 
  if (pline.lifted > 0) {
    z += pline.lifted;
    lifted = true;
  }
  area               = pline.area;
  from               = Vector3d(pline.from.x(), pline.from.y(), z);
  to                 = Vector3d(pline.to.x(),   pline.to.y(),   z);
  speed              = pline.speed;
  extrusionfactor    = pline.feedrate;
  absolute_extrusion = pline.absolute_feed;
  arc                = pline.arc;
  arcangle           = pline.angle;
  if (arc != 0) {
    const Vector2d arcIJK2 = pline.arccenter - pline.from;
    arcIJK = Vector3d(arcIJK2.x(), arcIJK2.y(), 0);
  }
}

int PLine3::getCommands(Vector3d &lastpos, vector<Command> &commands,
			double extrusion,
			const Settings &settings) const
{
  const double 
    minspeed  = settings.Hardware.MinPrintSpeedXY,
    //maxspeed  = settings.Hardware.MaxPrintSpeedXY,
    movespeed = settings.Hardware.MoveSpeed,
    minZspeed = settings.Hardware.MinPrintSpeedZ,
    maxZspeed = settings.Hardware.MaxPrintSpeedZ,
    maxEspeed = settings.Hardware.EMaxSpeed;

  int count=0;

  // insert move first if necessary
  if ((lastpos-from).squared_length() > 0.005) {
    // get recursive ...
    PLine3 move3(area, lastpos, from, movespeed, 0);
    vector<Command> movecommands;
    count += move3.getCommands(lastpos, movecommands, 0, settings);
    commands.insert(commands.end(), movecommands.begin(), movecommands.end());
    lastpos = from;
  }

  const double len = length();
  double extrudedMaterial = len * extrusionfactor * extrusion;

  double comm_speed = this->speed;
  if (abs(absolute_extrusion) < 0.00001)
    comm_speed = max(minspeed, this->speed); // in case speed is too low

  double espeed = maxEspeed;
  if (len > 0.00001)
    espeed = extrudedMaterial*comm_speed/len;
  if (abs(extrudedMaterial) < 0.00001) // no matter what additional absolute_extrusion
    comm_speed = movespeed;
  else
    if (espeed > maxEspeed)
      comm_speed *= maxEspeed/espeed;
  extrudedMaterial += absolute_extrusion; // allowed to push/pull at arbitrary speed

  // slow down if too fast for z axis
  const double dZ = to.z() - from.z();
  if (abs(dZ) > 0.00001) {
    const double xyztime = len / comm_speed;
    const double zspeed = abs(dZ / xyztime);
    if ( zspeed > maxZspeed ) {
      comm_speed *= maxZspeed / zspeed;
      // insert feedrate-only command to avoid deceleration on z move
      Command preFeedrate(COORDINATEDMOTION, from, 0, comm_speed);
      commands.push_back(preFeedrate);
      count++;
    }
  }
  comm_speed = max(minZspeed, comm_speed);

  Command command;
  if (arc)
    {
      GCodes gc = (arc==-1 ? ARC_CCW : ARC_CW);
      command = Command (gc, to, extrudedMaterial, comm_speed);
      command.arcIJK = arcIJK;
      ostringstream o;
      o << (int)(arcangle*180/M_PI) << "° ";
      if (arc<0) o << "c";
      o << "cw arc";
      command.comment += o.str();
    }
  else
    {
      command = Command (COORDINATEDMOTION, to, extrudedMaterial, comm_speed);
    }
  command.not_layerchange = lifted;
  command.abs_extr += absolute_extrusion;
  if (!command.hasNoEffect(from,0,0,true)) {
    commands.push_back(command);
    count++;
  }
  else cerr << command.info() << endl;
  lastpos = to;
  return count;
}

double PLine3::time() const
{
  return length()/speed;
}

double PLine3::length() const
{
  if (!arc) 
    return (to-from).length();
  else {
    double radius = arcIJK.length();
    return radius * arcangle;
  }
}

string PLine3::info() const
{
  ostringstream ostr;
  ostr << "line "<< AreaNames[area]
       << " " << from;
  if (to!=from) ostr << to;
  ostr << " speed=" << speed 
       << " extrf=" << extrusionfactor << " arc=" << arc;
  if (absolute_extrusion!=0)
    ostr << " abs_extr="<<absolute_extrusion;
  if (arc!=0)
    ostr << " arcIJK=" << arcIJK;
  if (lifted)
    ostr << " lifted";
  return ostr.str();
}




///////////// PLine: single 2D printline //////////////////////

PLine::PLine(PLineArea area_, const Vector2d &from_, const Vector2d &to_, double speed_, 
	     double feedrate_, double lifted_)
  : area(area_), from(from_), to(to_), speed(speed_), 
    feedrate(feedrate_), absolute_feed(0),
    arc(0), lifted(lifted_)
{
  angle = calcangle();
}

// for arc line
PLine::PLine(PLineArea area_, const Vector2d &from_, const Vector2d &to_, double speed_, 
	     double feedrate_, short arc_, const Vector2d &arccenter_, double angle_,
	     double lifted_)
  : area(area_), from(from_), to(to_), speed(speed_), feedrate(feedrate_), absolute_feed(0),
    angle(angle_), arccenter(arccenter_), arc(arc_), lifted(lifted_)
{
}

double PLine::calcangle() const
{
  assert(!arc);
  return angleBetween(Vector2d(1,0), to-from);
}
double PLine::calcangle(const PLine rhs) const
{
  return angleBetween( (to-from), (rhs.to-rhs.from) );
}

double PLine::lengthSq() const
{
  if (!arc) 
    return (to-from).squared_length();
  else
    return pow(length(),2);
}
double PLine::length() const
{
  if (!arc) 
    return sqrt(lengthSq());
  else {
    double radius = (from-arccenter).length();
    return radius * angle;
  }
}

double PLine::time() const
{
  return length()/speed;
}

PLine3 PLine::getPrintline(double z) const
{
  return PLine3(*this, z);
}

void PLine::addAbsoluteExtrusionAmount(double amount, double max_absspeed)
{
  absolute_feed += amount;
  // slowdown if max speed given and not enough time for absolute feed:
  if (max_absspeed > 0) { // is given
    const double t = time(); 
    const double feedt = absolute_feed/max_absspeed;
    if ( feedt > t ) // too fast
      speed *= t / feedt;
  }
}

bool PLine::is_noop() const 
{
  return (from == to && feedrate == 0 && absolute_feed == 0);
}

string PLine::info() const
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
       << " feedr=" << feedrate;
  if (arc!=0)
    ostr << " center=" << arccenter;
  ostr <<  " feedrate=" << feedrate << " abs.extr="<< absolute_feed;
  if (lifted !=0)
    ostr << " lifted=" << lifted;
  return ostr.str();
}



///////////// Printlines //////////////////////


Printlines::Printlines(const Layer * layer, const Settings * settings, double z_offset) :
  Zoffset(z_offset), name(""), slowdownfactor(1.)
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
};

void Printlines::addLine(PLineArea area, vector<PLine> &lines, 
			 const Vector2d &from, const Vector2d &to, 
			 double speed, double movespeed, double feedrate) const
{
  if (to==from) return;
  Vector2d lfrom = from;
  if (lines.size() > 0) {
    Vector2d lastpos = lines.back().to;
    if (lfrom.squared_distance(lastpos) > 0.01) { // add moveline
      PLine move(area, lastpos, lfrom, movespeed, 0);
      if (settings->Slicing.ZliftAlways)
	move.lifted = settings->Slicing.AntioozeZlift;
      lines.push_back(move);
    } else {
      lfrom = lastpos;
    }
  }
  lines.push_back(PLine(area, lfrom, to, speed, feedrate));
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
  // Take a copy of the reference poly
  m_poly = new Poly(poly);

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

void PrintPoly::addToLines(vector<PLine> &lines, int startindex, 
			   double movespeed) const
{
  vector<Vector2d> pvert;
  m_poly->getLines(pvert,startindex);
  if (pvert.size() == 0) return;
  assert(pvert.size() % 2 == 0);
  for (uint i=0; i<pvert.size(); i+=2) {
    printlines->addLine(area, lines, pvert[i], pvert[i+1], 
			speed, movespeed, m_poly->getExtrusionFactor());
  }
}

uint PrintPoly::getDisplacedStart(uint start) const
{
  if (displace_start) { 
    // find next sharp corner (>pi/4)
    uint oldstart = start; // if none found, stay here
    start = m_poly->nextVertex(start);
    while (start != oldstart &&
	   abs(m_poly->angleAtVertex(start) < M_PI/4))
      start = m_poly->nextVertex(start);
  }
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
  if (maxspeed == 0) maxspeed = settings->Hardware.MaxPrintSpeedXY; // default
  for(size_t q = 0; q < polys.size(); q++) { 
    PrintPoly *ppoly = new PrintPoly(polys[q], this, /* Takes a copy of the poly */
		                    maxspeed, settings->Slicing.MaxOverhangSpeed,
		                    min_time, displace_start, area);

    printpolys.push_back(ppoly);
  }
  setZ(polys.back().getZ());
}


// bool priority_sort(const PrintPoly &p1, const PrintPoly &p2)
// {
//   return (p1.getPriority() >= p2.getPriority());
// }

// return total speedfactor due to single poly slowdown
double Printlines::makeLines(Vector2d &startPoint,
			     vector<PLine> &lines)
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
  double movespeed = settings->Hardware.MoveSpeed;
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
      if (npindex >= 0 && npindex >=0) {
	printpolys[npindex]->addToLines(lines, nvindex, movespeed);
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
// 			      vector<PLine> &lines,
// 			      double maxspeed)
// {
//   //cerr << "makeLines " << AreaNames[area] <<  " " << layer->info() << endl;
//   // double linewidthratio = hardware.ExtrudedMaterialWidthRatio;
//   //double linewidth = layerthickness/linewidthratio;
//   if ( maxspeed == 0 ) maxspeed = settings->Hardware.MaxPrintSpeedXY;
//   double movespeed = settings->Hardware.MoveSpeed;

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
			  vector<PLine> &lines)
{
  //optimizeLinedistances(linewidth);
  // double OPTRATIO = 1.5;
  // double optratio = OPTRATIO; //corner cap
  // optimizeCorners(linewidth,linewidthratio,optratio);
  // double E=0;Vector3d start(0,0,0);
  // cout << GCode(start,E,1,1000);
  //cerr << "optimize" << endl;
  makeArcs(linewidth, lines);
  double minarclength = settings->Slicing.MinArcLength;
  if (!settings->Slicing.UseArcs) minarclength = cornerradius; 
  if (settings->Slicing.RoundCorners) 
    roundCorners(cornerradius, minarclength, lines);
  slowdownTo(slowdowntime, lines);
  double totext = total_Extrusion(lines);
  makeAntioozeRetract(lines);
  double totext2 = total_Extrusion(lines);
  if (abs(totext-totext2)>0.01) 
    cerr << "extrusion difference after antiooze " << totext2-totext << endl;
  // else
  //   cerr << " ok" << endl;
}


#define FITARC_FIND 0
#if FITARC_FIND

// find center for best fit of arclines
bool fit_arc(const vector<PLine> &lines, uint fromind, uint toind,
	     double sq_error, 
	     Vector2d &result_center, double &result_radiussq)
{
  if (toind-fromind < 2) return false;
  if (toind > lines.size()) return false;
  const int n_par = 3; // center x,y and arc radius_sq
  // start values:
  const Vector2d &P = lines[fromind].getFrom();
  const Vector2d &Q = lines[toind].getTo();
  const Vector2d startxy = (P+Q)/2.;
  double par[3] = { startxy.x(), startxy.y(), P.squared_distance(Q) };

  int m_dat = toind-fromind+1;
  arc_data_struct data;
  data.px = new double[m_dat];
  data.py = new double[m_dat];
  data.px[0] = P.x();
  data.py[0] = P.y();
  for (int i = 0; i < m_dat; i++) {
    data.px[i] = lines[fromind+i].getTo().x();
    data.py[i] = lines[fromind+i].getTo().y();
  }
  return fit_arc(m_dat, data, n_par, par, sq_error,
		 result_center, result_radiussq);
}


// max offset of the arc from the line
double arc_offset(const Vector2d &center, const PLine &line)
{
  const double r = center.distance(line.getFrom());
  const double angle = abs(angleBetween(line.getFrom()-center, line.getTo()-center));
  const double off =  r - r*sin(angle/2);
  //cerr << "offset " << off << endl;
  return off;
}

bool continues_arc(const Vector2d &center, 
		   uint index, double maxAngle,
		   const vector<PLine> &lines)
{
  if (index < 2 || index >= lines.size()) return false;
  const PLine &l1 = lines[index-2]; 
  const PLine &l2 = lines[index-1]; 
  const PLine &l3 = lines[index]; 
  const double angle1 = l1.calcangle(l2);
  const double angle2 = l2.calcangle(l3);
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
			  vector<PLine> &lines) const
{
  if (!slicing.UseArcs) return 0;
  if (lines.size() < 3) return 0;
  const double maxAngle = settings->Slicing.ArcsMaxAngle * M_PI/180;
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
Vector2d Printlines::arcCenter(const PLine &l1, const PLine &l2,
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
			  vector<PLine> &lines) const
{
  if (!settings->Slicing.UseArcs) return 0;
  if (lines.size() < 2) return 0;
  double maxAngle = settings->Slicing.ArcsMaxAngle * M_PI/180;
  if (maxAngle < 0) return 0;
  double arcRadiusSq = 0;  
  Vector2d arccenter(1000000,1000000);
  guint arcstart = 0;
  for (uint i=1; i < lines.size(); i++) {
    const PLine &l1 = lines[i-1];
    const PLine &l2 = lines[i];
    if (l1.arc) { arcstart = i+1; cerr << "1 arc" << endl;continue; }
    if (l2.arc) { i++; arcstart = i+1;cerr << "2 arc" << endl; continue; }
    double dangle         = l2.calcangle(l1);
    double feedratechange = l2.feedrate - l1.feedrate;
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
			     vector<PLine> &lines) const
{
  if (toind < fromind+1 || toind+1 > lines.size()) return 0;
  const Vector2d &P = lines[fromind].from;
  const Vector2d &Q = lines[toind].to;
  bool fullcircle = (P==Q);
  double angle;
  if (fullcircle) angle = 2*M_PI;
  else            angle = angleBetween(P-center, Q-center);
  bool ccw = isleftof(center, lines[fromind].from, lines[fromind].to);
  if (!ccw) angle = -angle;
  if (angle<=0) angle+=2*M_PI;
  short arctype = ccw ? -1 : 1; 
  PLine newline(lines[fromind].area, P, Q, 
		lines[fromind].speed, lines[fromind].feedrate,
		arctype, center, angle, lines[fromind].lifted);
  lines[fromind] = newline;
  lines.erase(lines.begin()+fromind+1, lines.begin()+toind+1);
  return toind-fromind;
}

// return how many lines are removed 
uint Printlines::makeIntoArc(guint fromind, guint toind,
			     vector<PLine> &lines) const
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
			      vector<PLine> &lines) const
{
  if (lines.size() < 2) return 0;
  uint num = 0;
  for (uint i=0; i < lines.size()-1; i++) {
    uint n = makeCornerArc(maxdistance, minarclength, i, lines);
    i+=n;
    num+=n;
  }
  return num;
}

// make corner of lines[ind], lines[ind+1] into arc
// maxdistance is distance of arc begin from corner
uint Printlines::makeCornerArc(double maxdistance, double minarclength, 
			       uint ind, 
			       vector<PLine> &lines) const
{
  if (ind > lines.size()-2) return 0;
  if (lines[ind].arc != 0 || lines[ind+1].arc != 0) return 0;
  // movement in between?
  if ((lines[ind].to - lines[ind+1].from).squared_length() > 0.01) return 0;
  // if ((lines[ind].from - lines[ind+1].to).squared_length() 
  //     < maxdistance*maxdistance) return 0;
  double l1 = lines[ind].length();
  double l2 = lines[ind+1].length();
  maxdistance   = min(maxdistance, l1); // ok to eat up line 1
  maxdistance   = min(maxdistance, l2 / 2.1); // only eat up less than half of second line
  Vector2d dir1 = lines[ind].to - lines[ind].from;
  Vector2d dir2 = lines[ind+1].to - lines[ind+1].from;
  double angle  = angleBetween(dir1, dir2);
  // arc start and end point:
  Vector2d p1   = lines[ind].to     - normalized(dir1)*maxdistance;
  Vector2d p2   = lines[ind+1].from + normalized(dir2)*maxdistance;
  Intersection inter;
  Vector2d center, I1;
  // intersect perpendiculars at arc start/end
  int is = intersect2D_Segments(p1, p1 + Vector2d(-dir1.y(),dir1.x()),
				p2, p2 + Vector2d(-dir2.y(),dir2.x()),
   				center, I1);
  if (is==0) return 0;
  double radius = center.distance(p1);
  if (radius > 10*maxdistance) return 0; // calc error(?)
  bool ccw = isleftof(center, p1, p2);
  if (!ccw) angle = -angle;
  if (angle <= 0) angle += 2*M_PI;
  short arctype = ccw ? -1 : 1; 
  // need 2 half arcs?
  bool split = (lines[ind].feedrate != lines[ind+1].feedrate);
  // too small arc, replace by 2 straight lines
  bool toosmall = ((radius * angle) < (split?minarclength:(minarclength*2)));
  // too small to make 2 lines, just make 1 line
  bool toosmallfortwo  =  ((radius * angle) < (split?(minarclength/2):minarclength));
  if (toosmallfortwo) return 0;

  vector<PLine> newlines;
  uint numnew = 0;
  if (p1 != lines[ind].from) { // straight line 1
    newlines.push_back(PLine(lines[ind].area, lines[ind].from, p1, 
			     lines[ind].speed, lines[ind].feedrate, lines[ind].lifted));
    numnew++;
  }
  if (p2 != p1)  {
    if (toosmallfortwo) { // 1 line
      const double feedr = ( lines[ind].feedrate + lines[ind+1].feedrate ) / 2;
      newlines.push_back(PLine(lines[ind].area, p1, p2, lines[ind].speed, feedr, 
			       feedr!=0?lines[ind].lifted:0));
      numnew++;
    }
    else if (split || toosmall) { // calc arc midpoint
      const Vector2d splitp = rotated(p1, center, angle/2, ccw);
      if (toosmall) { // 2 straight lines
	newlines.push_back(PLine(lines[ind].area, p1, splitp, lines[ind].speed,
				 lines[ind].feedrate, lines[ind].lifted));
	newlines.push_back(PLine(lines[ind+1].area, splitp, p2, lines[ind+1].speed,
				 lines[ind+1].feedrate, lines[ind+1].lifted));
      }
      else if (split) { // 2 arcs
	newlines.push_back(PLine(lines[ind].area, p1, splitp, 
				 lines[ind].speed,   lines[ind].feedrate,
				 arctype, center, angle/2, lines[ind].lifted));
	newlines.push_back(PLine(lines[ind+1].area, splitp, p2, 
				 lines[ind+1].speed, lines[ind+1].feedrate,
				 arctype, center, angle/2, lines[ind+1].lifted));
      }
      numnew+=2;
    } else { // 1 arc
      newlines.push_back(PLine(lines[ind].area, p1, p2, 
			       lines[ind].speed, lines[ind].feedrate,
			       arctype, center, angle, lines[ind].lifted));
      numnew++;
    }
  }
  if (p2 != lines[ind+1].to) { // straight line 2
    newlines.push_back(PLine(lines[ind].area, p2, 
			     lines[ind+1].to, lines[ind+1].speed,
			     lines[ind+1].feedrate, lines[ind+1].lifted));
    numnew++;
  }
  if (numnew>0) 
    lines[ind] = newlines[0];
  if (numnew>1) 
    lines[ind+1] = newlines[1];
  if (numnew>2) 
    lines.insert(lines.begin()+ind+2, newlines.begin()+2, newlines.end());
  return max(0, (int)numnew - 2);
}


// #define NEW_ANTIOOZE 1
// #if NEW_ANTIOOZE

// find ranges for retract and repush
bool Printlines::find_nextmove(double minlength, uint startindex, 
			       uint &movestart,  uint &moveend, 
			       uint &tractstart, uint &pushend,
			       const vector<PLine> &lines) const
{
  uint i = startindex;
  while (i < lines.size()-2) {
    // find move start
    while (i < lines.size() && lines[i].feedrate != 0 ) {
      i++; movestart = i; 
    }
    if (lines[movestart].feedrate != 0) return false;
    if (i == lines.size()-1) return false;
    // find move end
    while (i < lines.size() && lines[i].feedrate == 0 ) {
      moveend = i; i++;
    }
    if (lines[moveend].feedrate != 0) return false;
    if (moveend > lines.size()-1) moveend = lines.size()-1;
    // long enough?
    double totaldistance=0;
    for (uint j = movestart; j <= moveend; j++)
      totaldistance += lines[j].length();

    if (totaldistance >= minlength)  {
      // find previous move
      if (movestart == 0) tractstart = 0;
      else {
	i = movestart-1;
	while (i > 0 && lines[i].feedrate != 0 ) {
	  tractstart = i; i--; 
	}
      }
      if (moveend == lines.size()-1) pushend = moveend;
      else {
	i = moveend+1;
	while (i < lines.size() && lines[i].feedrate != 0 ) {
	  pushend = i; i++;
	}
      }
      // cerr << "found move " << tractstart << "..." <<movestart 
      // 	   << "--"<<moveend<< "..."<< pushend <<  " length " << totaldistance << endl;
      return true;
    }
  }
  return false;
}

uint Printlines::insertAntioozeHaltBefore(uint index, double amount, double speed, 
					  vector<PLine> &lines) const
{
  Vector2d where; 
  if (index > lines.size()) return 0;
  if (index == lines.size()) where = lines.back().getTo();
  else where = lines[index].getFrom();
  PLine halt (lines.front().area, where, where, speed, 0);
  halt.addAbsoluteExtrusionAmount(amount);
  lines.insert(lines.begin()+index, halt); // (inserts before)
  return 1;
}

int Printlines::distribute_AntioozeAmount(double AOamount, double AOspeed, 
					  uint fromline, uint toline, 
					  bool at_end,  // add to end of range (retract)
					  vector<PLine> &lines,
					  double &havedistributed) const
{
  if (fromline > toline) { // no lines to distribute the amount could be found
    uint added = 0;
    if (at_end) {  // add retracting halt after toline
      added = insertAntioozeHaltBefore(toline+1, AOamount, AOspeed, lines);
      havedistributed += AOamount;  
      //cerr << "halt tract "<< AOamount << " inserted after " << toline << endl;
    } else {       // add repushing halt before fromline
      added = insertAntioozeHaltBefore(fromline, AOamount, AOspeed, lines);
      havedistributed += AOamount;
      //cerr << "halt push "<< AOamount << " inserted before " << fromline << endl;
    }
    return added;
  }
  double AOtime = abs(AOamount) / AOspeed; // time needed for AO on move
  double linestime = 0;  // time all lines normally need
  for (uint i=fromline; i<=toline; i++) linestime += lines[i].time();
  if (linestime > 0 && linestime < AOtime) {
    // slow down lines to get enough time for AOamount
    double speedfactor = linestime / AOtime;
    //cerr << "slow down factor "<< speedfactor<< endl;
    for (uint i=fromline; i<=toline; i++) 
      lines[i].speed *= speedfactor;
  }
  bool negative = (AOamount < 0);
  // now distribute
  double restamount = AOamount; // +-
  uint   iline = fromline; if (at_end) iline = toline; // start here
  double dist = 0;
  while ( abs(restamount) > 0.01 
	  && (restamount < 0) == (AOamount < 0) ) { // while same sign
    double lineamount = lines[iline].time() * AOspeed; // max. possible amount at line
    if (negative) lineamount = -lineamount;
    if (abs(lineamount) > abs(restamount)) // line longer than necessary
      { // -> split line
	const double linelength  = lines[iline].length();
	const double resttime    = abs(restamount) / AOspeed;
	double splitlength = lines[iline].speed * resttime;
	if (at_end) splitlength = linelength - splitlength;
	int added = 0;
	if (at_end  && splitlength > 0.1 )
	//     !at_end && splitlength > linelength-0.1) // have minimum split else use whole line
	  added = divideline(iline, splitlength, lines);
	if (at_end) // add to second half of split (retract)
	  lines[iline+added].addAbsoluteExtrusionAmount(restamount, AOspeed); // +-
	else        // add to first half of split  (repush)
	  lines[iline].addAbsoluteExtrusionAmount(restamount, AOspeed); // +-
	dist += restamount;
	havedistributed += restamount;
	if (abs(dist-AOamount)>0.01) cerr << " wrong longline split dist "<< dist << endl;
	if (abs(havedistributed-AOamount)>0.01) cerr << " wrong amount " << havedistributed << endl;
	//	if (iline == fromline) cerr << "split line " << iline << " - " << splitlength<<" added " << added << endl;
	return added; // finished here
      }
    else
      {
	lines[iline].addAbsoluteExtrusionAmount(lineamount, AOspeed); // +-
	havedistributed += lineamount;
	dist += lineamount;
	restamount -= lineamount;
      }
    if (at_end) {
      iline--; 
      if (iline < fromline) break; // finished
    } else {
      iline++;
      if (iline > toline) break; // finished
    }
  }
  if (abs(dist-AOamount)>0.01) cerr << " wrong norm line  "<< dist << endl;
  if (abs(havedistributed-AOamount)>0.1) cerr << " wrong amount " << havedistributed << endl;
  if (abs(restamount)>0.1) cerr << " wrong restamount " << restamount << endl;
  return 0; // normally no split was done
}


// call after lines have been slowed down!
uint Printlines::makeAntioozeRetract(vector<PLine> &lines) const
{
  if (!settings->Slicing.EnableAntiooze) return 0;
  double 
    AOmindistance = settings->Slicing.AntioozeDistance,
    AOamount      = settings->Slicing.AntioozeAmount,
    AOspeed       = settings->Slicing.AntioozeSpeed;
    //AOonhaltratio = settings->Slicing.AntioozeHaltRatio;
  if (lines.size() < 2 || AOmindistance <=0 || AOamount == 0) return 0;
  // const double onhalt_amount = AOamount * AOonhaltratio;
  // const double onmove_amount = AOamount - onhalt_amount;

  uint total_added = 0;
  double total_extrusionsum = 0;
  uint 
    movestart  = 0, moveend = 0,
    tractstart = 0, pushend = 0;
  while ( find_nextmove(AOmindistance, moveend+1, 
			movestart,  moveend, 
			tractstart, pushend,
			lines) ) {
    double extrusionsum = 0;
    uint   added = 0;
    // do all from behind to keep indices right
    // find lines to distribute repush
    if (moveend > lines.size()-2) moveend = lines.size()-2;
    if (settings->Slicing.AntioozeZlift > 0)
      for (uint i = movestart; i <= moveend; i++) 
	lines[i].lifted = settings->Slicing.AntioozeZlift;

    

    double dist = 0;
    uint newl = distribute_AntioozeAmount(AOamount, AOspeed,
					  moveend+1, pushend,
					  false, lines, dist);
    added += newl;
    double linesext = 0;
    for (uint i = moveend+1; i<=pushend+newl; i++) linesext+=lines[i].absolute_feed;
    if (abs(linesext-AOamount)>0.01) cerr  << "wrong lines dist push " << linesext << endl;
    extrusionsum += dist;
    if (abs(dist-AOamount)>0.01) cerr << " wrong distrib push " << dist << endl;
    // on-halt repush and retract 
    // if (onhalt_amount != 0) {
    //   added += insertAntioozeHaltBefore(moveend+1, onhalt_amount, AOspeed, lines);
    //   extrusionsum += onhalt_amount;
    //   added += insertAntioozeHaltBefore(movestart, -onhalt_amount, AOspeed, lines);
    //   movestart--; // inserted line before
    //   extrusionsum -= onhalt_amount;
    // }
    // find lines to distribute retract
    if (movestart < 1) movestart = 1;
    dist = 0;
    double linesextbefore = 0;
    for (uint i = tractstart; i<=movestart-1; i++) 
      linesextbefore += lines[i].absolute_feed;
    newl = distribute_AntioozeAmount(-AOamount, AOspeed,
				     tractstart, movestart-1,
				     true, lines, dist);
    linesext=-linesextbefore;
    for (uint i = tractstart; i<=movestart-1+newl; i++) 
      linesext += lines[i].absolute_feed;
    if (abs(linesext+AOamount)>0.01) cerr  << "wrong lines dist tract " << linesext << " ("<<dist <<") != " << -AOamount << " - " << tractstart << "->" <<  movestart << " new: "<< newl<<endl;
    added += newl;
    extrusionsum += dist;
    if (abs(dist+AOamount)>0.01) cerr << " wrong distrib tract " << dist << endl;

    moveend += added;
    total_added += added;
    if (abs(extrusionsum) > 0.01) cerr << "wrong AO extr.: " << extrusionsum << endl;
    total_extrusionsum += extrusionsum;
  }
  if (abs(total_extrusionsum) > 0.01) cerr << "wrong total AO extr.: " << total_extrusionsum << endl;
  return total_added;
}


// split line at given length 
uint Printlines::divideline(uint lineindex, const double length, vector<PLine> &lines) const
{
  PLine *l = &lines[lineindex];
  double linelen = l->length();
  if (length > linelen) return 0;
  if ( !l->arc ) {
    Vector2d dir = l->to - l->from;
    vector<Vector2d> points(1);
    points[0] = l->from + dir * (length/linelen);
    uint nlines = divideline(lineindex, points, lines);
    return nlines;
  } else {
    double angle = l->angle * length/linelen;
    Vector2d arcpoint = rotated(l->from, l->arccenter, angle, (l->arc < 0));
    PLine line1(l->area, l->from, arcpoint, l->speed, l->feedrate, 
		l->arc, l->arccenter, angle, l->lifted);
    PLine line2(l->area, arcpoint, l->to,   l->speed, l->feedrate, 
		l->arc, l->arccenter, l->angle-angle, l->lifted);
    if (l->absolute_feed != 0) { // distribute absolute extrusion 
      double totlength = line1.length() + line2.length();
      line1.absolute_feed = l->absolute_feed * line1.length()/totlength;
      line2.absolute_feed = l->absolute_feed * line2.length()/totlength;
    }
    lines[lineindex] = line1;
    lines.insert(lines.begin() + lineindex + 1, line2);
    return 1;
  }
}
uint Printlines::divideline(uint lineindex, const Vector2d &point,
			    vector<PLine> &lines) const
{
  vector<Vector2d> p(1); p[0] = point;
  return divideline(lineindex, p, lines);
}

uint Printlines::divideline(uint lineindex, const vector<Vector2d> &points,
			    vector<PLine> &lines) const
{
  uint npoints = points.size();
  if (npoints == 0) return 0;
  vector<PLine> newlines;
  PLine *l = &lines[lineindex];
  newlines.push_back(PLine(l->area, l->from, points[0], 
			   l->speed, l->feedrate, l->lifted));
  for (uint i = 0; i < npoints-1; i++) {
    newlines.push_back(PLine(l->area, points[i], points[i+1], 
			     l->speed, l->feedrate, l->lifted));
  }
  newlines.push_back(PLine(l->area, points[npoints-1], 
			   l->to, l->speed, l->feedrate, l->lifted));
  if (l->absolute_feed != 0) { // distribute absolute extrusion 
    double totlength = 0;
    for (uint i = 0; i < newlines.size(); i++){
      totlength += newlines[i].length();
    }
    for (uint i = 0; i < newlines.size(); i++){
      newlines[i].absolute_feed = l->absolute_feed * newlines[i].length()/totlength;
    }
  }
  if (newlines.size() > 0 && lines.size() > lineindex) {
    //cerr << newlines.size() << " new lines" << endl;
    lines[lineindex] = newlines[0];
    if (newlines.size() > 1)
      lines.insert(lines.begin()+lineindex+1, newlines.begin()+1, newlines.end());
    return newlines.size()-1; // return how many more lines now 
  }
  return 0;
}

// walk around holes
#define NEWCLIP 1
#if NEWCLIP
// polys are clippolys (shells)
void Printlines::clipMovements(const vector<Poly> &polys, vector<PLine> &lines,
			       bool findnearest, double maxerr) const
{
  if (polys.size()==0 || lines.size()==0) return;
  vector<PLine> newlines;
  for (guint i=0; i < lines.size(); i++) {
    if (lines[i].feedrate == 0) {
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
	  div+=(divideline(i, path, lines));
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
void Printlines::clipMovements(const vector<Poly> &polys, vector<PLine> &lines,
			       double maxerr) const
{
  if (polys.size()==0 || lines.size()==0) return;
  vector<PLine> newlines;
  for (guint i=0; i < lines.size(); i++) {
    if (lines[i].feedrate == 0) {
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


void Printlines::setSpeedFactor(double speedfactor, vector<PLine> &lines) const
{
  if (speedfactor == 1) return;
  for (uint i=0; i < lines.size(); i++){
    if (lines[i].feedrate>0)
      lines[i].speed *= speedfactor;
  }
}
double Printlines::slowdownTo(double totalseconds, vector<PLine> &lines) 
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
void Printlines::mergelines(PLine &l1, PLine &l2, double maxdist) const
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


void Printlines::optimizeLinedistances(double maxdist, vector<PLine> &lines) const
{
 uint count = lines.size();
 for (uint i=0; i<count; i++){
   for (uint j=i+1; i<count; i++){
     if (abs(lines[i].calcangle(lines[j]))<0.1){ // parallel
	 mergelines(lines[i],lines[j],maxdist);
	 
       // if (distance(lines[i].from, lines[j]) < maxdist){
       // 	 lines[i].to = lines[i].from;
       // 	 cerr<< "removed "<<i<< endl;
	 // delete line?
     }
   }
 }
}

bool Printlines::capCorner(PLine &l1, PLine &l2, 
			   double linewidth, double linewidthratio, 
			   double optratio) const
{
  double MINLEN = 4; // minimum line length to handle
  bool done = false;
  if (l1.to!=l2.from) return done;  // only handle adjacent lines
  //if ((l1.to-l2.from).length() > linewidth ) return done;
  double da = l1.calcangle(l2);
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
				 vector<PLine> &lines) const
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


void Printlines::getLines(const vector<PLine> &lines, 
			  vector<Vector2d> &olines) const
{
  for (lineCIt lIt = lines.begin(); lIt!=lines.end(); ++lIt){
    if (lIt->is_noop()) continue;
    olines.push_back(lIt->from);
    olines.push_back(lIt->to);
  }
}
void Printlines::getLines(const vector<PLine> &lines, 
			  vector<Vector3d> &olines) const
{
  for (lineCIt lIt = lines.begin(); lIt!=lines.end(); ++lIt){
    if (lIt->is_noop()) continue;
    olines.push_back(Vector3d(lIt->from.x(),lIt->from.y(),z));
    olines.push_back(Vector3d(lIt->to.x(),lIt->to.y(),z));
  }
}

void Printlines::getLines(const vector<PLine> &lines, 
			  vector<PLine3> &plines) const
{
  for (lineCIt lIt = lines.begin(); lIt!=lines.end(); ++lIt){
    if (lIt->is_noop()) continue;
    plines.push_back( PLine3(*lIt,z) );
  }
}

double Printlines::totalLength(const vector<PLine> &lines) const 
{
  double l = 0;
  for (lineCIt lIt = lines.begin(); lIt!=lines.end();++lIt){
    l += lIt->length();
  }
  return l;
}

double Printlines::total_rel_Extrusion(const vector<PLine> &lines) const 
{
  double l = 0;
  for (lineCIt lIt = lines.begin(); lIt!=lines.end();++lIt){
    l += lIt->length() * lIt->feedrate;
  }
  return l;
}

double Printlines::total_Extrusion(const vector<PLine> &lines) const 
{
  double l = 0;
  for (lineCIt lIt = lines.begin(); lIt!=lines.end();++lIt){
    l += lIt->absolute_feed;
  }
  return l + total_rel_Extrusion(lines);;
}

double Printlines::totalSeconds(const vector<PLine> &lines) const 
{
  double t = 0;
  for (lineCIt lIt = lines.begin(); lIt!=lines.end();++lIt){
    t += lIt->time() ;
  }
  return t * 60;
}
double Printlines::totalSecondsExtruding(const vector<PLine> &lines) const 
{
  double t = 0;
  for (lineCIt lIt = lines.begin(); lIt!=lines.end();++lIt){
    if (lIt->feedrate>0 || lIt->absolute_feed!=0)
      t += lIt->time() ;
  }
  return t * 60;
}

// not used
// string Printlines::GCode(Vector3d &lastpos, double &E, double feedrate, 
// 			 double minspeed, double maxspeed, double movespeed, 
// 			 bool relativeE) const
// {
//   ostringstream o;
//   // E is total E so far (if absolute Ecode)
//   for (lineCIt lIt = lines.begin(); lIt!=lines.end();++lIt){
//     o << GCode(*lIt, lastpos, E, feedrate, minspeed, maxspeed, movespeed, relativeE);
//   }
//   cerr << "PL gcode " << o.str()<< endl;
//   return o.str();
// }


string Printlines::info() const
{
  ostringstream ostr;
  ostr << "Printlines "<<name<<" at z=" <<z;
  return ostr.str();
}
