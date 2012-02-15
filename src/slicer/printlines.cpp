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
			 double speed, double feedrate)
{
  if (to==from) return;
  if (lines.size() > 0) {
    struct line lastline = lines.back();
    if (!(lastline.to == from)) { // add moveline
      struct line l;
      l.from = lastline.to;
      l.to = from;
      l.speed=speed;
      l.feedrate=0;
      l.angle = angle(l);
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
  lines.push_back(l);
}

void Printlines::addPoly(const Poly poly, int startindex, double speed)
{
  vector<Vector2d> pvert;
  poly.getLines(pvert,startindex);
  assert(pvert.size() % 2 == 0);
  for (uint i=0; i<pvert.size();i+=2){
    addLine(pvert[i], pvert[i+1], speed, poly.getExtrusionFactor());
  }
}

void Printlines::makeLines(const vector<Poly> polys, 
			   Vector2d &startPoint,
			   bool displace_startpoint, 
			   double minspeed, double maxspeed, // mm/s
			   double linewidth, double linewidthratio, double optratio,
			   bool linelengthsort)
{
  uint count = polys.size();
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
      if (linelengthsort && npindex>=0) lastavlength = polys[npindex].averageLinelengthSq();
      nstdist = 1000000;
      for(size_t q=0; q<count; q++) { // find nearest polygon
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
      addPoly(polys[npindex], nvindex, maxspeed);
      done[npindex]=true;
      ndone++;
      if (lines.size()>0)
	startPoint = lastPoint();
    }
  if (count) {
    setZ(polys.back().getZ());
    optimize(minspeed, maxspeed, linewidth, linewidthratio, optratio);
  }
}

double Printlines::angle(Vector2d p) const
{
  double a = atan2(p.y,p.x);
  if (a<0) a+=2.*M_PI;
  return a;
}
double Printlines::angle(const line l) const
{
  return angle(l.to-l.from);
}
double Printlines::angle(const line l1,const line l2) const
{
  double da=l2.angle-l1.angle;
  
  // double da = angle(l2)-angle(l1);
  return da;
}

double Printlines::lengthSq(const line l) const
{
  return (l.to-l.from).lengthSquared();
}

double Printlines::length(const line l) const
{
  return sqrt(lengthSq(l));
}

void Printlines::optimize(double minspeed, double maxspeed,
			  double linewidth, double linewidthratio, double optratio)
{
  //cout << "optimize " ; printinfo();
  //optimizeLinedistances(linewidth);
  // optimizeCorners(linewidth,linewidthratio,optratio);
  // double E=0;Vector3d start(0,0,0);
  // cout << GCode(start,E,1,1000);
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
    if (i<0) nl.from = l.from;
    else nl.from=points[i];
    if (i>(int)points.size()-2) nl.to=l.to;
    else nl.to=points[i+1];
    nl.speed = l.speed;
    nl.feedrate = l.feedrate;
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
void Printlines::clipMovements(const vector<Poly> polys, double maxerr) 
{
  if (lines.size()==0) return;
  vector<line> newlines;
  for (uint i=0; i < lines.size(); i++) {
    if (lines[i].feedrate == 0) {
      for (uint p = 0; p < polys.size(); p++) {
	vector<Intersection> pinter = 
	  polys[p].lineIntersections(lines[i].from,lines[i].to, maxerr);
	if (pinter.size() > 0) {
	  if (pinter.size()%2 == 0) { // holes
	    std::sort(pinter.begin(), pinter.end());
	    //cerr << pinter.size() << " intersections at poly " << p <<  endl;
	    vector<Vector2d> path = 
	      polys[p].getPathAround(pinter.front().p, pinter.back().p);
	    //cerr << i << " -- " << lines.size() << " --  path " << path.size()<< endl;
	    i += (divideline(i,path)); //if (i>0) i--; // test new lines again?
	  }
	  else if (0) {//(pinter.size()==1){ // going out
	    // find polys that are connected by this line
	    int frompoly=-1, topoly=-1;
	    for (uint op = 0; op < polys.size(); op++) {
	      if (polys[op].vertexInside(lines[i].from)) frompoly=(int)op;
	      if (polys[op].vertexInside(lines[i].to)) topoly=(int)op;
	    }
	    cerr <<i << " : "<<frompoly << " p>> " << topoly << endl;
	    if (frompoly>-1 && topoly>-1 && frompoly!=topoly) {
	      int fromind,toind;
	      polys[frompoly].nearestIndices(polys[topoly], fromind, toind);
	      cerr <<fromind << " i>> " << toind << endl;
	      vector<Vector2d> path;
	      //path.push_back(lines[i].from);
	      path.push_back(polys[frompoly].vertices[fromind]);
	      path.push_back(polys[topoly].vertices[toind]);
	      //path.push_back(lines[i].to);
	      i+= (divideline(i,path)); // test new lines again?
	    }
	    //   cerr << pinter.size()  <<" uneven intersections at poly " << p << endl;
	  }
	}
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
      pline.from = Vector3d(lIt->from.x,lIt->from.y,z);
      pline.to = Vector3d(lIt->to.x,lIt->to.y,z);
      pline.speed = lIt->speed;
      pline.extrusionfactor = lIt->feedrate;
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
  return o.str();
}


// not used
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
  if (distmove>0.01)
    o << "G1 X"<<from.x<<" Y"<<from.y<<" Z"<<from.z<<" E0 F"<<speed << endl;
  if (distline>0.01) 
    o << "G1 X"<<to.x<<" Y"<<to.y<<" Z"<<to.z<<" E"<<EX<<" F"<<speed << endl;
  return o.str();
}

string Printlines::info() const
{
  ostringstream ostr;
  ostr << "Printlines " <<size()<<" lines"
       << ", total "<<totalLength()<< "mm" ;
  return ostr.str();
}
