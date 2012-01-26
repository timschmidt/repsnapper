
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

Layer::Layer(int layerno, double thick, uint skins) 
  : LayerNo(layerno), thickness(thick), skins(skins)
{
  normalInfill = NULL;//new Infill(this,1.);
  fullInfill = NULL;//new Infill(this,1.);
  bridgeInfill = NULL;//new Infill(this,1.);
  supportInfill = NULL;//new Infill(this,1.);
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
  delete bridgeInfill;//->clear();
  delete supportInfill;//->clear();
  skinFullInfills.clear();
  clearpolys(polygons);
  clearpolys(shellPolygons);
  clearpolys(fillPolygons);
  clearpolys(fullFillPolygons);
  clearpolys(bridgePolygons);
  bridge_angles.clear();
  clearpolys(supportPolygons);
  clearpolys(skinPolygons);
  clearpolys(skinFullFillPolygons);
  hullPolygon.clear();
  skirtPolygon.clear();
}

// void Layer::setBBox(Vector2d min, Vector2d max) 
// {
//   Min.x = MIN(Min.x,min.x);
//   Min.y = MIN(Min.y,min.y);
//   Max.x = MAX(Max.x,max.x);
//   Max.y = MAX(Max.y,max.y);
// }	
// void Layer::setBBox(vector<Vector2d> minmax) 
// {
//   setBBox(minmax[0],minmax[1]);
// }
// void Layer::setBBox(Vector3d min, Vector3d max) 
// {
//   Min.x = MIN(Min.x,min.x);
//   Min.y = MIN(Min.y,min.y);
//   Max.x = MAX(Max.x,max.x);
//   Max.y = MAX(Max.y,max.y);
// }	


void Layer::SetPolygons(vector<Poly> polys) {
  this->polygons = polys;
  for(uint i=0;i<polygons.size();i++){
    polygons[i].setZ(Z); 
  }
}
void Layer::SetPolygons(const Matrix4d &T, const Shape shape, 
			double z) {
  double offsetZ = Z;
  bool polys_ok=false;
  while (!polys_ok) {
    double max_grad;
    polys_ok = shape.getPolygonsAtZ(T, offsetZ, polygons, max_grad);
    offsetZ+=thickness/10.;
  }
  for(uint i=0;i<polygons.size();i++){
    polygons[i].cleanup(thickness/2);
    polygons[i].setZ(Z); 
  }
}

int Layer::addShape(Matrix4d T, const Shape shape, double z, 
		     double &max_gradient)
{
  double hackedZ = z;
  bool polys_ok = false;
  vector<Poly> polys;
  int num_polys=-1;
  // try to slice all objects until polygons can be made, otherwise hack z
  while (!polys_ok && hackedZ < z+thickness) {
    polys.clear();
    polys_ok=shape.getPolygonsAtZ(T, hackedZ,  // slice shape at hackedZ
				  polys, max_gradient);
    if (polys_ok) {
      num_polys = polys.size();
      addPolygons(polys);
    } else {
      num_polys=-1;
      cerr << "hacked Z=" << hackedZ << endl;
    }
    hackedZ += thickness/10;
  }
  return num_polys;
}


void Layer::addPolygons(vector<Poly> polys)
{
  for(uint i=0;i<polys.size();i++){
    polys[i].cleanup(thickness/2);
    polys[i].setZ(Z); 
  }
  polygons.insert(polygons.end(),polys.begin(),polys.end());
}

// these are used for the bridge polys of the layer above 
vector <double> Layer::getBridgeRotations(const vector<Poly> polys) const{
  vector<double> angles; angles.resize(polys.size());
  Clipping clipp;
  vector<Poly> offset = polygons;//clipp.getOffset(polygons,3*thickness);
  for (uint i=0; i<polys.size(); i++) 
    {
      // intersect bridge poly with polygons below (=pillars of bridge)
      clipp.clear();
      clipp.addPolys(offset,subject); 
      clipp.addPolys(clipp.getOffset(polys[i],2*thickness),clip); 
      vector<Poly> pillars = clipp.intersect();
      // get average direction of the connection lines of all the intersections
      Vector2d dir(0,0);
      for (uint p=0; p<pillars.size(); p++){
	for (uint q=p+1; q<pillars.size(); q++){
	  dir+=pillars[q].center-pillars[p].center;
	}
      }
      //cerr << pillars.size() << " - " << dir << endl;
      angles[i] = atan2(dir.y,dir.x);
    }
  return angles;
}

