/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010  Kulitorum
    Copyright (C) 2011  Michael Meeks <michael.meeks@novell.com>
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
#include "config.h"
#define  MODEL_IMPLEMENTATION
#include <vector>
#include <string>
#include <cerrno>
#include <functional>
//#include <memory>

// should move to platform.h with com port fun.
#include <sys/types.h>
#include <dirent.h>

#include <libreprap/comms.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "stdafx.h"
#include "model.h"
#include "objtree.h"
#include "file.h"
#include "settings.h"
#include "connectview.h"
#include "progress.h"

#include "slicer/layer.h"
#include "slicer/infill.h"
#include "slicer/clipping.h"


void Model::MakeRaft(GCodeState &state, double &z)
{
  if (layers.size() == 0) return;
  vector<Poly> raftpolys = 
    Clipping::getOffset(layers[0]->GetHullPolygon(), settings.Raft.Size, jround);
  for (uint i = 0; i< raftpolys.size(); i++) 
    raftpolys[i].cleanup(settings.Hardware.LayerThickness/4);

  Settings::RaftSettings::PhasePropertiesType basesettings = 
    settings.Raft.Phase[0];
  Settings::RaftSettings::PhasePropertiesType interfacesettings = 
    settings.Raft.Phase[1];

  vector<Layer*> raft_layers;

  double rotation = basesettings.Rotation;
  double basethickness = settings.Hardware.LayerThickness 
    * basesettings.Thickness;
  double interthickness = settings.Hardware.LayerThickness 
    * interfacesettings.Thickness;

  double totalthickness = basesettings.LayerCount * basethickness 
    + interfacesettings.LayerCount * interthickness;

  double raft_z = -totalthickness + basethickness * settings.Slicing.FirstLayerHeight;

  for (uint i = 0; i < basesettings.LayerCount; i++) {
    Layer * layer = new Layer(lastlayer,
			      -interfacesettings.LayerCount-basesettings.LayerCount + i, 
			      basethickness, 1);
    layer->setZ(raft_z);
    layer->CalcRaftInfill(raftpolys,basesettings.MaterialDistanceRatio,
			  basesettings.Distance, rotation);
    raft_layers.push_back(layer);
    lastlayer = layer;
    rotation += basesettings.RotationPrLayer*M_PI/180;
    raft_z += basethickness;
  }
  rotation = interfacesettings.Rotation;
  for (uint i = 0; i < interfacesettings.LayerCount; i++) {
    Layer * layer = new Layer(lastlayer, -basesettings.LayerCount + i, 
			      interthickness, 1);
    layer->setZ(raft_z);
    layer->CalcRaftInfill(raftpolys,interfacesettings.MaterialDistanceRatio,
			  interfacesettings.Distance, rotation);
    
    raft_layers.push_back(layer);
    lastlayer = layer;
    rotation += interfacesettings.RotationPrLayer*M_PI/180;
    raft_z += interthickness;
  }
  layers.insert(layers.begin(),raft_layers.begin(),raft_layers.end());
  z += totalthickness;
}

