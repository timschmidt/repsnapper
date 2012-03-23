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
}


Infill::Infill (Layer *mlayer, double extrfactor) 
  : cached(false)
{
  layer = mlayer;
  extrusionfactor = extrfactor;
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
void Infill::addInfill(double z, const Poly poly, InfillType type, 
		       double infillDistance, double offsetDistance, double rotation)
{
  vector<Poly> polys; polys.push_back(poly);
  addInfill(z, polys, type, infillDistance, offsetDistance, rotation);
}
void Infill::addInfill(double z, const vector<Poly> polys, InfillType type, 
		       double infillDistance, double offsetDistance, double rotation)
{
  this->infillDistance = infillDistance;

#ifdef _OPENMP
  omp_set_lock(&save_lock);
#endif
  ClipperLib::Polygons patterncpolys = 
    makeInfillPattern(type, polys, infillDistance, offsetDistance, rotation);
#ifdef _OPENMP
  omp_unset_lock(&save_lock);
#endif
  addInfill(z, polys, patterncpolys, offsetDistance);
}

void Infill::addInfill(double z, ExPoly expoly, InfillType type, 
		       double infillDistance, double offsetDistance, double rotation)
{
  vector<Poly> polys = Clipping::getPolys(expoly);
  addInfill (z, polys, type, infillDistance, offsetDistance, rotation);
}

// calculates angles for bridges
// void Infill::addBridgeInfill(double z, const vector<Poly> polys,
// 			     double infillDistance, double offsetDistance, 
// 			     vector<polys> bridgepillars)
// {
//   type = BridgeInfill;
//   this->infillDistance = infillDistance;

//   omp_set_lock(&save_lock);
//   ClipperLib::Polygons patterncpolys = 
//     makeInfillPattern(type, polys, infillDistance, offsetDistance, rotation);
//   addInfill(z, polys, patterncpolys, offsetDistance);
//   omp_unset_lock(&save_lock);
// }

// fill polys with fillpolys
void Infill::addInfill(double z, const vector<Poly> polys, 
		       const vector<Poly> fillpolys,
		       double offsetDistance)
{
  addInfill(z, polys, Clipping::getClipperPolygons(fillpolys), offsetDistance);
}

// clip infill pattern polys against polys
void Infill::addInfill(double z, const vector<Poly> polys, 
		       const ClipperLib::Polygons patterncpolys,
		       double offsetDistance)
{
  ClipperLib::Polygons cpolys;
  //   ClipperLib::OffsetPolygons(Clipping::getClipperPolygons(polys), cpolys, 100,
  // 			       ClipperLib::jtMiter,1);
  // else 
  cpolys = Clipping::getClipperPolygons(polys);
  ClipperLib::Clipper clpr;
  clpr.AddPolygons(patterncpolys,ClipperLib::ptSubject);
  clpr.AddPolygons(cpolys,ClipperLib::ptClip);
  ClipperLib::Polygons result;
  clpr.Execute(ClipperLib::ctIntersection, result, 
	       ClipperLib::pftEvenOdd, ClipperLib::pftEvenOdd);
  if (type==PolyInfill) { // reversal from evenodd clipping
    for (uint i = 0; i<result.size(); i+=2)
      std::reverse(result[i].begin(),result[i].end());
  }
  //cerr << z <<": "<< name << ": "<<polys.size() << "/"<<patterncpolys.size() << " == "<< result.size() << endl;
  addInfillPolys(Clipping::getPolys(result, z, extrusionfactor));
}



// generate infill pattern as a vector of polygons
ClipperLib::Polygons Infill::makeInfillPattern(InfillType type, 
					       const vector<Poly> tofillpolys,
					       double infillDistance,
					       double offsetDistance,
					       double rotation) 
{
  ClipperLib::Polygons cpolys;
  if (tofillpolys.size()==0) return cpolys;
  this->type = type;
  cached = false;
  //cerr << "have " << savedPatterns.size()<<" saved patterns " << endl;
  // look for saved pattern for this rotation
  const Vector2d Min = layer->getMin();
  const Vector2d Max = layer->getMax();
  while (rotation>2*M_PI) rotation -= 2*M_PI;
  while (rotation<0) rotation += 2*M_PI;
  this->angle= rotation;

  //omp_set_lock(&save_lock);
  //int tid = omp_get_thread_num( );
  //cerr << "thread "<<tid << " looking for pattern " << endl;
  if (type != PolyInfill) { // can't save PolyInfill
    if (savedPatterns.size()>0){
      //cerr << savedPatterns.size() << " patterns" << endl;
      vector<uint> too_small;
      //vector<vector<struct pattern>::iterator> too_small;
      for (vector<struct pattern>::iterator sIt=savedPatterns.begin();
      	   sIt != savedPatterns.end(); sIt++){
	//cerr << sIt->Min << sIt->Max <<endl;
	if (sIt->type == type &&
	    abs(sIt->distance-infillDistance) < 0.01 &&
	    abs(sIt->angle-rotation) < 0.01 )
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
	      //cerr <<"return "<<  sIt->cpolys.size()<< endl;
	      cached = true;
	      return sIt->cpolys;
	    }
	  }
      }
      sort(too_small.rbegin(), too_small.rend());
      for (uint i = 0; i < too_small.size(); i++) {
	//cerr << i << " - " ;
	savedPatterns.erase(savedPatterns.begin()+too_small[i]);
      }
      //cerr << endl;
    }
  }
  //omp_unset_lock(&save_lock);
  // none found - make new:
  bool zigzag = false;
  switch (type)
    {
    case SmallZigzagInfill: // small zigzag lines
    zigzag = true;
    //case ZigzagInfill: // long zigzag lines, make lines later
    case SupportInfill: // stripes, but leave them as polygons
    case RaftInfill:      // stripes, make them to lines later
    case BridgeInfill:    // stripes, make them to lines later
    case ParallelInfill:  // stripes, make them to lines later
      {
	Vector2d center = (Min+Max)/2.;
	// make square that masks everything even when rotated
	Vector2d diag = Max-Min;
	double square = max(diag.x(),diag.y());
	Vector2d sqdiag(square*2/3,square*2/3);
	Vector2d pMin=center-sqdiag, pMax=center+sqdiag;
	//cerr << pMin << "--"<<pMax<< "::"<< center << endl;
	Poly poly(this->layer->getZ());
	for (double x = pMin.x(); x < pMax.x(); x += 2*infillDistance) {
	  poly.addVertex(Vector2d(x, pMin.y()));
	  if (zigzag){
	    for (double y = pMin.y()+infillDistance; y < pMax.y(); y += 2*infillDistance) {
	      poly.addVertex(Vector2d(x,y));
	      poly.addVertex(Vector2d(x+infillDistance,y+infillDistance));
	    }
	    for (double y = pMax.y(); y > pMin.y(); y -= 2*infillDistance) {
	      poly.addVertex(Vector2d(x+infillDistance,y+infillDistance));
	      poly.addVertex(Vector2d(x+2*infillDistance,y));
	    }
	  } else {
	    poly.addVertex(Vector2d(x+infillDistance,   pMin.y()));
	    poly.addVertex(Vector2d(x+infillDistance,   pMax.y()));
	    poly.addVertex(Vector2d(x+2*infillDistance, pMax.y()));
	  }
	}
	poly.addVertex(Vector2d(pMax.x(), pMin.y()-infillDistance));
	poly.addVertex(Vector2d(pMin.x(), pMin.y()-infillDistance));
	poly.rotate(center,rotation);
	vector<Poly> polys(1);
	polys[0] = poly;
	cpolys = Clipping::getClipperPolygons(polys);
      }
      break;
    case HilbertInfill:  
      {
	Poly poly(this->layer->getZ());
	Vector2d center = (Min+Max)/2.;
	Vector2d diag = Max-Min;
	double square = MAX(Max.x(),Max.y());
	if (infillDistance<=0) break;
	int level = (int)ceil(log2(2*square/infillDistance));
	if (level<0) break;
	//cerr << "level " << level;
	// start at 0,0 to get the same location for all layers
	poly.addVertex(Vector2d(0,0)); 
	hilbert(level, 0, infillDistance,  poly.vertices);
	vector<Poly> polys(1);
	polys[0] = poly;
	cpolys = Clipping::getClipperPolygons(polys);
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
  if (type != PolyInfill && type != ZigzagInfill) // can't save these
    {
      struct pattern newPattern;
      newPattern.type=type;
      newPattern.angle=rotation;
      newPattern.distance=infillDistance;
      newPattern.cpolys=cpolys;
      newPattern.Min=Min;
      newPattern.Max=Max;
      savedPatterns.push_back(newPattern);
      //cerr << "saved pattern " << endl;
    }
  return cpolys;
}


void Infill::addInfillPolys(vector<Poly> polys)
{
  for (uint i=0; i<polys.size();i++)
    addInfillPoly(polys[i]);
}

void Infill::addInfillPoly(Poly p)
{
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
      for (uint i=0; i < p.size() ; i+=1 )
  	{
  	  l = (p.getVertexCircular(i+1) - p.getVertexCircular(i));     
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
		newpoly.vertices.push_back(p.getVertexCircular(i));
		newpoly.vertices.push_back(p.getVertexCircular(i+1));
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
      p.setExtrusionFactor(extrusionfactor);
      infillpolys.push_back(p);
    }
  }
}



// not used
void Infill::getLines(vector<Vector3d> &lines) const
{
  cerr << "infill getlines" << endl;
  for (uint i=0; i<infillpolys.size(); i++)
    {
      infillpolys[i].getLines(lines);
    }
}

string Infill::info() const
{ 
  ostringstream ostr;
  ostr << "Infill " << name 
       << ": extrf=" << extrusionfactor 
       << ", polygons: " << infillpolys.size() 
       << ", vertices: "<< infillvertices.size();
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
  case RIGHT: d.x()=infillDistance;break;
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
