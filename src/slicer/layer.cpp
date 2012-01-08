
/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010  Kulitorum
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

#include "layer.h"
#include "poly.h"
#include "shape.h"
#include "infill.h"
#include "printlines.h"

Layer::Layer(int layerno, double thick) : LayerNo(layerno), thickness(thick) 
{
  normalInfill = NULL;//new Infill(this,1.);
  fullInfill = NULL;//new Infill(this,1.);
  supportInfill = NULL;//new Infill(this,1.);
  skins=1;
  Min = Vector2d(G_MAXDOUBLE, G_MAXDOUBLE);
  Max = Vector2d(G_MINDOUBLE, G_MINDOUBLE);
}


Layer::~Layer()
{
  Clear();
  // delete normalInfill;
  // delete fullInfill;
  // delete supportInfill;
}


void clearpolys(vector<Poly> &polys){
  for (uint i=0; i<polys.size();i++)
    polys[i].clear();
  polys.clear();
}
void clearpolys(vector< vector<Poly> > &polys){
  for (uint i=0; i<polys.size();i++)
    clearpolys(polys[i]);
  polys.clear();
}

void Layer::Clear()
{
  delete normalInfill;//->clear();
  delete fullInfill;//->clear();
  delete supportInfill;//->clear();
  skinFullInfills.clear();
  clearpolys(polygons);
  clearpolys(shellPolygons);
  clearpolys(offsetPolygons);
  clearpolys(fullFillPolygons);
  clearpolys(supportPolygons);
  clearpolys(skinPolygons);
  clearpolys(skinFullFillPolygons);
  hullPolygon.clear();
  skirtPolygon.clear();
}

void Layer::setBBox(Vector2d min, Vector2d max) 
{
  Min.x = MIN(Min.x,min.x);
  Min.y = MIN(Min.y,min.y);
  Max.x = MAX(Max.x,max.x);
  Max.y = MAX(Max.y,max.y);
}	
void Layer::setBBox(vector<Vector2d> minmax) 
{
  setBBox(minmax[0],minmax[1]);
}
void Layer::setBBox(Vector3d min, Vector3d max) 
{
  Min.x = MIN(Min.x,min.x);
  Min.y = MIN(Min.y,min.y);
  Max.x = MAX(Max.x,max.x);
  Max.y = MAX(Max.y,max.y);
}	


void Layer::SetPolygons(vector<Poly> polys) {
  this->polygons = polys;
}
void Layer::SetPolygons(const Matrix4d &T, const Shape shape, 
			double z, double Optimization) {
  double offsetZ = Z;
  bool have_polys=false;
  while (!have_polys) {
    clearpolys(polygons);
    have_polys = shape.getPolygonsAtZ(T, Optimization, offsetZ, polygons);
    offsetZ+=thickness/10.;
  }
  for(uint i=0;i<polygons.size();i++)
    polygons[i].setZ(Z);
}

void Layer::addPolygons(vector<Poly> polys)
{
  for (uint i =0;i<polys.size();i++)
    {
      polygons.push_back(polys[i]);
      setBBox(polys[i].getMinMax());
    }
}

void Layer::CalcInfill (double InfillDistance, 
			double FullInfillDistance,
			double InfillRotation, 
			double InfillRotationPrLayer,
			bool ShellOnly, // only infill for fullfill (vertical shells)
			bool DisplayDebuginFill)
{
  // relative extrusion for skins:
  double skinfillextrf = 1./skins/skins; 

  normalInfill = new Infill(this,1.);
  normalInfill->setName("normal");
  //cout << "new "; normalInfill->printinfo();
  fullInfill = new Infill(this,1.);
  fullInfill->setName("full");
  //cout << "new " ;fullInfill->printinfo();
  skinFullInfills.clear();
  //skinFullInfills.resize(skins);
  supportInfill = new Infill(this,1.);
  supportInfill->setName("support");
  double rot = (InfillRotation + (double)LayerNo*InfillRotationPrLayer)/180.0*M_PI;
  if (!ShellOnly)
    normalInfill->addInfill(Z, offsetPolygons, ParallelInfill,
			    InfillDistance, FullInfillDistance, rot);
  fullInfill->addInfill(Z, fullFillPolygons, PolyInfill,
			FullInfillDistance, FullInfillDistance, 0);
  if (skins>1) {
    double skindistance = FullInfillDistance/skins;
    for (uint s = 0; s<skins; s++){
      //double drot = InfillRotationPrLayer/180.0*M_PI*s;
      double sz = Z-thickness + (s+1)*thickness/skins;
      //cerr << Z << " : " << skins << " - "<< s << " - " << sz << " - " << thickness <<endl;
      Infill *inf = new Infill(this, skinfillextrf);
      inf->setName("skin");
      //cout << "new " ;inf->printinfo();
      inf->addInfill(sz, skinFullFillPolygons, PolyInfill,
		     skindistance, skindistance, 0);
      skinFullInfills.push_back(inf);
    }
  }
  supportInfill->addInfill(Z, supportPolygons, PolyInfill, 
			   InfillDistance, InfillDistance, 0);//InfillRotation/180.0*M_PI);
}


