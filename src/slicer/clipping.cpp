
/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2012  martin.dieringer@gmx.de

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

#include "clipping.h"

//CL::Clipper Clipping::clpr; // not static

void printCLpolygons(vector<CL::Polygons> &p) {
  uint numpolys = 0;
  for (uint i = 0; i < p.size(); i++) numpolys += p[i].size();
  cout << numpolys << endl;
  for (uint i = 0; i < p.size(); i++) {
    for (uint j = 0; j < p[i].size(); j++) {
      cout << p[i][j].size() << endl;
      for (uint k = 0; k < p[i][j].size(); k++) {
	cout << p[i][j][k].X <<", " << p[i][j][k].Y << "," << endl;
      }
      cout << endl;
    }
  }
}


void Clipping::clear()
{
  clpr.Clear();
  if(debug) {
    subjpolygons.clear();
    clippolygons.clear();
  }
}

CL::IntPoint Clipping::ClipperPoint(const Vector2d &v)
{
  return CL::IntPoint(CL_FACTOR*(v.x()+CL_OFFSET),
		      CL_FACTOR*(v.y()+CL_OFFSET));
}
Vector2d Clipping::getPoint(const CL::IntPoint &p)
{
  return Vector2d(p.X/CL_FACTOR-CL_OFFSET,
		  p.Y/CL_FACTOR-CL_OFFSET);
}

CL::Polygon Clipping::getClipperPolygon(const Poly &poly)
{
  CL::Polygon cpoly(poly.vertices.size());
  for(size_t i=0; i<poly.vertices.size();i++){
    Vector2d P1;
    // if (reverse)
      P1 = poly.getVertexCircular(-i); // have to reverse from/to clipper
    // else
    //   P1 = poly.getVertexCircular(i);
    cpoly[i]=(ClipperPoint(P1));
  }
  // doesn't work...:
  // cerr<< "poly is hole? "<< hole;
  // cerr<< " -- orient=" <<ClipperLib::Orientation(cpoly) << endl;
  // if (ClipperLib::Orientation(cpoly) == hole)
  //   std::reverse(cpoly.begin(),cpoly.end());
  return cpoly;
}

CL::Polygons Clipping::getClipperPolygons(const vector<Poly> &polys)
{
  ClipperLib::Polygons cpolys(polys.size());
  for (uint i=0; i<polys.size(); i++)
    {
      cpolys[i] = getClipperPolygon(polys[i]);
    }
  return cpolys;
}
CL::Polygons Clipping::getClipperPolygons(const ExPoly &expoly)
{
  return getClipperPolygons(getPolys(expoly));
}

CL::ExPolygons Clipping::getClipperPolygons(const vector<ExPoly> &expolys)
{
  ClipperLib::ExPolygons excpolys(expolys.size());
  for (uint i=0; i<expolys.size(); i++)     {
    excpolys[i].outer = getClipperPolygon(expolys[i].outer);
    for (uint h=0; h<expolys.size(); h++)
      excpolys[i].holes[h] = getClipperPolygon(expolys[i].holes[h]);
    }
  return excpolys;
}

CL::JoinType Clipping::CLType(JoinType type)
{
  switch (type)
    {
    case jsquare: return CL::jtSquare;
    case jmiter:  return CL::jtMiter;
    case jround:  return CL::jtRound;
    }
  return CL::jtMiter; // default
}
CL::PolyType Clipping::CLType(PolyType type)
{
  switch (type)
    {
    case subject: return CL::ptSubject;
    case clip: return CL::ptClip;
    }
  return CL::ptSubject; // default
}

void Clipping::addPoly(const Poly &poly, PolyType type)
{
  clpr.AddPolygon(getClipperPolygon(poly),CLType(type));
  lastZ = poly.getZ();
  lastExtrF = poly.getExtrusionFactor();
}

void Clipping::addPolys(const vector<Poly> &polys, PolyType type)
{
  CL::Polygons cp = getClipperPolygons(polys);
  if(debug) {
    if (type==clip)
      clippolygons.push_back(cp);
    else  if (type==subject)
      subjpolygons.push_back(cp);
  }
  clpr.AddPolygons(cp,CLType(type));
  if (polys.size()>0) {
    lastZ = polys.back().getZ();
    lastExtrF = polys.back().getExtrusionFactor();
  }
}
void Clipping::addPolys(const ExPoly &expoly, PolyType type)
{
  vector<Poly> polys = getPolys(expoly);
  addPolys(polys, type);
}
void Clipping::addPolys(const vector<ExPoly> &expolys, PolyType type)
{
  for (uint i = 0; i < expolys.size(); i++)
    addPolys(expolys[i], type);
}
void Clipping::addPolygons(const CL::Polygons &cp, PolyType type)
{
  clpr.AddPolygons(cp, CLType(type));
}