#if 0
// old raft
void Model::MakeRaft(GCodeState &state, double &z)
{
  vector<Intersection> HitsBuffer;

  double raftSize = settings.Raft.Size;
  Vector3d raftMin =  settings.Hardware.PrintMargin + Min;
  Vector3d raftMax =  settings.Hardware.PrintMargin + Max + 2 * raftSize;
  Vector2d Center = Vector2d((raftMin.x + raftMax.x) / 2,
			     (raftMin.y + raftMax.y) / 2);

  // bbox of object
  double Length = (std::max(raftMax.x,raftMax.y) -
		   std::min(raftMin.x, raftMin.y))/sqrt(2.0);

  double rot;
  uint LayerNr = 0;
  uint layerCount = settings.Raft.Phase[0].LayerCount +
		    settings.Raft.Phase[1].LayerCount;
  Settings::RaftSettings::PhasePropertiesType *props = &settings.Raft.Phase[0];

  double thickness = props->Thickness * settings.Hardware.LayerThickness;
  double extrusionfactor = settings.Hardware.GetExtrudeFactor(thickness)
    * props->MaterialDistanceRatio;


  while(LayerNr < layerCount)
    {
      // If we finished phase 0, start phase 1 of the raft...
      if (LayerNr >= settings.Raft.Phase[0].LayerCount)
	props = &settings.Raft.Phase[1];

      rot = (props->Rotation+(double)LayerNr * props->RotationPrLayer)/180.0*M_PI;
      Vector2d InfillDirX(cosf(rot), sinf(rot));
      Vector2d InfillDirY(-InfillDirX.y, InfillDirX.x);

      Vector3d LastPosition;
      bool reverseLines = false;

      Vector2d P1, P2;
      double maxerr = 0.1*props->Distance;
      for(double x = -Length ; x < Length ; x+=props->Distance)
	{
	  P1 = (InfillDirX *  Length)+(InfillDirY*x) + Center;
	  P2 = (InfillDirX * -Length)+(InfillDirY*x) + Center;

	  if(reverseLines)
	    {
	      Vector2d tmp = P1;
	      P1 = P2;
	      P2 = tmp;
	    }

	  //			glBegin(GL_LINES);
	  //			glVertex2fv(&P1.x);
	  //			glVertex2fv(&P2.x);

	  // Crop lines to bbox*size
	  Vector3d point;
	  Intersection hit;
	  HitsBuffer.clear();
	  Vector2d P3(raftMin.x, raftMin.y);
	  Vector2d P4(raftMin.x, raftMax.y);
	  //			glVertex2fv(&P3.x);
	  //			glVertex2fv(&P4.x);
	  if(IntersectXY(P1,P2,P3,P4,hit,maxerr))	//Intersect edges of bbox
	    HitsBuffer.push_back(hit);
	  P3 = Vector2d(raftMax.x,raftMax.y);
	  //			glVertex2fv(&P3.x);
	  //			glVertex2fv(&P4.x);
	  if(IntersectXY(P1,P2,P3,P4,hit,maxerr))
	    HitsBuffer.push_back(hit);
	  P4 = Vector2d(raftMax.x,raftMin.y);
	  //			glVertex2fv(&P3.x);
	  //			glVertex2fv(&P4.x);
	  if(IntersectXY(P1,P2,P3,P4,hit,maxerr))
	    HitsBuffer.push_back(hit);
	  P3 = Vector2d(raftMin.x,raftMin.y);
	  //			glVertex2fv(&P3.x);
	  //			glVertex2fv(&P4.x);
	  if(IntersectXY(P1,P2,P3,P4,hit,maxerr))
	    HitsBuffer.push_back(hit);
	  //			glEnd();

	  if(HitsBuffer.size() == 0)	// it can only be 2 or zero
	    continue;
	  if(HitsBuffer.size() != 2)
	    continue;

	  std::sort(HitsBuffer.begin(), HitsBuffer.end());

	  P1 = HitsBuffer[0].p;
	  P2 = HitsBuffer[1].p;

	  state.MakeGCodeLine (Vector3d(P1.x,P1.y,z), 
			       Vector3d(P2.x,P2.y,z),
			       Vector3d(0,0,0), 0,
			       settings.Hardware.MaxPrintSpeedXY,
			       extrusionfactor, 0,
			       z,
			       settings.Slicing, settings.Hardware);
	  reverseLines = !reverseLines;
	}
      // Set startspeed for Z-move
      Command g;
      g.Code = SETSPEED;
      g.where = Vector3d(P2.x, P2.y, z);
      g.f=settings.Hardware.MinPrintSpeedZ;
      g.comment = "Move Z";
      g.e = 0;
      gcode.commands.push_back(g);
      z += thickness;

      // Move Z
      g.Code = ZMOVE;
      g.where = Vector3d(P2.x, P2.y, z);
      g.f = settings.Hardware.MinPrintSpeedZ;
      g.comment = "Move Z";
      g.e = 0;
      gcode.commands.push_back(g);

      LayerNr++;
    }

  // restore the E state
  // Command gotoE;
  // gotoE.Code = GOTO;
  // gotoE.e = 0;
  // gotoE.comment = _("Reset E for the remaining print");
  // gcode.commands.push_back(gotoE);
}
#endif // 0

// this is of not much use, too fast
void Model::CleanupLayers()
{
  int count = (int)layers.size();
  if (count == 0) return;
  if(!m_progress->restart (_("Cleanup"), count)) return;
  int progress_steps=(int)(count/100);
  if (progress_steps==0) progress_steps=1;
  bool cont = true;

#ifdef _OPENMP
  omp_lock_t progress_lock;
  omp_init_lock(&progress_lock);
#pragma omp parallel for schedule(dynamic) 
#endif
  for (int i=0; i < count; i++)
    {
      if (i%progress_steps==0) {
#ifdef _OPENMP
	omp_set_lock(&progress_lock);
#endif
	cont = m_progress->update(i);
#ifdef _OPENMP
	omp_unset_lock(&progress_lock);
#endif
      }
      layers[i]->cleanupPolygons();
      if (!cont) i=count;
    }
#ifdef _OPENMP
  omp_destroy_lock(&progress_lock);
#endif
}


