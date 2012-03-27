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

// polygons will be simplified to thickness/CLEANFACTOR
#define CLEANFACTOR 5

Layer::Layer(int layerno, double thick, uint skins) 
  : LayerNo(layerno), thickness(thick), skins(skins)
{
  normalInfill = NULL;
  fullInfill = NULL;
  supportInfill = NULL;
  decorInfill = NULL;
  Min = Vector2d(G_MAXDOUBLE, G_MAXDOUBLE);
  Max = Vector2d(G_MINDOUBLE, G_MINDOUBLE);
}


Layer::~Layer()
{
  Clear();
}


void clearpolys(vector<Poly> &polys){
  for (uint i=0; i<polys.size();i++)
    polys[i].clear();
  polys.clear();
}
void clearpolys(vector<ExPoly> &polys){
  for (uint i=0; i<polys.size();i++) {
    polys[i].outer.clear();
    for (uint j=0; j<polys[i].holes.size();j++) {
      polys[i].holes[j].clear();
    }
  }
  polys.clear();
}
void clearpolys(vector< vector<Poly> > &polys){
  for (uint i=0; i<polys.size();i++)
    clearpolys(polys[i]);
  polys.clear();
}

void Layer::Clear()
{
  delete normalInfill;
  delete fullInfill;
  delete supportInfill;
  delete decorInfill;
  skinFullInfills.clear();
  clearpolys(polygons);
  clearpolys(shellPolygons);
  clearpolys(fillPolygons);
  clearpolys(fullFillPolygons);
  clearpolys(bridgePolygons);
  clearpolys(bridgePillars);
  bridge_angles.clear();
  bridgeInfills.clear();
  clearpolys(decorPolygons);
  clearpolys(supportPolygons);
  clearpolys(skinPolygons);
  clearpolys(skinFullFillPolygons);
  hullPolygon.clear();
  skirtPolygon.clear();
}

// void Layer::setBBox(Vector2d min, Vector2d max) 
// {
//   Min.x() = MIN(Min.x(),min.x());
//   Min.y = MIN(Min.y,min.y);
//   Max.x() = MAX(Max.x(),max.x());
//   Max.y = MAX(Max.y,max.y);
// }	
// void Layer::setBBox(vector<Vector2d> minmax) 
// {
//   setBBox(minmax[0],minmax[1]);
// }
// void Layer::setBBox(Vector3d min, Vector3d max) 
// {
//   Min.x() = MIN(Min.x(),min.x());
//   Min.y = MIN(Min.y,min.y);
//   Max.x() = MAX(Max.x(),max.x());
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
  cerr << "Layer::SetPolygons" << endl;
  double offsetZ = Z;
  bool polys_ok=false;
  while (!polys_ok) {
    double max_grad;
    polys_ok = shape.getPolygonsAtZ(T, offsetZ, polygons, max_grad);
    offsetZ+=thickness/10.;
  }
  for(uint i=0;i<polygons.size();i++){
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
  cleanupPolygons();
  return num_polys;
}

void Layer::cleanupPolygons()
{
  double clean = thickness/CLEANFACTOR;
  for(uint i=0; i < polygons.size(); i++){
    polygons[i].cleanup(clean);
  }
}

void Layer::addPolygons(vector<Poly> polys)
{
  for(uint i=0;i<polys.size();i++){
    polys[i].setZ(Z); 
  }
  polygons.insert(polygons.end(),polys.begin(),polys.end());
}


void Layer::calcBridgeAngles(const Layer *layerbelow) {
  bridge_angles.resize(bridgePolygons.size());
  Clipping clipp;
  vector<Poly> polysbelow = layerbelow->GetInnerShell();//clipp.getOffset(polygons,3*thickness);
  bridgePillars.resize(bridgePolygons.size());
  for (uint i=0; i<bridgePolygons.size(); i++) 
    {
      // intersect bridge poly with polygons below (=pillars of bridge)
      clipp.clear();
      clipp.addPolys(polysbelow,subject); 
      clipp.addPolys(clipp.getOffset(bridgePolygons[i].outer,thickness),clip); 
      bridgePillars[i] = clipp.intersect();
      
      // TODO detect circular bridges -> rotating infill?

      // get average direction of the mutual connections of all the intersections
      Vector2d dir(0,0);
      for (uint p=0; p<bridgePillars[i].size(); p++){
	for (uint q=p+1; q<bridgePillars[i].size(); q++){
	  // Vector2d p1,p2; 
	  // bridgePillars[i][q].shortestConnectionSq(bridgePillars[i][p], p1,p2);
	  // dir += (p2-p1);
	  dir += bridgePillars[i][q].center - bridgePillars[i][p].center;
	}
      }
      //cerr << pillars.size() << " - " << dir << endl;
      bridge_angles[i] = atan2(dir.y(),dir.x());
      if (bridge_angles[i]<0) bridge_angles[i]+=M_PI;
    }
}