// // return intersection polys
// vector< vector<Vector2d> > Clipping::intersect(const Poly poly1, const Poly poly2)
// {
//   CL::Polygon cpoly1 = getClipperPolygon(poly1),
//     cpoly2 =  getClipperPolygon(poly2);
//   clpr.Clear();
//   CL::Polygons sol;
//   clpr.AddPolygon(cpoly1,CL::ptSubject);
//   clpr.AddPolygon(cpoly2,CL::ptClip);
//   clpr.Execute(CL::ctIntersection, sol, CL::pftEvenOdd, CL::pftEvenOdd);

//   vector< vector<Vector2d> > result;
//   for(size_t i=0; i<sol.size();i++)
//     {
//       vector<Vector2d> polypoints;
//       CL::Polygon cpoly = sol[i];
//       for(size_t j=0; j<cpoly.size();j++){
// 	polypoints.push_back(Vector2d(cpoly[j].X,cpoly[j].Y));
//       }
//       result.push_back(polypoints);
//     }
//     return result;
// }

// have added Polyons by addPolygon(s)
vector<Poly> Clipping::intersect(CL::PolyFillType sft,
				 CL::PolyFillType cft)
{
  CL::Polygons inter;
  clpr.Execute(CL::ctIntersection, inter, sft, cft);
  return getPolys(inter, lastZ, lastExtrF);
}
vector<ExPoly> Clipping::ext_intersect(CL::PolyFillType sft,
				       CL::PolyFillType cft)
{
  CL::ExPolygons inter;
  clpr.Execute(CL::ctIntersection, inter, sft, cft);
  return getExPolys(inter, lastZ, lastExtrF);
}

// have added Polyons by addPolygon(s)
vector<Poly> Clipping::unite(CL::PolyFillType sft,
			     CL::PolyFillType cft)
{
  CL::Polygons united;
  clpr.Execute(CL::ctUnion, united, sft, cft);
  return getPolys(united, lastZ, lastExtrF);
}
vector<ExPoly> Clipping::ext_unite(CL::PolyFillType sft,
				   CL::PolyFillType cft)
{
  CL::ExPolygons inter;
  clpr.Execute(CL::ctUnion, inter, sft, cft);
  return getExPolys(inter, lastZ, lastExtrF);
}


// have added Polyons by addPolygon(s)
vector<Poly> Clipping::subtract(CL::PolyFillType sft,
				CL::PolyFillType cft)
{
  CL::Polygons diff;
  clpr.Execute(CL::ctDifference, diff, sft, cft);
  return getPolys(diff, lastZ, lastExtrF);
}
vector<ExPoly> Clipping::ext_subtract(CL::PolyFillType sft,
				      CL::PolyFillType cft)
{
  CL::ExPolygons diff;
  if (debug) {
    try {
      clpr.Execute(CL::ctDifference, diff, sft, cft);
    } catch (int e){
      cerr << lastZ<<" - " << lastExtrF<< endl;
      if (e == 22){
	cout << "Subject" << endl;
	printCLpolygons(subjpolygons);
	cout << "Clip" << endl;
	printCLpolygons(clippolygons);
	// vector<ExPoly> empty;
	// return empty;
      }
    }
  }
  else
    clpr.Execute(CL::ctDifference, diff,
		 CL::pftEvenOdd, CL::pftEvenOdd);
  return getExPolys(diff, lastZ, lastExtrF);
}
vector<Poly> Clipping::subtractMerged(double dist,
				      CL::PolyFillType sft,
				      CL::PolyFillType cft)
{
  CL::Polygons diff;
  clpr.Execute(CL::ctDifference, diff, sft, cft);
  return getPolys(getMerged(diff, dist), lastZ, lastExtrF);
}

vector<Poly> Clipping::Xor(CL::PolyFillType sft,
			   CL::PolyFillType cft)
{
  CL::Polygons xored;
  clpr.Execute(CL::ctXor, xored, sft, cft);
  return getPolys(xored, lastZ, lastExtrF);
}

vector<Poly> Clipping::getOffset(const Poly &poly, double distance,
				 JoinType jtype, double miterdist)
{
  CL::Polygons cpolys(1); cpolys[0]=getClipperPolygon(poly);
  CL::Polygons offset = CLOffset(cpolys, CL_FACTOR*distance, CLType(jtype), miterdist);
  return getPolys(offset, poly.getZ(), poly.getExtrusionFactor());
}
vector<Poly> Clipping::getOffset(const vector<Poly> &polys, double distance,
				 JoinType jtype, double miterdist)
{
  CL::Polygons cpolys = getClipperPolygons(polys);
  CL::Polygons offset = CLOffset(cpolys, CL_FACTOR*distance, CLType(jtype), miterdist);
  double z=0, extrf=1.;;
  if (polys.size()>0) {
    z = polys.back().getZ();
    extrf = polys.back().getExtrusionFactor();
  }
  return getPolys(offset,z,extrf);
}
vector<Poly> Clipping::getOffset(const ExPoly &expoly, double distance,
				 JoinType jtype, double miterdist)
{
  vector<Poly> polys = getPolys(expoly);
  return getOffset(polys,distance,jtype,miterdist);
}