void Layer::CalcInfill (int normalfilltype, int fullfilltype, int supportfilltype,
			double InfillDistance, 	double FullInfillDistance,
			double InfillRotation, 	double InfillRotationPrLayer,
			bool ShellOnly, // only infill for fullfill (vertical shells)
			bool DisplayDebuginFill)
{
  // relative extrusion for skins:
  double skinfillextrf = 1./skins/skins; 

  //cerr <<"infill "<< normalfilltype << " - " <<fullfilltype<< endl;
  normalInfill = new Infill(this,1.);
  normalInfill->setName("normal");
  //cout << "new "; normalInfill->printinfo();
  fullInfill = new Infill(this,1.);
  fullInfill->setName("full");
  bridgeInfill = new Infill(this,1.);
  bridgeInfill->setName("bridge");
  //cout << "new " ;fullInfill->printinfo();
  skinFullInfills.clear();
  //skinFullInfills.resize(skins);
  supportInfill = new Infill(this,0.5); // thinner walls for support
  supportInfill->setName("support");
  double rot = (InfillRotation + (double)LayerNo*InfillRotationPrLayer)/180.0*M_PI;
  //cerr << fillPolygons.size();
  if (!ShellOnly)
    normalInfill->addInfill(Z, fillPolygons, (InfillType)normalfilltype, //ParallelInfill,
			    InfillDistance, FullInfillDistance, rot);
  //normalInfill->printinfo();
  
  fullInfill->addInfill(Z, fullFillPolygons, (InfillType)fullfilltype,
			FullInfillDistance, FullInfillDistance, rot);
  
  //  cerr << LayerNo << ": "<<bridgePolygons.size() << " = " << bridge_angles.size() << endl;
  
  assert(bridge_angles.size() == bridgePolygons.size());
  for (uint b=0; b<bridgePolygons.size(); b++){
    bridgeInfill->addInfill(Z, bridgePolygons[b], BridgeInfill,
			    FullInfillDistance, FullInfillDistance, bridge_angles[b]+M_PI/2);
  }
  if (skins>1) {
    double skindistance = FullInfillDistance/skins;
    for (uint s = 0; s<skins; s++){
      double drot = rot + InfillRotationPrLayer/180.0*M_PI*s;
      double sz = Z-thickness + (s+1)*thickness/skins;
      //cerr << Z << " : " << skins << " - "<< s << " - " << sz << " - " << thickness <<endl;
      Infill *inf = new Infill(this, skinfillextrf);
      inf->setName("skin");
      inf->addInfill(sz, skinFullFillPolygons, (InfillType)fullfilltype,
		     skindistance, skindistance, drot);
      skinFullInfills.push_back(inf);
    }
  }
  supportInfill->addInfill(Z, supportPolygons, (InfillType)supportfilltype, 
			   1.*InfillDistance, 1.*InfillDistance, 0);
}


// call before full fill areas are multiplied
void Layer::makeSkinPolygons() 
{
  if (skins<2) return;
  vector<Poly> fp = GetFullFillPolygons();
  //double dist = thickness/skins;
  for (uint i=0; i<fp.size(); i++){
    skinFullFillPolygons.push_back(fp[i]);
    fullFillPolygons[i].clear();
    //fp.erase(fp.begin()+i);
    //i--;
  }
}

