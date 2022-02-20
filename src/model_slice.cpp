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

#define  MODEL_IMPLEMENTATION
#include <vector>
#include <string>
#include <cerrno>
#include <functional>
//#include <memory>

// should move to platform.h with com port fun.
#include <sys/types.h>
#include <dirent.h>

#include <QDir>

#include "stdafx.h"
#include "model.h"
#include "settings.h"
#include "ui/progress.h"
#include "slicer/layer.h"
#include "slicer/infill.h"
#include "slicer/clipping.h"

#ifdef _OPENMP
#include <omp.h>
#endif


double Model::MakeRaft()
{
  if (layers.size() == 0) return 0;
  vector<Poly> raftpolys =
    Clipping::getOffset(layers[0]->GetHullPolygon(),
            settings->get_double("Raft/Size"), jround);
  double layerthickness =settings->get_double("Slicing/LayerThickness");

  for (uint i = 0; i< raftpolys.size(); i++)
    raftpolys[i].cleanup(layerthickness/4);

  vector<Layer*> raft_layers;

  double basethickness =
    layerthickness * settings->get_double("Raft/Base_Thickness");
  double interthickness =
    layerthickness * settings->get_double("Raft/Interface.Thickness");

  double totalthickness = settings->get_integer("Raft/Base.LayerCount") * basethickness
    + settings->get_double("Raft/Interface.LayerCount") * interthickness;

  double raft_z = -totalthickness + basethickness
    * settings->get_double("Slicing/FirstLayerHeight");

  Infill * raftInfill = new Infill
          (ParallelInfill,
           settings->get_double("Raft/Base.Distance"),
           settings->get_double("Raft/Base.MaterialDistanceRatio"),
           settings->get_double("Raft/Base.Rotation"),
           settings->get_double("Raft/Base.RotationPrLayer")*M_PI/180);

  for (int i = 0; i < settings->get_integer("Raft/Base.LayerCount"); i++) {
    Layer * layer = new Layer(lastlayer,
                              -settings->get_integer("Raft/Interface.LayerCount")
                              -settings->get_integer("Raft/Base.LayerCount") + i,
                              basethickness, 1);
    layer->setMinMax(raftpolys);
    layer->setZ(raft_z);
    layer->addToInfill(raftInfill, raftpolys);
    raft_layers.push_back(layer);
    lastlayer = layer;
    raft_z += basethickness;
  }
  Infill * ifaceInfill = new Infill
          (ParallelInfill,
           settings->get_double("Raft/Interface.Distance"),
           settings->get_double("Raft/Interface.MaterialDistanceRatio"),
           settings->get_double("Raft/Interface.Rotation"),
           settings->get_double("Raft/Interface.RotationPrLayer")*M_PI/180);

  int if_layers = settings->get_integer("Raft/Interface.LayerCount");
  for (int i = 0; i < if_layers; i++) {
    Layer * layer = new Layer(lastlayer,
                              -settings->get_integer("Raft/Base.LayerCount") + i,
                              interthickness, 1);
    layer->setMinMax(raftpolys);
    layer->setZ(raft_z);
    layer->addToInfill(ifaceInfill, raftpolys);
    raft_layers.push_back(layer);
    lastlayer = layer;
    raft_z += interthickness;
  }
  layers.insert(layers.begin(),raft_layers.begin(),raft_layers.end());
  return totalthickness;
}

