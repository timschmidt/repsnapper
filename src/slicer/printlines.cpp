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

void Printlines::addLine(const Vector2d from, const Vector2d to, 
			 double speed, double movespeed, double feedrate)
{
  if (to==from) return;
  if (lines.size() > 0) {
    struct line lastline = lines.back();
    if (!(lastline.to == from)) { // add moveline
      struct line l;
      l.from = lastline.to;
      l.to = from;
      l.speed=movespeed;
      l.feedrate=0;
      l.angle = angle(l);
      l.arc = false;
      lines.push_back(l);
    }
  }
  struct line l;
  l.from = from;
  l.to=to;
  l.speed=speed;
  l.feedrate=feedrate;
  //l.lengthSq = lengthSq(l);
  l.angle = angle(l);
  l.arc = false;
  lines.push_back(l);
}

void Printlines::addPoly(const Poly poly, int startindex, 
			 double speed, double movespeed)
{
  vector<Vector2d> pvert;
  poly.getLines(pvert,startindex);
  assert(pvert.size() % 2 == 0);
  for (uint i=0; i<pvert.size();i+=2){
    addLine(pvert[i], pvert[i+1], speed, movespeed, poly.getExtrusionFactor());
  }
}

void Printlines::makeLines(const vector<Poly> polys, 
			   const vector<Poly> *clippolys,
			   Vector2d &startPoint,
			   bool displace_startpoint, 
			   double minspeed, double maxspeed, double movespeed, // mm/s
			   double linewidth, double linewidthratio, double optratio,
			   double maxArcAngle, bool linelengthsort)
{
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
	addPoly(polys[npindex], nvindex, maxspeed, movespeed);
	done[npindex]=true;
	ndone++;
      }
      if (lines.size()>0)
	startPoint = lastPoint();
    }
  if (count == 0) return;
  setZ(polys.back().getZ());
  clipMovements(clippolys, linewidth/2.);
  optimize(minspeed, maxspeed, movespeed, 
	   linewidth, linewidthratio, optratio, maxArcAngle);
}

double Printlines::angle(const line l) const
{
  return angleBetween(l.from, l.to);
}
double Printlines::angle(const line l1,const line l2) const
{
  return angleBetween((l2.to-l2.from), (l1.to-l1.from));
}

double Printlines::lengthSq(const line l) const
{
  if (!l.arc) 
    return (l.to-l.from).lengthSquared();
  else
    return pow(length(l),2);
}

double Printlines::length(const line l) const
{
  if (!l.arc) 
    return sqrt(lengthSq(l));
  else {
    double radius = (l.from-l.arccenter).length();
    return radius * l.angle;
  }
}

void Printlines::optimize(double minspeed, double maxspeed, double movespeed,
			  double linewidth, double linewidthratio, double optratio,
			  double maxArcAngle)
{
  //cout << "optimize " ; printinfo();
  //optimizeLinedistances(linewidth);
  // optimizeCorners(linewidth,linewidthratio,optratio);
  // double E=0;Vector3d start(0,0,0);
  // cout << GCode(start,E,1,1000);
  makeArcs(maxArcAngle);
}

// gets center of common arc of 2 lines if radii inside maxerr range
Vector2d Printlines::arcCenter(const struct line l1, const struct line l2,
			       double maxerr) const
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
	    (l2p1-center).lengthSquared()) < maxerr)
      return center;
  }
  return Vector2d(10000000,10000000);
}

guint Printlines::makeArcs(double maxAngle)
{
  if (lines.size() < 2) return 0;
  if (maxAngle < 0) return 0;
  double arcRadiusSq = 0;  
  Vector2d arccenter;
  guint  arcstart = 0;
  for (guint i=1; i < lines.size(); i++) {
    double dangle         = angle(lines[i], lines[i-1]);
    double feedratechange = lines[i].feedrate - lines[i-1].feedrate;
    Vector2d center       = arcCenter(lines[i-1], lines[i], 0.03*arcRadiusSq);
    double radiusSq       = (center - lines[i].from).lengthSquared();
    // test if NOT continue arc:
    if (lines[i].arc != 0                  // is an arc
	|| (lines[i].from-lines[i-1].to).lengthSquared() > 0.01 // not adjacent
	|| abs(feedratechange) > 0.1       // different feedrate
	|| abs(dangle) < 0.001             // straight continuation
	|| abs(dangle) > maxAngle && 2*M_PI-abs(dangle) > maxAngle  // too big angle
	|| (arccenter-center).lengthSquared() > 0.03*radiusSq // center displacement
	) { 
      arccenter   = center;
      arcRadiusSq = radiusSq;
      // this one doesn't fit, so i-1 is the last line that fits
      if (arcstart+2 < i-1) // at least three arc lines
	i -= makeIntoArc(arcstart, i-1) ; // straight lines are being removed
      //else 
      // not in arc, set start for potential next arc
      arcstart = i;
    } 
  }  
  // remaining
  if (arcstart+2 < lines.size()-1) 
    makeIntoArc(arcstart, lines.size()-1); 
  return 0;
}