vector<Poly> Clipping::getOffset(const vector<ExPoly> &expolys, double distance,
				 JoinType jtype, double miterdist)
{
  return getOffset(getPolys(expolys),distance,jtype,miterdist);
}


// vector<ExPoly> Clipping::getOffset(const vector<ExPoly> expolys, double distance,
// 				   JoinType jtype, double miterdist)
// {
//   CL::ExPolygons excpolys = getClipperPolygons(expolys);
//   double z=0, extrf=1.;;
//   vector<Poly> polys(excpolys.size());
//   if (expolys.size()>0) {
//     z = expolys.back().outer.getZ();
//     extrf = expolys.back().outer.getExtrusionFactor();
//   }
//  for (uint i = 0 ; i < expolys.size(); i++) {
//     CL::Polygons outer = CLOffset(excpolys[i].outer,
// 				  CL_FACTOR*distance, CLType(jtype), miterdist);
//     polys[i].outer = getPoly(outer, z, extrusionfactor);
//     vector<Poly> polys[i].holes;
//     for (uint h = 0 ; h < polys[i].holes.size(); h++) {
//       CL::Polygons holes = CLOffset(excpolys[i].holes[h],
// 				    CL_FACTOR*distance, CLType(jtype), miterdist);
//       vector<Poly> hpolys = CL:getPolys(holes, z, extrusionfactor);
//       polys[i].holes.insert(polys[i].holes.end(),hpolys.begin(),hpolys.end());
//     }
//   }
//   return getExPolys(offset,z,extrf);
// }

// first goes in then out to get capped corners
vector<Poly> Clipping::getShrinkedCapped(const vector<Poly> &polys, double distance,
					 JoinType jtype, double miterdist)
{
  CL::Polygons cpolys = getClipperPolygons(polys);
  CL::Polygons offset1 = CLOffset(cpolys, -2*CL_FACTOR*distance, CLType(jtype), 0);// CL::jtRound);
  CL::Polygons offset = CLOffset(offset1, 1*CL_FACTOR*distance, CLType(jtype), 0);
  double z=0, extrf=1.;;
  if (polys.size()>0) {
    z= polys.back().getZ();
    extrf = polys.back().getExtrusionFactor();
  }
  return getPolys(offset,z,extrf);
}


// offset with reverse test
 CL::Polygons Clipping::CLOffset(const CL::Polygons &cpolys, int cldist,
				 CL::JoinType cljtype, double miter_limit, bool reverse)
{
  CL::Polygons opolys;
  if (reverse)
    CL::ReversePolygons(opolys);
  CL::OffsetPolygons(cpolys, opolys, cldist, cljtype, miter_limit);
  CL::SimplifyPolygons(opolys);//, CL::pftNonZero);
  return opolys;
}

// overlap a bit and unite to merge adjacent polys
vector<Poly> Clipping::getMerged(const vector<Poly> &polys, double overlap)
{
  CL::Polygons cpolys = getClipperPolygons(polys);
  CL::Polygons merged = getMerged(cpolys, CL_FACTOR*overlap);
  double z=0, extrf = 1.;
  if (polys.size()>0) {
    z= polys.back().getZ();
    extrf = polys.back().getExtrusionFactor();
  }
  return getPolys(merged, z, extrf);
}
// overlap a bit and unite to merge adjacent polys
CL::Polygons Clipping::getMerged(const CL::Polygons &cpolys, int overlap)
{
  CL::Clipper clpr;
  //  return polys;
  // make wider to get overlap
  CL::Polygons offset;
  offset = CLOffset(cpolys, overlap, CL::jtMiter, 1);
  //CL::OffsetPolygons(cpolys, offset, 10, ClipperLib::jtMiter, 1);
  // return getPolys(offset, polys.back().getZ(),polys.back().getExtrusionFactor());
  clpr.AddPolygons(offset, CL::ptSubject);
  CL::Polygons cpolys3;
  clpr.Execute(CL::ctUnion, cpolys3, CL::pftEvenOdd, CL::pftEvenOdd);
  //cerr << cpolys3.size() << " - "<<offset.size() << endl;
  // shrink the result
  return CLOffset(cpolys3, -overlap, CL::jtMiter, 1);
}

Poly Clipping::getPoly(const CL::Polygon &cpoly, double z, double extrusionfactor)
{
  Poly p(z, extrusionfactor);
  p.vertices.clear();
  uint count = cpoly.size();
  p.vertices.resize(count);
  for (uint i = 0 ; i<count;  i++) {
    p.vertices[count-i-1] = getPoint(cpoly[i]);  // reverse!
  //   //p.vertices[i] = getPoint(cpoly[i]);
  }
  p.calcHole();
  return p;
}