#if 0
// old raft
void Model::MakeRaft(GCodeState &state, double &z)
{
  vector<Intersection> HitsBuffer;

  double raftSize = settings->Raft.Size;
  Vector3d raftMin =  settings->Hardware.PrintMargin + Min;
  Vector3d raftMax =  settings->Hardware.PrintMargin + Max + 2 * raftSize;
  Vector2d Center = Vector2d((raftMin.x + raftMax.x) / 2,
                 (raftMin.y + raftMax.y) / 2);

  // bbox of object
  double Length = (std::max(raftMax.x,raftMax.y) -
           std::min(raftMin.x, raftMin.y))/sqrt(2.0);

  double rot;
  uint LayerNr = 0;
  uint layerCount = settings->Raft.Phase[0].LayerCount +
            settings->Raft.Phase[1].LayerCount;
  Settings::RaftSettings::PhasePropertiesType *props = &settings->Raft.Phase[0];

  double thickness = props->Thickness * settings->Hardware.LayerThickness;
  double extrusionfactor = settings->Hardware.GetExtrudeFactor(thickness)
    * props->MaterialDistanceRatio;


  while(LayerNr < layerCount)
    {
      // If we finished phase 0, start phase 1 of the raft...
      if (LayerNr >= settings->Raft.Phase[0].LayerCount)
    props = &settings->Raft.Phase[1];

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
                   settings->Hardware.MaxPrintSpeedXY * 60,
                   extrusionfactor, 0,
                   z,
                   settings->Slicing, settings->Hardware);
      reverseLines = !reverseLines;
    }
      // Set startspeed for Z-move
      Command g;
      g.Code = SETSPEED;
      g.where = Vector3d(P2.x, P2.y, z);
      g.f=settings->Hardware.MinPrintSpeedZ * 60;
      g.comment = "Move Z";
      g.e = 0;
      gcode.commands.push_back(g);
      z += thickness;

      // Move Z
      g.Code = ZMOVE;
      g.where = Vector3d(P2.x, P2.y, z);
      g.f = settings->Hardware.MinPrintSpeedZ * 60;
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
  ulong count = layers.size();
  if (count == 0) return;
  if(!m_progress->restart (_("Cleanup"), count)) return;
  ulong progress_steps=max<ulong>(1, count/20);

#ifdef _OPENMP
  #pragma omp parallel for schedule(dynamic)
#endif
  for (ulong i=0; i < count; i++) {
      if (!m_progress->do_continue)
#ifdef _OPENMP
          continue;
#else
          break;
#endif
      if (i%progress_steps==0) {
          m_progress->emit update_signal(i);
      }
      layers[i]->cleanupPolygons();
  }
}


bool layersort(const Layer * l1, const Layer * l2){
  return (l1->Z < l2->Z);
}

