
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

//////////////////////////////////////////////////////////////////////////////////////////
//
// old API compatibility
// polytree to expolygons
// http://www.angusj.com/delphi/clipper/documentation/Docs/Overview/Compatibility%20with%20Prior%20Versions.htm

void Clipping::AddOuterPolyNodeToExPolygons(const CL::PolyNode * polynode,
					    ExPolygons& expolygons)
{
  size_t cnt = expolygons.size();
  expolygons.resize(cnt + 1);
  expolygons[cnt].outer = polynode->Contour;
  expolygons[cnt].holes.resize(polynode->ChildCount());
  for (int i = 0; i < polynode->ChildCount(); ++i)
  {
    expolygons[cnt].holes[i] = polynode->Childs[i]->Contour;
    //Add outer polygons contained by (nested within) holes ...
    for (int j = 0; j < polynode->Childs[i]->ChildCount(); ++j)
      AddOuterPolyNodeToExPolygons(polynode->Childs[i]->Childs[j], expolygons);
  }
}

void Clipping::PolyTreeToExPolygons(const CL::PolyTree * polytree, ExPolygons& expolygons)
{
  expolygons.clear();
  for (int i = 0; i < polytree->ChildCount(); ++i)
    AddOuterPolyNodeToExPolygons(polytree->Childs[i], expolygons);
}

//
//////////////////////////////////////////////////////////////////////////////////////////




void printCLpolygon(CL::Path &p) {
    cout << p.size() << endl;
    for (uint k = 0; k < p.size(); k++) {
      cout << p[k].X <<", " << p[k].Y << "," << endl;
    }
    cout << endl;
}

