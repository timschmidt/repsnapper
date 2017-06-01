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

#include "infill.h"
#include "poly.h"
#include "layer.h"


vector<struct Infill::pattern> Infill::savedPatterns;
#ifdef _OPENMP
omp_lock_t Infill::save_lock;
#endif

void hilbert(int level,int direction, double infillDistance, vector<Vector2d> &v);


Infill::Infill()
  : extrusionfactor(1), cached(false)
{
  m_tofillpolys.clear();
}


Infill::Infill (Layer *mlayer, double extrfactor)
  : cached(false)
{
  layer = mlayer;
  extrusionfactor = extrfactor;
  m_tofillpolys.clear();
}

Infill::~Infill()
{
  clear();
  extrusionfactor = 1.;
}

void Infill::clear()
{
  infillpolys.clear();
  infillvertices.clear();
  m_tofillpolys.clear();
}

void Infill::clearPatterns() {
  for (uint i=0; i<savedPatterns.size(); i++) {
    savedPatterns[i].type = INVALIDINFILL;
    savedPatterns[i].cpolys.clear();
  }
  savedPatterns.clear();
  //cerr << "clearpatterns " << savedPatterns.size() << endl;
#ifdef _OPENMP
  omp_destroy_lock(&save_lock);
  omp_init_lock(&save_lock);
#endif
}



// fill polys with type etc.
void Infill::addPoly(double z, const Poly &poly, InfillType type,
		     double infillDistance, double offsetDistance, double rotation)
{
  vector<Poly> polys; polys.push_back(poly);
  addPolys(z, polys, type, infillDistance, offsetDistance, rotation);
}
void Infill::addPolys(double z, const vector<Poly> &polys, InfillType type,
		      double infillDistance, double offsetDistance, double rotation)
{
  this->infillDistance = infillDistance;

#ifdef _OPENMP
  omp_set_lock(&save_lock);
#endif
  ClipperLib::Paths patterncpolys =
    makeInfillPattern(type, polys, infillDistance, offsetDistance, rotation);
#ifdef _OPENMP
  omp_unset_lock(&save_lock);
#endif
  addPolys(z, polys, patterncpolys, offsetDistance);
}

void Infill::addPoly(double z, const ExPoly &expoly, InfillType type,
		     double infillDistance, double offsetDistance, double rotation)
{
  vector<Poly> polys = Clipping::getPolys(expoly);
  addPolys (z, polys, type, infillDistance, offsetDistance, rotation);
}

void Infill::addPolys(double z, const vector<ExPoly> &expolys, InfillType type,
		      double infillDistance, double offsetDistance, double rotation)
{
  vector<Poly> polys = Clipping::getPolys(expolys);
  addPolys (z, polys, type, infillDistance, offsetDistance, rotation);
}

// calculates angles for bridges
// void Infill::addBridgeInfill(double z, const vector<Poly> polys,
// 			     double infillDistance, double offsetDistance,
// 			     vector<polys> bridgepillars)
// {
//   type = BridgeInfill;
//   this->infillDistance = infillDistance;

//   omp_set_lock(&save_lock);
//   ClipperLib::Paths patterncpolys =
//     makeInfillPattern(type, polys, infillDistance, offsetDistance, rotation);
//   addPolys(z, polys, patterncpolys, offsetDistance);
//   omp_unset_lock(&save_lock);
// }

// fill polys with fillpolys
void Infill::addPolys(double z, const vector<Poly> &polys,
		      const vector<Poly> &fillpolys,
		      double offsetDistance)
{
  addPolys(z, polys, Clipping::getClipperPolygons(fillpolys), offsetDistance);
}

// clip infill pattern polys against polys
void Infill::addPolys(double z, const vector<Poly> &polys,
		      const ClipperLib::Paths &patterncpolys,
		      double offsetDistance)
{
  Clipping clipp;
  clipp.addPolys   (polys,         subject);
  clipp.addPolygons(patterncpolys, clip);
  clipp.setExtrusionFactor(extrusionfactor); // set my extfactor
  clipp.setZ(z);
  vector<Poly> result = clipp.intersect();
  if (m_type==PolyInfill)  // reversal from evenodd clipping
    for (uint i = 0; i<result.size(); i+=2)
      result[i].reverse();
  addInfillPolys(result);
}