bool layersort(const Layer * l1, const Layer * l2){
  return (l1->Z < l2->Z);
}

void Model::Slice() 
{
  if (is_calculating) {
    return;
  }
  vector<Shape*> shapes;
  vector<Matrix4d> transforms;

  if (settings.Slicing.SelectedOnly)
    objtree.get_selected_shapes(m_current_selectionpath, shapes, transforms);
  else
    objtree.get_all_shapes(shapes,transforms);
  
  if (shapes.size() == 0) return;

  assert(shapes.size() == transforms.size());

  is_calculating=true;

  CalcBoundingBoxAndCenter(settings.Slicing.SelectedOnly);

  for (uint i = 0; i<transforms.size(); i++)
    transforms[i] = settings.getBasicTransformation(transforms[i]);

  assert(shapes.size() == transforms.size());

  int LayerNr = 0;
  bool varSlicing = settings.Slicing.Varslicing;

  uint max_skins = max(1, settings.Slicing.Skins);
  double thickness = (double)settings.Hardware.LayerThickness;
  double skin_thickness = thickness / max_skins;
  uint skins = max_skins; // probably variable 

  // - Start at z~=0, cut off everything below
  // - Offset it a bit in Z, z = 0 gives a empty slice because no triangle crosses this Z value
  double minZ = thickness * settings.Slicing.FirstLayerHeight;// + Min.z; 
  Vector3d volume = settings.Hardware.Volume;
  double maxZ = min(Max.z(), volume.z() - settings.Hardware.PrintMargin.z());

  double max_gradient = 0;

  m_progress->set_terminal_output(settings.Display.TerminalProgress);
  m_progress->start (_("Slicing"), maxZ);
  // for (vector<Layer *>::iterator pIt = layers.begin();
  //      pIt != layers. end(); pIt++)
  //   delete *pIt;
  layers.clear();

  bool flatshapes = shapes.front()->dimensions() == 2;
  if (flatshapes) {
    layers.resize(1);
    layers[0] = new Layer(lastlayer, 0, thickness  , 1); 
    lastlayer = layers[0];
    layers[0]->setZ(0); // set to real z
    for (uint nshape= 0; nshape < shapes.size(); nshape++) {
      layers[0]->addShape(transforms[nshape], *shapes[nshape],  0, max_gradient);
    }
    is_calculating=false;
    return;
  }

  int progress_steps=(int)(maxZ/thickness/100);
  if (progress_steps==0) progress_steps=1;

  if ( !(varSlicing && skins > 1) &&
       !(settings.Slicing.BuildSerial && shapes.size() > 1) )
    { // simple, can do multihreading
      if (progress_steps==0) progress_steps=1;
      int num_layers = (int)ceil((maxZ - minZ) / thickness);
      layers.resize(num_layers);
      int nlayer;
      bool cont = true;

#ifdef _OPENMP
      omp_lock_t progress_lock;
      omp_init_lock(&progress_lock);
#pragma omp parallel for schedule(dynamic) 
#endif
      for (nlayer = 0; nlayer < num_layers; nlayer++) {
	double z = minZ + thickness * nlayer;
	if (nlayer%progress_steps==0) {
#ifdef _OPENMP
	  omp_set_lock(&progress_lock);
#endif
	cont = (m_progress->update(z));
#ifdef _OPENMP
	  omp_unset_lock(&progress_lock);
#endif
	}
	if (!cont) continue;
	Layer * layer = new Layer(NULL, nlayer, thickness, skins); 
	layer->setZ(z); // set to real z
	int new_polys=0;
	for (uint nshape= 0; nshape < shapes.size(); nshape++) {
	  new_polys += layer->addShape(transforms[nshape], *shapes[nshape],
				       z, max_gradient);
	}
	if (cont) 
	  layers[nlayer] = layer;
      }
#ifdef _OPENMP
      if (cont)
	std::sort(layers.begin(), layers.end(), layersort);
      else layers.clear();
      omp_destroy_lock(&progress_lock);
#endif
      for (uint nlayer = 1; nlayer < layers.size(); nlayer++) {
	layers[nlayer]->setPrevious(layers[nlayer-1]);
      }
      if (layers.size()>0)
	lastlayer = layers.back();
    }
  else 
    { // have skins and/or serial build
      uint currentshape   = 0;
      double serialheight = maxZ; // settings.Slicing.SerialBuildHeight; 
      double z            = minZ;
      double shape_z      = z;
      double max_shape_z  = z + serialheight;
      Layer * layer = new Layer(lastlayer, LayerNr, thickness, 1); // first layer no skins
      layer->setZ(shape_z);
      LayerNr = 1;
      int new_polys=0;
      bool cont = true;
      while(cont && z < maxZ)
	{
	  shape_z = z; 
	  max_shape_z = min(shape_z + serialheight, maxZ); 
	  while ( cont && currentshape < shapes.size() && shape_z <= max_shape_z ) {
	    if (LayerNr%progress_steps==0) cont = m_progress->update(shape_z);
	    layer->setZ(shape_z); // set to real z
	    if (shape_z == minZ) { // the layer is on the platform
	      layer->LayerNo = 0;
	      layer->setSkins(1);
	      LayerNr = 1; 
	    }
	    new_polys = layer->addShape(transforms[currentshape], *shapes[currentshape],
					shape_z, 
					max_gradient);
	    // cerr << "Z="<<z<<", shapez="<< shape_z << ", shape "<<currentshape 
	    //      << " of "<< shapes.size()<< " polys:" << new_polys<<endl;
	    if (shape_z >= max_shape_z) { // next shape, reset z
	      currentshape++; 
	      shape_z = z;
	    } else {  // next z, same shape
	      if (varSlicing && LayerNr!=0) { 
		// higher gradient -> slice thinner with fewer skin divisions
		skins = max_skins-(uint)(max_skins* max_gradient);
		thickness = skin_thickness*skins;
	      }
	      shape_z += thickness; 
	      max_gradient = 0;
	      if (new_polys > -1){
		layers.push_back(layer);
		lastlayer = layer;
		layer = new Layer(lastlayer, LayerNr++, thickness, skins);
	      }
	    }
	  }
	  //thickness = max_thickness-(max_thickness-min_thickness)*max_gradient;
	  if (currentshape < shapes.size()-1) { // reached max_shape_z, next shape
	    currentshape++;
	  } else { // end of shapes
	    if (new_polys > -1){ 
	      if (varSlicing) {
		skins = max_skins-(uint)(max_skins* max_gradient);
		thickness = skin_thickness*skins;
	      }
	      layers.push_back(layer);
	      lastlayer = layer;
	      layer = new Layer(lastlayer, LayerNr++, thickness, skins);
	    }
	    z = max_shape_z + thickness;
	    currentshape = 0; // all shapes again
	  }
	  max_gradient=0;
	  //cerr << "    Z="<<z << "Max.z="<<Max.z<<endl;
	}
      delete layer; // have made one more than needed
    }
  is_calculating=false;
  // shapes.clear();
  //m_progress->stop (_("Done"));
}