// these are used for the bridge polys of the layer above 
// NOT USED, NOT TESTED, using calcBridgeAngles
vector <double> Layer::getBridgeRotations(const vector<Poly> polys) const{
  cerr << "Layer::getBridgeRotations" << endl;
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
      angles[i] = atan2(dir.y(),dir.x());
    }
  return angles;
}

// fills polys with raft type infill
// so this layer will have the raft as normal infill
void Layer::CalcRaftInfill (const vector<Poly> polys, 
			    double extrusionfactor, double infilldistance,
			    double rotation)
{
  setMinMax(polys);
  normalInfill = new Infill(this, extrusionfactor);
  normalInfill->setName("Raft");
  normalInfill->addInfill(Z, polys, RaftInfill, 
			  infilldistance, infilldistance, rotation);
}

void Layer::CalcInfill (const Settings &settings)
{
  // inFill distances in real mm:
  // for full polys/layers:
  double fullInfillDistance=0;
  double infillDistance=0; // normal fill
  double altInfillDistance=0;
  double infilldist=0;
  bool shellOnly = settings.Slicing.ShellOnly;
  fullInfillDistance = settings.GetInfillDistance(thickness, 100);
  if (settings.Slicing.InfillPercent == 0) 
    shellOnly = true;
  else 
    infillDistance = settings.GetInfillDistance(thickness,
						settings.Slicing.InfillPercent);
  if (settings.Slicing.AltInfillPercent != 0) 
    altInfillDistance = settings.GetInfillDistance(thickness,
						   settings.Slicing.AltInfillPercent);
  if (settings.Slicing.AltInfillLayers!=0 
      && LayerNo % settings.Slicing.AltInfillLayers == 0) 
    infilldist = altInfillDistance;
  else
    infilldist = infillDistance;
  if (LayerNo < (int)settings.Slicing.FirstLayersNum) {
    double first_infdist = 
      fullInfillDistance * (1+(double)settings.Slicing.FirstLayersInfillDist);
    infilldist = max(infilldist, first_infdist);
    fullInfillDistance = max(fullInfillDistance, first_infdist);
  }
  // relative extrusion for skins:
  double skinfillextrf = settings.Slicing.FullFillExtrusion/skins/skins; 
  normalInfill = new Infill(this,settings.Slicing.NormalFillExtrusion);
  normalInfill->setName("normal");
  fullInfill = new Infill(this,settings.Slicing.FullFillExtrusion);
  fullInfill->setName("full");
  skinFullInfills.clear();
  supportInfill = new Infill(this,settings.Slicing.SupportExtrusion); // thinner walls for support
  supportInfill->setName("support");
  decorInfill = new Infill(this,1.);
  decorInfill->setName("decor");

  double rot = (settings.Slicing.InfillRotation 
		+ (double)LayerNo*settings.Slicing.InfillRotationPrLayer)/180.0*M_PI;
  if (!shellOnly)
    normalInfill->addInfill(Z, fillPolygons, (InfillType)settings.Slicing.NormalFilltype, 
			    infilldist, fullInfillDistance, rot);
  
  fullInfill->addInfill(Z, fullFillPolygons, (InfillType)settings.Slicing.FullFilltype,
			fullInfillDistance, fullInfillDistance, rot);

  decorInfill->addInfill(Z, decorPolygons, (InfillType)settings.Slicing.DecorFilltype,
			 settings.Slicing.DecorInfillDistance,
			 settings.Slicing.DecorInfillDistance,
			 settings.Slicing.DecorInfillRotation/180.0*M_PI);
  
  assert(bridge_angles.size() >= bridgePolygons.size());
  bridgeInfills.resize(bridgePolygons.size());
  for (uint b=0; b < bridgePolygons.size(); b++){  
    bridgeInfills[b] = new Infill(this, settings.Slicing.BridgeExtrusion);
    bridgeInfills[b]->addInfill(Z, bridgePolygons[b], BridgeInfill, 
				fullInfillDistance, fullInfillDistance,
				bridge_angles[b]+M_PI/2);
  }

  if (skins>1) {
    double skindistance = fullInfillDistance/skins;
    for (uint s = 0; s<skins; s++){
      double drot = rot + settings.Slicing.InfillRotationPrLayer/180.0*M_PI*s;
      double sz = Z-thickness + (s+1)*thickness/skins;
      Infill *inf = new Infill(this, skinfillextrf);
      inf->setName("skin");
      inf->addInfill(sz, skinFullFillPolygons, (InfillType)settings.Slicing.FullFilltype,
		     skindistance, skindistance, drot);
      skinFullInfills.push_back(inf);
    }
  }
  supportInfill->addInfill(Z, supportPolygons, (InfillType)settings.Slicing.SupportFilltype,
			   infilldist, infilldist, 0);
}