// generate infill pattern as a vector of polygons
ClipperLib::Paths Infill::makeInfillPattern(InfillType type,
					       const vector<Poly> &tofillpolys,
					       double infillDistance,
					       double offsetDistance,
					       double rotation)
{
  ClipperLib::Paths cpolys;
  m_tofillpolys = tofillpolys;
  m_type = type;

  if (tofillpolys.size()==0) return cpolys;
  cached = false;
  //cerr << "have " << savedPatterns.size()<<" saved patterns " << endl;
  // look for saved pattern for this rotation
  const Vector2d Min = layer->getMin();
  const Vector2d Max = layer->getMax();
  while (rotation > 2*M_PI) rotation -= 2*M_PI;
  while (rotation < 0) rotation += 2*M_PI;
  m_angle = rotation;

  if (type == HexInfill) {
    if (layer->LayerNo%2 != 0)
      m_angle = M_PI/2;
    else
      m_angle = 0.;
  }
  //omp_set_lock(&save_lock);
  //int tid = omp_get_thread_num( );
  //cerr << "thread "<<tid << " looking for pattern " << endl;
  if (type != PolyInfill) { // can't save PolyInfill
    if (savedPatterns.size()>0){
      vector<uint> too_small;
      for (vector<struct pattern>::iterator sIt=savedPatterns.begin();
      	   sIt != savedPatterns.end(); sIt++){
	if (sIt->type == type &&
	    abs((sIt->distance-infillDistance)/infillDistance) < 0.01 &&
	    abs((sIt->angle-m_angle)/m_angle) < 0.01 )
	  {
	    //cerr << name << " found saved pattern no " << sIt-savedPatterns.begin() << " with " << sIt->cpolys.size() <<" polys"<< endl << "type "<< sIt->type << sIt->Min << sIt->Max << endl;
	    // is it too small for this layer?
	    if (sIt->Min.x() > Min.x() || sIt->Min.y() > Min.y() ||
		sIt->Max.x() < Max.x() || sIt->Max.y() < Max.y())
	      {
		too_small.push_back(sIt-savedPatterns.begin());
		//break; // there is no other match
	      }
	    else {
	      cached = true;
	      return sIt->cpolys;
	    }
	  }
      }
      sort(too_small.rbegin(), too_small.rend());
      for (uint i = 0; i < too_small.size(); i++) {
	savedPatterns.erase(savedPatterns.begin()+too_small[i]);
      }
    }
  }
  //omp_unset_lock(&save_lock);
  // none found - make new:
  bool zigzag = false;
  switch (type)
    {
    case HexInfill:
      {
	Vector2d pMin=Vector2d::ZERO, pMax=Max*1.1;
	double hexd = infillDistance;// /(1+sqrt(3.)/4.);
	double hexa = hexd*sqrt(3.)/2.;
	// the two parts have to fit
	Poly poly(this->layer->getZ());
	if (layer->LayerNo%2 != 0) { // two alternating parts
	  double ymax = pMax.y();;
	  for (double x = pMin.x(); x < pMax.x(); x += 2*hexa) {
	    poly.addVertex(x, pMin.y());
	    for (double y = pMin.y(); y < pMax.y(); ) {
	      poly.addVertex(x, y);
	      poly.addVertex(x+hexa, y+hexd/2);
	      y+=1.5*hexd;
	      poly.addVertex(x+hexa, y);
	      poly.addVertex(x, y+hexd/2);
	      y+=1.5*hexd;
	      ymax = y;
	    }
	    for (double y = ymax; y > pMin.y(); y-=2*hexd) {
	      double x2 = x+hexa+layer->thickness/10.; // offset to not combine polys
	      y+=0.5*hexd;
	      poly.addVertex(x2, y);
	      poly.addVertex(x2+hexa, y-hexd/2);
	      y-=1.5*hexd;
	      poly.addVertex(x2+hexa, y);
	      poly.addVertex(x2, y-hexd/2);
	    }
	  }
	  poly.addVertex(pMax.x(), pMin.y()-infillDistance);
	  poly.addVertex(pMin.x(), pMin.y()-infillDistance);
	} else { // other layer, simpler
	  double xmax = pMax.x();;
	  for (double y = pMin.y(); y < pMax.y(); y += 3*hexd) {
	    for (double x = pMin.x(); x < pMax.x(); ) {
	      poly.addVertex(x, y);
	      poly.addVertex(x+hexa, y+hexd/2.);
	      x += 2*hexa;
	      xmax = x;
	    }
	    for (double x = xmax; x > pMin.y(); x -= 2*hexa) {
	      double y2 = y+1.5*hexd;
	      poly.addVertex(x+hexa,  y2);
	      poly.addVertex(x, y2+hexd/2);
	    }
	  }
	  poly.addVertex(pMin.x(), pMax.y()-infillDistance);
	  poly.addVertex(pMin.x(), pMin.y()-infillDistance);
	}
	// Poly poly2 = poly; poly2.move(Vector2d(infillDistance/2,0));
	vector<Poly> polys(1);
	polys[0] = poly;
	// polys[1] = poly2;
	cpolys = Clipping::getClipperPolygons(polys);
      }
      break;
    case SmallZigzagInfill: // small zigzag lines -> square pattern
      zigzag = true;
    //case ZigzagInfill: // long zigzag lines
    case SupportInfill:
    case RaftInfill:
    case BridgeInfill:
    case ParallelInfill:
      {
	Vector2d center = (Min+Max)/2.;
	// make square that masks everything even when rotated
	Vector2d diag = Max-Min;
	double square = max(diag.x(),diag.y());
	Vector2d sqdiag(square*2/3,square*2/3);
	Vector2d pMin=center-sqdiag, pMax=center+sqdiag;
	if (zigzag) pMin=Vector2d::ZERO; // fixed position
	// cerr << pMin << "--"<<pMax<< "::"<< center << endl;
	Poly poly(this->layer->getZ());
	uint count = 0;
	for (double x = pMin.x(); x < pMax.x(); ) {
	  double x2 = x+infillDistance;
	  poly.addVertex(x, pMin.y());
	  if (zigzag) { // zigzag -> squares
	    double ymax=pMax.y();;
	    for (double y = pMin.y(); y < pMax.y(); y += 2*infillDistance) {
	      poly.addVertex(x, y);
	      poly.addVertex(x2, y+infillDistance);
	      ymax = y;
	    }
	    for (double y = ymax; y > pMin.y(); y -= 2*infillDistance) {
	      poly.addVertex(x2, y+infillDistance);
	      poly.addVertex(x2+infillDistance, y);
	    }
	    x += 2*infillDistance;
	  } else {      // normal line infill
	    if (count%2){
	      poly.addVertex(x,  pMin.y());
	      poly.addVertex(x2, pMin.y());
	    } else {
	      poly.addVertex(x,  pMax.y());
	      poly.addVertex(x2, pMax.y());
	    }
	    x += infillDistance;
	  }
	  count ++;
	}
	poly.addVertex(pMax.x(), pMin.y()-infillDistance);
	poly.addVertex(pMin.x(), pMin.y()-infillDistance);
	// Poly poly2 = poly; poly2.move(Vector2d(infillDistance/2,0));
	if (!zigzag)
	  poly.rotate(center,m_angle);
	// poly2.rotate(center,rotation);
	vector<Poly> polys(1);
	polys[0] = poly;
	// polys[1] = poly2;
	cpolys = Clipping::getClipperPolygons(polys);
      }
      break;
    case HilbertInfill:
      {
	Poly poly(this->layer->getZ());
	double square = MAX(Max.x()-Min.x(),Max.y()-Min.y());
	if (infillDistance<=0) break;
	int level = (int)ceil(log2(2*square/infillDistance));
	if (level<0) break;
	//cerr << "level " << level;
	// start at 0,0 to get the same location for all layers
	poly.addVertex(0,0);
	hilbert(level, 0, infillDistance,  poly.vertices);
	vector<Poly> polys(1);
	polys[0] = poly;
	cpolys = Clipping::getClipperPolygons(polys);
      }
      break;
    case ThinInfill:
      {
	// just use the poly itself at half extrusion rate
	cpolys = Clipping::getClipperPolygons(tofillpolys);

	// adjust extrusion rate - see how thin it is:
	const uint num_div = 10;
	double shrink = 0.5*infillDistance/num_div;
	//cerr << "shrink " << shrink << endl;
	uint count = 0;
//	uint num_polys = tofillpolys.size();
	vector<Poly> shrinked = tofillpolys;
	while (true) {
	  shrinked = Clipping::getOffset(shrinked,-shrink);
	  count++;
	  //cerr << shrinked.size() << " - " << num_polys << endl;
	  if (shrinked.size() == 0) break; // stop when poly is gone
	}
	extrusionfactor = 0.5 + 0.5/num_div * count;
	//cerr << "ex " << extrusionfactor << endl;
	//cpolys = Clipping::getClipperPolygons(opolys);
      }
      break;
    case PolyInfill: // fill all polygons with their shrinked polys
      {
	vector< vector<Poly> > ipolys; // all offset shells
	for (uint i=0; i < tofillpolys.size(); i++){
	  double parea = Clipping::Area(tofillpolys[i]);
	  // make first larger to get clip overlap
	  double firstshrink = 0.5*infillDistance;
	  if (parea<0) firstshrink = -firstshrink;
	  vector<Poly> shrinked  = Clipping::getOffset(tofillpolys[i], firstshrink);
	  vector<Poly> shrinked2 = Clipping::getOffset(shrinked, 0.5*infillDistance);
	  for (uint i=0;i<shrinked2.size();i++)
	    shrinked2[i].cleanup(0.1*infillDistance);
	  ipolys.push_back(shrinked2);
	  double area = Clipping::Area(shrinked);
	  //cerr << "shr " << parea << " - " <<area<< " - " << " : " <<endl;
	  // int lastnumpolys=0;
	  // int shrcount=0;
	  while (shrinked.size()>0){
	    if (area*parea < 0)  break; // went beyond zero size
	    // cerr << "shr " <<parea << " - " <<area<< " - " << shrcount << " : " <<endl;
	    shrinked2 = Clipping::getOffset(shrinked, 0.5*infillDistance);
	    for (uint i=0;i<shrinked2.size();i++)
	      shrinked2[i].cleanup(0.1*infillDistance);
	    ipolys.push_back(shrinked2);
	    //lastnumpolys = shrinked.size();
	    shrinked = Clipping::getOffset(shrinked,-infillDistance);
	    for (uint i=0;i<shrinked.size();i++)
	      shrinked[i].cleanup(0.1*infillDistance);
	    // cerr << "shr2 " <<parea << " - " <<area<< " - " << shrcount << " : " <<
	    //   shrinked.size()<<endl;
	    // shrcount++;
	    area = Clipping::Area(shrinked);
	  }
	}
	vector<Poly> opolys;
	for (uint i=0;i<ipolys.size();i++){
	  opolys.insert(opolys.end(),ipolys[i].begin(),ipolys[i].end());
	}
	//cerr << "opolys " << opolys.size() << endl;
	cpolys = Clipping::getClipperPolygons(opolys);
	//cerr << "cpolys " << cpolys.size() << endl;
      }
      break;
    default:
      cerr << "infill type " << type << " unknown "<< endl;
    }
  // save
  //tid = omp_get_thread_num( );
  //cerr << "thread "<<tid << " saving pattern " << endl;
  //cerr << "cpolys " << cpolys.size() << endl;
  if (type != PolyInfill &&
      type != ZigzagInfill &&
      type != ThinInfill ) // can't save these
    {
      struct pattern newPattern;
      newPattern.type=type;
      newPattern.angle=m_angle;
      newPattern.distance=infillDistance;
      newPattern.cpolys=cpolys;
      newPattern.Min=Min;
      newPattern.Max=Max;
      savedPatterns.push_back(newPattern);
      //cerr << "saved pattern " << endl;
    }
  return cpolys;
}