void Model::Slice()
{
  vector<Shape*> shapes;
//  vector<Matrix4d> transforms;

  if (settings->get_boolean("Slicing/SelectedOnly"))
      shapes = objectList.get_selected_shapes(main->getSelectedIndexes());
  else
      shapes = objectList.get_all_shapes();

  if (shapes.size() == 0) return;

//  assert(shapes.size() == transforms.size());

  CalcBoundingBoxAndCenter(settings->get_boolean("Slicing/SelectedOnly"));

//  for (uint i = 0; i<transforms.size(); i++)
//    transforms[i] = settings->getBasicTransformation(transforms[i]);

  Matrix4d basicTrans = settings->getBasicTransformation(Matrix4d::IDENTITY);

  uint LayerNr = 0;
  bool varSlicing = settings->get_boolean("Slicing/Varslicing");

  uint max_skins = uint(max(1, settings->get_integer("Slicing/Skins")));
  double thickness = settings->get_double("Slicing/LayerThickness");
  double skin_thickness = thickness / max_skins;
  uint skins = max_skins; // probably variable

  // - Start at z~=0, cut off everything below
  // - Offset it a bit in Z, z = 0 gives a empty slice because no triangle crosses this Z value
  double minZ = thickness * settings->get_double("Slicing/FirstLayerHeight");// + Min.z;
  Vector3d volume = settings->getPrintVolume();
  double maxZ = min(Max.z(), volume.z() - settings->getPrintMargin().z());

  double max_gradient = 0;
  double supportangle = settings->get_double("Slicing/SupportAngle")*M_PI/180.;
  if (!settings->get_boolean("Slicing/Support")) supportangle = -1;

  m_progress->set_terminal_output(settings->get_boolean("Display/TerminalProgress"));
  if (!m_progress->restart (_("Slicing"), maxZ)) return;
  // for (vector<Layer *>::iterator pIt = layers.begin();
  //      pIt != layers. end(); pIt++)
  //   delete *pIt;
  ClearLayers();

  bool flatshapes = shapes.front()->dimensions() == 2;
  if (flatshapes) {
    layers.resize(1);
    layers[0] = new Layer(lastlayer, 0, thickness  , 1);
    lastlayer = layers[0];
    layers[0]->setZ(0); // set to real z
    for (uint nshape= 0; nshape < shapes.size(); nshape++) {
      layers[0]->addShape(basicTrans, *shapes[nshape], max_gradient, -1);
    }
    return;
  }

  uint progress_steps = uint(max(1., maxZ/thickness/20));

  if ((varSlicing && skins > 1) ||
          (settings->get_boolean("Slicing/BuildSerial") && shapes.size() > 1)) {
      // have skins and/or serial build, so can't parallelise
      uint currentshape   = 0;
      double serialheight = maxZ; // settings.Slicing.SerialBuildHeight;
      double z            = minZ;
      double shape_z      = z;
      double max_shape_z  = z + serialheight;
      Layer * layer = new Layer(lastlayer, int(LayerNr), thickness, 1); // first layer no skins
      layer->setZ(shape_z);
      LayerNr = 1;
      int new_polys=0;
      bool cont = true;
      while(cont && z < maxZ)
      {
          shape_z = z;
          max_shape_z = min(shape_z + serialheight, maxZ);
          while ( cont && currentshape < shapes.size() && shape_z <= max_shape_z ) {
              if (LayerNr%progress_steps==0)
                  m_progress->emit update_signal(shape_z);
              if (!m_progress->do_continue) break;
              layer->setZ(shape_z); // set to real z
              if (abs(shape_z - minZ) < 0.001) { // the layer is on the platform
                  layer->LayerNo = 0;
                  layer->setSkins(1);
                  LayerNr = 1;
              }
              new_polys = layer->addShape(basicTrans, *shapes[currentshape],
                                          max_gradient, supportangle);
              // cerr << "Z="<<z<<", shapez="<< shape_z << ", shape "<<currentshape
              //      << " of "<< shapes.size()<< " polys:" << new_polys<<endl;
              if (shape_z >= max_shape_z) { // next shape, reset z
                  currentshape++;
                  shape_z = z;
              } else {  // next z, same shape
                  if (varSlicing && LayerNr!=0) {
                      // higher gradient -> slice thinner with fewer skin divisions
                      skins = max_skins-uint(max_skins* max_gradient);
                      thickness = skin_thickness*skins;
                  }
                  shape_z += thickness;
                  max_gradient = 0;
                  if (new_polys > -1){
                      layers.push_back(layer);
                      lastlayer = layer;
                      layer = new Layer(lastlayer, int(LayerNr++), thickness, skins);
                  }
              }
          }
          //thickness = max_thickness-(max_thickness-min_thickness)*max_gradient;
          if (currentshape < shapes.size()-1) { // reached max_shape_z, next shape
              currentshape++;
          } else { // end of shapes
              if (new_polys > -1){
                  if (varSlicing) {
                      skins = max_skins-uint(max_skins* max_gradient);
                      thickness = skin_thickness*skins;
                  }
                  layers.push_back(layer);
                  lastlayer = layer;
                  layer = new Layer(lastlayer, int(LayerNr++), thickness, skins);
              }
              z = max_shape_z + thickness;
              currentshape = 0; // all shapes again
          }
          max_gradient=0;
          //cerr << "    Z="<<z << "Max.z="<<Max.z<<endl;
      }
      delete layer; // have made one more than needed
      return;
  }

  // simple case, can do multihreading

  uint num_layers = uint(ceil((maxZ - minZ) / thickness));
  layers.resize(num_layers);
  uint nlayer;

#ifdef _OPENMP
  omp_lock_t progress_lock;
  omp_init_lock(&progress_lock);
#pragma omp parallel for schedule(dynamic)
#endif
  for (nlayer = 0; nlayer < num_layers; nlayer++) {
      double z = minZ + thickness * nlayer;
      if (nlayer%progress_steps==0) {
          m_progress->emit update_signal(z);
      }
      if (!m_progress->do_continue)
#ifdef _OPENMP
          continue;
#else
          break;
#endif
    Layer * layer = new Layer(NULL, int(nlayer), thickness, nlayer>0?skins:1);
    layer->setZ(z); // set to real z
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic)
#endif
    for (ulong nshape= 0; nshape < shapes.size(); nshape++) {
      layer->addShape(basicTrans, *shapes[nshape],
                      max_gradient, supportangle);
    }
    layers[nlayer] = layer;
  }
  if (!m_progress->do_continue)
    ClearLayers();