// call when all full fills are outer areas
void Layer::makeSkinPolygons(uint mskins) 
{
  skins = mskins;
  vector<Poly> fp = GetFullFillPolygons();
  //double dist = thickness/skins;
  for (uint i=0; i<fp.size(); i++){
    skinFullFillPolygons.push_back(fp[i]);
    fullFillPolygons[i].clear();
    //fp.erase(fp.begin()+i);
    //i--;
  }
}


// add full fill and substract them from normal fill polys 
void Layer::addFullPolygons(const vector<Poly> fullpolys) 
{
  Clipping clipp;
  clipp.clear();
  // full fill only where already normal fill
  clipp.addPolys(GetOffsetPolygons(),subject); 
  clipp.addPolys(fullpolys,clip);
  vector<Poly> inter = clipp.intersect();
  addFullFillPolygons(inter);
  //substract from normals
  clipp.clear();
  clipp.addPolys(GetOffsetPolygons(),subject);
  clipp.addPolys(fullpolys,clip);
  setNormalFillPolygons(clipp.substract());
  mergeFullPolygons();
}


void Layer::mergeFullPolygons() 
{
  setFullFillPolygons(Clipping::getMerged(GetFullFillPolygons()));
}
void Layer::mergeSupportPolygons() 
{
  setSupportPolygons(Clipping::getMerged(GetSupportPolygons()));
}

vector<Poly> Layer::GetInnerShell() const
{
  if (shellPolygons.size()>0) return shellPolygons.back();
  // no shells:
  if (skinPolygons.size()>0) return skinPolygons;
  // no skins
  if (offsetPolygons.size()>0) return offsetPolygons;
  // no offset
  return polygons;
}

// circular numbering
vector<Poly> Layer::GetShellPolygonsCirc(int number) const
{
  number = (shellPolygons.size() +  number) % shellPolygons.size();
  return shellPolygons[number];
}

void Layer::setNormalFillPolygons(const vector<Poly> polys)
{
  clearpolys(offsetPolygons);
  offsetPolygons = polys;
  for (uint i=0; i<offsetPolygons.size();i++)
    offsetPolygons[i].setZ(Z);
}

void Layer::setFullFillPolygons(const vector<Poly>  polys)
{
  clearpolys(fullFillPolygons);
  fullFillPolygons = polys;
  for (uint i=0; i<fullFillPolygons.size();i++)
    fullFillPolygons[i].setZ(Z);
}

void Layer::addFullFillPolygons(const vector<Poly> polys)
{
  fullFillPolygons.insert(fullFillPolygons.end(),polys.begin(),polys.end());
}

void Layer::setSupportPolygons(const vector<Poly> polys)
{
  clearpolys(supportPolygons);
  supportPolygons = polys;
}

void Layer::setSkirtPolygon(const Poly poly)
{
  skirtPolygon = poly;
  skirtPolygon.setZ(Z);
}