int smallest(const vector<double> &nums, double &minimum)
{
  minimum = INFTY;
  int minind = -1;
  for (uint i = 0; i < nums.size(); i++ ) {
    if (nums[i] < minimum) {
      minimum = nums[i];
      minind = i;
    }
  }
  return minind;
}

bool intersectsPolys(const Vector2d &P1, const Vector2d &P2,
		     const vector<Poly> &polys, double err=0.01)
{
  Intersection inter;
  for (size_t ci = 0; ci < polys.size(); ci++) {
    // bool in1 = polys[ci].vertexInside(P1);
    // bool in2 = polys[ci].vertexInside(P2);
    // if (in1 != in2) return true;
    vector<Intersection> inter = polys[ci].lineIntersections(P1, P2, err);
    if (inter.size() > 0) {
      return true;
    }
  }
  return false;
}

// sort (parallel) lines into polys so that each poly can be an extrusion path
// polys will later be connected by moves (as printlines)
// that is: connect nearest lines, but when connection intersects anything,
//          start a new path (poly)
vector<Poly> Infill::sortedpolysfromlines(const vector<infillline> &lines, double z)
{
  vector<Poly> polys;
  uint count = lines.size();
  if (count == 0) return polys;
  vector<Poly> clippolys = Clipping::getOffset(m_tofillpolys,0.1);

  vector<bool> done(count);
  for (uint i = 0; i < count; i++ ) done[i] = false;
  Poly p(z, extrusionfactor);
  p.setClosed(false);
  p.addVertex(lines[0].from);
  p.addVertex(lines[0].to);
  uint donelines = 1;
  while(donelines < count)
    {
      vector<double> dist(4);
      Vector2d l1,l2;
      double mindistline = INFTY;
      uint minindline = 0;
      uint minind = 0;
      double mindist = INFTY;
      // find nearest line for current p endpoints:
      for (uint j = 1; j < count; j++)
	if (!done[j]) {
	  dist[0] = (p.front()-lines[j].from).squared_length();
	  dist[1] = (p.front()-lines[j].to  ).squared_length();
	  dist[2] = (p.back() -lines[j].from).squared_length();
	  dist[3] = (p.back() -lines[j].to  ).squared_length();
	  uint mind = smallest(dist, mindist);
	  if (mindist < mindistline) {
	    minindline = j;
	    mindistline = mindist;
	    minind = mind;
	  }
	}
      uint i = minindline;
      // make new line l1--l2:
      if (minind % 2 == 0) { l1 = lines[i].from; l2 = lines[i].to; }
      else                 { l1 = lines[i].to; l2 = lines[i].from; }

      // connection to last/first
      Vector2d conn1, conn2 = l1;
      if (minind < 2) conn1 = p.front();
      else            conn1 = p.back();

      // try polygons intersect
      bool intersects = intersectsPolys(conn1, conn2, clippolys);

      if (!intersects) {
        // detect crossings: try intersect with any line
      	Intersection inter;
      	for (uint li = 0; li<lines.size(); li++) {
      	  if (IntersectXY(conn1, conn2, lines[li].from, lines[li].to, inter,
			  infillDistance/2.))  {
            double t = conn2.x() - conn1.x();
            if (t != 0)
              t = (inter.p.x() - conn1.x()) / t;

	    // don't catch endpoint intersections (continuations)
	    if (t > 0.1 && t < 0.9) {
		intersects = true;
		break;
      	    }
          }
      	}
      }

      if (intersects) { // start new poly
	if (p.size() > 0) {
	  p.cleanup(infillDistance/2.);
	  polys.push_back(p);
	}
	p = Poly(z, extrusionfactor);
	p.setClosed(false);
      }
      // add new line to current p
      if (minind < 2) { p.push_front(l1); p.push_front(l2); }
      else            { p.push_back(l1);  p.push_back(l2);  }
      done[i] = true;
      donelines++;
    }
  if (p.size() > 0) {
    p.cleanup(infillDistance/2.);
    polys.push_back(p);
  }
  return polys;
}

