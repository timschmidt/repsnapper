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
#include "slicer/gcode.h"


///////////// PLine3: single 3D printline //////////////////////

PLine3::PLine3(const PLine pline, double z)
{
  from               = Vector3d(pline.from.x, pline.from.y, z);
  to                 = Vector3d(pline.to.x,   pline.to.y,   z);
  speed              = pline.speed;
  extrusionfactor    = pline.feedrate;
  absolute_extrusion = pline.absolute_feed;
  arc                = pline.arc;
  arcangle           = pline.angle;
  if (arc) {
    Vector2d arcIJK2 = pline.arccenter - pline.from;
    arcIJK = Vector3d(arcIJK2.x, arcIJK2.y, 0);
    // double linearlength = (pline.to - pline.from).length();
    // if (linearlength!=0) // not full circle
    //   extrusionfactor *= pline.length() / linearlength;
  }
}

int PLine3::getCommands(Vector3d &lastpos, vector<Command> &commands,
			double extrusion,
			double minspeed, double maxspeed, double movespeed,
			double maxEspeed) const
{
  int count=0;
  if (lastpos != from) {  // move first
    commands.push_back( Command(COORDINATEDMOTION, from, 0, movespeed) );
    count++;
  }  
  double comm_speed = max(minspeed, this->speed); // in case maxspeed is too low
  double len = length();
  double espeed = 0;
  double extrudedMaterial = len * extrusionfactor * extrusion;
  if (len > 0)
    espeed = extrudedMaterial*comm_speed/len;
  if (extrudedMaterial == 0) // no matter what additional absolute_extrusion
    comm_speed = movespeed;
  else 
    if (espeed > maxEspeed)
      comm_speed *= maxEspeed/espeed;
  extrudedMaterial += absolute_extrusion; // allowed to push/pull at arbitrary speed    
  Command command;
  if (arc) 
    {
      GCodes gc = (arc==-1 ? ARC_CCW : ARC_CW);
      command = Command (gc, to, extrudedMaterial, comm_speed);
      command.arcIJK = arcIJK;
      ostringstream o;
      o << (int)(arcangle*180/M_PI) << "° arc";
      command.comment += o.str();
    }
  else
    {
      command = Command (COORDINATEDMOTION, to, extrudedMaterial, comm_speed);
    }
  if (absolute_extrusion!=0) {
    command.comment += _(" Absolute Extrusion");
  }
  commands.push_back(command);
  count++;
  lastpos = to;
  return count;
}

double PLine3::time() const
{
  return length()/speed;
}

// // not used
// string PLine3::GCode(Vector3d &lastpos, double &lastE, double feedrate, 
// 		     double minspeed, double maxspeed, double movespeed, 
// 		     bool relativeE) const
// {
//   ostringstream o;
//   double distline = length();
//   double movefirst = (from-lastpos).length();
//   double speed = movespeed;
//   if (movefirst > 0.001) {
//     speed = CLAMP(speed, minspeed, movespeed); // ???
//     o << "G1 X"<<from.x<<" Y"<<from.y<<" Z"<<from.z<<" F"<<speed << endl;
//   }

//   double EX = distline * feedrate * feedrate; // this line's E
//   bool noextrusion = (EX==0);
//   if (!relativeE) { // if absolute E
//     lastE += EX;  // add to total E
//     EX = lastE;   // write total E
//   } // else write single line EX
//   if ( !(distline > 0.001 && noextrusion)) {
//     speed = CLAMP(maxspeed, minspeed, maxspeed); // ???
//     o << "G1 X" << to.x  << " Y" << to.y  << " Z" << to.z  
//       << " E" << EX << " F" << speed << endl;
//   }
//   cerr << "PL gcode " << o.str()<< endl;
//   lastpos = to;
//   return o.str();
// }


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
  ostr << "line "<< from;
  if (to!=from) ostr << to;
  ostr << " speed=" << speed 
       << " extrf=" << extrusionfactor << " arc=" << arc;
  if (absolute_extrusion!=0)
    ostr << " abs_extr="<<absolute_extrusion;
  if (arc!=0)
    ostr << " arcIJK=" << arcIJK;
  return ostr.str();
}




///////////// PLine: single 2D printline //////////////////////