void Model::MakeFullSkins()
{
  // not bottom layer

  if(!m_progress->restart (_("Skins"), layers.size())) return;
  int progress_steps=(int)(layers.size()/100);
  if (progress_steps==0) progress_steps=1;
  // #pragma omp parallel for schedule(dynamic) ordered
  for (int i=1; i < (int)layers.size(); i++) {
    if (i%progress_steps==0) if(!m_progress->update(i)) break;
    layers[i]->makeSkinPolygons();
  }
  //m_progress->stop (_("Done"));
}


void Model::MakeUncoveredPolygons(bool make_decor, bool make_bridges)
{
  int count = (int)layers.size();
  if (count == 0 ) return;
  if (!m_progress->restart (_("Find Uncovered"), 2*count+2)) return;
  int progress_steps=(int)((2*count+2)/100);
  if (progress_steps==0) progress_steps=1;
  // bottom to top: uncovered from above -> top polys
  for (int i = 0; i < count-1; i++) 
    {
      if (i%progress_steps==0) if(!m_progress->update(i)) return ;
      layers[i]->addFullPolygons(GetUncoveredPolygons(layers[i],layers[i+1]), make_decor);
    }  
  // top to bottom: uncovered from below -> bridge polys
  for (uint i = count-1; i > 0; i--) 
    {
      //cerr << "layer " << i << endl;
      if (i%progress_steps==0) if(!m_progress->update(count + count - i)) return;
      //make_bridges = false;
      // no bridge on marked layers (serial build)
      bool mbridge = make_bridges && (layers[i]->LayerNo != 0); 
      if (mbridge) {
	vector<Poly> uncovered = GetUncoveredPolygons(layers[i],layers[i-1]);
	layers[i]->addBridgePolygons(Clipping::getExPolys(uncovered));
	layers[i]->calcBridgeAngles(layers[i-1]);
      }
      else {
	const vector<Poly> &uncovered = GetUncoveredPolygons(layers[i],layers[i-1]);
	layers[i]->addFullPolygons(uncovered,make_decor);
      }
    }
  m_progress->update(2*count+1);
  layers.front()->addFullPolygons(layers.front()->GetFillPolygons(), make_decor);
  m_progress->update(2*count+2);
  layers.back()->addFullPolygons(layers.back()->GetFillPolygons(), make_decor);
  //m_progress->stop (_("Done"));
}