bool sameAngle(double angle1, double angle2, double err)
{
  while (angle1 < 0) angle1+=2.*M_PI;
  while (angle2 < 0) angle2+=2.*M_PI;
  while (angle1 >= 2*M_PI) angle1-=2.*M_PI;
  while (angle2 >= 2*M_PI) angle2-=2.*M_PI;
  return (abs(angle1-angle2) < err);
}


void Infill::addInfillPolys(const vector<Poly> &polys)
{
  if (polys.size() == 0) return;
#define NEWINFILL 1
#if NEWINFILL
  switch (m_type) {
  case BridgeInfill:
  case RaftInfill:
  case ParallelInfill:
    {
      vector<infillline> lines;
      const Vector2d UNITX(1,0);
      for (uint j = 0; j < polys.size(); j++) {
  	for (uint i = 0; i < polys[j].size() ; i++ )
  	  {
	    Vector2d l = (polys[j][i+1] - polys[j][i]);
	    double langle = planeAngleBetween(UNITX, l) + M_PI/2;
	    if (sameAngle(langle,      m_angle, 0.2) ||
		sameAngle(langle+M_PI, m_angle, 0.2))
  	      {
		infillline il = { polys[j][i], polys[j][i+1] };
  		lines.push_back( il );;
  	      }
  	  }
      }
      infillpolys = sortedpolysfromlines(lines, polys.back().getZ());
      break;
    }
  default:
#endif
#if NEWINFILL
    infillpolys = polys;
    // Poly newpoly(p.getZ(), extrusionfactor);
    // infillpolys.push_back(newpoly);
  }
#else
  for (uint i=0; i<polys.size();i++)
    addInfillPoly(polys[i]);
#endif
}