void Layer::MakeShells(uint shellcount, double extrudedWidth, 
		       double optimization, 
		       bool makeskirt, uint mskins,
		       bool useFillets)
{
  skins = mskins;
  double distance = 0.5 * extrudedWidth;
  // go in, then out to get rounded corners
  vector<Poly> shrinked = Clipping::getOffset(polygons,-3*distance,jround);
  shrinked = Clipping::getOffset(shrinked,2*distance,jmiter,0);
  if (skins>1) {
    for (uint i = 0; i<shrinked.size(); i++) 
      shrinked[i].setExtrusionFactor(1./skins);
    skinPolygons = shrinked; 
  } else {
    clearpolys(shellPolygons);
    shellPolygons.push_back(shrinked); // outer
  }
  distance = extrudedWidth;
  for (uint i = 1; i<shellcount; i++) // shrink from shell to shell
    {
      shrinked = Clipping::getOffset(shrinked,-distance); 
      shellPolygons.push_back(shrinked);
    }
  offsetPolygons = Clipping::getOffset(shrinked,-distance);

  if (makeskirt) {
    MakeSkirt(2*distance);
  }

  //cerr << " .. made " << offsetPolygons.size() << " offsetpolys "  << endl;
  // for (uint i =0; i<shellPolygons.size(); i++) {
  //   cout << "shell " << i << endl;
  //   for (uint j =1; j<shellPolygons[i].size(); j++) {
  //     shellPolygons[i][j].printinfo();
  //   }
  // }
}

void Layer::MakeSkirt(double distance)
{
  calcConvexHull();
  skirtPolygon.clear();
  vector<Poly> skp = Clipping::getOffset(hullPolygon,distance,jround);
  if (skp.size()>0)
    skirtPolygon = skp.front();
}


// is B left of A wrt center?
bool isleftof(Vector2d center, Vector2d A, Vector2d B)
{
  double position = (B.x-A.x)*(center.y-A.y) - (B.y-A.y)*(center.x-A.x);
  // position = sign((p2-p1).cross(center-p1)[2]) ; // this would be 3d
  return (position > 0);
}

// gift wrapping algo
void Layer::calcConvexHull() 
{
  hullPolygon.clear();
  hullPolygon=Poly(Z);
  vector<Vector2d> p;
  for (uint i = 0; i<polygons.size(); i++)
    p.insert(p.end(), polygons[i].vertices.begin(), polygons[i].vertices.end());
  uint np = p.size();
  //cerr << np << " points"<< endl;
  if (np==0) return;
  uint current=np;
  double minx=G_MAXDOUBLE;
  double miny=G_MAXDOUBLE;
  for (uint i = 0; i < np; i++) { // get leftmost bottom
    if (p[i].x < minx){
      minx = p[i].x;
      miny = p[i].y;
      current = i;
    }
    else if (p[i].x==minx && p[i].y<miny){
      minx = p[i].x;
      miny = p[i].y;
      current = i;
    }
  }
  assert (current != np);

  hullPolygon.addVertex(p[current]);
  uint start = current;
  uint leftmost ;
  do 
    {
      leftmost=0;
      for (uint i=1; i < np; i++)
	if (leftmost == current || isleftof(p[current], p[i], p[leftmost])) // i more to the left?
	  leftmost = i;
      current = leftmost;
      hullPolygon.addVertex(p[current], true); // to front: reverse order
    }
  while (current != start);
}

void Layer::getOrderedPrintLines(const vector<Poly> polys, 
				 Vector2d &startPoint,
				 //				Printlines &printlines) const
				 vector<printline> &plines,
				 double linewidth,double linewidthratio,double optratio) const
{
  Printlines printlines;
  uint count  = polys.size();
  uint nvindex=-1;
  uint npindex=-1;
  uint nindex;
  vector<bool> done; // polys not yet handled
  done.resize(count);
  for(size_t q=0;q<count;q++) done[q]=false;
  uint ndone=0;
  double pdist, nstdist;
  //double nlength;
  while (ndone < count) 
    {
      nstdist = 1000000;
      for(size_t q=0;q<count;q++) // find nearest polygon
	{
	  if (!done[q])
	    {
	      pdist = 1000000;
	      nindex = polys[q].nearestDistanceSqTo(startPoint,pdist);
	      // nlength = polys[q].getLineLengthSq(nindex); // ...feature to come...
	      if (pdist<nstdist){
		npindex = q;      // index of nearest poly in polysleft
		nstdist = pdist;  // distance of nearest poly
		nvindex = nindex; // nearest point in nearest poly
	      }
	    }
	}
      printlines.addPoly(polys[npindex],nvindex);
      //polys[npindex].getLines(lines,nvindex); // add poly lines to lines
      done[npindex]=true;
      ndone++;
      startPoint = printlines.lastPoint();//Vector2d(lines.back().x,lines.back().y);
    }
  printlines.setZ(polys.back().getZ());
  printlines.optimize(linewidth, linewidthratio, optratio);
  printlines.getLines(plines);
}