// add bridge polys and substract them from normal and full fill polys 
void Layer::addBridgePolygons(const vector<Poly> newpolys) 
{
  if (newpolys.size()==0) return;
  bridgePolygons.clear();
  Clipping clipp;
  clipp.clear();
  clipp.addPolys(GetFillPolygons(),subject); 
  clipp.addPolys(newpolys,clip);
  vector<Poly> inter = clipp.intersect();
  bridgePolygons= inter;//.insert(bridgePolygons.end(),inter.begin(),inter.end());
  clipp.clear();
  clipp.addPolys(GetFillPolygons(),subject);  
  clipp.addPolys(inter,clip);
  setNormalFillPolygons(clipp.substract());
  mergeFullPolygons(true);
}

// add full fill and substract them from normal fill polys 
void Layer::addFullPolygons(const vector<Poly> newpolys) 
{
  if (newpolys.size()==0) return;
  Clipping clipp;
  clipp.clear();
  // full fill only where already normal fill
  clipp.addPolys(GetFillPolygons(),subject); 
  clipp.addPolys(newpolys,clip);
  vector<Poly> inter = clipp.intersect();
  fullFillPolygons.insert(fullFillPolygons.end(),inter.begin(),inter.end());
  //substract from normal fills
  clipp.clear();
  clipp.addPolys(GetFillPolygons(),subject);  
  clipp.addPolys(inter,clip);
  setNormalFillPolygons(clipp.substract());
  mergeFullPolygons(false);
}


void Layer::mergeFullPolygons(bool bridge) 
{
  if (bridge) {
    //cerr << bridgePolygons.size() ;
    setBridgePolygons(Clipping::getMerged(bridgePolygons));
    //cerr << " --> " << bridgePolygons.size() << endl;
  } else  
    setFullFillPolygons(Clipping::getMerged(fullFillPolygons));
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
  if (fillPolygons.size()>0) return fillPolygons;
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
  clearpolys(fillPolygons);
  fillPolygons = polys;
  for (uint i=0; i<fillPolygons.size();i++)
    fillPolygons[i].setZ(Z);
}

void Layer::setFullFillPolygons(const vector<Poly>  polys)
{
  clearpolys(fullFillPolygons);
  fullFillPolygons = polys;
  for (uint i=0; i<fullFillPolygons.size();i++)
    fullFillPolygons[i].setZ(Z);
}
void Layer::setBridgePolygons(const vector<Poly>  polys)
{
  clearpolys(bridgePolygons);
  bridgePolygons = polys;
  for (uint i=0; i<bridgePolygons.size();i++)
    bridgePolygons[i].setZ(Z);
}
void Layer::setBridgeAngles(const vector<double> angles)
{
  bridge_angles=angles; // .insert(bridge_angles.begin(),angles.begin(),angles.end());
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
		       bool makeskirt, 
		       bool useFillets)
{
  double distance = 0.5 * extrudedWidth;
  double cleandist = min(extrudedWidth/3., thickness/2.);
  vector<Poly> shrinked = Clipping::getOffset(polygons,-distance);
  for (uint i = 0; i<shrinked.size(); i++)  
    shrinked[i].cleanup(cleandist);
  //vector<Poly> shrinked = Clipping::getShrinkedCapped(polygons,distance);
  // outmost shells
  if (skins>1) { // either skins
    for (uint i = 0; i<shrinked.size(); i++)  {
      shrinked[i].setExtrusionFactor(1./skins);
    }
    skinPolygons = shrinked; 
  } else {  // or normal shell
    clearpolys(shellPolygons);
    shellPolygons.push_back(shrinked); 
  }
  // inner shells
  distance = extrudedWidth;
  for (uint i = 1; i<shellcount; i++) // shrink from shell to shell
    {
      shrinked = Clipping::getOffset(shrinked,-distance);
      for (uint i = 0; i<shrinked.size(); i++)  
	shrinked[i].cleanup(cleandist);
      //shrinked = Clipping::getShrinkedCapped(shrinked,distance); 
      shellPolygons.push_back(shrinked);
    }
  // the filling polygon
  fillPolygons = Clipping::getOffset(shrinked,-distance);
  for (uint i = 0; i<fillPolygons.size(); i++)  
    fillPolygons[i].cleanup(cleandist);
  //fillPolygons = Clipping::getShrinkedCapped(shrinked,distance);
  //cerr << LayerNo << " > " << fillPolygons.size()<< endl;
  calcConvexHull();
  if (makeskirt) {
    MakeSkirt(3*distance); // skirt distance = 3 * shell distance
  }

  //cerr << " .. made " << fillPolygons.size() << " offsetpolys "  << endl;
  // for (uint i =0; i<shellPolygons.size(); i++) {
  //   cout << "shell " << i << endl;
  //   for (uint j =1; j<shellPolygons[i].size(); j++) {
  //     shellPolygons[i][j].printinfo();
  //   }
  // }
}