PLine::PLine(const Vector2d from_, const Vector2d to_, double speed_, 
	     double feedrate_)
  : from(from_), to(to_), speed(speed_), 
    feedrate(feedrate_), absolute_feed(0),
    arc(0)
{
  angle = calcangle();
}

// for arc line
PLine::PLine(const Vector2d from_, const Vector2d to_, double speed_, 
	     double feedrate_, short arc_, Vector2d arccenter_, double angle_)
  : from(from_), to(to_), speed(speed_), feedrate(feedrate_), absolute_feed(0),
    angle(angle_), arccenter(arccenter_), arc(arc_)
{
}

double PLine::calcangle() const
{
  assert(!arc);
  return angleBetween(from, to);
}
double PLine::calcangle(const PLine rhs) const
{
  assert(!arc);
  return angleBetween((rhs.to-rhs.from), (to-from));
}

double PLine::lengthSq() const
{
  if (!arc) 
    return (to-from).lengthSquared();
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

void PLine::addAbsoluteExtrusionAmount(double amount)
{
  absolute_feed += amount;
}

bool PLine::is_noop() const 
{
  return (from == to && feedrate == 0 && absolute_feed == 0);
}

string PLine::info() const
{
  ostringstream ostr;
  ostr << "line ";
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
  return ostr.str();
}



///////////// Printlines //////////////////////


Printlines::Printlines(double z_offset) :
  Zoffset(z_offset), name("")
{}


void Printlines::addLine(vector<PLine> &lines, const Vector2d from, const Vector2d to, 
			 double speed, double movespeed, double feedrate) const
{
  if (to==from) return;
  if (lines.size() > 0) {
    PLine *lastline = &lines.back();
    if (!(lastline->to == from)) { // add moveline
      lines.push_back(PLine(lastline->to, from, movespeed, 0));
    }
  }
  lines.push_back(PLine(from, to, speed, feedrate));
}

void Printlines::addPoly(vector<PLine> &lines, const Poly poly, int startindex, 
			 double speed, double movespeed) const
{
  vector<Vector2d> pvert;
  poly.getLines(pvert,startindex);
  assert(pvert.size() % 2 == 0);
  for (uint i=0; i<pvert.size();i+=2){
    addLine(lines, pvert[i], pvert[i+1], speed, movespeed, poly.getExtrusionFactor());
  }
}

void Printlines::makeLines(const vector<Poly> polys,
			   bool displace_startpoint, 
			   const Settings::SlicingSettings &slicing,
			   const Settings::HardwareSettings &hardware,
			   Vector2d &startPoint,
			   vector<PLine> &lines)
{
  // double linewidthratio = hardware.ExtrudedMaterialWidthRatio;
  //double linewidth = layerthickness/linewidthratio;
  //double minspeed = hardware.MinPrintSpeedXY,
  double maxspeed = hardware.MaxPrintSpeedXY,
    movespeed = hardware.MoveSpeed;
  bool linelengthsort = slicing.LinelengthSort;

  uint count = polys.size();
  if (polys.size()==0) return;
  int nvindex=-1;
  int npindex=-1;
  uint nindex;
  vector<bool> done(count); // polys not yet handled
  for(size_t q=0;q<count;q++) done[q]=false;
  uint ndone=0;
  double pdist, nstdist;
  //double nlength;
  double lastavlength = -1;
  while (ndone < count) 
    {
      if (linelengthsort && npindex>=0) 
	lastavlength = polys[npindex].averageLinelengthSq();
      nstdist = 1000000;
      for(size_t q=0; q<count; q++) { // find nearest polygon
	if (polys[q].size() == 0) {done[q] = true; ndone++;}
	if (!done[q])
	  {
	    pdist = 1000000;
	    nindex = polys[q].nearestDistanceSqTo(startPoint,pdist);
	    if (pdist<nstdist){
	      npindex = q;      // index of nearest poly in polysleft
	      nstdist = pdist;  // distance of nearest poly
	      nvindex = nindex; // nearest point in nearest poly
	    }
	  }
      }
      // find next nearest polygon, but with similar line length
      if (linelengthsort && lastavlength > 0) { 
	double minavlengthdiff = 10000000;
	for(size_t q=0; q<count; q++) {
	  if (!done[q]) 
	    {
	      nindex = polys[q].nearestDistanceSqTo(startPoint,pdist);
	      if ( pdist < 400 ){ // nearer than 20mm // || 
		//(nstdist==0 && pdist < 10000*linewidth*linewidth) ) {
		double avlength = polys[q].averageLinelengthSq();		
		double avldiff = abs(avlength-lastavlength);
		if (avldiff < minavlengthdiff) {
		  //cerr << npindex << " : " <<avlength << " - " <<avldiff << endl;
		  minavlengthdiff = avldiff;
		  npindex = q;
		  nvindex = nindex;
		}
	      }
	    }
	}
      }
      if (displace_startpoint && ndone==0)  // displace first point
	nvindex = (nvindex+1)%polys[npindex].size();
      if (npindex >= 0 && npindex >=0) {
	addPoly(lines, polys[npindex], nvindex, maxspeed, movespeed);
	done[npindex]=true;
	ndone++;
      }
      if (lines.size()>0)
	startPoint = lines.back().to;
    }
  if (count == 0) return;
  setZ(polys.back().getZ());
  // clipMovements(clippolys, linewidth/2.);
  // optimize(linewidth, linewidthratio, optratio, maxArcAngle,
  // 	   hardware, slicing, lines);
}


void Printlines::optimize(const Settings::HardwareSettings &hardware,
			  const Settings::SlicingSettings &slicing,
			  vector<PLine> &lines) const
{
  //cout << "optimize " ; printinfo();
  //optimizeLinedistances(linewidth);
  // double OPTRATIO = 1.5;
  // double optratio = OPTRATIO; //corner cap
  // optimizeCorners(linewidth,linewidthratio,optratio);
  // double E=0;Vector3d start(0,0,0);
  // cout << GCode(start,E,1,1000);

  makeArcs(slicing, lines);
  makeAntioozeRetraction(slicing, lines);
}

// gets center of common arc of 2 lines if radii match inside maxSqerr range
Vector2d Printlines::arcCenter(const PLine l1, const PLine l2,
			       double maxSqerr) const
{
  Vector2d l1p1,l1p2;
  center_perpendicular(l1.from, l1.to, l1p1, l1p2);
  Vector2d l2p1,l2p2;
  center_perpendicular(l2.from, l2.to, l2p1, l2p2);
  Vector2d center, ip;
  double t0, t1;
  int is = intersect2D_Segments(l1p1, l1p2, l2p1, l2p2,
   				center, ip, t0,t1);
  if (is > 0) {
    // radii match?
    if (abs((l1p1-center).lengthSquared() -
	    (l2p1-center).lengthSquared()) < maxSqerr)
      return center;
  }
  return Vector2d(10000000,10000000);
}

uint Printlines::makeArcs(const Settings::SlicingSettings &slicing,
			   vector<PLine> &lines) const
{
  if (!slicing.UseArcs) return 0;
  if (lines.size() < 2) return 0;
  double maxAngle = slicing.ArcsMaxAngle * M_PI/180;
  if (maxAngle < 0) return 0;
  double arcRadiusSq = 0;  
  Vector2d arccenter;
  guint arcstart = 0;
  for (guint i=1; i < lines.size(); i++) {
    double dangle         = lines[i].calcangle(lines[i-1]);
    double feedratechange = lines[i].feedrate - lines[i-1].feedrate;
    Vector2d center       = arcCenter(lines[i-1], lines[i], 0.06*arcRadiusSq);
    double radiusSq       = (center - lines[i].from).lengthSquared();
    // test if NOT continue arc:
    if (lines[i].arc != 0                  // is an arc already
	|| (lines[i].from-lines[i-1].to).lengthSquared() > 0.05 // not adjacent
	|| abs(feedratechange) > 0.1       // different feedrate
	|| abs(dangle) < 0.0001            // straight continuation
	|| abs(dangle) > maxAngle          // too big angle
	|| ( i>1 && (arccenter-center).lengthSquared() > 0.06*radiusSq ) // center displacement
	) 
      { 
	arccenter   = center;
	arcRadiusSq = radiusSq;
	// this one doesn't fit, so i-1 is the last line that fits
	if (arcstart+2 < i-1) // at least three lines to make an arc
	  i -= makeIntoArc(arcstart, i-1, lines); // straight lines are being removed
	//else 
	// not in arc, set start for potential next arc
	  arcstart = i;
      }
  }  
  // remaining
  if (arcstart+2 < lines.size()-1) 
    makeIntoArc(arcstart, lines.size()-1, lines); 
  return 0;
}

// return how many lines are removed 
uint Printlines::makeIntoArc(guint fromind, guint toind, vector<PLine> &lines) const
{
  if (toind < fromind || toind+1 > lines.size()) return 0;
  //cerr<< "arcstart = " << fromind << endl;
  Vector2d P = lines[fromind].from;
  Vector2d Q = lines[toind].to;
  // get center: intersection of center perpendiculars of 2 chords
  // center perp of start -- endpoint:
  bool fullcircle = (P==Q);
  guint end1ind = toind;
  if (fullcircle) { // take midpoint for first center_perp
    end1ind = (toind+fromind)/2;
  }
  Vector2d chord1p1, chord1p2;
  center_perpendicular(P, lines[end1ind].to, chord1p1, chord1p2);  
  // center perp of midpoint -- endpoint:
  guint start2ind = (toind+fromind)/2;
  Vector2d chord2p1, chord2p2;
  center_perpendicular(lines[start2ind].from, Q, chord2p1, chord2p2);
  // intersection = center
  Vector2d center, ip;
  double t0, t1;
  int is = intersect2D_Segments(chord1p1, chord1p2, chord2p1, chord2p2, 
   				center, ip, t0,t1);
  if (is > 0) {
    double angle;
    if (fullcircle) angle = 2*M_PI;
    else            angle = angleBetween(P-center, Q-center);
    bool ccw = isleftof(center, lines[fromind].from, lines[fromind].to);
    if (!ccw) angle = -angle;
    if (angle<=0) angle+=2*M_PI;
    short arctype = ccw ? -1 : 1; 
    PLine newline(P, Q, lines[fromind].speed, lines[fromind].feedrate,
		  arctype, center, angle);
    // if (fullcircle) {
    //   //cerr << newline.info() << endl;
    //   cerr << newline.getPrintline(0).info() << endl;
    // }
    lines[fromind] = newline;
    lines.erase(lines.begin()+fromind+1, lines.begin()+toind+1);
    return toind-fromind;
  } else cerr << "no Intersection of arc perpendiculars!" << endl;
  return 0;
}

uint Printlines::makeAntioozeRetraction(const Settings::SlicingSettings &slicing,
					vector<PLine> &lines) const
{
  double AOmindistance = slicing.AntioozeDistance,
    AOamount = slicing.AntioozeAmount,
    AOspeed =  slicing.AntioozeSpeed,
    AOrepushratio = slicing.AntioozeRepushRatio;
  // 1. retract: 
  // either halt and retract or retract on the way before the move?
  // 2. repush:
  // simple idea: re-push partially on the end of move 
  // idea 2: move most of the way without repush, then 
  // start repushing so that repush speed matches last part of movement.
  if (lines.size() < 2 || AOmindistance <=0 || AOamount == 0) return 0;
  uint movestart = 0, moveend = 0;
  uint count = lines.size();
  uint i = 0;
  while (i < count) {
    while (i < count && lines[i].feedrate != 0) {
      i++; movestart = i;
    }
    while (i < count && lines[i].feedrate == 0) {
      moveend = i; i++;
    }
    if (moveend < movestart) continue;
    double totaldistance=0, totaltime=0;
    for (uint j = movestart; j <= moveend; j++) {
      double len = lines[j].length();
      totaldistance += len;
      totaltime += len/lines[j].speed;
    }
    if (totaldistance > AOmindistance) {
      double repushamount = AOamount*AOrepushratio;
      // double repushtime = repushamount/AOspeed;
      // double repushdistance = repushtime * lines[moveend].speed;
      // double enddistance = repushdistance;
      // uint firstlinetosplit = moveend+1;
      // while (enddistance > 0 && firstlinetosplit > movestart) {
      // 	firstlinetosplit--;
      // 	enddistance -= lines[firstlinetosplit].length();
      // }
      // if (enddistance > 0) {  // move distance too short, slow down
      // 	double speedratio = AOspeed / totaldistance*totaltime;

      // }
      //cerr << "retract " << totaldistance << ": " <<movestart << "--" << moveend << " num " << moveend-movestart+1 << " split " << firstlinetosplit << " time " << repushtime << " dist " << repushdistance << endl;
      PLine retractl(lines[movestart].from, lines[movestart].from, AOspeed, 0); 
      if (movestart > 0) { // add partial extrusion to line before move
	lines[movestart-1].addAbsoluteExtrusionAmount(-AOamount+repushamount);
	retractl.addAbsoluteExtrusionAmount(-repushamount);
      }
      else 
	retractl.addAbsoluteExtrusionAmount(-AOamount);
      PLine repushl (lines[moveend].to,     lines[moveend].to,     AOspeed, 0);
      repushl.addAbsoluteExtrusionAmount(repushamount);
      if (moveend+1 < lines.size()) { // add rest to next line
	lines[moveend+1].addAbsoluteExtrusionAmount(AOamount-repushamount);
      }
      lines.insert(lines.begin()+moveend+1, repushl); // (inserts before)
      lines.insert(lines.begin()+movestart, retractl);
      i+=2;
    }
    count = lines.size();
  }
  return 0;
}

// split at length 0 < t < 1
uint Printlines::divideline(uint lineindex, const double t, vector<PLine> &lines) const
{
  PLine *l = &lines[lineindex];
  Vector2d d = l->to - l->from;
  vector<Vector2d> points(1);
  points[0] = l->from + d * t * d.length();
  uint nlines = divideline(lineindex, points, lines);
  delete l;
  return nlines;
}

uint Printlines::divideline(uint lineindex, const vector<Vector2d> points,
			    vector<PLine> &lines) const
{
  vector<PLine> newlines;
  PLine l = lines[lineindex];
  for (int i = -1; i < (int)points.size(); i++) {
    Vector2d from, to;
    if (i < 0) from = l.from;
    else       from = points[i];
    if (i > (int)points.size()-2) 
      to = l.to;
    else 
      to = points[i+1];
    newlines.push_back(PLine(from, to, l.speed, l.feedrate));
  }
  if (newlines.size() > 0 && lines.size() > lineindex) {
    //cerr << newlines.size() << " new lines" << endl;
    lines.erase(lines.begin()+lineindex);
    lines.insert(lines.begin()+lineindex, newlines.begin(), newlines.end());
    return newlines.size()-1; // return how many more lines now 
  }
  return 0;
}

// walk around holes
void Printlines::clipMovements(vector<Poly> *polys, vector<PLine> &lines,
			       double maxerr) const
{
  if (polys==NULL) return;
  if (lines.size()==0) return;
  vector<PLine> newlines;
  for (guint i=0; i < lines.size(); i++) {
    if (lines[i].feedrate == 0) {
      int frompoly=-1, topoly=-1;
      for (uint p = 0; p < polys->size(); p++) {
	if ((frompoly==-1) && (*polys)[p].vertexInside(lines[i].from, maxerr))
	  frompoly=(int)p;
	if ((topoly==-1) && (*polys)[p].vertexInside(lines[i].to, maxerr))
	  topoly=(int)p;
      }
      if ((frompoly == -1) || (topoly == -1)) {
	//cerr <<frompoly << " -- " << topoly << endl;
	continue; 
      }
      // line outside polys - ?
      // line inside same poly, find path:
      if (topoly == frompoly) {
#define FASTPATH 0
#if FASTPATH // find shortest path through polygon
	vector<Poly> holes;
	//holes.push_back((*polys)[frompoly]);
	for (uint p = 0; p < polys->size(); p++) {
	  //   //if ((*polys)[p].isHole())
	  if ((*polys)[frompoly].polyInside(&(*polys)[p])) 
	    holes.push_back((*polys)[p]);
	}
	vector<Vector2d> path;
	bool ispath = shortestPath(lines[i].from,lines[i].to,
				   holes, frompoly, path, maxerr);
	if (ispath) {
	  int divisions = (divideline(i,path)); 
	  i += divisions;
	  if (divisions>0)
	    cerr << divisions << " div in poly " << topoly << " - " << ispath << " path " << path.size()<<endl;
	}
	continue; // next line
#else // walk along perimeters
	for (uint p = 0; p < polys->size(); p++) {
	  vector<Intersection> pinter = 
	    (*polys)[p].lineIntersections(lines[i].from,lines[i].to, maxerr);
	  if (pinter.size() > 0) {
	    if (pinter.size()%2 == 0) { // holes
	      std::sort(pinter.begin(), pinter.end());
	      vector<Vector2d> path = 
		(*polys)[p].getPathAround(pinter.front().p, pinter.back().p);
	      // after divide, skip number of added lines -> test remaining line later
	      i += (divideline(i, path, lines)); 
	    }
	  }
	}
#endif
      }
      if (0 && frompoly != -1 && topoly != -1 && frompoly != topoly) {
	cerr <<i << " : "<<frompoly << " p>> " << topoly << endl;	
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
	(*polys)[frompoly].nearestIndices((*polys)[topoly], fromind, toind);
	vector<Vector2d> path;
	//path.push_back(lines[i].from);
	path.push_back((*polys)[frompoly].vertices[fromind]);
	path.push_back((*polys)[topoly].vertices[toind]);
	//path.push_back(lines[i].to);
	//i+= (divideline(i,path)); // test new lines again?
      }
    }
  }
}

void Printlines::setSpeedFactor(double speedfactor, vector<PLine> &lines) const
{
  if (speedfactor == 1) return;
  for (uint i=0; i < lines.size(); i++){
    if (lines[i].feedrate>0)
      lines[i].speed *= speedfactor;
  }
}
double Printlines::slowdownTo(double totalseconds, vector<PLine> &lines) const
{
  double totalnow = totalSecondsExtruding(lines);
  if (totalseconds == 0 || totalnow == 0) return 1;
  double speedfactor = totalnow / totalseconds;
  if (speedfactor >= 1.) return speedfactor;
  setSpeedFactor(speedfactor,lines);
  return speedfactor;
}

// merge too near parallel lines
void Printlines::mergelines(PLine &l1, PLine &l2, double maxdist) const
{
  Vector2d d2 = l2.to - l2.from;
  double len2 = d2.length();
  if (len2==0) return;
  double dist2 = abs(d2.cross(l1.from-l2.from)) / len2 ; // by area of parallelogram 
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

// double Printlines::distance(const Vector2d p, const line l2) const
// {
//   double d=0;
//   Vector2d d2 = l2.to-l2.from;
//   Vector2d dp = p-l2.from;
//   double l = dot(dp,d2);
//   Vector2d pproj = l2.from + d2 *l;
//   d = (p-pproj).length();
//   cerr << "dist " << d<< endl;
//   return d;
// }

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
  while (da>M_PI) da-=M_PI;
  while (da<-M_PI)da+=M_PI;
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


void Printlines::getLines(const vector<PLine> lines, 
			  vector<Vector2d> &olines) const
{
  for (lineCIt lIt = lines.begin(); lIt!=lines.end(); ++lIt){
    if (lIt->is_noop()) continue;
    olines.push_back(lIt->from);
    olines.push_back(lIt->to);
  }
}
void Printlines::getLines(const vector<PLine> lines, 
			  vector<Vector3d> &olines) const
{
  for (lineCIt lIt = lines.begin(); lIt!=lines.end(); ++lIt){
    if (lIt->is_noop()) continue;
    olines.push_back(Vector3d(lIt->from.x,lIt->from.y,z));
    olines.push_back(Vector3d(lIt->to.x,lIt->to.y,z));
  }
}

void Printlines::getLines(const vector<PLine> lines, 
			  vector<PLine3> &plines) const
{
  for (lineCIt lIt = lines.begin(); lIt!=lines.end(); ++lIt){
    if (lIt->is_noop()) continue;
    plines.push_back( PLine3(*lIt,z) );
  }
}

double Printlines::totalLength(const vector<PLine> lines) const 
{
  double l = 0;
  for (lineCIt lIt = lines.begin(); lIt!=lines.end();++lIt){
    l += lIt->length();
  }
  return l;
}

double Printlines::totalSeconds(const vector<PLine> lines) const 
{
  double t = 0;
  for (lineCIt lIt = lines.begin(); lIt!=lines.end();++lIt){
    t += lIt->time() ;
  }
  return t * 60;
}
double Printlines::totalSecondsExtruding(const vector<PLine> lines) const 
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