void Infill::addInfillPoly(const Poly &p) // p is result of a clipped pattern
{
#if NEWINFILL==0
  // Poly *zigzagpoly = NULL;
  switch (type) {
  // case ZigzagInfill: // take parallel lines and connect ends
  //   zigzagpoly = new Poly(p.getZ(),extrusionfactor);
  case BridgeInfill:
  case RaftInfill:
  case ParallelInfill:
    { // make lines instead of closed polygons
      Vector2d l,rotl;
      double sina = sin(-angle);
      double cosa = cos(-angle);
      // use the lines that have the angle of this Infill
      //uint counter = 0;
      for (uint i=0; i < p.size() ; i+=1 )
  	{
  	  l = (p[i+1] - p[i]);
	  // rotate with neg. infill angle and see whether it's 90° as infill lines
	  rotl = Vector2d(l.x()*cosa-l.y()*sina,
			  l.y()*cosa+l.x()*sina);
	  if (abs(rotl.x()) < 0.1 && abs(rotl.y()) > 0.1)
  	    {
	      // if (zigzagpoly) {
	      // 	zigzagpoly->addVertex(p.getVertexCircular(i+i%2));
	      // 	zigzagpoly->addVertex(p.getVertexCircular(i+1+i%2));
	      // } else
	      {
		Poly newpoly(p.getZ(), extrusionfactor);
		newpoly.vertices.push_back(p[i]);
		newpoly.vertices.push_back(p[i+1]);
		infillpolys.push_back(newpoly);
	      }
  	    }
	  // else
	  //   if (zigzagpoly) {
	  //     zigzagpoly->addVertex(p.getVertexCircular(i));
	  //   }
  	}
      // if (zigzagpoly) {
      // 	cerr << zigzagpoly->size()<< endl;
      // 	if (zigzagpoly->size()>0)
      // 	  infillpolys.push_back(*zigzagpoly);
      // 	else delete zigzagpoly;
      // 	cerr << infillpolys.size()<< endl;
      // }
    }
    break;
  default:
    {
      Poly newpoly(p.getZ(), extrusionfactor);
      infillpolys.push_back(newpoly);
    }
  }
#endif
}



