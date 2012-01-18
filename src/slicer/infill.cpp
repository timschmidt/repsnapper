/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2011  martin.dieringer@gmx.de

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

// #include <vmmlib/vmmlib.h> 

using namespace std; 
using namespace vmml;


vector<struct Infill::pattern> Infill::savedPatterns;


Infill::Infill(){
  this->extrusionfactor = 1.;
}


Infill::Infill(Layer *mlayer, double extrfactor) 
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
  savedPatterns.clear();
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
  ClipperLib::Polygons patterncpolys = 
    makeInfillPattern(type, polys, infillDistance, offsetDistance, rotation);
  addInfill(z, polys, patterncpolys, offsetDistance);
}



// fill polys with fillpolys
void Infill::addInfill(double z, const vector<Poly> polys, const vector<Poly> fillpolys,
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
	       ClipperLib::pftEvenOdd, ClipperLib::pftNonZero);
  if (type==PolyInfill) { // reversal from evenodd clipping
    for (uint i = 0; i<result.size(); i+=2)
      std::reverse(result[i].begin(),result[i].end());
  }
  addInfillPolys(Clipping::getPolys(result, z, extrusionfactor));
}



// generate infill pattern as a vector of polygons
ClipperLib::Polygons Infill::makeInfillPattern(InfillType type, 
					       const vector<Poly> tofillpolys,
					       double infillDistance,
					       double offsetDistance,
					       double rotation) 
{
  this->type = type;
  //cerr << "have " << savedPatterns.size()<<" saved patterns " << endl;
  // look for saved pattern for this rotation
  if (type != PolyInfill) // can't save PolyInfill
    {
      while (rotation>2*M_PI) rotation -= 2*M_PI;
      while (rotation<0) rotation += 2*M_PI;
      this->angle= rotation;
      for (uint i = 0; i < savedPatterns.size(); i++) {
	if (savedPatterns[i].type == type &&
	    abs(savedPatterns[i].distance-infillDistance) < 0.01 &&
	    abs(savedPatterns[i].angle-rotation) < 0.01 )
	  {
	    //cerr << "found saved pattern no " << i << "with" <<savedPatterns[i].cpolys.size() <<" polys"<<endl;
	    return savedPatterns[i].cpolys;
	  }
      }
    }
  // none found - make new:
  ClipperLib::Polygons cpolys;
  bool zigzag = false;
  switch (type)
    {
    case ZigzagLineInfill: // zigzag lines
    zigzag = true;
    case SupportInfill: // stripes, but leave them as polygons
    case RaftInfill:    // stripes, but leave them as polygons
    case BridgeInfill:    // stripes, make them to lines later
    case ParallelInfill:  // stripes, make them to lines later
      {
	Vector2d Min = layer->getMin();
	Vector2d Max = layer->getMax();
	Vector2d center = (Min+Max)/2.;
	// make square that masks everything even when rotated
	Vector2d diag = Max-Min;
	double square = MAX(diag.x,diag.y);
	Vector2d sqdiag(square,square);
	Min=center-sqdiag;
	Max=center+sqdiag;
	cerr << Min << "--"<<Max<< "::"<< center << endl;
	Poly poly(this->layer->getZ());
	for (double x = Min.x; x < Max.x; x += 2*infillDistance) {
	  poly.addVertex(Vector2d(x,Min.y));
	  if (zigzag){
	    for (double y = Min.y+infillDistance; y < Max.y; y += 2*infillDistance) {
	      poly.addVertex(Vector2d(x,y));
	      poly.addVertex(Vector2d(x+infillDistance,y+infillDistance));
	    }
	    for (double y = Max.y; y > Min.y; y -= 2*infillDistance) {
	      poly.addVertex(Vector2d(x+infillDistance,y+infillDistance));
	      poly.addVertex(Vector2d(x+2*infillDistance,y));
	    }
	  } else {
	    poly.addVertex(Vector2d(x+infillDistance,Min.y));
	    poly.addVertex(Vector2d(x+infillDistance,Max.y));
	    poly.addVertex(Vector2d(x+2*infillDistance,Max.y));
	  }
	}
	poly.addVertex(Vector2d(Max.x,Min.y-infillDistance));
	poly.addVertex(Vector2d(Min.x,Min.y-infillDistance));
	poly.rotate(center,rotation);
	vector<Poly> polys;
	polys.push_back(poly);
	cpolys = Clipping::getClipperPolygons(polys);
      }
      break;
    case PolyInfill: // fill all polygons with their shrinked polys
      {
	vector< vector<Poly> > ipolys; // "shells"
	for (uint i=0;i<tofillpolys.size();i++){
	  double parea = Clipping::Area(tofillpolys[i]);
	  // make first larger to get clip overlap
	  double firstshrink=0.5*infillDistance;
	  if (parea<0) firstshrink=-0.5*infillDistance;
	  vector<Poly> shrinked = Clipping::getOffset(tofillpolys[i],firstshrink);
	  ipolys.push_back(Clipping::getOffset(shrinked,0.5*infillDistance));
	  //ipolys.insert(ipolys.end(),shrinked.begin(),shrinked.end());
	  double area = Clipping::Area(shrinked);
	  //cerr << "shr " <<parea << " - " <<area<< " - " << " : " <<endl;
	  //int shrcount =1;
	  int lastnumpolys=0;
	  while (shrinked.size()>0){ 
	    if (area*parea<0)  break;
	    //cerr << "shr " <<parea << " - " <<area<< " - " << shrcount << " : " <<endl;
	    ipolys.push_back(Clipping::getOffset(shrinked,0.5*infillDistance));
	    //ipolys.insert(ipolys.end(),shrinked.begin(),shrinked.end());
	    lastnumpolys = shrinked.size();
	    shrinked = Clipping::getOffset(shrinked,-infillDistance);
	    area = Clipping::Area(shrinked);
	    //shrcount++;
	  }
	  // // remove last because they are too small
	  // ipolys.erase(ipolys.end()-lastnumpolys,ipolys.end());
	}
	vector<Poly> opolys;
	for (uint i=0;i<ipolys.size();i++){
	  opolys.insert(opolys.end(),ipolys[i].begin(),ipolys[i].end());
	}
	cpolys = Clipping::getClipperPolygons(opolys);
	//cerr << "cpolys " << cpolys.size() << endl; 
      }
      break;
    default:
      cerr << "infill type " << type << " unknown "<< endl;
    }
  // save 
  struct pattern newPattern;
  newPattern.type=type;
  newPattern.angle=rotation;
  newPattern.distance=infillDistance;
  newPattern.cpolys=cpolys;
  savedPatterns.push_back(newPattern);
  return newPattern.cpolys;
}




void Infill::addInfillPolys(vector<Poly> polys)
{
  for (uint i=0; i<polys.size();i++)
    addInfillPoly(polys[i]);
}

void Infill::addInfillPoly(Poly p)
{
  switch (type) {
  case BridgeInfill:
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
	  rotl = Vector2d(l.x*cosa-l.y*sina, 
			  l.y*cosa+l.x*sina);
	  if (abs(rotl.x) < 0.1 && abs(rotl.y) > 0.1)
  	    {
	      Poly newpoly(p.getZ(), extrusionfactor);
	      newpoly.vertices.push_back(p.getVertexCircular(i));
	      newpoly.vertices.push_back(p.getVertexCircular(i+1));
	      infillpolys.push_back(newpoly);
  	    }
  	}
    } break;
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

void Infill::printinfo() const
{ 
  cout << "Infill " << name 
       << ": extrf=" << extrusionfactor 
       << ", polygons: " << infillpolys.size() 
       << ", vertices: "<< infillvertices.size()
       <<endl;
}

