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

// Infill::Infill(){
//   infill.clear();
// }
Infill::Infill(CuttingPlane *plane) 
{
  this->plane = plane;
  // helpers for pattern generation and rotation
  center = (plane->Min+plane->Max)/2.;
  Min=center+(plane->Min-center)*2;  // make bigger for rotation
  Max=center+(plane->Max-center)*2;
}

void Infill::clearPatterns() {
  savedPatterns.clear();
}


// clip infill pattern against polygons
void Infill::calcInfill(const vector<Poly> polys, InfillType type, 
			double infillDistance, double offsetDistance, double rotation)
{
  ClipperLib::Polygons patterncpolys = 
    makeInfillPattern(type,infillDistance, offsetDistance, rotation);
  calcInfill(polys, patterncpolys, infillDistance, offsetDistance);
}


void Infill::calcInfill(const vector<Poly> polys, const vector<Poly> fillpolys,
			double infillDistance, double offsetDistance)
{
  calcInfill(polys, plane->getClipperPolygons(fillpolys), infillDistance, offsetDistance);
}


void Infill::calcInfill(const vector<Poly> polys, 
			const ClipperLib::Polygons patterncpolys,
			double infillDistance, double offsetDistance)
{
  bool reverse=true;
  // problem with offsetting is orientation so that a few polys won't get filled
  ClipperLib::Polygons offset;
  while (offset.size()==0){ // try to reverse poly vertices if no result
    ClipperLib::Polygons cpolys = plane->getClipperPolygons(polys,reverse);
    // offset by half infillDistance
    ClipperLib::OffsetPolygons(cpolys, offset, -1000.*offsetDistance/2.,
			     ClipperLib::jtMiter,2);
    reverse=!reverse;
    if (reverse) break;
  }
  ClipperLib::Clipper clpr;
  clpr.AddPolygons(patterncpolys,ClipperLib::ptSubject);
  clpr.AddPolygons(offset,ClipperLib::ptClip);
  ClipperLib::Polygons result;
  ClipperLib::PolyFillType filltype = ClipperLib::pftNonZero; //?
  clpr.Execute(ClipperLib::ctIntersection, result, filltype, ClipperLib::pftNonZero);
  for (uint i=0;i<result.size();i++)
    {
      Poly p = Poly(plane, result[i]);
      infillpolys.push_back(p);
    }
}

// generate infill pattern as a vector of polygons
ClipperLib::Polygons Infill::makeInfillPattern(InfillType type, 
					       double infillDistance,
					       double offsetDistance,
					       double rotation) 
{
  this->type = type;
  //cerr << "have " << savedPatterns.size()<<" saved patterns " << endl;
  while (rotation>2*M_PI) rotation -= 2*M_PI;
  while (rotation<-2*M_PI) rotation += 2*M_PI;
  for (uint i = 0; i < savedPatterns.size(); i++) {
    if (savedPatterns[i].type == type &&
	ABS(savedPatterns[i].distance-infillDistance) < 0.01 &&
	ABS(savedPatterns[i].angle-rotation) < 0.01 )
      {
	//cerr << "found saved pattern no " << i << endl;
	return savedPatterns[i].cpolys;
      }
  }
  Vector2d center = (Min+Max)/2.;
  // none found - make new:
  ClipperLib::Polygons cpolys;
  vector<Poly> polys;
  switch (type)
    {
    case ParallelInfill: // stripes
      {
	Poly poly(this->plane);
	for (double x=Min.x; x <Max.x; x+= 2*infillDistance) {
	  poly.addVertex(Vector2d(x,Min.y));
	  poly.addVertex(Vector2d(x+infillDistance,Min.y));
	  poly.addVertex(Vector2d(x+infillDistance,Max.y));
	  poly.addVertex(Vector2d(x+2*infillDistance,Max.y));
	}
	poly.addVertex(Vector2d(Max.x,Min.y-infillDistance));
	poly.addVertex(Vector2d(Min.x,Min.y-infillDistance));
	poly.rotate(center,rotation);
	polys.push_back(poly);
	cpolys = plane->getClipperPolygons(polys);
      }
      break;
    case LinesInfill: // lines only -- not possible
      {
	Poly poly(this->plane);
	for (double x=Min.x; x <Max.x; x+= infillDistance) {
	  poly.addVertex(Vector2d(x,Min.y));
	  poly.addVertex(Vector2d(x,Max.y));
	  poly.rotate(center,rotation);
	  polys.push_back(poly);
	}
	cpolys = plane->getClipperPolygons(polys);
	cerr << cpolys.size() << endl; 
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



void Infill::getLines(vector<Vector3d> &lines) const
{
  for (uint i=0; i<infillpolys.size(); i++)
    {
      infillpolys[i].getLines(lines);
    }
}

void Infill::printinfo() const
{ 
  cout << "Infill with " << infillpolys.size() << " polygons" <<endl;
}