// find polys in subjlayer that are not covered by shell of cliplayer
vector<Poly> Model::GetUncoveredPolygons(const Layer * subjlayer,
					 const Layer * cliplayer)
{
  Clipping clipp;
  clipp.clear();
  clipp.addPolys(subjlayer->GetFillPolygons(),     subject); 
  clipp.addPolys(subjlayer->GetFullFillPolygons(), subject); 
  clipp.addPolys(subjlayer->GetBridgePolygons(),   subject); 
  clipp.addPolys(subjlayer->GetDecorPolygons(),    subject); 
  //clipp.addPolys(cliplayer->GetOuterShell(),       clip); // have some overlap
  clipp.addPolys(*(cliplayer->GetInnerShell()),       clip); // have some more overlap
  vector<Poly> uncovered = clipp.subtractMerged();
  return uncovered; 
}				 
				 
void Model::MultiplyUncoveredPolygons()
{
  if (!settings.Slicing.DoInfill && settings.Slicing.SolidThickness == 0.0) return;
  int shells = (int)ceil(settings.Slicing.SolidThickness/settings.Hardware.LayerThickness);
  shells = max(shells, (int)settings.Slicing.ShellCount);
  if (shells<1) return;
  int count = (int)layers.size();
  
  int numdecor = 0;
  // add another full layer if making decor
  if (settings.Slicing.MakeDecor) 
    numdecor = settings.Slicing.DecorLayers;
  shells += numdecor;

  if (!m_progress->restart (_("Uncovered Shells"), count*3)) return;
  int progress_steps=(int)(count*3/100);
  if (progress_steps==0) progress_steps=1;
  // bottom-up: mulitply downwards
  int i,s;
  for (i=0; i < count; i++) 
    {
      if (i%progress_steps==0) if(!m_progress->update(i)) return;
      // (brigdepolys are not multiplied downwards)
      const vector<Poly> &fullpolys     = layers[i]->GetFullFillPolygons();
      const vector<Poly> &skinfullpolys = layers[i]->GetSkinFullPolygons();
      const vector<Poly> &decorpolys    = layers[i]->GetDecorPolygons();
      for (s=1; s < shells; s++) 
	if (i-s > 1) {
	  layers[i-s]->addFullPolygons (fullpolys,     false);
	  layers[i-s]->addFullPolygons (skinfullpolys, false);
	  layers[i-s]->addFullPolygons (decorpolys,    s < numdecor);
	}
    }
  // top-down: mulitply upwards
  for (int i=count-1; i>=0; i--) 
    {
      if (i%progress_steps==0) if (!m_progress->update(count + count -i)) return;
      const vector<Poly>   &fullpolys     = layers[i]->GetFullFillPolygons();
      const vector<ExPoly> &bridgepolys   = layers[i]->GetBridgePolygons();
      const vector<Poly>   &skinfullpolys = layers[i]->GetSkinFullPolygons();
      const vector<Poly>   &decorpolys    = layers[i]->GetDecorPolygons();
      for (int s=1; s < shells; s++) 
	if (i+s < count){
	  layers[i+s]->addFullPolygons (fullpolys,     false);
	  layers[i+s]->addFullPolygons (bridgepolys,   false);
	  layers[i+s]->addFullPolygons (skinfullpolys, false);
	  layers[i+s]->addFullPolygons (decorpolys,    s < numdecor);
	}
    }

  m_progress->set_label(_("Merging Full Polygons"));
  // merge results
  bool cont = true;

#ifdef _OPENMP
  omp_lock_t progress_lock;
  omp_init_lock(&progress_lock);
#pragma omp parallel for schedule(dynamic) 
#endif
  for (i=0; i < count; i++) 
    {
      if (i%progress_steps==0) {
#ifdef _OPENMP
	omp_set_lock(&progress_lock);
#endif
	cont = (m_progress->update(count + count +i));
#ifdef _OPENMP
	omp_unset_lock(&progress_lock);
#endif
      }
      if (!cont) continue;
      layers[i]->mergeFullPolygons(false);
    }
#ifdef _OPENMP
  omp_destroy_lock(&progress_lock);
#endif  //m_progress->stop (_("Done"));
}


