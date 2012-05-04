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
#define CLEANFACTOR 7

Layer::Layer(Layer * prevlayer, int layerno, double thick, uint skins) 
  : LayerNo(layerno), thickness(thick), previous(prevlayer), skins(skins)
{
  normalInfill = NULL;
  fullInfill = NULL;
  supportInfill = NULL;
  decorInfill = NULL;
  thinInfill = NULL;
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
  delete thinInfill;
  skinFullInfills.clear();
  clearpolys(polygons);
  clearpolys(shellPolygons);
  clearpolys(fillPolygons);
  clearpolys(thinPolygons);
  clearpolys(fullFillPolygons);
  clearpolys(bridgePolygons);
  clearpolys(bridgePillars);
  bridge_angles.clear();
  bridgeInfills.clear();
  clearpolys(decorPolygons);
  clearpolys(supportPolygons);
  clearpolys(toSupportPolygons);
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


void Layer::SetPolygons(vector<Poly> &polys) {
  this->polygons = polys;
  for(uint i=0;i<polygons.size();i++){
    polygons[i].setZ(Z); 
  }
}

// //not used
// void Layer::SetPolygons(const Matrix4d &T, const Shape &shape, 
// 			double z) {
//   cerr << "Layer::SetPolygons" << endl;
//   double offsetZ = Z;
//   bool polys_ok=false;
//   while (!polys_ok) {
//     double max_grad;
//     polys_ok = shape.getPolygonsAtZ(T, offsetZ, polygons, max_grad);
//     offsetZ+=thickness/10.;
//   }
//   for(uint i=0;i<polygons.size();i++){
//     polygons[i].setZ(Z); 
//   }
// }


bool Layer::pointInPolygons(const Vector2d &p) const 
{ 
  for (uint i = 0; i<polygons.size(); i++) 
    if (!polygons[i].isHole() && polygons[i].vertexInside(p)) return true;
  return false;
}


int Layer::addShape(const Matrix4d &T, const Shape &shape, double z, 
		    double &max_gradient, double max_supportangle)
{
  double hackedZ = z;
  bool polys_ok = false;
  vector<Poly> polys;
  int num_polys=-1;
  // try to slice until polygons can be made, otherwise hack z
  while (!polys_ok && hackedZ < z+thickness) {
    polys.clear();
    polys_ok = shape.getPolygonsAtZ(T, hackedZ,  // slice shape at hackedZ
				    polys, max_gradient, 
				    toSupportPolygons, max_supportangle,
				    thickness);
    if (polys_ok) {
      num_polys = polys.size();
      addPolygons(polys);
    } else {
      num_polys=-1;
      cerr << "hacked Z " << z << " -> " << hackedZ << endl;
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

void Layer::addPolygons(vector<Poly> &polys)
{
  for(uint i=0;i<polys.size();i++){
    polys[i].setZ(Z); 
  }
  polygons.insert(polygons.end(),polys.begin(),polys.end());
}


void Layer::calcBridgeAngles(const Layer *layerbelow) {
  bridge_angles.resize(bridgePolygons.size());
  Clipping clipp;
  vector<Poly> polysbelow = *(layerbelow->GetInnerShell());//clipp.getOffset(polygons,3*thickness);
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
vector <double> Layer::getBridgeRotations(const vector<Poly> &polys) const{
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
void Layer::CalcRaftInfill (const vector<Poly> &polys, 
			    double extrusionfactor, double infilldistance,
			    double rotation)
{
  setMinMax(polys);
  normalInfill = new Infill(this, extrusionfactor);
  normalInfill->setName("Raft");
  normalInfill->addPolys(Z, polys, RaftInfill, 
			 infilldistance, infilldistance, rotation);
}

void Layer::CalcInfill (const Settings &settings)
{
  // inFill distances in real mm:
  // for full polys/layers:
  double fullInfillDistance=0;
  double infillDistance=0; // normal fill
  double altInfillDistance=0;
  double normalInfilldist=0;
  bool shellOnly = !settings.Slicing.DoInfill;
  fullInfillDistance = settings.GetInfillDistance(thickness, 100);
  if (settings.Slicing.InfillPercent == 0) 
    shellOnly = true;
  else 
    infillDistance = settings.GetInfillDistance(thickness,
						settings.Slicing.InfillPercent);
  if (settings.Slicing.AltInfillPercent != 0) 
    altInfillDistance = settings.GetInfillDistance(thickness,
						   settings.Slicing.AltInfillPercent);
  if (settings.Slicing.AltInfillLayers != 0 
      && LayerNo % settings.Slicing.AltInfillLayers == 0) 
    normalInfilldist = altInfillDistance;
  else
    normalInfilldist = infillDistance;
  if (LayerNo < (int)settings.Slicing.FirstLayersNum) {
    double first_infdist = 
      fullInfillDistance * (1+(double)settings.Slicing.FirstLayersInfillDist);
    normalInfilldist   = max(normalInfilldist,   first_infdist);
    fullInfillDistance = max(fullInfillDistance, first_infdist);
  }
  // relative extrusion for skins:
  double skinfillextrf = settings.Slicing.FullFillExtrusion/skins/skins; 
  normalInfill = new Infill(this,settings.Slicing.NormalFillExtrusion);
  normalInfill->setName("normal");
  fullInfill = new Infill(this,settings.Slicing.FullFillExtrusion);
  fullInfill->setName("full");
  skinFullInfills.clear();
  supportInfill = new Infill(this,settings.Slicing.SupportExtrusion);
  supportInfill->setName("support");
  decorInfill = new Infill(this,1.);
  decorInfill->setName("decor");
  thinInfill = new Infill(this,1.);
  thinInfill->setName("thin");

  double rot = (settings.Slicing.InfillRotation 
		+ (double)LayerNo*settings.Slicing.InfillRotationPrLayer)/180.0*M_PI;
  if (!shellOnly)
    normalInfill->addPolys(Z, fillPolygons, (InfillType)settings.Slicing.NormalFilltype, 
			   normalInfilldist, fullInfillDistance, rot);
  
  fullInfill->addPolys(Z, fullFillPolygons, (InfillType)settings.Slicing.FullFilltype,
		       fullInfillDistance, fullInfillDistance, rot);
  
  decorInfill->addPolys(Z, decorPolygons, (InfillType)settings.Slicing.DecorFilltype,
			settings.Slicing.DecorInfillDistance,
			settings.Slicing.DecorInfillDistance,
			settings.Slicing.DecorInfillRotation/180.0*M_PI);
  
  assert(bridge_angles.size() >= bridgePolygons.size());
  bridgeInfills.resize(bridgePolygons.size());
  for (uint b=0; b < bridgePolygons.size(); b++){  
    bridgeInfills[b] = new Infill(this, settings.Slicing.BridgeExtrusion);
    bridgeInfills[b]->addPoly(Z, bridgePolygons[b], BridgeInfill, 
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
      inf->addPolys(sz, skinFullFillPolygons, (InfillType)settings.Slicing.FullFilltype,
		    skindistance, skindistance, drot);
      skinFullInfills.push_back(inf);
    }
  }
  supportInfill->addPolys(Z, supportPolygons, (InfillType)settings.Slicing.SupportFilltype,
			  settings.Slicing.SupportInfillDistance, 
			  settings.Slicing.SupportInfillDistance, 0);

  thinInfill->addPolys(Z, thinPolygons, ThinInfill,
		       fullInfillDistance, fullInfillDistance, 0);
}


// call before full fill areas are multiplied
void Layer::makeSkinPolygons() 
{
  if (skins<2) return;
  skinFullFillPolygons = fullFillPolygons;
  fullFillPolygons.clear();
}

// add bridge polys and subtract them from normal and full fill polys 
// each given ExPoly is a single bridge with its holes
void Layer::addBridgePolygons(const vector<ExPoly> &newexpolys) 
{
  // clip against normal fill and make these areas into bridges:
  Clipping clipp;
  uint num_bridges = newexpolys.size();
  if (num_bridges==0) return;
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
  // subtract from normal fill
  clipp.clear();
  clipp.addPolys(fillPolygons,subject); 
  clipp.addPolys(newexpolys, clip);
  setNormalFillPolygons(clipp.subtract());
}

void Layer::addFullPolygons(const vector<ExPoly> &newpolys, bool decor)
{
  addFullPolygons(Clipping::getPolys(newpolys),decor);
}

// add full fill and subtract them from normal fill polys 
void Layer::addFullPolygons(const vector<Poly> &newpolys, bool decor) 
{
  if (newpolys.size()==0) return;
  Clipping clipp;
  clipp.clear();
  // full fill only where already normal fill
  clipp.addPolys(fillPolygons,subject); 
  if (decor) clipp.addPolys(fullFillPolygons,subject); 
  clipp.addPolys(newpolys,clip);
  clipp.setZ(Z);
  vector<Poly> inter = clipp.intersect();
  vector<Poly> normals = clipp.subtractMerged(thickness/2.);
  if (decor) {//  && LayerNo != 0) // no decor on base layers
    decorPolygons.insert(decorPolygons.end(), inter.begin(), inter.end());
    Clipping clipp;
    clipp.addPolys(fullFillPolygons,subject);
    clipp.addPolys(inter,clip);
    clipp.setZ(Z);
    setFullFillPolygons(clipp.subtract());
  }
  else {
    fullFillPolygons.insert(fullFillPolygons.end(),inter.begin(),inter.end());
  }

  setNormalFillPolygons(normals);
  //  mergeFullPolygons(false); // done separately
}

void cleanup(vector<Poly> &polys, double error)
{
  for (uint i = 0; i<polys.size(); i++)
    polys[i].cleanup(error);
}

void Layer::mergeFullPolygons(bool bridge) 
{
  // if (bridge) {
  //   // setBridgePolygons(Clipping::getMerged(bridgePolygons, thickness));
  //   // clipp.addPolys(bridgePolygons,clip);
  // } else {
  //subtract decor from full 
  Clipping clipp;
  // clipp.addPolys(fullFillPolygons,subject);
  // clipp.addPolys(decorPolygons,clip);
  // setFullFillPolygons(clipp.subtract());

  setFullFillPolygons(Clipping::getMerged(fullFillPolygons, thickness));
  cleanup(fullFillPolygons, thickness/CLEANFACTOR);
  //subtract from normal fills
  clipp.clear();
  cleanup(fillPolygons, thickness/CLEANFACTOR);
  clipp.addPolys(fillPolygons,subject);  
  clipp.addPolys(fullFillPolygons,clip);
  clipp.addPolys(decorPolygons,clip);
  vector<Poly> normals = clipp.subtractMerged();
  setNormalFillPolygons(normals);
  // }
}
void Layer::mergeSupportPolygons()
{
  setSupportPolygons(Clipping::getMerged(GetSupportPolygons()));
}

const vector<Poly> * Layer::GetInnerShell() const
{
  // if (fillPolygons.size()>0) return fillPolygons;
  // // no offset
  if (shellPolygons.size()>0) return &(shellPolygons.back());
  // no shells:
  if (skinPolygons.size()>0) return &skinPolygons;
  // no skins
  return &polygons;
}
const vector<Poly> * Layer::GetOuterShell() const
{
  if (skinPolygons.size()>0) return &skinPolygons;
  // no skins
  if (shellPolygons.size()>0) return &(shellPolygons.front());
  // no shells:
  if (fillPolygons.size()>0) return &fillPolygons;
  // no offset
  return &polygons;
}

vector<ExPoly> Layer::GetExPolygons() const
{
  return Clipping::getExPolys(polygons,0,0);
}

// circular numbering
vector<Poly> Layer::GetShellPolygonsCirc(int number) const
{
  number = (shellPolygons.size() +  number) % shellPolygons.size();
  return shellPolygons[number];
}

void Layer::setNormalFillPolygons(const vector<Poly> &polys)
{
  clearpolys(fillPolygons);
  fillPolygons = polys;
  for (uint i=0; i<fillPolygons.size();i++)
    fillPolygons[i].setZ(Z);
}

void Layer::setFullFillPolygons(const vector<Poly> &polys)
{
  clearpolys(fullFillPolygons);
  fullFillPolygons = polys;
  for (uint i=0; i<fullFillPolygons.size();i++)
    fullFillPolygons[i].setZ(Z);
}
void Layer::setBridgePolygons(const vector<ExPoly> &expolys)
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
void Layer::setBridgeAngles(const vector<double> &angles)
{
  bridge_angles=angles; // .insert(bridge_angles.begin(),angles.begin(),angles.end());
}

void Layer::setSupportPolygons(const vector<Poly> &polys)
{
  clearpolys(supportPolygons);
  supportPolygons = polys;
  const double minarea = 10*thickness*thickness;
  for (int i = supportPolygons.size()-1; i >= 0; i--) {
    supportPolygons[i].cleanup(thickness/CLEANFACTOR);
    if (abs(Clipping::Area(supportPolygons[i])) < minarea) {
      supportPolygons.erase(supportPolygons.begin() + i);
      continue;
    }
    vector<Vector2d> minmax = supportPolygons[i].getMinMax();
    Min.x() = min(minmax[0].x(),Min.x());
    Min.y() = min(minmax[0].y(),Min.y());
    Max.x() = max(minmax[1].x(),Max.x());
    Max.y() = max(minmax[1].y(),Max.y());
  }
}

void Layer::setSkirtPolygon(const Poly &poly)
{
  skirtPolygon = poly;
  skirtPolygon.cleanup(thickness);
  skirtPolygon.setZ(Z);
}


void Layer::MakeShells(const Settings &settings)
{
  double extrudedWidth        = settings.Hardware.GetExtrudedMaterialWidth(thickness);
  double roundline_extrfactor = 
    settings.Hardware.RoundedLinewidthCorrection(extrudedWidth,thickness);
  double distance       = 0.5 * extrudedWidth;
  double cleandist      = min(distance/CLEANFACTOR, thickness/CLEANFACTOR);
  double shelloffset    = settings.Slicing.ShellOffset;
  uint shellcount       = settings.Slicing.ShellCount;
  double infilloverlap  = settings.Slicing.InfillOverlap;
  vector<Poly> shrinked = Clipping::getOffset(polygons,-distance-shelloffset);


  // find thin areas: expand again and see what's left, rest is thin
  Clipping clipp; 
  clipp.addPolys(polygons, subject);
  vector<Poly> shrexp = Clipping::getOffset(shrinked, (distance+shelloffset)*1.1);
  clipp.addPolys(shrexp, clip);
  thinPolygons = clipp.subtract();
  for (uint i = 0; i<thinPolygons.size(); i++)  
    thinPolygons[i].cleanup(cleandist);

  for (uint i = 0; i<shrinked.size(); i++)  
    shrinked[i].cleanup(cleandist);

  //vector<Poly> shrinked = Clipping::getShrinkedCapped(polygons,distance);
  // outmost shells
  if (shellcount > 0) {
    if (skins>1) { // either skins 
      for (uint i = 0; i<shrinked.size(); i++)  {
	shrinked[i].setExtrusionFactor(1./skins*roundline_extrfactor);
      }
      skinPolygons = shrinked; 
    } else {  // or normal shell
      clearpolys(shellPolygons);
      for (uint i = 0; i<shrinked.size(); i++)  {
	shrinked[i].setExtrusionFactor(roundline_extrfactor);
      }
      shellPolygons.push_back(shrinked); 
    }
    // inner shells
    distance = extrudedWidth;
    for (uint i = 1; i<shellcount; i++) // shrink from shell to shell
      {
	shrinked = Clipping::getOffset(shrinked,-distance);
	for (uint j = 0; j<shrinked.size(); j++) 
	  shrinked[j].cleanup(cleandist);
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
  vector<Poly> all;
  all.push_back(hullPolygon);
  all.insert(all.end(),supportPolygons.begin(),supportPolygons.end());
  Poly hull = convexHull2D(all);
  vector<Poly> skp = Clipping::getOffset(hull, distance, jround);
  if (skp.size()>0){
    skirtPolygon = skp.front();
    skirtPolygon.setZ(Z);
    skirtPolygon.cleanup(thickness);
  }
}

// 2D cross product of OA and OB vectors, i.e. z-component of their 3D cross product.
// Returns a positive value, if OAB makes a counter-clockwise turn,
// negative for clockwise turn, and zero if the points are collinear.
// double cross(const Vector2d &O, const Vector2d &A, const Vector2d &B)
// {
//   return (A.x() - O.x()) * (B.y() - O.y()) - (A.y() - O.y()) * (B.x() - O.x());
// }
// struct sortable_point {
//   Vector2d v;
//   bool operator <(const sortable_point &p) const {
//     return (v.x() < p.v.x()) || ((v.x() == p.v.x()) && (v.y() < p.v.y()));
//   }
// };

// calc convex hull and Min and Max of layer
// Monotone chain algo
// http://en.wikibooks.org/wiki/Algorithm_Implementation/Geometry/Convex_hull/Monotone_chain
void Layer::calcConvexHull() 
{
  hullPolygon = convexHull2D(polygons);
  hullPolygon.setZ(Z);
  setMinMax(hullPolygon);
}

bool Layer::setMinMax(const vector<Poly> &polys) 
{
  Vector2d NewMin = Vector2d( INFTY, INFTY);
  Vector2d NewMax = Vector2d(-INFTY, -INFTY);
  for (uint i = 0; i< polys.size(); i++) {
    vector<Vector2d> minmax = polys[i].getMinMax();
    NewMin.x() = min(NewMin.x(), minmax[0].x()); 
    NewMin.y() = min(NewMin.y(), minmax[0].y());
    NewMax.x() = max(NewMax.x(), minmax[1].x()); 
    NewMax.y() = max(NewMax.y(), minmax[1].y());
  }
  if (NewMin==Min && NewMax==Max) return false;
  Min = NewMin;
  Max = NewMax;
  return true;
}
bool Layer::setMinMax(const Poly &poly) 
{
  vector<Vector2d> minmax = poly.getMinMax();
  if (minmax[0]==Min && minmax[1]==Max) return false;
  Min = minmax[0];
  Max = minmax[1];
  return true;
}

// Convert to GCode
void Layer::MakeGcode(Vector3d &lastPos, //GCodeState &state,
		      vector<Command> &commands,
		      double offsetZ,
		      const Settings &settings) const
{

  const double linewidthratio = settings.Hardware.ExtrudedMaterialWidthRatio;
  const double linewidth      = thickness*linewidthratio;
  const double cornerradius   = linewidth*settings.Slicing.CornerRadius;

  const bool clipnearest      = settings.Slicing.MoveNearest;

  Vector2d startPoint(lastPos.x(),lastPos.y());

  const double extrf = settings.Hardware.GetExtrudeFactor(thickness);

  vector<PLine3> lines3;
  Printlines printlines(this, &settings, offsetZ);

  vector<PLine> lines;

  vector<Poly> polys; // intermediate collection

  // polys to keep line movements inside
  //const vector<Poly> * clippolys = &polygons; 
  const vector<Poly> * clippolys = GetOuterShell();

  // 1. Skins, all but last, because they are the lowest lines, below layer Z
  if (skins > 1) {
    for(uint s = 0; s < skins; s++) { 
      // z offset from bottom to top:
      double skin_z = Z - thickness + (s+1)*thickness/skins;
      if ( skin_z < 0 )
	continue;
      //cerr << s << " -- " << Z << " -- "<<skin_z <<" -- " << thickness <<  endl;

      // skin infill polys:
      if (skinFullInfills[s])
	polys.insert(polys.end(),
		     skinFullInfills[s]->infillpolys.begin(),
		     skinFullInfills[s]->infillpolys.end());
      // add skin infill to lines
      printlines.addPolys(INFILL, polys, false);

      polys.clear();

      // make polygons at skin_z:
      for(size_t p = 0; p < skinPolygons.size(); p++) {
	polys.push_back(Poly(skinPolygons[p], skin_z));
      }

      // add skin to lines
      printlines.addPolys(SKIN, polys, (s==0), //displace at first skin
			  settings.Hardware.MaxShellSpeed,
			  settings.Slicing.MinShelltime);
      if (s < skins-1) { // not on the last layer, this handle with all other lines
	// have to get all these separately because z changes
	printlines.makeLines(startPoint, lines);
	if (!settings.Slicing.ZliftAlways)
	  printlines.clipMovements(*clippolys, lines, clipnearest, linewidth);
	printlines.optimize(linewidth,
			    settings.Slicing.MinShelltime, cornerradius, lines);
	printlines.getLines(lines, lines3);
	printlines.clear();
	lines.clear();
      }
      polys.clear();
    }
  } // last skin layer now still in lines
  lines.clear();

  // 2. Skirt
  vector <Poly> skirts(1); skirts[0] = skirtPolygon;
  printlines.addPolys(SKIRT, skirts, false, 
		      settings.Hardware.MaxShellSpeed,
		      settings.Slicing.MinShelltime);

  // 3. Support
  if (supportInfill)
    printlines.addPolys(SUPPORT, supportInfill->infillpolys, false);

  // 4. all other polygons:  

  //  Shells
  for(int p=shellPolygons.size()-1; p>=0; p--) { // inner to outer
    printlines.addPolys(SHELL, shellPolygons[p],
			(p==(int)(shellPolygons.size())-1),
			settings.Hardware.MaxShellSpeed,
			settings.Slicing.MinShelltime);
  }

  //  Infill
  if (normalInfill)
    printlines.addPolys(INFILL, normalInfill->infillpolys, false);
  if (thinInfill)
    printlines.addPolys(INFILL, thinInfill->infillpolys, false);
  if (fullInfill)
    printlines.addPolys(INFILL, fullInfill->infillpolys, false);
  if (decorInfill)
    printlines.addPolys(INFILL, decorInfill->infillpolys, false);
  for (uint b=0; b < bridgeInfills.size(); b++)
    if (bridgeInfills[b])
      printlines.addPolys(INFILL, bridgeInfills[b]->infillpolys, false);

  double polyspeedfactor = printlines.makeLines(startPoint, lines);

  // FINISH

  Command lchange(LAYERCHANGE, LayerNo);
  lchange.comment += info();
  lchange.where = Vector3d(0.,0.,Z);
  commands.push_back(lchange);

  if (!settings.Slicing.ZliftAlways)
    printlines.clipMovements(*clippolys, lines, clipnearest, linewidth);
  printlines.optimize(linewidth, 
		      settings.Slicing.MinLayertime, cornerradius, lines);
  if ((guint)LayerNo < settings.Slicing.FirstLayersNum)
    printlines.setSpeedFactor(settings.Slicing.FirstLayersSpeed, lines);
  double slowdownfactor = printlines.getSlowdownFactor() * polyspeedfactor;

  if (settings.Slicing.FanControl) {
    int fanspeed = settings.Slicing.MinFanSpeed;
    if (slowdownfactor < 1 && slowdownfactor > 0) { 
      double fanfactor = 1-slowdownfactor; 
      fanspeed += 
	int(fanfactor * (settings.Slicing.MaxFanSpeed-settings.Slicing.MinFanSpeed));
      fanspeed = CLAMP(fanspeed, settings.Slicing.MinFanSpeed, settings.Slicing.MaxFanSpeed);
      //cerr << slowdownfactor << " - " << fanfactor << " - " << fanspeed << " - " << endl;
    } 
    Command fancommand(FANON, fanspeed);
    commands.push_back(fancommand);
  }

  printlines.getLines(lines, lines3);

  const double 
    minspeed  = settings.Hardware.MinPrintSpeedXY,
    maxspeed  = settings.Hardware.MaxPrintSpeedXY,
    movespeed = settings.Hardware.MoveSpeed,
    emax      = settings.Hardware.EMaxSpeed;

  // push all lines to gcode
  // start3 = state.LastPosition();
  PLineArea lastArea= UNDEF;
  for (uint i = 0; i < lines3.size(); i++) {
    if (lines3[i].area != lastArea) {
      lastArea = lines3[i].area;
      commands.push_back(Command(AreaNames[lastArea]));
    }
    lines3[i].getCommands(lastPos, commands, extrf, minspeed, maxspeed, movespeed, emax);
  }
}


double Layer::area() const
{
  return Clipping::Area(polygons);
}

string Layer::info() const 
{
  ostringstream ostr;
  ostr <<"Layer at Z=" << Z << " No=" << LayerNo 
       <<", thickn=" << thickness
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
  
  if (previous != NULL)
    ostr << " prev.No=" << previous->LayerNo;

  return ostr.str();
}
 

vector<Poly> Layer::getOverhangs() const
{
  vector<Poly> overhangs;
  if (previous!=NULL) {
    Clipping clipp;
    clipp.addPolys(polygons, subject);
    vector<Poly> prevoffset = Clipping::getOffset(previous->polygons, thickness/2);
    clipp.addPolys(prevoffset, clip);
    clipp.setZ(Z);
    overhangs = clipp.subtract();//CL::pftNonZero,CL::pftNonZero);
  }
  return overhangs;
}

string Layer::SVGpath(const Vector2d &trans) const
{
  ostringstream ostr;
  ostr << "\t<g id=\"Layer_"<< LayerNo 
       << "_z:" << getZ() << "\">" << endl;
  for (uint i = 0; i<polygons.size(); i++) {
    ostr << polygons[i].SVGpath(trans) << endl;
  }
  ostr << "\t</g>";
  return ostr.str();
}


void draw_poly(const Poly &poly, int gl_type, int linewidth, int pointsize,
	       const float *rgb, float a, bool randomized = false)
{
  glColor4f(rgb[0],rgb[1],rgb[2],a);
  glLineWidth(linewidth);
  glPointSize(pointsize);
  poly.draw(gl_type, randomized);
}
void draw_polys(const vector <Poly> &polys, int gl_type, int linewidth, int pointsize,
		const float *rgb, float a, bool randomized = false)
{
  glColor4f(rgb[0],rgb[1],rgb[2], a);
  glLineWidth(linewidth);
  glPointSize(pointsize);
  for(size_t p=0; p<polys.size();p++) {
    polys[p].draw(gl_type, randomized);
  }
}

void draw_polys_surface(const vector <Poly> &polys, 
			const Vector2d &Min, const Vector2d &Max, 
			double z,
			double cleandist,
			const float *rgb, float a)
{
  glColor4f(rgb[0],rgb[1],rgb[2], a);
  glDrawPolySurfaceRastered(polys, Min, Max, z, cleandist);

  // glColor4f(rgb[0],rgb[1],rgb[2], a);
  // for(size_t p=0; p<polys.size();p++) {
  //   polys[p].cleanup(cleandist);
  //   ::cleandist(polys[p].vertices, cleandist);
  //   polys[p].draw_as_surface();
  // }
}
void draw_polys_surface(const vector< vector<Poly> > &polys, 
			const Vector2d &Min, const Vector2d &Max, 
			double z,
			double cleandist,
			const float *rgb, float a)
{
  for(size_t p=0; p < polys.size();p++)
    draw_polys_surface(polys[p], Min, Max, z, cleandist, rgb, a);
}

void draw_polys(const vector< vector <Poly> > &polys, 
		int gl_type, int linewidth, int pointsize,
		const float *rgb, float a, bool randomized = false)
{
  for(size_t p=0; p<polys.size();p++)
    draw_polys(polys[p], gl_type, linewidth, pointsize, rgb, a, randomized);
}
void draw_polys(const vector <ExPoly> &expolys, int gl_type, int linewidth, int pointsize,
		const float *rgb, float a, bool randomized = false)
{
  for(size_t p=0; p < expolys.size();p++) {
    draw_poly(expolys[p].outer,  gl_type, linewidth, pointsize, rgb, a, randomized);
    draw_polys(expolys[p].holes, gl_type, linewidth, pointsize, rgb, a, randomized);
  }
}

float const GREEN[] = {0.1, 1, 0.1};
float const GREEN2[] = {0.3, 0.8, 0.3};
float const BLUEGREEN[] = {0.1, 0.9, 0.7};
float const BLUE2[] = {0.5,0.5,1.0};
float const RED[] = {1, 0, 0};
float const RED2[] = {0.8,0.5,0.5};
float const RED3[] = {0.8,0.3,0.1};
float const ORANGE[] = {1, 0.5, 0};
float const YELLOW[] = {1, 1, 0};
float const YELLOW2[] = {1, 1, 0.2};
float const WHITE[] = {1, 1, 1};
float const GREY[] = {0.5,0.5,0.5};
float const VIOLET[] = {0.8,0.0,0.8};

void Layer::Draw(const Settings &settings)
{
  bool randomized = settings.Display.RandomizedLines;
  bool filledpolygons = settings.Display.DisplayFilledAreas;
  // glEnable(GL_LINE_SMOOTH);
  // glHint(GL_LINE_SMOOTH_HINT,  GL_NICEST);
  draw_polys(polygons, GL_LINE_LOOP, 1, 3, RED, 1, randomized);
  draw_polys(polygons, GL_POINTS,    1, 3, RED, 1, randomized);

  if(settings.Display.DrawCPOutlineNumbers)
    for(size_t p=0; p<polygons.size();p++)
      {
	ostringstream oss;
	oss << p;
	Vector2d center = polygons[p].getCenter();
	drawString(Vector3d(center.x(), center.y(), Z), oss.str());
      }

  draw_poly(hullPolygon,    GL_LINE_LOOP, 3, 3, ORANGE,  0.5, randomized);
  draw_poly(skirtPolygon,   GL_LINE_LOOP, 3, 3, YELLOW,  1, randomized);
  draw_polys(shellPolygons, GL_LINE_LOOP, 1, 3, YELLOW2, 1, randomized);
  draw_polys(thinPolygons,  GL_LINE_LOOP, 2, 3, YELLOW,  1, randomized);

  glColor4f(0.5,0.9,1,1);
  glLineWidth(1);
  double zs = Z;
  for(size_t s=0;s<skins;s++) {
    for(size_t p=0; p < skinPolygons.size();p++) {
      //cerr << s << ": " << p << " _ " << zs << endl;
      skinPolygons[p].draw(GL_LINE_LOOP, zs, randomized);
    }
    zs-=thickness/skins;
  }
  draw_polys(fillPolygons,         GL_LINE_LOOP, 1, 3, WHITE, 0.6, randomized);
  if (supportPolygons.size()>0) {
    if (filledpolygons) 
      draw_polys_surface(supportPolygons,  Min, Max, Z, thickness/2., BLUE2, 0.4);
    draw_polys(supportPolygons,      GL_LINE_LOOP, 3, 3, BLUE2, 1,   randomized);
    if(settings.Display.DrawVertexNumbers)
      for(size_t p=0; p<supportPolygons.size();p++)
	supportPolygons[p].drawVertexNumbers();
  } // else
    // draw_polys(toSupportPolygons,    GL_LINE_LOOP, 1, 1, BLUE2, 1,   randomized);
  draw_polys(bridgePolygons,       GL_LINE_LOOP, 3, 3, RED2,  0.7, randomized);
  draw_polys(bridgePillars,        GL_LINE_LOOP, 3, 3, YELLOW,0.7, randomized);
  draw_polys(fullFillPolygons,     GL_LINE_LOOP, 1, 1, GREY,  0.6, randomized);
  draw_polys(decorPolygons,        GL_LINE_LOOP, 1, 3, WHITE, 1,   randomized);
  draw_polys(skinFullFillPolygons, GL_LINE_LOOP, 1, 3, GREY,  0.6, randomized);
  if (filledpolygons) {
    draw_polys_surface(fullFillPolygons,  Min, Max, Z, thickness/2., GREEN, 0.5);
    draw_polys_surface(fillPolygons,  Min, Max, Z, thickness/2., GREEN2, 0.25);
    draw_polys_surface(decorPolygons,  Min, Max, Z, thickness/2., GREY, 0.2);
  }
  if(settings.Display.DisplayinFill)
    {
      bool DebugInfill = settings.Display.DisplayDebuginFill;
      if (normalInfill)
	draw_polys(normalInfill->infillpolys, GL_LINE_LOOP, 1, 3, 
		   (normalInfill->cached?BLUEGREEN:GREEN), 1, randomized);
      if(DebugInfill && normalInfill->cached)
	draw_polys(normalInfill->getCachedPattern(Z), GL_LINE_LOOP, 1, 3, 
		   ORANGE, 0.5, randomized);
      if (thinInfill)
	draw_polys(thinInfill->infillpolys, GL_LINE_LOOP, 1, 3, 
		   GREEN, 1, randomized);
      if (fullInfill)
	draw_polys(fullInfill->infillpolys, GL_LINE_LOOP, 1, 3, 
		   (fullInfill->cached?BLUEGREEN:GREEN), 0.8, randomized);
      if(DebugInfill && fullInfill->cached)
	draw_polys(fullInfill->getCachedPattern(Z), GL_LINE_LOOP, 1, 3, 
		   ORANGE, 0.5, randomized);
      if (decorInfill)
	draw_polys(decorInfill->infillpolys, GL_LINE_LOOP, 1, 3, 
		   (decorInfill->cached?BLUEGREEN:GREEN), 0.8, randomized);
      if(DebugInfill && decorInfill->cached)
	draw_polys(decorInfill->getCachedPattern(Z), GL_LINE_LOOP, 1, 3, 
		   ORANGE, 0.5, randomized);
      uint bridgecount = bridgeInfills.size();
      if (bridgecount>0)
	for (uint i = 0; i<bridgecount; i++)
	  draw_polys(bridgeInfills[i]->infillpolys, GL_LINE_LOOP, 2, 3, 
		     RED3,0.9, randomized);
      if (supportInfill)
	draw_polys(supportInfill->infillpolys, GL_LINE_LOOP, 1, 3, 
		   (supportInfill->cached?BLUEGREEN:GREEN), 0.8, randomized);
      if(DebugInfill && supportInfill->cached)
	draw_polys(supportInfill->getCachedPattern(Z), GL_LINE_LOOP, 1, 3, 
		   ORANGE, 0.5, randomized);
      for(size_t s=0;s<skinFullInfills.size();s++) 
	draw_polys(skinFullInfills[s]->infillpolys, GL_LINE_LOOP, 1, 3, 
		   (skinFullInfills[s]->cached?BLUEGREEN:GREEN), 0.6, randomized);
    }
  //draw_polys(GetInnerShell(), GL_LINE_LOOP, 2, 3, WHITE,  1);
  glLineWidth(1);  
  if(settings.Display.DrawCPVertexNumbers) // poly vertex numbers
    for(size_t p=0; p<polygons.size();p++)
      polygons[p].drawVertexNumbers();
      //polygons[p].drawVertexAngles();
  
  if(settings.Display.DrawCPLineNumbers)  // poly line numbers
    for(size_t p=0; p<polygons.size();p++)
      polygons[p].drawLineNumbers();

  if(settings.Display.DrawVertexNumbers) { // infill vertex numbers
    for(size_t p=0; p<fillPolygons.size();p++)
      fillPolygons[p].drawVertexNumbers();
    for(size_t p=0; p<fullFillPolygons.size();p++)
      fullFillPolygons[p].drawVertexNumbers();
    for(size_t p=0; p<decorPolygons.size();p++)
      decorPolygons[p].drawVertexNumbers();
    for(size_t p=0; p<shellPolygons.size();p++)
      for(size_t q=0; q<shellPolygons[p].size();q++)
	shellPolygons[p][q].drawVertexNumbers();
  }


  if (settings.Display.ShowLayerOverhang) {
    if (previous!=NULL) {
      vector<Poly> overhangs = getOverhangs();
      draw_polys(overhangs, GL_LINE_LOOP, 1, 3, VIOLET, 0.8, randomized);
      //draw_polys_surface(overhangs, Min, Max, Z, thickness/5, VIOLET , 0.5);
  
      Cairo::RefPtr<Cairo::ImageSurface> surface;
      Cairo::RefPtr<Cairo::Context>      context;
      if (rasterpolys(overhangs, Min, Max, thickness/5, surface, context))
	if(surface!=0) {
	  glColor4f(RED[0],RED[1],RED[2], 0.5);
	  glDrawCairoSurface(surface, Min, Max, Z);
	  glColor4f(RED[0],RED[1],RED[2], 0.6);
	  glPointSize(3);
 	  glBegin(GL_POINTS);
 	  for (double x = Min.x(); x<Max.x(); x+=thickness)
	    for (double y = Min.y(); y<Max.y(); y+=thickness)
	      if (getCairoSurfaceDatapoint(surface, Min, Max, Vector2d(x,y))!=0)
	  	glVertex3d(x,y,Z);
	  glEnd();
	}
    }
  }

#if 0
  // test point-in-polygons
  const vector<Poly> *polys = GetOuterShell();
  glColor4f(RED[0],RED[1],RED[2], 0.6);
  glPointSize(3);
  glBegin(GL_POINTS);
  for (double x = Min.x(); x<Max.x(); x+=thickness/1)
    for (double y = Min.y(); y<Max.y(); y+=thickness/1) {
      bool inpoly = false;
      for (uint i=0; i<polys->size(); i++) {
	if ((*polys)[i].vertexInside(Vector2d(x,y))){
	  inpoly=true; break;
	}
      }
      if (inpoly)
	glVertex3d(x,y,Z);
      // else
      //   glColor4f(0.3,0.3,0.3, 0.6);
    }
  glEnd();
#endif

}

void Layer::DrawRulers(const Vector2d &point)
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
  glLineWidth(1);
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