#ifdef _OPENMP
    //std::sort(layers.begin(), layers.end(), layersort);
#endif

  for (uint nlayer = 1; nlayer < layers.size(); nlayer++) {
    layers[nlayer]->setPrevious(layers[nlayer-1]);
    assert(layers[nlayer]->Z > layers[nlayer-1]->Z);
  }
  if (layers.size()>0)
    lastlayer = layers.back();

  // shapes.clear();
  //m_progress->stop (_("Done"));
}

void Model::MakeFullSkins()
{
  // not bottom layer

  if(!m_progress->restart (_("Skins"), layers.size())) return;
  uint progress_steps=max<uint>(1,uint(layers.size()/20));
  ulong count = layers.size();
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic) //ordered
#endif
  for (ulong i=1; i < count; i++) {
    if (i%progress_steps==0)
        m_progress->emit update_signal(i);
    if (!m_progress->do_continue)
#ifdef _OPENMP
        continue;
#else
        break;
#endif
    layers[i]->makeSkinPolygons();
  }
  //m_progress->stop (_("Done"));
}


void Model::MakeUncoveredPolygons(bool make_decor, bool make_bridges, bool fillbottom)
{
  uint nLayers = uint(layers.size());
  if (nLayers == 0) return;
  if (!m_progress->restart (_("Find Uncovered"), 2*nLayers+2)) return;
  uint progress_steps=max<uint>(1,uint((2*nLayers+2)/20));
  // bottom to top: uncovered from above -> top polys
  for (uint i = 0; i < nLayers-1; i++)
    {
      if (i%progress_steps==0) m_progress->emit update_signal(i);
      if (!m_progress->do_continue) return;
      layers[i]->addFullPolygons(GetUncoveredPolygons(layers[i],layers[i+1]), make_decor);
    }
  // top to bottom: uncovered from below -> bridge polys
  for (uint i = nLayers-1; i > 0; i--)
    {
      //cerr << "layer " << i << endl;
      if (i%progress_steps==0) m_progress->emit update_signal(nLayers + nLayers - i);
      if (!m_progress->do_continue) return;
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
  m_progress->emit update_signal(2*nLayers+1);
  if (fillbottom)
      layers.front()->addFullPolygons(layers.front()->GetFillPolygons(), make_decor);
  m_progress->emit update_signal(2*nLayers+2);
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
  clipp.addPolys(cliplayer->GetInnerShell(),       clip); // have some more overlap
  vector<Poly> uncovered = clipp.subtractMerged();
  return uncovered;
}

void Model::MultiplyUncoveredPolygons()
{
  if (!settings->get_boolean("Slicing/DoInfill") &&
      settings->get_double("Slicing/SolidThickness") == 0.0) return;
  if (settings->get_boolean("Slicing/NoTopAndBottom")) return;
  uint shells = uint(ceil(settings->get_double("Slicing/SolidThickness")/
                        settings->get_double("Slicing/LayerThickness")));
  shells = max(shells, uint(settings->get_integer("Slicing/ShellCount")));
  if (shells<1) return;
  ulong count = layers.size();

  uint numdecor = 0;
  // add another full layer if making decor
  if (settings->get_boolean("Slicing/MakeDecor"))
      numdecor = uint(settings->get_integer("Slicing/DecorLayers"));
  shells += numdecor;

  if (!m_progress->restart (_("Uncovered Shells"), count*3)) return;
  uint progress_steps=max<uint>(1,uint(count*3/20));
  // bottom-up: mulitply downwards
  ulong i,s;
  for (i=0; i < count; i++)
    {
      if (i%progress_steps==0) m_progress->emit update_signal(i);
      if (!m_progress->do_continue)  return;
      // (brigdepolys are not multiplied downwards)
      const vector<Poly> &fullpolys     = layers[i]->GetFullFillPolygons();
      const vector<Poly> &skinfullpolys = layers[i]->GetSkinFullPolygons();
      const vector<Poly> &decorpolys    = layers[i]->GetDecorPolygons();
      for (s=1; s < shells; s++)
          if (i > s+1) {
              layers[i-s]->addFullPolygons (fullpolys,     false);
              layers[i-s]->addFullPolygons (skinfullpolys, false);
              layers[i-s]->addFullPolygons (decorpolys,    s < numdecor);
          }
    }
  // top-down: mulitply upwards
  for (int i=int(count)-1; i>=0 ; i--)
    {
        if (i%progress_steps==0) m_progress->emit update_signal(count + count -i);
        if (!m_progress->do_continue)  return;
        const vector<Poly>   &fullpolys     = layers[i]->GetFullFillPolygons();
        const vector<ExPoly> &bridgepolys   = layers[i]->GetBridgePolygons();
        const vector<Poly>   &skinfullpolys = layers[i]->GetSkinFullPolygons();
        const vector<Poly>   &decorpolys    = layers[i]->GetDecorPolygons();
        for (uint s=1; s < shells; s++)
            if (i+s < count){
                layers[i+s]->addFullPolygons (fullpolys,     false);
                layers[i+s]->addFullPolygons (bridgepolys,   false);
                layers[i+s]->addFullPolygons (skinfullpolys, false);
                layers[i+s]->addFullPolygons (decorpolys,    s < numdecor);
            }
    }

  m_progress->set_label(_("Merging Full Polygons"));
  // merge results
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic)
#endif
  for (i=0; i < count; i++)
    {
      if (i%progress_steps==0) {
          m_progress->emit update_signal(count + count +i);
      }
      if (!m_progress->do_continue)
#ifdef _OPENMP
          continue;
#else
          break;
#endif
      layers[i]->mergeFullPolygons(false);
    }
}