void Model::MakeSupportPolygons(Layer * subjlayer, // lower -> will change
				const Layer * cliplayer,  // upper 
				double widen)
{
  Clipping clipp;
  clipp.clear();
  clipp.addPolys(cliplayer->GetPolygons(),        subject);
  clipp.addPolys(cliplayer->GetSupportPolygons(), subject); // previous 
  clipp.addPolys(subjlayer->GetPolygons(),        clip);
  clipp.setZ(subjlayer->getZ());
  vector<Poly> spolys;
  if (widen != 0)
    // widen from layer to layer
    spolys = clipp.getOffset(clipp.subtract(),
			     widen * subjlayer->thickness);
  else
    spolys = clipp.subtractMerged();
  subjlayer->setSupportPolygons(spolys);
}

void Model::MakeSupportPolygons(double widen)
{ 
  int count = layers.size();
  if (!m_progress->restart (_("Support"), count*2)) return;
  int progress_steps=(int)(count*2/100);
  if (progress_steps==0) progress_steps=1;

  for (int i=count-1; i>0; i--) 
    {
      if (i%progress_steps==0) if(! m_progress->update(count-i)) return;
      if (layers[i]->LayerNo == 0) continue;
      MakeSupportPolygons(layers[i-1], layers[i], widen);
    }
  // shrink a bit
  for (int i=0; i<count; i++) 
    {
      //cerr << "shrink support layer "<< i << endl;
      //if (layers[i]->LayerNo == 0) continue;
      double distance = 2*settings.Hardware.GetExtrudedMaterialWidth(layers[i]->thickness);
      if (i%progress_steps==0) if(!m_progress->update(i+count)) return;
      // vector<Poly> merged = Clipping::getMerged(layers[i]->GetSupportPolygons());
      // cerr << merged.size() << " polys" << endl;
      vector<Poly> offset = 
	Clipping::getOffset(layers[i]->GetSupportPolygons(),-distance);
      layers[i]->setSupportPolygons(offset);
    }
  //  m_progress->stop (_("Done"));
}

void Model::MakeSkirt()
{

  if (!settings.Slicing.Skirt) return;
  double skirtdistance  = settings.Slicing.SkirtDistance;

  Clipping clipp;
  guint count = layers.size();
  guint endindex = 0;
  // find maximum of all calculated skirts
  clipp.clear();
  for (guint i=0; i < count; i++) 
    {
      if (layers[i]->getZ() > settings.Slicing.SkirtHeight)
	break;
      layers[i]->MakeSkirt(skirtdistance);
      Poly sp = layers[i]->GetSkirtPolygon();
      if (sp.size()>0) {
	clipp.addPoly(sp,subject);
	endindex = i;
      }
    }
  vector<Poly> skirts = clipp.unite();
  // set this skirt for all skirted layers 
  if (skirts.size()>0)
    for (guint i=0; i<=endindex; i++) {
      layers[i]->setSkirtPolygon(skirts.front());
  }
}

void Model::MakeShells()
{
  int count = (int)layers.size();
  if (count == 0) return;
  if (!m_progress->restart (_("Shells"), count)) return;
  int progress_steps=(int)(count/100);
  if (progress_steps==0) progress_steps=1;
  bool cont = true;

#ifdef _OPENMP
  omp_lock_t progress_lock;
  omp_init_lock(&progress_lock);
#pragma omp parallel for schedule(dynamic) 
#endif
  for (int i=0;  i < count; i++) 
    {
      if (i%progress_steps==0) {
#ifdef _OPENMP
	omp_set_lock(&progress_lock);
#endif
	cont = (m_progress->update(i));
#ifdef _OPENMP
	omp_unset_lock(&progress_lock);
#endif
      }
      if (!cont) continue;
      layers[i]->MakeShells(settings);
    }
#ifdef _OPENMP
  omp_destroy_lock(&progress_lock);
#endif
  m_progress->update(count);
  //m_progress->stop (_("Done"));
}