// call before full fill areas are multiplied
void Layer::makeSkinPolygons() 
{
  if (skins<2) return;
  skinFullFillPolygons = fullFillPolygons;
  fullFillPolygons.clear();
  // vector<Poly> fp = fullFillPolygons;
  // //double dist = thickness/skins;
  // for (uint i=0; i<fullFillPolygons.size(); i++){
  //   //   skinFullFillPolygons.push_back(fp[i]);
  //   fullFillPolygons[i].clear();
  // //   //fp.erase(fp.begin()+i);
  // //   //i--;
  //  }
}

// add bridge polys and subtract them from normal and full fill polys 
// each given ExPoly is a single bridge with its holes
void Layer::addBridgePolygons(const vector<ExPoly> newexpolys) 
{
  // clip against normal fill and make these areas into bridges:
  uint num_bridges = newexpolys.size();
  if (num_bridges==0) return;
  Clipping clipp; // making bridge
  clipp.clear();
  bridgePolygons.clear();
  for (uint i=0; i < num_bridges; i++){
    vector<Poly> newpolys = Clipping::getPolys(newexpolys[i]);
    clipp.clear();
    clipp.addPolys(fillPolygons,subject); 
    clipp.addPolys(newpolys, clip);
    vector<ExPoly> exbridges = clipp.ext_intersect();
    bridgePolygons.insert(bridgePolygons.end(),exbridges.begin(),exbridges.end());
  }
  // subtract from normal fill to get new normal fills:
  clipp.clear();
  clipp.addPolys(fillPolygons,subject); 
  clipp.addPolys(newexpolys, clip);
  setNormalFillPolygons(clipp.subtract());
  mergeFullPolygons(true);
}

void Layer::addFullPolygons(const vector<ExPoly> newpolys, bool decor)
{
  addFullPolygons(Clipping::getPolys(newpolys),decor);
}

// add full fill and subtract them from normal fill polys 
void Layer::addFullPolygons(const vector<Poly> newpolys, bool decor) 
{
  if (newpolys.size()==0) return;
  Clipping clipp;
  clipp.clear();
  // full fill only where already normal fill
  clipp.addPolys(fillPolygons,subject); 
  clipp.addPolys(newpolys,clip);
  vector<Poly> inter = clipp.intersect();
  if (decor)//  && LayerNo != 0) // no decor on base layers
    decorPolygons.insert(decorPolygons.end(),inter.begin(),inter.end());
  else
    fullFillPolygons.insert(fullFillPolygons.end(),inter.begin(),inter.end());
  //mergeFullPolygons(false); // done separately
}

void cleanup(vector<Poly> &polys, double error)
{
  for (uint i = 0; i<polys.size(); i++)
    polys[i].cleanup(error);
}

void Layer::mergeFullPolygons(bool bridge) 
{
  if (bridge) {
    // setBridgePolygons(Clipping::getMerged(bridgePolygons, thickness));
    // clipp.addPolys(bridgePolygons,clip);
  } else {
    setFullFillPolygons(Clipping::getMerged(fullFillPolygons, thickness));
    cleanup(fullFillPolygons, thickness/CLEANFACTOR);
    //subtract from normal fills
    Clipping clipp;
    cleanup(fillPolygons, thickness/CLEANFACTOR);
    clipp.addPolys(fillPolygons,subject);  
    clipp.addPolys(fullFillPolygons,clip);
    vector<Poly> normals = clipp.subtractMerged();
    setNormalFillPolygons(normals);
  }
}
void Layer::mergeSupportPolygons() 
{
  setSupportPolygons(Clipping::getMerged(GetSupportPolygons()));
}