// Convert to GCode
void Layer::MakeGcode(GCodeState &state,
		      double offsetZ,
		      const Settings::SlicingSettings &slicing,
		      const Settings::HardwareSettings &hardware)
{
  Vector3d start3 = state.LastPosition();
  Vector2d startPoint(start3.x,start3.y);
  
  double extrf = hardware.GetExtrudeFactor(thickness);
  
  double OPTRATIO = 1.5;
  double optratio = OPTRATIO;
  double linewidthratio = hardware.ExtrudedMaterialWidthRatio;
  double linewidth = thickness/linewidthratio;
	
  //vector<Vector3d> lines;
  vector<printline> lines;
  //cerr << "gcode layer " << LayerNo << "z="<<Z<<endl;
  
  // Skirt
  lines.clear();
  skirtPolygon.getLines(lines, startPoint);
  
  // Support
  getOrderedPrintLines(supportInfill->infillpolys, startPoint, lines,
		       linewidth, linewidthratio, optratio);
  state.AddLines(lines, extrf, offsetZ, slicing, hardware);
  
  // Skin (different extrusion factor)
  if (skins>1){
    vector<printline> skinlines;
    //lines.clear();
    for(size_t p=0;p<skinPolygons.size();p++) {
      skinPolygons[p].getLines(skinlines,startPoint);
    }
    // getOrderedPrintLines(skinPolygons, startPoint,lines, linewidth, linewidthratio, optratio);
    // state.AddLines(lines, skinextrf, offsetZ, E, slicing, hardware);
    for(uint s=1;s <= skins;s++) { // z offset of skin layers
      lines.clear();
      // first skin infill
      //cerr << s << ":"<<skins<< ":"<<skinFullInfills.size()<<endl;
      getOrderedPrintLines(skinFullInfills[s-1]->infillpolys, startPoint, lines,
			   linewidth, linewidthratio, optratio);
      state.AddLines(lines, extrf, offsetZ, slicing, hardware);
      // then skin polys 
      double sz = Z - thickness + (s)*thickness/skins;
      lines.clear();
      for(size_t p=0;p<skinlines.size();p++) {
	printline sl = skinlines[p];
	sl.from.z=sz;
	sl.to.z=sz;
	lines.push_back(sl);
      }
      state.AddLines(lines, extrf, offsetZ, slicing, hardware);
    }
  }
  
  // Shells
  vector<Poly> shinfill; // shells plus infill, order them together
  lines.clear();
  for(size_t p=0;p<shellPolygons.size();p++) // outer to inner, in this order
    shinfill.insert(shinfill.end(), shellPolygons[p].begin(),shellPolygons[p].end());
  shinfill.insert(shinfill.end(),
		  normalInfill->infillpolys.begin(), normalInfill->infillpolys.end());
  shinfill.insert(shinfill.end(),
		  fullInfill->infillpolys.begin(), fullInfill->infillpolys.end());
  
  getOrderedPrintLines(shinfill, startPoint, lines, 
		       linewidth, linewidthratio, optratio); // sorted
  // Infill
  // getOrderedPrintLines(startPoint,lines, 
  // 		    linewidth, linewidthratio, optratio);
  // getOrderedPrintLines(fullInfill->infillpolys, startPoint,lines,
  // 		    linewidth, linewidthratio, optratio);
  state.AddLines(lines, extrf, offsetZ, slicing, hardware); 
}


double Layer::area() const
{
  return Clipping::Area(polygons);
}