void Layer::MakeSkirt(double distance)
{
  skirtPolygon.clear();
  vector<Poly> skp = Clipping::getOffset(hullPolygon,distance,jround);
  if (skp.size()>0){
    skirtPolygon = skp.front();
    skirtPolygon.cleanup(distance/3.);
  }
}


// is B left of A wrt center?
bool isleftof(Vector2d center, Vector2d A, Vector2d B)
{
  double position = (B.x-A.x)*(center.y-A.y) - (B.y-A.y)*(center.x-A.x);
  // position = sign((p2-p1).cross(center-p1)[2]) ; // this would be 3d
  return (position > 0);
}


// 2D cross product of OA and OB vectors, i.e. z-component of their 3D cross product.
// Returns a positive value, if OAB makes a counter-clockwise turn,
// negative for clockwise turn, and zero if the points are collinear.
double cross(const Vector2d &O, const Vector2d &A, const Vector2d &B)
{
  return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
}
struct sortable_point {
  Vector2d v;
  bool operator <(const sortable_point &p) const {
    return (v.x < p.v.x) || ((v.x == p.v.x) && (v.y < p.v.y));
  }
};

// calc convex hull and Min and Max of layer
// Monotone chain algo
// http://en.wikibooks.org/wiki/Algorithm_Implementation/Geometry/Convex_hull/Monotone_chain
void Layer::calcConvexHull() 
{
  hullPolygon.clear();
  hullPolygon=Poly(Z);
  vector<struct sortable_point> P;
  for (uint i = 0; i<polygons.size(); i++) 
    for (uint j = 0; j<polygons[i].size(); j++) {
      struct sortable_point point;
      point.v = polygons[i].vertices[j];
      P.push_back(point);
    }
  
  int n = P.size();
  vector<Vector2d> H(2*n);
  //cerr << n << " points"<< endl;
  if (n<2) return;
  if (n<4) {
    for (int i = 0; i < n; i++) 
      hullPolygon.addVertex(P[i].v);
    return;
  }
  sort(P.begin(), P.end());
  // for (int i = 0; i < n; i++) {
  //   cerr <<P[i].v<< endl;
  // }
  // Build lower hull
  int k=0;
  for (int i = 0; i < n; i++) {
    while (k >= 2 && cross(H[k-2], H[k-1], P[i].v) <= 0) k--;
    H[k++] = P[i].v;
  }
 
  // Build upper hull
  for (int i = n-2, t = k+1; i >= 0; i--) {
    while (k >= t && cross(H[k-2], H[k-1], P[i].v) <= 0) k--;
    H[k++] = P[i].v;
  }
  H.resize(k);
  hullPolygon.vertices = H;
  vector<Vector2d> minmax = hullPolygon.getMinMax();
  Min = minmax[0];
  Max = minmax[1];
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

  Printlines printlines;
  
  lines.clear(); // will contain everything

  vector<Poly> polys; // intermediate collection

  // 1. Skins, because they are the lowest lines, below layer Z
  if (skins>1){
    for(uint s=1;s <= skins;s++) { // z offset from bottom to top
      polys.clear();
      double sz = Z - thickness + (s)*thickness/skins;
      // outlines
      for(size_t p=0;p<skinPolygons.size();p++) { 
	Poly sp(skinPolygons[p],sz);
	polys.push_back(sp);
      }
      // skin infill polys
      polys.insert(polys.end(),
		   skinFullInfills[s-1]->infillpolys.begin(),
		   skinFullInfills[s-1]->infillpolys.end());
      // add all of this skin layer to lines
      printlines.clear();
      printlines.makeLines(polys, startPoint, lines, linewidth, linewidthratio, optratio);
    }
  }
  polys.clear();

  // 2. Skirt
  skirtPolygon.getLines(lines, startPoint);

  // 3. Support
  printlines.clear();
  printlines.makeLines(supportInfill->infillpolys, startPoint, lines, 
		       linewidth, linewidthratio, optratio);

  // 4. all other polygons:
  
  //  Shells
  for(size_t p=0;p<shellPolygons.size();p++) // outer to inner, in this order
    polys.insert(polys.end(), shellPolygons[p].begin(),shellPolygons[p].end());
  polys.insert(polys.end(),
	       normalInfill->infillpolys.begin(), normalInfill->infillpolys.end());
  polys.insert(polys.end(),
	       fullInfill->infillpolys.begin(), fullInfill->infillpolys.end());
  polys.insert(polys.end(),
	       bridgeInfill->infillpolys.begin(), bridgeInfill->infillpolys.end());
  
  printlines.clear();
  printlines.makeLines(polys, startPoint, lines, 
		       linewidth, linewidthratio, optratio);

  // push all lines to gcode
  state.AddLines(lines, extrf, offsetZ, slicing, hardware); 
}