vector<Poly> Layer::GetInnerShell() const
{
  // if (fillPolygons.size()>0) return fillPolygons;
  // // no offset
  if (shellPolygons.size()>0) return shellPolygons.back();
  // no shells:
  if (skinPolygons.size()>0) return skinPolygons;
  // no skins
  return polygons;
}
vector<Poly> Layer::GetOuterShell() const
{
  if (skinPolygons.size()>0) return skinPolygons;
  // no skins
  if (shellPolygons.size()>0) return shellPolygons.front();
  // no shells:
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

void Layer::setFullFillPolygons(const vector<Poly> polys)
{
  clearpolys(fullFillPolygons);
  fullFillPolygons = polys;
  for (uint i=0; i<fullFillPolygons.size();i++)
    fullFillPolygons[i].setZ(Z);
}
void Layer::setBridgePolygons(const vector<ExPoly> expolys)
{
  uint count = expolys.size();
  // vector<Poly> polygroups;
  // vector<bool> done; done.resize(count);
  // for (uint i=0; i<count;i++) done[i] = false;
  // for (uint i=0; i<count;i++) {
  //   if (!done[i]){
  //     vector<Poly> group;
  //     group.push_back(polys[i]);
  //     done[i] = true;
  //   }
  //   for (uint j=i+1; j<polys.size();j++){
  //     if (!done[j]){
  // 	if (polys[.polyInside(polys[j]){
  // 	    done[j] = true;
  // 	  g
  //   }
  // }
  clearpolys(bridgePolygons);
  bridgePolygons = expolys;
  for (uint i=0; i<count; i++) { 
    bridgePolygons[i].outer.setZ(Z);
    for (uint h = 0; h < bridgePolygons[i].holes.size(); h++) 
      bridgePolygons[i].holes[h].setZ(Z);
  }
}
void Layer::setBridgeAngles(const vector<double> angles)
{
  bridge_angles=angles; // .insert(bridge_angles.begin(),angles.begin(),angles.end());
}

void Layer::setSupportPolygons(const vector<Poly> polys)
{
  clearpolys(supportPolygons);
  supportPolygons = polys;
  for (uint i=0; i<supportPolygons.size(); i++) {
    supportPolygons[i].cleanup(thickness/CLEANFACTOR);
    vector<Vector2d> minmax = supportPolygons[i].getMinMax();
    Min.x() = min(minmax[0].x(),Min.x());
    Min.y() = min(minmax[0].y(),Min.y());
    Max.x() = max(minmax[1].x(),Max.x());
    Max.y() = max(minmax[1].y(),Max.y());
  }
}

void Layer::setSkirtPolygon(const Poly poly)
{
  skirtPolygon = poly;
  skirtPolygon.setZ(Z);
}


void Layer::MakeShells(uint shellcount, double extrudedWidth, double shelloffset,
		       bool makeskirt, double skirtdistance, double infilloverlap)
{
  double distance = 0.5 * extrudedWidth;
  double cleandist = min(distance/CLEANFACTOR, thickness/CLEANFACTOR);
  vector<Poly> shrinked = Clipping::getOffset(polygons,-distance-shelloffset);
  for (uint i = 0; i<shrinked.size(); i++)  
    shrinked[i].cleanup(cleandist);
  //vector<Poly> shrinked = Clipping::getShrinkedCapped(polygons,distance);
  // outmost shells
  if (shellcount > 0) {
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
  }
  // the filling polygon
  fillPolygons = Clipping::getOffset(shrinked,-(1.-infilloverlap)*distance);
  for (uint i = 0; i<fillPolygons.size(); i++)  
    fillPolygons[i].cleanup(cleandist);
  //fillPolygons = Clipping::getShrinkedCapped(shrinked,distance);
  //cerr << LayerNo << " > " << fillPolygons.size()<< endl;
  calcConvexHull();
  if (makeskirt) {
    MakeSkirt(skirtdistance); // skirt distance = 3 * shell distance
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
  vector<Poly> skp = Clipping::getOffset(hullPolygon, distance, jround);
  if (skp.size()>0){
    skirtPolygon = skp.front();
    skirtPolygon.cleanup(thickness);
  }
}

// 2D cross product of OA and OB vectors, i.e. z-component of their 3D cross product.
// Returns a positive value, if OAB makes a counter-clockwise turn,
// negative for clockwise turn, and zero if the points are collinear.
double cross(const Vector2d &O, const Vector2d &A, const Vector2d &B)
{
  return (A.x() - O.x()) * (B.y() - O.y()) - (A.y() - O.y()) * (B.x() - O.x());
}
struct sortable_point {
  Vector2d v;
  bool operator <(const sortable_point &p) const {
    return (v.x() < p.v.x()) || ((v.x() == p.v.x()) && (v.y() < p.v.y()));
  }
};

// calc convex hull and Min and Max of layer
// Monotone chain algo
// http://en.wikibooks.org/wiki/Algorithm_Implementation/Geometry/Convex_hull/Monotone_chain
void Layer::calcConvexHull() 
{
  hullPolygon = convexHull2D(polygons);
  hullPolygon.setZ(Z);
  setMinMax(hullPolygon);
}

void Layer::setMinMax(const vector<Poly> polys) 
{
  Min = Vector2d( INFTY, INFTY);
  Max = Vector2d(-INFTY, -INFTY);
  for (uint i = 0; i< polys.size(); i++) {
    vector<Vector2d> minmax = polys[i].getMinMax();
    Min.x() = min(Min.x(), minmax[0].x()); Min.y() = min(Min.y(), minmax[0].y());
    Max.x() = max(Max.x(), minmax[1].x()); Max.y() = max(Max.y(), minmax[1].y());
  }
}
void Layer::setMinMax(const Poly poly) 
{
  vector<Vector2d> minmax = poly.getMinMax();
  Min = minmax[0];
  Max = minmax[1];
}

// Convert to GCode
void Layer::MakeGcode(Vector3d &lastPos, //GCodeState &state,
		      vector<Command> &commands,
		      double offsetZ,
		      const Settings::SlicingSettings &slicing,
		      const Settings::HardwareSettings &hardware) const
{

  double linewidthratio = hardware.ExtrudedMaterialWidthRatio;
  double linewidth = thickness*linewidthratio;
  double cornerradius = linewidth*slicing.CornerRadius;

  Vector2d startPoint(lastPos.x(),lastPos.y());

  double extrf = hardware.GetExtrudeFactor(thickness);

  vector<PLine3> lines3;
  Printlines printlines(offsetZ);

  vector<PLine> lines;

  vector<Poly> polys; // intermediate collection

  // polys to keep line movements inside
  vector<Poly> clippolys = GetOuterShell();

  // 1. Skins, all but last, because they are the lowest lines, below layer Z
  if (skins > 1) {
    for(uint s = 1; s <= skins; s++) { // z offset from bottom to top
      double skin_z = Z - thickness + (s)*thickness/skins;
      if ( skin_z < 0 )
	continue;
      //cerr << s << " -- " << Z << " -- "<<skin_z <<" -- " << thickness <<  endl;
      // outlines:
      //if (s<skins)
      for(size_t p = 0; p < skinPolygons.size(); p++) {
	Poly sp(skinPolygons[p], skin_z);
	polys.push_back(sp);
      }
      // skin infill polys:
      if (skinFullInfills[s-1])
	polys.insert(polys.end(),
		     skinFullInfills[s-1]->infillpolys.begin(),
		     skinFullInfills[s-1]->infillpolys.end());
      // add all of this skin layer to lines
      printlines.makeLines(polys, (s==1), //displace at first skin
			   slicing, hardware,
			   startPoint, lines, hardware.MaxShellSpeed);
      if (s < skins) { // not on the last layer, this handle with all other lines
	// have to get all these separately because z changes
	printlines.clipMovements(&clippolys, lines, linewidth/2.);
	printlines.optimize(hardware, slicing, slicing.MinLayertime/skins, lines);
	printlines.getLines(lines, lines3);
	lines.clear();
      }
      polys.clear();
    }
  } // last skin layer now still in lines

  // 2. Skirt
  vector <Poly> skirts(1); skirts[0] = skirtPolygon;
  printlines.makeLines(skirts, false,
		       slicing, hardware,
		       startPoint, lines, hardware.MaxShellSpeed);

  // 3. Support
  if (supportInfill)
    printlines.makeLines(supportInfill->infillpolys, false,
			 slicing, hardware,
			 startPoint, lines);

  // 4. all other polygons:  

  // TODO: calculate extrusionfactor
  // for rectangle vs. ellipsis (inner shell vs. outer shell)

  //  Shells
  for(int p=shellPolygons.size()-1; p>=0; p--) // inner to outer
    polys.insert(polys.end(), shellPolygons[p].begin(),shellPolygons[p].end());
  // printlines.makeLines(shellPolygons[p], (p==(int)(shellPolygons.size())-1), 
  printlines.makeLines(polys, true, //displace at beginning
		       slicing, hardware,
		       startPoint, lines, hardware.MaxShellSpeed);
  // TODO:  sort inner to outer in printlines
  if (slicing.UseArcs && slicing.RoundCorners) 
    printlines.roundCorners(cornerradius,lines);
  polys.clear();

  //  Infill
  if (normalInfill)
    polys.insert(polys.end(),
		 normalInfill->infillpolys.begin(), normalInfill->infillpolys.end());
  if (fullInfill)
    polys.insert(polys.end(),
		 fullInfill->infillpolys.begin(), fullInfill->infillpolys.end());
  if (decorInfill)
    polys.insert(polys.end(),
		 decorInfill->infillpolys.begin(), decorInfill->infillpolys.end());
  for (uint b=0; b < bridgeInfills.size(); b++)
    if (bridgeInfills[b])
      polys.insert(polys.end(),
		   bridgeInfills[b]->infillpolys.begin(),
		   bridgeInfills[b]->infillpolys.end());
  
  printlines.makeLines(polys, true, //displace at beginning
		       slicing, hardware, 
		       startPoint, lines);
  if (slicing.UseArcs && slicing.RoundCorners) 
    printlines.roundCorners(cornerradius,lines);
  polys.clear();

  // FINISH

  Command comment(LAYERCHANGE, LayerNo);
  comment.comment += info();
  commands.push_back(comment);

  float speedfactor = 1;
  if ((guint)LayerNo < slicing.FirstLayersNum)
    speedfactor = slicing.FirstLayersSpeed;

  printlines.clipMovements(&clippolys, lines, linewidth/2.);
  printlines.optimize(hardware, slicing, slicing.MinLayertime, lines);
  printlines.setSpeedFactor(speedfactor, lines);
  double slowdownfactor = printlines.getSlowdownFactor();

  if (slicing.FanControl) {
    int fanspeed = slicing.MinFanSpeed;
    if (slowdownfactor < 1 && slowdownfactor > 0) { 
      double fanfactor = 1-slowdownfactor; 
      fanspeed += 
	int(fanfactor * (slicing.MaxFanSpeed-slicing.MinFanSpeed));
      fanspeed = CLAMP(fanspeed, slicing.MinFanSpeed, slicing.MaxFanSpeed);
      //cerr << slowdownfactor << " - " << fanfactor << " - " << fanspeed << " - " << endl;
    } 
    Command fancommand(FANON, fanspeed);
    commands.push_back(fancommand);
  }

  printlines.getLines(lines, lines3);


  double minspeed = hardware.MinPrintSpeedXY,
    maxspeed = hardware.MaxPrintSpeedXY,
    movespeed = hardware.MoveSpeed,
    emax = hardware.EMaxSpeed;
  
  // push all lines to gcode
  // start3 = state.LastPosition();
  for (uint i = 0; i < lines3.size(); i++) {
    lines3[i].getCommands(lastPos, commands, extrf, minspeed, maxspeed, movespeed, emax);
    //state.AppendCommands(cc, slicing.RelativeEcode);
  }
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
  if (bridgeInfills.size()>0)
    ostr <<", bridges "<<bridgeInfills.size() ;
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
	       const float *rgb, float a)
{
  glColor4f(rgb[0],rgb[1],rgb[2],a);
  glLineWidth(linewidth);
  glPointSize(pointsize);
  poly.draw(gl_type);
}
void draw_polys(vector <Poly> polys, int gl_type, int linewidth, int pointsize,
		const float *rgb, float a)
{
  glColor4f(rgb[0],rgb[1],rgb[2], a);
  glLineWidth(linewidth);
  glPointSize(pointsize);
  for(size_t p=0; p<polys.size();p++)
    polys[p].draw(gl_type);
}
void draw_polys(vector< vector <Poly> > polys, int gl_type, int linewidth, int pointsize,
		const float *rgb, float a)
{
  for(size_t p=0; p<polys.size();p++)
    draw_polys(polys[p], gl_type, linewidth, pointsize, rgb,a);
}
void draw_polys(vector <ExPoly> expolys, int gl_type, int linewidth, int pointsize,
		const float *rgb, float a)
{
  for(size_t p=0; p < expolys.size();p++) {
    draw_poly(expolys[p].outer,  gl_type, linewidth, pointsize, rgb, a);
    draw_polys(expolys[p].holes, gl_type, linewidth, pointsize, rgb, a);
  }
}

float const GREEN[] = {0.1, 1, 0.1};
float const BLUEGREEN[] = {0.1, 0.9, 0.7};
float const BLUE2[] = {0.5,0.5,1.0};
float const RED[] = {1, 0, 0};
float const RED2[] = {0.8,0.5,0.5};
float const RED3[] = {0.8,0.3,0.1};
float const ORANGE[] = {1, 0, 0};
float const YELLOW[] = {1, 1, 0};
float const YELLOW2[] = {1, 1, 0.2};
float const WHITE[] = {1, 1, 1};
float const GREY[] = {0.5,0.5,0.5};

void Layer::Draw(bool DrawVertexNumbers, bool DrawLineNumbers, 
		 bool DrawOutlineNumbers, bool DrawCPLineNumbers, 
		 bool DrawCPVertexNumbers, bool DisplayInfill, 
		 bool DebugInfill) 
{
  draw_polys(polygons, GL_LINE_LOOP, 1, 3, RED, 1);
  draw_polys(polygons, GL_POINTS,    1, 3, RED, 1);

  if(DrawOutlineNumbers)
    for(size_t p=0; p<polygons.size();p++)
      {
	ostringstream oss;
	oss << p;
	Vector2d center = polygons[p].getCenter();
	drawString(Vector3d(center.x(), center.y(), Z), oss.str());
      }

  draw_poly(hullPolygon,    GL_LINE_LOOP, 3, 3, ORANGE,  0.5);
  draw_poly(skirtPolygon,   GL_LINE_LOOP, 3, 3, YELLOW,  1);
  draw_polys(shellPolygons, GL_LINE_LOOP, 1, 3, YELLOW2, 1);

  glColor4f(0.5,0.9,1,1);
  glLineWidth(1);
  double zs = Z;
  for(size_t s=0;s<skins;s++){
    for(size_t p=0;p<skinPolygons.size();p++)
      skinPolygons[p].draw(GL_LINE_LOOP,zs);
    zs-=thickness/skins;
  }

  draw_polys(fillPolygons,         GL_LINE_LOOP, 1, 3, WHITE, 0.6);
  draw_polys(supportPolygons,      GL_LINE_LOOP, 3, 3, BLUE2, 1);
  draw_polys(bridgePolygons,       GL_LINE_LOOP, 3, 3, RED2,  0.7);
  draw_polys(bridgePillars,        GL_LINE_LOOP, 3, 3, YELLOW, 0.7);
  draw_polys(fullFillPolygons,     GL_LINE_LOOP, 1, 3, GREY,  0.6);
  draw_polys(decorPolygons,        GL_LINE_LOOP, 1, 3, GREY,  1);
  draw_polys(skinFullFillPolygons, GL_LINE_LOOP, 1, 3, GREY,  0.6);
  if(DisplayInfill)
    {
      if (normalInfill)
	draw_polys(normalInfill->infillpolys, GL_LINE_LOOP, 1, 3, 
		   (normalInfill->cached?BLUEGREEN:GREEN), 1);
      if(DebugInfill && normalInfill->cached)
	draw_polys(normalInfill->getCachedPattern(Z), GL_LINE_LOOP, 1, 3, 
		   ORANGE, 0.5);
      
      if (fullInfill)
	draw_polys(fullInfill->infillpolys, GL_LINE_LOOP, 1, 3, 
		   (fullInfill->cached?BLUEGREEN:GREEN), 0.8);
      if(DebugInfill && fullInfill->cached)
	draw_polys(fullInfill->getCachedPattern(Z), GL_LINE_LOOP, 1, 3, 
		   ORANGE, 0.5);
      if (decorInfill)
	draw_polys(decorInfill->infillpolys, GL_LINE_LOOP, 1, 3, 
		   (decorInfill->cached?BLUEGREEN:GREEN), 0.8);
      if(DebugInfill && decorInfill->cached)
	draw_polys(decorInfill->getCachedPattern(Z), GL_LINE_LOOP, 1, 3, 
		   ORANGE, 0.5);
      uint bridgecount = bridgeInfills.size();
      if (bridgecount>0)
	for (uint i = 0; i<bridgecount; i++)
	  draw_polys(bridgeInfills[i]->infillpolys, GL_LINE_LOOP, 2, 3, 
		     RED3,0.9);
      if (supportInfill)
	draw_polys(supportInfill->infillpolys, GL_LINE_LOOP, 1, 3, 
		   (supportInfill->cached?BLUEGREEN:GREEN), 0.8);
      if(DebugInfill && supportInfill->cached)
	draw_polys(supportInfill->getCachedPattern(Z), GL_LINE_LOOP, 1, 3, 
		   ORANGE, 0.5);
      for(size_t s=0;s<skinFullInfills.size();s++) 
	draw_polys(skinFullInfills[s]->infillpolys, GL_LINE_LOOP, 3, 3, 
		   (skinFullInfills[s]->cached?BLUEGREEN:GREEN), 0.6);
    }
  //draw_polys(GetInnerShell(), GL_LINE_LOOP, 2, 3, WHITE,  1);
  glLineWidth(1);  
  if(DrawCPVertexNumbers)
    for(size_t p=0; p<polygons.size();p++)
      polygons[p].drawVertexNumbers();
  
  if(DrawCPLineNumbers)
    for(size_t p=0; p<polygons.size();p++)
      polygons[p].drawLineNumbers();
}

void Layer::DrawMeasures(Vector2d point)
{
  Vector2d x0(Min.x()-10, point.y());
  Vector2d x1(Max.x()+10, point.y());
  Vector2d y0(point.x(), Min.y()-10);
  Vector2d y1(point.x(), Max.y()+10);

  // cut axes with layer polygons
  vector<Intersection> xint, yint;
  Intersection hit;
  hit.p = Vector2d(Min.x(),point.y()); hit.d = 10;
  xint.push_back(hit);
  hit.p = Vector2d(Max.x(),point.y()); hit.d = Max.x()-Min.x()+10;
  xint.push_back(hit);
  hit.p = Vector2d(point.x(),Min.y()); hit.d = 10;
  yint.push_back(hit);
  hit.p = Vector2d(point.x(),Max.y()); hit.d = Max.y()-Min.y()+10;
  yint.push_back(hit);

  for(size_t p=0; p<polygons.size();p++) {
    vector<Intersection> lint = polygons[p].lineIntersections(x0,x1,0.1);
    xint.insert(xint.end(),lint.begin(),lint.end());
    lint = polygons[p].lineIntersections(y0,y1,0.1);
    yint.insert(yint.end(),lint.begin(),lint.end());
  }
  //cerr << xint.size() << " - "<< xint.size() << endl;
  std::sort(xint.begin(),xint.end());
  std::sort(yint.begin(),yint.end());

  glColor4f(1.,1.,1.,1.);
  glBegin(GL_LINES);
  // draw lines
  glVertex3d(Min.x(), x0.y(), Z);
  glVertex3d(Max.x(), x1.y(), Z);
  glVertex3d(y0.x(), Min.y(), Z);
  glVertex3d(y1.x(), Max.y(), Z);
  // draw ticks
  double ticksize=2;
  for(guint i = 0; i<xint.size(); i++) {
    glVertex3d(xint[i].p.x(), xint[i].p.y()-ticksize, Z);    
    glVertex3d(xint[i].p.x(), xint[i].p.y()+ticksize, Z);    
  }
  for(guint i = 0; i<yint.size(); i++) {
    glVertex3d(yint[i].p.x()-ticksize, yint[i].p.y(), Z);    
    glVertex3d(yint[i].p.x()+ticksize, yint[i].p.y(), Z);    
  }
  // draw BBox
  glVertex3d(Min.x(), Min.y(), Z);
  glVertex3d(Max.x(), Min.y(), Z);
  glVertex3d(Max.x(), Min.y(), Z);
  glVertex3d(Max.x(), Max.y(), Z);
  glVertex3d(Max.x(), Max.y(), Z);
  glVertex3d(Min.x(), Max.y(), Z);
  glVertex3d(Min.x(), Max.y(), Z);
  glVertex3d(Min.x(), Min.y(), Z);
  glEnd();
  // draw numbers
  ostringstream val;
  val.precision(1);
  for(guint i = 1; i<xint.size(); i++) {
    val.str("");
    double v = xint[i].p.x()-xint[i-1].p.x();
    val << fixed << v;
    drawString(Vector3d((xint[i].p.x()+xint[i-1].p.x())/2.,xint[i].p.y()+1,Z),val.str());
  }
  for(guint i = 1; i<yint.size(); i++) {
    val.str("");
    double v = yint[i].p.y()-yint[i-1].p.y();
    val << fixed << v;
    drawString(Vector3d(yint[i].p.x()+1,(yint[i].p.y()+yint[i-1].p.y())/2.,Z),val.str());
  }

}