void Layer::printinfo() const 
{
  cout <<"Layer at Z="<<Z<<" Lno="<<LayerNo 
       <<", "<<skins <<" skins"
       <<", "<<polygons.size() <<" polys"
       <<", "<<shellPolygons.size() <<" shells"
       <<", "<<fullFillPolygons.size() <<" fullfill polys"
       <<", "<<skinFullFillPolygons.size() <<" skin fullfill polys"
       <<", "<<supportPolygons.size() <<" support polys";
  // if (next)
  //   cout <<", next: "<<next->LayerNo;
  // if (previous)
  // cout <<", previous: "<<previous->LayerNo;
  cout << endl;
}
 


void draw_polys(vector <Poly> polys, int gl_type, int linewidth, int pointsize,
		 float r, float g, float b, float a)
{
  glColor4f(r,g,b,a);
  glLineWidth(linewidth);
  glPointSize(pointsize);
  for(size_t p=0; p<polys.size();p++)
    polys[p].draw(gl_type);
}
void draw_polys(vector< vector <Poly> > polys, int gl_type, int linewidth, int pointsize,
		 float r, float g, float b, float a)
{
  for(size_t p=0; p<polys.size();p++)
    draw_polys(polys[p], gl_type, linewidth, pointsize, r,g,b,a);
}

void Layer::Draw(bool DrawVertexNumbers, bool DrawLineNumbers, 
		 bool DrawOutlineNumbers, bool DrawCPLineNumbers, 
		 bool DrawCPVertexNumbers, bool DisplayInfill) const 
{
  draw_polys(polygons, GL_LINE_LOOP, 1, 3, 1,0,0,1);
  draw_polys(polygons, GL_POINTS, 1, 3, 1,0,0,1);

  if(DrawOutlineNumbers)
    for(size_t p=0; p<polygons.size();p++)
      {
	ostringstream oss;
	oss << p;
	renderBitmapString(Vector3f(polygons[p].center.x, polygons[p].center.y, Z) , 
			   GLUT_BITMAP_8_BY_13 , oss.str());
      }

  draw_polys(shellPolygons, GL_LINE_LOOP, 1, 3, 1,1,.2,1);

  glColor4f(0.5,0.9,1,1);
  glLineWidth(1);
  double zs = Z;
  for(size_t s=0;s<skins;s++){
    for(size_t p=0;p<skinPolygons.size();p++)
      skinPolygons[p].draw(GL_LINE_LOOP,zs);
    zs-=thickness/skins;
  }

  draw_polys(shellPolygons, GL_LINE_LOOP, 2, 3, 0.5,0.8,0.8,.7);
  draw_polys(offsetPolygons, GL_LINE_LOOP, 1, 3, 1,1,1,1);
  draw_polys(supportPolygons, GL_LINE_LOOP, 3, 3, 0.5,0.5,1.0,1);
  
  if(DisplayInfill)
    {
      draw_polys(fullFillPolygons, GL_LINE_LOOP, 1, 3, 0.5,0.5,0.5,1);
      draw_polys(skinFullFillPolygons, GL_LINE_LOOP, 1, 3, 0.5,0.5,0.5,1);
      if (normalInfill)
	draw_polys(normalInfill->infillpolys, GL_LINE_LOOP, 1, 3, 0.1,1,0.1,0.8);
      if (fullInfill)
	draw_polys(fullInfill->infillpolys, GL_LINE_LOOP, 1, 3, 0.1,1,0.1,0.8);
      if (supportInfill)
	draw_polys(supportInfill->infillpolys, GL_LINE_LOOP, 1, 3, 0.1,1,0.1,0.8);
      for(size_t s=0;s<skinFullInfills.size();s++) 
	draw_polys(skinFullInfills[s]->infillpolys, GL_LINE_LOOP, 3, 3, 0.2,1,0.2,0.6);
    }
  glLineWidth(1);  
  if(DrawCPVertexNumbers)
    for(size_t p=0; p<polygons.size();p++)
      polygons[p].drawVertexNumbers();
  
  if(DrawCPLineNumbers)
    for(size_t p=0; p<polygons.size();p++)
      polygons[p].drawLineNumbers();
}