// return how many lines are removed 
guint Printlines::makeIntoArc(guint fromind, guint toind)
{
  if (toind < fromind || toind+1 > lines.size()) return 0;
  Vector2d P = lines[fromind].from;
  Vector2d Q = lines[toind].to;
  // center perp of start -- endpoint:
  Vector2d connp1,connp2;
  center_perpendicular(P, Q, connp1, connp2);  
  // center perp of midpoint -- endpoint:
  guint midind = (toind+fromind)/2;
  Vector2d midp1,midp2;
  center_perpendicular(lines[midind].from, lines[toind].to, midp1, midp2);
  // intersection = center
  Vector2d center, ip;
  double t0, t1;
  int is = intersect2D_Segments(connp1, connp2, midp1, midp2, 
   				center, ip, t0,t1);
  if (is > 0) {
    double angle = angleBetween(P-center, Q-center);
    bool ccw = isleftof(center, lines[fromind].from, lines[fromind].to);
    if (!ccw) angle = -angle;
    if (angle<=0) angle+=2*M_PI;
    short arctype = ccw ? -1 : 1; 
    struct line newline;
    newline.from      = P;
    newline.to        = Q;
    newline.speed     = lines[fromind].speed;
    newline.feedrate  = lines[fromind].feedrate;
    newline.angle     = angle;  // for arc length calculation (->extrusion)
    newline.arccenter = center;
    newline.arc       = arctype;
    lines[fromind] = newline;
    lines.erase(lines.begin()+fromind+1, lines.begin()+toind+1);
    return toind-fromind;
  } else cerr << "no Intersection of arc perpendiculars!" << endl;
  return 0;
}

// split at length 0 < t < 1
uint Printlines::divideline(uint lineindex, const double t) 
{
  line *l = &lines[lineindex];
  Vector2d d = l->to - l->from;
  vector<Vector2d> points(1);
  points[0] = l->from + d * t * d.length();
  uint nlines = divideline(lineindex, points);
  delete l;
  return nlines;
}