void Model::MakeSupportPolygons(Layer * layer, // lower -> will change
                const Layer * layerabove,  // upper
                uint extruder, double widen)
{
  const double distance =
    settings->GetExtrudedMaterialWidth(layer->thickness, extruder);
  // vector<Poly> tosupport = Clipping::getOffset(layerabove->GetToSupportPolygons(),
  //  					       distance/2.);
  //vector<Poly> tosupport = Clipping::getMerged(layerabove->GetToSupportPolygons(),
  // 					       distance);
  vector<Poly> tosupport = layerabove->GetToSupportPolygons();

  Clipping clipp;
  clipp.addPolys(layerabove->GetSupportPolygons(),  subject);
  clipp.addPolys(tosupport,                         subject);
  clipp.addPolys(layer->GetPolygons(),              clip);
  clipp.setZ(layer->getZ());

  vector<Poly> spolys = clipp.subtract(CL::pftNonZero,CL::pftEvenOdd);

  if (widen != 0.) // widen from layer to layer
    spolys = clipp.getOffset(spolys, widen * layer->thickness);

  spolys = clipp.getMerged(spolys,distance);

  layer->setSupportPolygons(spolys);
}

void Model::MakeSupportPolygons(uint extruder, double widen)
{
  ulong count = layers.size();
  if (count==0) return;
  if (!m_progress->restart (_("Support"), count*2)) return;
  uint progress_steps=max<uint>(1,uint(count*2/20));

  for (ulong i=count-1; i>0; i--) {
      if (i%progress_steps==0) m_progress->emit update_signal(count-i);
      if (!m_progress->do_continue)  return;
      if (layers[i]->LayerNo == 0) continue;
      MakeSupportPolygons(layers[i-1], layers[i], extruder, widen);
  }

  // // shrink a bit
  // Clipping clipp;
  // for (int i=0; i<count; i++)
  //   {
  //     const double distance =
  // 	settings.Hardware.GetExtrudedMaterialWidth(layers[i]->thickness);
  //     if (i%progress_steps==0) if(!m_progress->update(i+count)) return;
  //     vector<Poly> offset =
  // 	Clipping::getOffset(layers[i]->GetSupportPolygons(), -distance);
  //     layers[i]->setSupportPolygons(offset);
  //   }

  //  m_progress->stop (_("Done"));
}