// not used
void Infill::getLines(vector<Vector3d> &lines) const
{
  cerr << "infill getlines" << endl;
  for (uint i=0; i<infillpolys.size(); i++)
    {
      infillpolys[i].makeLines(lines);
    }
}

vector<Poly> Infill::getCachedPattern(double z) {
  vector<Poly> cached;
  if (m_type != PolyInfill) // can't save PolyInfill
    if (savedPatterns.size()>0)
      for (vector<struct pattern>::iterator sIt=savedPatterns.begin();
      	   sIt != savedPatterns.end(); sIt++)
	if (sIt->type == m_type &&
	    abs(sIt->distance-infillDistance) < 0.01 &&
	    abs(sIt->angle-m_angle) < 0.01 )
	  {
	    cached = Clipping::getPolys(sIt->cpolys,z,extrusionfactor);
	    break;
	  }
  return cached;
};


string Infill::info() const
{
  ostringstream ostr;
  ostr << "Infill " << name
       << ": type=" << m_type
       << ", extrf=" << extrusionfactor
       << ", polygons: " << infillpolys.size()
       << ", vertices: " << infillvertices.size();
  return ostr.str();
}



// Hilbert curve
// http://www.compuphase.com/hilbert.htm
enum {
  UP,
  LEFT,
  DOWN,
  RIGHT,
};
void move(int direction, double infillDistance, vector<Vector2d> &v){
  Vector2d d(0,0);
  switch (direction) {
  case LEFT:  d.x()=-infillDistance;break;
  case RIGHT: d.x()= infillDistance;break;
  case UP:    d.y()=-infillDistance;break;
  case DOWN:  d.y()= infillDistance;break;
  }
  Vector2d last;
  if (v.size()==0) last = Vector2d(0,0);
  else last = v.back();
  //cerr <<"move " << direction << " : "<<last+d<<endl;
  v.push_back(last+d);
}
void hilbert(int level,int direction, double infillDistance, vector<Vector2d> &v)
{
  //cerr <<"hilbert level " << level<< endl;
  if (level==1) {
    switch (direction) {
    case LEFT:
      move(RIGHT,infillDistance,v);      /* move() could draw a line in... */
      move(DOWN, infillDistance,v);       /* ...the indicated direction */
      move(LEFT, infillDistance,v);
      break;
    case RIGHT:
      move(LEFT, infillDistance,v);
      move(UP,   infillDistance,v);
      move(RIGHT,infillDistance,v);
      break;
    case UP:
      move(DOWN, infillDistance,v);
      move(RIGHT,infillDistance,v);
      move(UP,   infillDistance,v);
      break;
    case DOWN:
      move(UP,   infillDistance,v);
      move(LEFT, infillDistance,v);
      move(DOWN, infillDistance,v);
      break;
    } /* switch */
  } else {
    switch (direction) {
    case LEFT:
      hilbert(level-1,UP,infillDistance,v);
      move(RIGHT,infillDistance,v);
      hilbert(level-1,LEFT,infillDistance,v);
      move(DOWN,infillDistance,v);
      hilbert(level-1,LEFT,infillDistance,v);
      move(LEFT,infillDistance,v);
      hilbert(level-1,DOWN,infillDistance,v);
      break;
    case RIGHT:
      hilbert(level-1,DOWN,infillDistance,v);
      move(LEFT,infillDistance,v);
      hilbert(level-1,RIGHT,infillDistance,v);
      move(UP,infillDistance,v);
      hilbert(level-1,RIGHT,infillDistance,v);
      move(RIGHT,infillDistance,v);
      hilbert(level-1,UP,infillDistance,v);
      break;
    case UP:
      hilbert(level-1,LEFT,infillDistance,v);
      move(DOWN,infillDistance,v);
      hilbert(level-1,UP,infillDistance,v);
      move(RIGHT,infillDistance,v);
      hilbert(level-1,UP,infillDistance,v);
      move(UP,infillDistance,v);
      hilbert(level-1,RIGHT,infillDistance,v);
      break;
    case DOWN:
      hilbert(level-1,RIGHT,infillDistance,v);
      move(UP,infillDistance,v);
      hilbert(level-1,DOWN,infillDistance,v);
      move(LEFT,infillDistance,v);
      hilbert(level-1,DOWN,infillDistance,v);
      move(DOWN,infillDistance,v);
      hilbert(level-1,LEFT,infillDistance,v);
      break;
    } /* switch */
  } /* if */
}