double Layer::area() const
{
  return Clipping::Area(polygons);
}

string Layer::info() const 
{
  ostringstream ostr;
  ostr <<"Layer at Z="<<Z<<" Lno="<<LayerNo 
       <<", "<<skins <<" skins"
       <<", "<<polygons.size() <<" polys"
       <<", "<<shellPolygons.size() <<" shells"
       <<", "<<fullFillPolygons.size() <<" fullfill polys"
       <<", "<<bridgePolygons.size() <<" bridge polys"
       <<", "<<skinFullFillPolygons.size() <<" skin fullfill polys"
       <<", "<<supportPolygons.size() <<" support polys";
  ostr <<", infill: ";
  if (normalInfill)
    ostr <<" normal "<<normalInfill->size() ;
  if (fullInfill)
    ostr <<", full "<<fullInfill->size()  ;
  if (bridgeInfill)
    ostr <<", bridge "<<bridgeInfill->size() ;
  if (supportInfill)
    ostr<<", support "<<supportInfill->size() ;
  ostr <<", skinfills "<<skinFullInfills.size() ;
  
  // if (next)
  //   cout <<", next: "<<next->LayerNo;
  // if (previous)
  // cout <<", previous: "<<previous->LayerNo;
  return ostr.str();
}
 


void draw_poly(Poly poly, int gl_type, int linewidth, int pointsize,
	       float r, float g, float b, float a)
{
  glColor4f(r,g,b,a);
  glLineWidth(linewidth);
  glPointSize(pointsize);
  poly.draw(gl_type);
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
  draw_polys(fillPolygons, GL_LINE_LOOP, 1, 3, 1,1,1,1);
  draw_polys(supportPolygons, GL_LINE_LOOP, 3, 3, 0.5,0.5,1.0,1);
  draw_poly(hullPolygon, GL_LINE_LOOP, 3, 3, 0.8,0.6,0.0,0.5);
  draw_polys(bridgePolygons, GL_LINE_LOOP, 3, 3, 0.8,0.5,0.5,0.8);
  draw_polys(fullFillPolygons, GL_LINE_LOOP, 1, 3, 0.5,0.5,0.5,1);
  draw_polys(skinFullFillPolygons, GL_LINE_LOOP, 1, 3, 0.5,0.5,0.5,1);
  if(DisplayInfill)
    {
      if (normalInfill)
	draw_polys(normalInfill->infillpolys, GL_LINE_LOOP, 1, 3, 0.1,1,0.1,0.8);
      if (fullInfill)
	draw_polys(fullInfill->infillpolys, GL_LINE_LOOP, 1, 3, 0.1,1,0.1,0.8);
      if (bridgeInfill)
	draw_polys(bridgeInfill->infillpolys, GL_LINE_LOOP, 1, 3, 0.8,0.3,0.1,0.8);
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