void Model::MakeSkirt()
{
  if (!settings->get_boolean("Slicing/Skirt")) return;
  if (settings->get_boolean("Slicing/Support")) return;
  double skirtdistance  = settings->get_double("Slicing/SkirtDistance");

  Clipping clipp;
  size_t count = layers.size();
  size_t endindex = 0;
  // find maximum of all calculated skirts
  clipp.clear();
  double skirtheight = settings->get_double("Slicing/SkirtHeight");
  bool singleskirt   = settings->get_boolean("Slicing/SingleSkirt");
  for (size_t i=0; i < count; i++)
  {
      if (layers[i]->getZ() > skirtheight)
          break;
      layers[i]->MakeSkirt(skirtdistance, singleskirt);
      vector<Poly> sp = layers[i]->GetSkirtPolygons();
      clipp.addPolys(sp,subject);
      endindex = i;
  }
  vector<Poly> skirts = clipp.unite(CL::pftPositive,CL::pftPositive);
  // set this skirt for all skirted layers
  if (skirts.size()>0)
    for (size_t i=0; i<=endindex; i++) {
      layers[i]->setSkirtPolygons(skirts);
  }
}

void Model::MakeShells()
{
  ulong count = layers.size();
  if (count == 0) return;
  if (!m_progress->restart (_("Shells"), count)) return;
  uint progress_steps=max<uint>(1,uint(count/20));
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic)
#endif
  for (ulong i=0;  i < count; i++)
    {
      if (i%progress_steps==0) {
          m_progress->emit update_signal(i);
      }
      if (!m_progress->do_continue)
#ifdef _OPENMP
          continue;
#else
          break;
#endif
      layers[i]->MakeShells(*settings, 0);
    }

  m_progress->emit update_signal(count);
  //m_progress->stop (_("Done"));
}


void Model::CalcInfill()
{
  if (!settings->get_boolean("Slicing/DoInfill") &&
      settings->get_double("Slicing/SolidThickness") == 0.0) return;

  ulong count = layers.size();
  if (!m_progress->restart (_("Infill"), count)) return;
  uint progress_steps=max<uint>(1,uint(count/20));
  //cerr << "make infill"<< endl;
  uint done = 0;

  Vector3d printOffset  = settings->getPrintMargin();

  InfillSet infills(*settings,
                    (Min+printOffset).get_sub_vector<2>(0),
                    (Max+printOffset).get_sub_vector<2>(0));
  uint firstLayers = uint(settings->get_integer("Slicing/FirstLayersNum"));
  uint altinfill = uint(settings->get_integer("Slicing/AltInfillLayers"));

#ifdef _OPENMP
  int maxt = max(1, omp_get_max_threads()-2);
#pragma omp parallel for schedule(dynamic) num_threads(maxt)
#endif
  for (ulong i=0; i < count ; i++)
    {
      //cerr << "thread " << omp_get_thread_num() << endl;
      if (done%progress_steps == 0){
          m_progress->emit update_signal(done);
      }
      if (!m_progress->do_continue)
#ifdef _OPENMP
          continue;
#else
          break;
#endif
      layers[i]->CalcInfill(*settings, infills, false,
                            altinfill > 0 && i % altinfill == 0,
                            i < firstLayers);
//      cerr << i << ": "<< layers[i]->info() << endl;
      done++;
    }
  //m_progress->stop (_("Done"));
}