void Model::CalcInfill()
{
  if (!settings.Slicing.DoInfill && settings.Slicing.SolidThickness == 0.0) return;

  int count = (int)layers.size();
  m_progress->start (_("Infill"), count);
  int progress_steps=(count/100);
  if (progress_steps==0) progress_steps=1;
  bool cont = true;

  //cerr << "make infill"<< endl;
#ifdef _OPENMP
  omp_lock_t progress_lock;
  omp_init_lock(&progress_lock);
#pragma omp parallel for schedule(dynamic) 
#endif
  for (int i=0; i < count ; i++) 
    {
      //cerr << "thread " << omp_get_thread_num() << endl;
      if (i%progress_steps==0){
#ifdef _OPENMP
	omp_set_lock(&progress_lock);
#endif
	cont = (m_progress->update(i));
#ifdef _OPENMP
	omp_unset_lock(&progress_lock);
#endif
      }
      if (!cont) continue;
      layers[i]->CalcInfill(settings);
    }
#ifdef _OPENMP
  omp_destroy_lock(&progress_lock);
#endif
  //m_progress->stop (_("Done"));
}


void Model::ConvertToGCode()
{
  if (is_calculating) {
    return;
  }

  string GcodeTxt;
  string GcodeStart = settings.GCode.getStartText();
  string GcodeLayer = settings.GCode.getLayerText();
  string GcodeEnd   = settings.GCode.getEndText();
  gcode.clear();
  GCodeState state(gcode);

  Infill::clearPatterns();

  Vector3d printOffset  = settings.Hardware.PrintMargin;
  double   printOffsetZ = settings.Hardware.PrintMargin.z();

  // Make Layers
  lastlayer = NULL;

  is_calculating=false;

  Slice();

  //CleanupLayers();

  MakeShells();

  if (settings.Slicing.DoInfill && 
      (settings.Slicing.SolidThickness > 0 || settings.Slicing.ShellCount > 0))
    // not bridging when support
    MakeUncoveredPolygons(settings.Slicing.MakeDecor,
			  !settings.Slicing.NoBridges && !settings.Slicing.Support);

  if (settings.Slicing.Support)
    // easier before having multiplied uncovered bottoms
    MakeSupportPolygons(settings.Slicing.SupportWiden); 


  MakeFullSkins(); // must before multiplied uncovered bottoms

  MultiplyUncoveredPolygons();

  CalcInfill();

  if (settings.Slicing.Skirt) 
    MakeSkirt();

  if (settings.RaftEnable)
    {
      printOffset += Vector3d (settings.Raft.Size, settings.Raft.Size, 0);
      MakeRaft (state, printOffsetZ); // printOffsetZ will have height of raft added
    }

  state.ResetLastWhere(Vector3d(0,0,0));
  uint count =  layers.size();

  m_progress->start (_("Making GCode"), count+1);
  
  state.AppendCommand(MILLIMETERSASUNITS,  false, _("Millimeters"));
  state.AppendCommand(ABSOLUTEPOSITIONING, false, _("Absolute Pos"));
  if (settings.Slicing.RelativeEcode)  
    state.AppendCommand(RELATIVE_ECODE, false, _("Relative E Code"));
  else
    state.AppendCommand(ABSOLUTE_ECODE, false, _("Absolute E Code"));

  bool cont = true;
  vector<Command> commands;
  Vector3d start = state.LastPosition();
  for (uint p=0; p<count; p++) {
    cont = (m_progress->update(p)) ;
    if (!cont) break;
    // cerr << "GCode layer " << (p+1) << " of " << count  
    // 	 << " offset " << printOffsetZ 
    // 	 << " have commands: " <<commands.size() 
    // 	 << " start " << start <<  endl;;
    // try {
    layers[p]->MakeGcode (start,
			  commands,
			  printOffsetZ,
			  settings);
    // } catch (Glib::Error e) {
    //   error("GCode Error:", (e.what()).c_str());
    // }
    // if (layers[p]->getPrevious() != NULL)
    //   cerr << p << ": " <<layers[p]->LayerNo << " prev: " 
    // 	   << layers[p]->getPrevious()->LayerNo << endl;
  }
  
  state.AppendCommands(commands, settings.Slicing.RelativeEcode);
  if (cont)
    gcode.MakeText (GcodeTxt, GcodeStart, GcodeLayer, GcodeEnd,
		    settings.Slicing.RelativeEcode,
		    m_progress);
  else {
    ClearLayers();
    ClearGCode();
    ClearPreview();
  }

  // display whole layer if flat shapes
  // if (shapes.back()->dimensions() == 2)
  //   gcode.layerchanges.push_back(0);
  
  m_progress->stop (_("Done"));

  int h = (int)state.timeused/3600;
  int m = ((int)state.timeused%3600)/60;
  int s = ((int)state.timeused-3600*h-60*m);
  std::ostringstream ostr;
  ostr << _("Time Estimation: ") ;
  if (h>0) ostr << h <<_("h") ;
  ostr <<m <<_("m") <<s <<_("s") ;

  double gctime = gcode.GetTimeEstimation();
  if (abs(state.timeused - gctime) > 10) {
    h = (int)(gctime/3600);
    m = ((int)gctime)%3600/60;
    s = (int)(gctime)-3600*h-60*m;
    ostr << _(" / GCode Estimation: ");
    if (h>0) ostr << h <<_("h");
    ostr<< m <<_("m") << s <<_("s") ;
  }

  double totlength = gcode.GetTotalExtruded(settings.Slicing.RelativeEcode);
  ostr << _(" - total extruded: ") << totlength << "mm";
  double ccm = totlength*settings.Hardware.FilamentDiameter*settings.Hardware.FilamentDiameter/4.*M_PI/1000 ;
  ostr << " = " << ccm << "cm^3 ";
  ostr << "(ABS~" << ccm*1.08 << "g, PLA~" << ccm*1.25 << "g)"; 
  if (statusbar)
    statusbar->push(ostr.str());
  else 
    cout << ostr.str() << endl;

  is_calculating=false;
}