vector<Poly> Clipping::getPolys(const CL::Polygons &cpolys, double z, double extrusionfactor)
{
  uint count = cpolys.size();
  vector<Poly> polys(count);
  for (uint i = 0 ; i<count;  i++)
    polys[i] = getPoly(cpolys[i], z, extrusionfactor);
  return polys;
}
vector<ExPoly> Clipping::getExPolys(const CL::ExPolygons &excpolys, double z,
				    double extrusionfactor)
{
  uint count = excpolys.size();
  vector<ExPoly> polys(count);
  for (uint i = 0 ; i < count; i++) {
    polys[i].outer = getPoly(excpolys[i].outer, z, extrusionfactor);
    polys[i].holes = getPolys(excpolys[i].holes, z, extrusionfactor);
  }
  return polys;
}
vector<Poly> Clipping::getPolys(const ExPoly &expoly)
{
  Clipping clipp;
  clipp.addPoly(expoly.outer,  subject);
  clipp.addPolys(expoly.holes, clip);
  vector<Poly> polys = clipp.subtract();
  return polys;
}
vector<Poly> Clipping::getPolys(const vector<ExPoly> &expolys)
{
  vector<Poly> polys;
  for (uint i = 0; i< expolys.size(); i++) {
    vector<Poly> p = getPolys(expolys[i]);
    polys.insert(polys.end(),p.begin(),p.end());
  }
  return polys;
}

vector<ExPoly> Clipping::getExPolys(const vector<Poly> &polys)
{
  // return getExPolys(getExClipperPolygons(polys),
  // 		    polys.back().getZ(),polys.back().getExtrusionFactor());
  vector<ExPoly> expolys;
  vector<Poly> holes;
  for (uint i = 0; i<polys.size(); i++) {
    if (polys[i].isHole()) {
      holes.push_back(polys[i]);
    } else {
      ExPoly expoly;
      expoly.outer = polys[i];
      expolys.push_back(expoly);
    }
  }
  for (uint j = 0; j<expolys.size(); j++)
    for (uint i = 0; i<holes.size(); i++)
      if (holes[i].isInside(expolys[j].outer))
	expolys[j].holes.push_back(holes[i]);
  return expolys;
}

vector<ExPoly> Clipping::getExPolys(const vector<Poly> &polys,
 				    double z, double extrusionfactor)
{
  vector<Poly> ppolys(polys.size());
  for (uint i = 0; i<polys.size(); i++) {
    ppolys[i]= Poly(polys[i],z);
    ppolys[i].setExtrusionFactor(extrusionfactor);
  }
  return getExPolys(ppolys);
}
//   CL::ExPolygons excpolys = getExClipperPolygons(polys);
//   uint count = excpolys.size();
//   vector<ExPoly> expolys(count);
//   for (uint i = 0 ; i < count;  i++)  {
//     expolys[i].outer = getPoly(excpolys[i].outer, z, extrusionfactor);
//     expolys[i].holes.resize(excpolys[i].holes.size());
//     for (uint j = 0 ; j < excpolys[i].holes.size();  i++) {
//       expolys[i].holes[j] = getPoly(excpolys[i].holes[j], z, extrusionfactor);
//     }
//   }
//   return expolys;
// }

CL::ExPolygons Clipping::getExClipperPolygons(const vector<Poly> &polys)
{
  CL::Polygons cpolys = getClipperPolygons(polys);
  CL::Clipper clpr;
  clpr.AddPolygons(cpolys, CL::ptSubject);
  CL::ExPolygons cexpolys;
  clpr.Execute(CL::ctUnion, cexpolys, CL::pftEvenOdd, CL::pftEvenOdd);
  return cexpolys;
}

double Clipping::Area(const Poly &poly){
  CL::Polygon cp = getClipperPolygon(poly);
  return (double)((long double)(CL::Area(cp))/CL_FACTOR/CL_FACTOR);
}
double Clipping::Area(const vector<Poly> &polys){
  double a=0;
  for (uint i=0; i<polys.size(); i++)
    a += Area(polys[i]);
  return a;
}

double Clipping::Area(const ExPoly &expoly){
  double a = Area(expoly.outer);
  for (uint i = 0; i < expoly.holes.size(); i++) {
    a -= Area(expoly.holes[i]);
  }
  return a;
}
double Clipping::Area(const vector<ExPoly> &expolys){
  double a=0;
  for (uint i=0; i<expolys.size(); i++)
    a += Area(expolys[i]);
  return a;
}

void Clipping::ReversePoints(vector<Poly> &polys) {
  for (uint i=0; i<polys.size(); i++)
    polys[i].reverse();
}