void Model::ConvertToGCode()
{
  if (is_calculating) {
    return;
  }
  is_calculating=true;

  uint currentExtruder = 0;

  QElapsedTimer start_time;
  start_time.start();

  Vector3d printOffset  = settings->getPrintMargin();
  double   printOffsetZ = printOffset.z();

  // Make Layers
  lastlayer = NULL;
  m_progress->start(_("Generating GCode"),1);

  Slice();

  //CleanupLayers();

  MakeShells();

  if (settings->get_boolean("Slicing/DoInfill") &&
          !settings->get_boolean("Slicing/NoTopAndBottom") &&
          (settings->get_double("Slicing/SolidThickness") > 0 ||
           settings->get_integer("Slicing/ShellCount") > 0))
      // not bridging when support
      MakeUncoveredPolygons(settings->get_boolean("Slicing/MakeDecor"),
                            !settings->get_boolean("Slicing/NoBridges") &&
                            !settings->get_boolean("Slicing/Support"),
                            !settings->get_boolean("Slicing/NoBottom"));

  if (settings->get_boolean("Slicing/Support"))
    // easier before having multiplied uncovered bottoms
      MakeSupportPolygons(settings->GetSupportExtruder(),
                          settings->get_double("Slicing/SupportWiden"));

  MakeFullSkins(); // must before multiplied uncovered bottoms

  MultiplyUncoveredPolygons();

  bool makeRaft = settings->get_boolean("Raft/Enable");
  if (!makeRaft && settings->get_boolean("Slicing/Skirt"))
    MakeSkirt();

  CalcInfill();

  if (makeRaft)
    {
      printOffset += Vector3d (settings->get_double("Raft/Size"), 0);
      printOffsetZ += MakeRaft(); // printOffsetZ will have height of raft added
    }

  ulong layercount = layers.size();
  if (m_progress->restart (_("Making Lines"), layercount+1)) {
      Vector3d start = Vector3d::ZERO;
      ulong progress_steps=max<ulong>(1, layercount/20);
      bool farthestStart = settings->get_boolean("Slicing/FarthestLayerStart");
      vector<PLine<3>*> pline3s;
//      vector<vector<PLine<3>*>> l_plines(layercount);
      ulong done = 0;
      for (ulong p=0; p<layercount; p++) {
          if (farthestStart && p > 0 ) {
              const Vector2d fartheststart = layers[p]->getFarthestPolygonPoint(start);
              start.set(fartheststart.x(), fartheststart.y());
          } else {
              //              Vector2d randstart = layers[p]->getRandomPolygonPoint();
              //              start.set(randstart.x(), randstart.y());
          }
          Vector2d start2 = Vector2d(start.x(), start.y());
          Printlines * printlines =
                  layers[p]->MakePrintlines(start, pline3s,
                                            printOffsetZ, *settings);
//          cerr <<  "Z " << layers[p]->Z <<  " = " << printlines->getZ() << endl;
          layers[p]->makePrintLines3(start2, printlines, pline3s, settings);
          delete printlines;
//           if (layers[p]->getPrevious() != NULL)
//             cerr << p << ": " <<layers[p]->LayerNo << " prev: "
//               << layers[p]->getPrevious()->LayerNo << endl;
          done++;
          if (done%progress_steps==0) {
              m_progress->emit update_signal(done);
          }
          if (!m_progress->do_continue)
              break;
      }
      Printlines::makeAntioozeRetract(pline3s, settings);

      gcode->clear();
      GCodeState state(*gcode);
      state.AppendCommand(MILLIMETERSASUNITS,  false, _("Millimeters"));
      state.AppendCommand(ABSOLUTEPOSITIONING, false, _("Absolute Pos"));
      if (settings->get_boolean("Slicing/RelativeEcode"))
          state.AppendCommand(RELATIVE_ECODE, false, _("Relative E Code"));
      else
          state.AppendCommand(ABSOLUTE_ECODE, false, _("Absolute E Code"));
      Printlines::toCommands(pline3s, settings, state, nullptr);
      gcode->findLayerChanges();
      pline3s.clear();
//      state.AppendCommands(commands, settings.Slicing.RelativeEcode);
  }

  QCoreApplication::processEvents();


  // display whole layer if flat shapes
  // if (shapes.back()->dimensions() == 2)
  //   gcode.layerchanges.push_back(0);


  gcode->calcTimeEstimation(Vector3d::ZERO);
  uint h = uint(gcode->totalTime/3600);
  uint m = gcode->totalTime%3600/60;
  uint s = uint(gcode->totalTime-3600*h-60*m);
  ostringstream ostr;
  ostr << _("GCode Estimation: ");
  if (h>0) ostr << h <<_(" h ");
  ostr << m <<_(" m ") << s <<_(" s ") ;

  ostr.setf( std::ios::fixed, std:: ios::floatfield );
  ostr.precision(1);
  double totlength = gcode->GetTotalExtruded(settings->get_boolean("Slicing/RelativeEcode"));
  ostr << _("- total extruded: ") << totlength << "mm";
  // TODO: ths assumes all extruders use the same filament diameter
  const double diam = settings->get_double(Settings::numbered("Extruder",currentExtruder)+
                                           "/FilamentDiameter");
  const double ccm = totlength * diam * diam / 4. * M_PI / 1000 ;
  ostr << " = " << ccm << "cm³ ";
  ostr << "(ABS~" << ccm*1.08 << "g, PLA~" << ccm*1.25 << "g, PETG~" << ccm*1.27 << "g)";
  if (statusbar)
    statusbar->showMessage(QString::fromStdString(ostr.str()));
  cout << ostr.str() << endl;

  const int time_used = start_time.elapsed()/1000; // seconds
  cerr << "GCode generated in " << time_used << " seconds. ";

  gcode->emit gcode_changed();

  m_progress->stop(_("Done"));
  is_calculating=false;
}