void printCLpolygons(CL::Paths &p) {
  for (uint j = 0; j < p.size(); j++) {
    printCLpolygon(p[j]);
    cout << endl;
  }
}
void printCLpolygons(vector<CL::Paths> &p) {
  uint numpolys = 0;
  for (uint i = 0; i < p.size(); i++) numpolys += p[i].size();
  cout << numpolys << endl;
  for (uint i = 0; i < p.size(); i++) {
    printCLpolygons(p[i]);
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

CL::Path Clipping::getClipperPolygon(const Poly &poly)
{
  CL::Path cpoly(poly.vertices.size());
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

CL::Paths Clipping::getClipperPolygons(const vector<Poly> &polys)
{
  ClipperLib::Paths cpolys(polys.size());
  for (uint i=0; i<polys.size(); i++)
    {
      cpolys[i] = getClipperPolygon(polys[i]);
    }
  return cpolys;
}
CL::Paths Clipping::getClipperPolygons(const ExPoly &expoly)
{
  return getClipperPolygons(getPolys(expoly));
}

/*  // not used
CL::PolyTree Clipping::getClipperTree(const vector<ExPoly> &expolys)
{
  CL::PolyTree polytree;
  // for (uint i=0; i<expolys.size(); i++)     {
  //   CL::PolyNode expoly;
  //   polytree[i].Contour = getClipperPolygon(expolys[i].outer);
  //   for (uint h=0; h<expolys[i].holes.size(); h++) {
  //     CL::PolyNode hole;
  //     hole.Contour = getClipperPolygon(expolys[i].holes[h]);
  //     hole.Parent  = &polytrees[i];
  //     polytrees[i].Childs.push_back(&hole);
  //   }
  // }
  return polytree;
}
*/

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
  clpr.AddPath(getClipperPolygon(poly), CLType(type), true);
  lastZ = poly.getZ();
  lastExtrF = poly.getExtrusionFactor();
}

void Clipping::addPolys(const vector<Poly> &polys, PolyType type)
{
  CL::Paths cp = getClipperPolygons(polys);
  if(debug) {
    if (type==clip)
      clippolygons.push_back(cp);
    else  if (type==subject)
      subjpolygons.push_back(cp);
  }
  // try {
    clpr.AddPaths(cp, CLType(type), true);
  // } catch (...) {
  //   vector<CL::Paths> vcp;
  //   vcp.push_back(cp);
  //   printCLpolygons(vcp);
  //   for (uint i = 0; i < cp.size(); i++) {
  //     cerr << "try polygon "<< i << endl;
  //     clpr.Clear();
  //     printCLpolygon(cp[i]);
  //     clpr.AddPath(cp[i],CLType(type), true);
  //   }
  //   throw("end");
  // }
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
void Clipping::addPolygons(const CL::Paths &cp, PolyType type)
{
  clpr.AddPaths(cp, CLType(type), true);
}



// // return intersection polys
// vector< vector<Vector2d> > Clipping::intersect(const Poly poly1, const Poly poly2)
// {
//   CL::Path cpoly1 = getClipperPolygon(poly1),
//     cpoly2 =  getClipperPolygon(poly2);
//   clpr.Clear();
//   CL::Paths sol;
//   clpr.AddPath(cpoly1,CL::ptSubject);
//   clpr.AddPath(cpoly2,CL::ptClip);
//   clpr.Execute(CL::ctIntersection, sol, CL::pftEvenOdd, CL::pftEvenOdd);

//   vector< vector<Vector2d> > result;
//   for(size_t i=0; i<sol.size();i++)
//     {
//       vector<Vector2d> polypoints;
//       CL::Path cpoly = sol[i];
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
  CL::Paths inter;
  clpr.Execute(CL::ctIntersection, inter, sft, cft);
  return getPolys(inter, lastZ, lastExtrF);
}
vector<ExPoly> Clipping::ext_intersect(CL::PolyFillType sft,
				       CL::PolyFillType cft)
{
  CL::PolyTree inter;
  clpr.Execute(CL::ctIntersection, inter, sft, cft);
  return getExPolys(inter, lastZ, lastExtrF);
}

// have added Polyons by addPolygon(s)
vector<Poly> Clipping::unite(CL::PolyFillType sft,
			     CL::PolyFillType cft)
{
  CL::Paths united;
  clpr.Execute(CL::ctUnion, united, sft, cft);
  return getPolys(united, lastZ, lastExtrF);
}
vector<ExPoly> Clipping::ext_unite(CL::PolyFillType sft,
				   CL::PolyFillType cft)
{
  CL::PolyTree inter;
  clpr.Execute(CL::ctUnion, inter, sft, cft);
  return getExPolys(inter, lastZ, lastExtrF);
}


// have added Polyons by addPolygon(s)
vector<Poly> Clipping::subtract(CL::PolyFillType sft,
				CL::PolyFillType cft)
{
  CL::Paths diff;
  clpr.Execute(CL::ctDifference, diff, sft, cft);
  return getPolys(diff, lastZ, lastExtrF);
}
vector<ExPoly> Clipping::ext_subtract(CL::PolyFillType sft,
				      CL::PolyFillType cft)
{
  CL::PolyTree diff;
  // if (debug) {
  //   try {
  //     clpr.Execute(CL::ctDifference, diff, sft, cft);
  //   } catch (int e){
  //     cerr << lastZ<<" - " << lastExtrF<< endl;
  //     if (e == 22){
  // 	cout << "Subject" << endl;
  // 	printCLpolygons(subjpolygons);
  // 	cout << "Clip" << endl;
  // 	printCLpolygons(clippolygons);
  // 	// vector<ExPoly> empty;
  // 	// return empty;
  //     }
  //   }
  // }
  // else
  clpr.Execute(CL::ctDifference, diff, sft, cft);//CL::pftEvenOdd, CL::pftEvenOdd);
  return getExPolys(diff, lastZ, lastExtrF);
}
vector<Poly> Clipping::subtractMerged(double dist,
				      CL::PolyFillType sft,
				      CL::PolyFillType cft)
{
  CL::Paths diff;
  clpr.Execute(CL::ctDifference, diff, sft, cft);
  return getPolys(getMerged(diff, dist), lastZ, lastExtrF);
}

vector<Poly> Clipping::Xor(CL::PolyFillType sft,
			   CL::PolyFillType cft)
{
  CL::Paths xored;
  clpr.Execute(CL::ctXor, xored, sft, cft);
  return getPolys(xored, lastZ, lastExtrF);
}

vector<Poly> Clipping::getOffset(const Poly &poly, double distance,
				 JoinType jtype, double miterdist)
{
  CL::Paths cpolys(1); cpolys[0]=getClipperPolygon(poly);
  CL::Paths offset = CLOffset(cpolys, CL_FACTOR*distance, CLType(jtype), miterdist);
  return getPolys(offset, poly.getZ(), poly.getExtrusionFactor());
}
vector<Poly> Clipping::getOffset(const vector<Poly> &polys, double distance,
				 JoinType jtype, double miterdist)
{
  CL::Paths cpolys = getClipperPolygons(polys);
  CL::Paths offset = CLOffset(cpolys, CL_FACTOR*distance, CLType(jtype), miterdist);
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
//     CL::Paths outer = CLOffset(excpolys[i].outer,
// 				  CL_FACTOR*distance, CLType(jtype), miterdist);
//     polys[i].outer = getPoly(outer, z, extrusionfactor);
//     vector<Poly> polys[i].holes;
//     for (uint h = 0 ; h < polys[i].holes.size(); h++) {
//       CL::Paths holes = CLOffset(excpolys[i].holes[h],
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
  CL::Paths cpolys = getClipperPolygons(polys);
  CL::Paths offset1 = CLOffset(cpolys, -2*CL_FACTOR*distance, CLType(jtype), 0);// CL::jtRound);
  CL::Paths offset = CLOffset(offset1, 1*CL_FACTOR*distance, CLType(jtype), 0);
  double z=0, extrf=1.;;
  if (polys.size()>0) {
    z= polys.back().getZ();
    extrf = polys.back().getExtrusionFactor();
  }
  return getPolys(offset,z,extrf);
}


// offset with reverse test
 CL::Paths Clipping::CLOffset(const CL::Paths &cpolys, int cldist,
				 CL::JoinType cljtype, double miter_limit, bool reverse)
{
  CL::Paths opolys;
  if (reverse)
    CL::ReversePaths(opolys);
  CL::ClipperOffset co(miter_limit, miter_limit);
  co.AddPaths(cpolys, cljtype, CL::etClosedPolygon);
  co.Execute(opolys, cldist);
  CL::SimplifyPolygons(opolys);//, CL::pftNonZero);
  return opolys;
}

// overlap a bit and unite to merge adjacent polys
vector<Poly> Clipping::getMerged(const vector<Poly> &polys, double overlap)
{
  CL::Paths cpolys = getClipperPolygons(polys);
  CL::Paths merged = getMerged(cpolys, CL_FACTOR*overlap);
  double z=0, extrf = 1.;
  if (polys.size()>0) {
    z= polys.back().getZ();
    extrf = polys.back().getExtrusionFactor();
  }
  return getPolys(merged, z, extrf);
}
// overlap a bit and unite to merge adjacent polys
CL::Paths Clipping::getMerged(const CL::Paths &cpolys, int overlap)
{
  CL::Clipper clpr;
  //  return polys;
  // make wider to get overlap
  CL::Paths offset;
  offset = CLOffset(cpolys, overlap, CL::jtMiter, 1);
  //CL::OffsetPolygons(cpolys, offset, 10, ClipperLib::jtMiter, 1);
  // return getPolys(offset, polys.back().getZ(),polys.back().getExtrusionFactor());
  clpr.AddPaths(offset, CL::ptSubject, true);
  CL::Paths cpolys3;
  clpr.Execute(CL::ctUnion, cpolys3, CL::pftEvenOdd, CL::pftEvenOdd);
  //cerr << cpolys3.size() << " - "<<offset.size() << endl;
  // shrink the result
  return CLOffset(cpolys3, -overlap, CL::jtMiter, 1);
}

Poly Clipping::getPoly(const CL::Path &cpoly, double z, double extrusionfactor)
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

vector<Poly> Clipping::getPolys(const CL::Paths &cpolys, double z, double extrusionfactor)
{
  uint count = cpolys.size();
  vector<Poly> polys(count);
  for (uint i = 0 ; i<count;  i++)
    polys[i] = getPoly(cpolys[i], z, extrusionfactor);
  return polys;
}

vector<ExPoly> Clipping::getExPolys(const CL::PolyTree &ctree, double z,
				    double extrusionfactor)
{
  ExPolygons cexpolys;
  PolyTreeToExPolygons(&ctree, cexpolys);
  vector<ExPoly> expolys(cexpolys.size());
  for (uint j = 0 ; j < cexpolys.size(); j++) {
    expolys[j].outer = getPoly(cexpolys[0].outer, z, extrusionfactor);
    for (uint i = 0 ; i < cexpolys[j].holes.size(); i++)
      expolys[j].holes.push_back(getPoly(cexpolys[j].holes[i], z, extrusionfactor));
  }
  return expolys;
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
  /*
  CL::PolyTree tree = getClipperTree(polys);
  return getExPolys(tree, polys.back().getZ(), polys.back().getExtrusionFactor());
  */
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
  for (uint i = 0; i<holes.size(); i++)
    for (uint j = 0; j<expolys.size(); j++)
      if (holes[i].size()>0)
	if (expolys[j].outer.vertexInside(holes[i].vertices[0])) // just test one point
	  //if (holes[i].isInside(expolys[j].outer)) // test all
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

CL::PolyTree Clipping::getClipperTree(const vector<Poly> &polys)
{
  CL::Paths cpolys = getClipperPolygons(polys);
  CL::Clipper clpr;
  clpr.AddPaths(cpolys, CL::ptSubject, true);
  CL::PolyTree ctree;
  clpr.Execute(CL::ctUnion, ctree, CL::pftEvenOdd, CL::pftEvenOdd);
  return ctree;
}

double Clipping::Area(const Poly &poly){
  CL::Path cp = getClipperPolygon(poly);
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