string Model::getSVG(int single_layer_no) const 
{
  Vector3d printOffset  = settings.Hardware.PrintMargin;
  
  ostringstream ostr;
  ostr << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>" <<endl
       << "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.0//EN\" \"http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd\">" << endl
       << "<!-- Created by Repsnapper -->" << endl
       << "<svg " << endl
       << "\txmlns:svg=\"http://www.w3.org/2000/svg\""<< endl
       << "\txmlns=\"http://www.w3.org/2000/svg\"" << endl
    //<< "\tversion=\"1.1\"" << endl
       << "\twidth=\"" << Max.x()-Min.x()+printOffset.x() 
       << "\" height=\""<< Max.y()-Min.y()+printOffset.y() << "\"" << endl
       << ">" << endl;
  if (single_layer_no<0) {
    ostr << "<g id=\"" << layers.size() << "_Layers\">" << endl;
    for (uint i = 0; i < layers.size(); i++) {
      ostr << "\t\t" << layers[i]->SVGpath() << endl;
    }
  } else {
    ostr << "<g id=\"" << "Layer_" << single_layer_no 
	 << "_of_" <<layers.size() << "\">" << endl;
    ostr << "\t\t" << layers[single_layer_no]->SVGpath() << endl;
  }
  ostr << "</g>" << endl;
  ostr << "</svg>" << endl;
  return ostr.str();
}

void Model::SliceToSVG(Glib::RefPtr<Gio::File> file, bool single_layer) 
{
  if (is_calculating) return;
  lastlayer = NULL;
  Slice();
  m_progress->stop (_("Done"));
  if (!single_layer) {
    Glib::file_set_contents (file->get_path(), getSVG());
  }
  else {
    uint n_layers = layers.size();
    m_progress->start (_("Saving Files"),n_layers);
    uint digits = log10(n_layers)+1;
    string base = file->get_path();
    ostringstream ostr;
    for (uint i = 0; i < n_layers; i++) {
      ostr.str("");
      ostr << base;
      uint nzero = (uint)(digits - log10(i+1));
      if (i==0) nzero = digits-1;
      for (uint d = 0; d < nzero; d++) 
	ostr << "0";
      ostr << i 
	   << ".svg";
      if (!m_progress->update(i)) break;
      Glib::file_set_contents (ostr.str(), getSVG(i));
    }
    m_progress->stop (_("Done"));
  }
}