/*
string Model::getSVG(int single_layer_no)
{
  Vector3d printOffset  = settings->getPrintMargin();

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
  if (single_layer_no < 0) { // -> all layers
    ostr << "<g id=\"" << layers.size() << "_Layers\">" << endl;
    for (uint i = 0; i < layers.size(); i++) {
      ostr << "\t\t" << layers[i]->SVGpath() << endl;
    }
  } else {
    ostr << "<g id=\"" << "Layer_" << single_layer_no
     << "_of_" <<layers.size() << "\">" << endl;
    ostr << "\t\t" << layers[uint(single_layer_no)]->SVGpath() << endl;
  }
  ostr << "</g>" << endl;
  ostr << "</svg>" << endl;
  return ostr.str();
}

void Model::SliceToSVG(QFile *file, bool single_layer)
{
  if (is_calculating) return;
  is_calculating=true;

  lastlayer = NULL;
  Slice();
  m_progress->stop (_("Done"));

  QFileInfo finfo(*file);
  if (!single_layer) {
      QTextStream fstr(file);
      fstr<<QString::fromStdString(getSVG());
      file->close();
  }
  else {
    ulong n_layers = layers.size();
    if (!m_progress->restart (_("Saving Files"),n_layers)) return;
    uint digits = uint(log10(n_layers)+1);
    string base = finfo.absolutePath().toUtf8().constData();
    ostringstream ostr;
    for (uint i = 0; i < n_layers; i++) {
        ostr.str("");
        ostr << base;
        uint nzero = uint(digits - log10(i+1));
        if (i==0) nzero = digits-1;
        for (uint d = 0; d < nzero; d++)
            ostr << "0";
        ostr << i
             << ".svg";
        if (i%10==0) m_progress->emit update_signal(i);
        if (!m_progress->do_continue)  break;
        QCoreApplication::processEvents();
        QFile sfile(QString::fromStdString(ostr.str()));
        QTextStream(&sfile) << QString::fromStdString(getSVG(int(i)));
        sfile.close();
        }
    m_progress->stop (_("Done"));
  }
  QString directory_path = finfo.dir().absolutePath();
  settings->STLPath = directory_path;
  is_calculating = false;
}

*/