uint Printlines::divideline(uint lineindex, const vector<Vector2d> points) 
{
  vector<line> newlines;
  line l = lines[lineindex];
  for (int i = -1; i < (int)points.size(); i++) {
    line nl;
    if (i < 0) nl.from = l.from;
    else nl.from=points[i];
    if (i > (int)points.size()-2) nl.to=l.to;
    else nl.to=points[i+1];
    nl.speed = l.speed;
    nl.feedrate = l.feedrate;
    nl.arc = 0;
    nl.angle = angle(nl);
    newlines.push_back(nl);
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
void Printlines::clipMovements(const vector<Poly> *polys, double maxerr) 
{
  if (polys==NULL) return;
  if (lines.size()==0) return;
  vector<line> newlines;
  for (guint i=0; i < lines.size(); i++) {
    if (lines[i].feedrate == 0) {
      int frompoly=-1, topoly=-1;
      for (uint p = 0; p < polys->size(); p++) {
	if (frompoly==-1 && (*polys)[p].vertexInside(lines[i].from))
	  frompoly=(int)p;
	if (topoly==-1 && (*polys)[p].vertexInside(lines[i].to))
	  topoly=(int)p;
	vector<Intersection> pinter = 
	  (*polys)[p].lineIntersections(lines[i].from,lines[i].to, maxerr);
	if (pinter.size() > 0) {
	  if (pinter.size()%2 == 0) { // holes
	    std::sort(pinter.begin(), pinter.end());
	    vector<Vector2d> path = 
	      (*polys)[p].getPathAround(pinter.front().p, pinter.back().p);
	    i += (divideline(i,path)); //if (i>0) i--; // test new lines again?
	  }
	}
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

void Printlines::setSpeedFactor(double speedfactor)
{
  if (speedfactor == 1) return;
  for (uint i=0; i < lines.size(); i++){
    if (lines[i].feedrate>0)
      lines[i].speed *= speedfactor;
  }
}
void Printlines::slowdownTo(double totalseconds) 
{
  if (totalseconds == 0) return;
  double totalnow = totalSecondsExtruding();
  if (totalnow == 0) return;
  double speedfactor = totalnow / totalseconds;
  if (speedfactor >= 1.) return;
  setSpeedFactor(speedfactor);
}

// merge too near parallel lines
void Printlines::mergelines(line &l1, line &l2, double maxdist) 
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

void Printlines::optimizeLinedistances(double maxdist)
{
 uint count = lines.size();
 for (uint i=0; i<count; i++){
   for (uint j=i+1; i<count; i++){
     if (abs(angle(lines[i], lines[j]))<0.1){ // parallel
       mergelines(lines[i],lines[j],maxdist);

       // if (distance(lines[i].from, lines[j]) < maxdist){
       // 	 lines[i].to = lines[i].from;
       // 	 cerr<< "removed "<<i<< endl;
	 // delete line?
     }
   }
 }
}

bool Printlines::capCorner(line &l1, line &l2, 
			  double linewidth, double linewidthratio, 
			  double optratio)
{
  double MINLEN = 4; // minimum line length to handle
  bool done = false;
  if (l1.to!=l2.from) return done;  // only handle adjacent lines
  //if ((l1.to-l2.from).length() > linewidth ) return done;
  double da = angle(l1,l2);
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

void Printlines::optimizeCorners(double linewidth, double linewidthratio, double optratio)
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


Vector2d Printlines::lastPoint() const
{
  return lines.back().to;
}


void Printlines::getLines(vector<Vector2d> &olines) const
{
  //cout << "getlines2 "; printinfo();
  for (lineCIt lIt = lines.begin(); lIt!=lines.end(); ++lIt){
    if (lIt->from != lIt->to){
      olines.push_back(lIt->from);
      olines.push_back(lIt->to);
    }
  }
}
void Printlines::getLines(vector<Vector3d> &olines) const
{
  //cout << "getlines3 "; printinfo();
  for (lineCIt lIt = lines.begin(); lIt!=lines.end(); ++lIt){
    if (lIt->from != lIt->to){
      olines.push_back(Vector3d(lIt->from.x,lIt->from.y,z));
      olines.push_back(Vector3d(lIt->to.x,lIt->to.y,z));
    }// else cerr<< "zero line" << endl;
  }
}

void Printlines::getLines(vector<printline> &plines) const
{
  for (lineCIt lIt = lines.begin(); lIt!=lines.end(); ++lIt){
    if (lIt->from != lIt->to){
      struct printline pline;
      pline.from            = Vector3d(lIt->from.x, lIt->from.y, z);
      pline.to              = Vector3d(lIt->to.x, lIt->to.y, z);
      pline.speed           = lIt->speed;
      pline.arc             = lIt->arc;
      pline.extrusionfactor = lIt->feedrate;
      if (lIt->arc) {
	Vector2d arcIJK2 = lIt->arccenter - lIt->from;
	pline.arcIJK = Vector3d(arcIJK2.x, arcIJK2.y, 0);
	double linearlength = (lIt->to - lIt->from).length();
	pline.extrusionfactor *= length(*lIt) / linearlength;
      }
      plines.push_back(pline);
    } // else cerr<< "zero line" << endl;
  }
}

double Printlines::totalLength() const 
{
  double l = 0;
  for (lineCIt lIt = lines.begin(); lIt!=lines.end();++lIt){
    l += length(*lIt);
  }
  return l;
}

double Printlines::totalSeconds() const 
{
  double t = 0;
  for (lineCIt lIt = lines.begin(); lIt!=lines.end();++lIt){
    t += length(*lIt) / lIt->speed ;
  }
  return t * 60;
}
double Printlines::totalSecondsExtruding() const 
{
  double t = 0;
  for (lineCIt lIt = lines.begin(); lIt!=lines.end();++lIt){
    if (lIt->feedrate>0)
      t += length(*lIt) / lIt->speed ;
  }
  return t * 60;
}

string Printlines::GCode(Vector3d &lastpos, double &E, double feedrate, double speed) const
{
  ostringstream o;
  for (lineCIt lIt = lines.begin(); lIt!=lines.end();++lIt){
    o << GCode(*lIt, lastpos, E, speed * lIt->feedrate, speed * lIt->speed);
  }
  cerr << "PL gcode " << o.str()<< endl;
  return o.str();
}


string Printlines::GCode(line l, Vector3d &lastpos, double &E, double feedrate, 
			 double speed) const
{
  double distline = (l.to-l.from).length();
  Vector3d from=Vector3d(l.from.x,l.from.y,z);
  Vector3d to=Vector3d(l.to.x,l.to.y,z);
  double distmove = (from-lastpos).length();
  double EX = distline * l.feedrate * feedrate;
  E+=EX;
  lastpos = to;
  ostringstream o;
  if (distmove>0.001)
    o << "G1 X"<<from.x<<" Y"<<from.y<<" Z"<<from.z<<" E0 F"<<speed << endl;
  if (distline>0.001) 
    o << "G1 X"<<to.x<<" Y"<<to.y<<" Z"<<to.z<<" E"<<EX<<" F"<<speed << endl;
  return o.str();
}

string Printlines::info() const
{
  ostringstream ostr;
  ostr << "Printlines "<<name<<": " <<size()<<" lines"
       << ", total "<<totalLength()<< "mm" ;
  return ostr.str();
}
