/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010  Kulitorum
    Copyright (C) 2011  Michael Meeks <michael.meeks@novell.com>

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

#include <glib/gutils.h>
#include <libreprap/comms.h>

#include <omp.h>

#include "stdafx.h"
#include "model.h"
#include "objtree.h"
#include "file.h"
#include "settings.h"
#include "connectview.h"

#include "slicer/layer.h"
#include "slicer/infill.h"
#include "slicer/clipping.h"


// void Model::MakeRaft() // TODO use layers with convex hull poly + infill
// {
  
// }


void Model::MakeRaft(GCodeState &state, double &z)
{
  vector<InFillHit> HitsBuffer;

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
	  InFillHit hit;
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

	  std::sort(HitsBuffer.begin(), HitsBuffer.end(), InFillHitCompareFunc);

	  P1 = HitsBuffer[0].p;
	  P2 = HitsBuffer[1].p;

	  state.MakeAcceleratedGCodeLine (Vector3d(P1.x,P1.y,z), 
					  Vector3d(P2.x,P2.y,z),
					  extrusionfactor,
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




void Model::Slice(double printOffsetZ)
{
  
  // get a linear collection of shapes
  vector<Shape*> shapes;
  vector<Matrix4d> transforms;
  for  (uint o = 0; o< objtree.Objects.size() ; o++){
    Matrix4d T = 
      settings.getBasicTransformation(objtree.GetSTLTransformationMatrix(o));
    for (uint s = 0; s < objtree.Objects[o].shapes.size(); s++)  {
      shapes.push_back(&objtree.Objects[o].shapes[s]);
      transforms.push_back(T);
    }
  }
  

  int LayerNr = 0;

  // double max_thickness = settings.Hardware.LayerThickness*1.5;
  // double min_thickness = settings.Hardware.LayerThickness/2;  
  // double thickness = (max_thickness+min_thickness)/2.;

  bool varSlicing = settings.Slicing.Varslicing;

  uint max_skins = settings.Slicing.Skins;
  double skin_thickness = settings.Hardware.LayerThickness/max_skins;
  uint skins = max_skins; 
  double thickness = skin_thickness*skins;

  // Offset it a bit in Z, z = 0 gives a empty slice because no triangle crosses this Z value
  // start at z=0, cut off everything below

  double minZ = thickness*0.5;// + Min.z; 
  double z = minZ;

  m_progress->start (_("Slicing"), Max.z);
  for (vector<Layer *>::iterator pIt = layers.begin();
       pIt != layers. end(); pIt++)
    delete *pIt;
  layers.clear();

  double serialheight = 0.;
  if (settings.Slicing.BuildSerial)
    // there will be problems with layer handling if objects at a given height
    serialheight = Max.z; // settings.Slicing.SerialBuildHeight; 
  
  uint currentshape = 0;

  double shape_z = z;
  double max_shape_z = z + serialheight;
  Layer * layer = new Layer(LayerNr, thickness, skins); 
  double max_gradient = 0;
  int new_polys=0;
  while(z < Max.z)
    {
      m_progress->update(z);	
      shape_z = z; 
      max_shape_z = min(shape_z + serialheight, Max.z); 
      while ( currentshape < shapes.size() && shape_z <= max_shape_z ) {
	layer->setZ(shape_z + printOffsetZ); // set to real z
	if (shape_z==minZ) LayerNr = 0; // these layers will not be handled als bridges etc.
	new_polys = layer->addShape(transforms[currentshape], *shapes[currentshape],
				    shape_z, settings.Slicing.Optimization,
				    max_gradient);
	// cerr << "Z="<<z<<", shapez="<< shape_z << ", shape "<<currentshape 
	//      << " of "<< shapes.size()<< " polys:" << new_polys<<endl;
	if (shape_z >= max_shape_z) { // next shape, reset z
	  currentshape++; 
	  shape_z = z;
	} else {  // next z, same shape
	  if (varSlicing) { // higher gradient -> slice thinner with fewer skin divisions
	    skins = max_skins-(uint)(max_skins* max_gradient);
	    thickness = skin_thickness*skins;
	  }
	  shape_z += thickness; 
	  max_gradient=0;
	  if (new_polys>-1){
	    layers.push_back(layer);
	    layer = new Layer(++LayerNr, thickness, skins);
	  }
	}
      }
      //thickness = max_thickness-(max_thickness-min_thickness)*max_gradient;
      if (currentshape < shapes.size()) { // reached max_shape_z, next shape
	currentshape++;
      } else {
	if (new_polys>-1){ 
	  if (varSlicing) {
	    skins = max_skins-(uint)(max_skins* max_gradient);
	    thickness = skin_thickness*skins;
	  }
	  layers.push_back(layer);
	  layer = new Layer(++LayerNr, thickness, skins);
	}
	z = max_shape_z + thickness; 
	currentshape = 0; // all shapes again
      }
      max_gradient=0;
      //cerr << "    Z="<<z << "Max.z="<<Max.z<<endl;
    }
  delete layer;
  // shapes.clear();
  //m_progress->stop (_("Done"));
}

void Model::MakeFullSkins()
{
  // not bottom layer

  m_progress->restart (_("Skins"), layers.size());
  // #pragma omp parallel for schedule(dynamic) ordered
  for (int i=1; i < (int)layers.size(); i++) {
    m_progress->update(i);
    layers[i]->makeSkinPolygons();
  }
  //m_progress->stop (_("Done"));
}


void Model::MakeUncoveredPolygons(bool make_bridges)
{
  int count = (int)layers.size();
  m_progress->restart (_("Find Uncovered"), 2*count+2);
  // bottom to top: uncovered from above -> top polys
  for (int i = 0; i < count-1; i++) 
    {
      m_progress->update(i);
      layers[i]->addFullPolygons(GetUncoveredPolygons(layers[i],layers[i+1]),false);
    }  
  // top to bottom: uncovered from below -> bridge polys
  for (uint i = count-1; i > 0; i--) 
    {
      //cerr << "layer " << i << endl;
      m_progress->update(count + count - i);
      vector<Poly> bridges = GetUncoveredPolygons(layers[i],layers[i-1]);
      //make_bridges = false;
      // no bridge on marked layers (serial build)
      bool mbridge = make_bridges && layers[i]->LayerNo != 0; 
      layers[i]->addFullPolygons(bridges, mbridge);
      if (mbridge) {
	vector<Poly> bridges_result = layers[i]->GetBridgePolygons();
	vector<double> angles = layers[i-1]->getBridgeRotations(bridges_result);
	layers[i]->setBridgeAngles(angles);
	//cerr << bridges_result.size() << " - " << angles.size() << endl;
      }
    }
  m_progress->update(2*count+1);
  layers.front()->addFullPolygons(layers.front()->GetFillPolygons(),false);
  m_progress->update(2*count+2);
  layers.back()->addFullPolygons(layers.back()->GetFillPolygons(),false);
  //m_progress->stop (_("Done"));
}

// find polys in subjlayer that are not covered by any filled polygons of cliplayer
vector<Poly> Model::GetUncoveredPolygons(Layer * subjlayer,
					 const Layer * cliplayer)
{
  Clipping clipp;
  clipp.clear();
  clipp.addPolys(subjlayer->GetFillPolygons(),subject); 
  clipp.addPolys(subjlayer->GetFullFillPolygons(),subject); 
  clipp.addPolys(cliplayer->GetInnerShell(),clip); // have some overlap
  //clipp.addPolys(cliplayer->GetFillPolygons(),clip);
  //clipp.addPolys(cliplayer->GetFullFillPolygons(),clip);
  vector<Poly> uncovered = clipp.substract();
  return uncovered;
}				 
				 
void Model::MultiplyUncoveredPolygons()
{
  int shells = (int)settings.Slicing.ShellCount;
  int count = (int)layers.size();
  m_progress->restart (_("Uncovered Shells"), count*3);
  // bottom-up: propagate downwards
  int i,s;
  for (i=0; i < count; i++) 
    {
      m_progress->update(i);
      vector<Poly> fullpolys = layers[i]->GetFullFillPolygons();
      vector<Poly> bridgepolys = layers[i]->GetBridgePolygons();
      // we removed from full when skinning, so add them too
      vector<Poly> skinfullpolys = layers[i]->GetSkinFullPolygons();
      for (s=1; s < shells; s++) 
	if (i-s > 1) {
	  layers[i-s]->addFullPolygons(fullpolys,false);
	  layers[i-s]->addFullPolygons(bridgepolys,false);
	  layers[i-s]->addFullPolygons(skinfullpolys,false);
	}
    }
  // top-down: propagate upwards
  for (int i=count-1; i>=0; i--) 
    {
      m_progress->update(count + count -i);
      vector<Poly> fullpolys = layers[i]->GetFullFillPolygons();
      vector<Poly> bridgepolys = layers[i]->GetBridgePolygons();
      vector<Poly> skinfullpolys = layers[i]->GetSkinFullPolygons();
      for (int s=1; s < shells; s++) 
	if (i+s < count){
	  layers[i+s]->addFullPolygons(fullpolys,false);
	  layers[i+s]->addFullPolygons(bridgepolys,false);
	  layers[i+s]->addFullPolygons(skinfullpolys,false);
	}
    }    
  // merge results
  for (int i=0; i < count; i++) 
    {
      m_progress->update(count + count +i);
      //layers[i]->mergeFullPolygons(true);
      layers[i]->mergeFullPolygons(false);
    }
  //m_progress->stop (_("Done"));
}


void Model::MakeSupportPolygons(Layer * subjlayer, // lower -> will change
				const Layer * cliplayer) // upper 
{
  Clipping clipp;
  clipp.clear();
  clipp.addPolys(cliplayer->GetPolygons(),subject);
  clipp.addPolys(cliplayer->GetSupportPolygons(),subject); // previous 
  clipp.addPolys(subjlayer->GetPolygons(),clip);
  // widen from layer to layer, afterwards substract enlarged shape polygons?
  //  subjlayer->setSupportPolygons(clipp.getOffset(clipp.substract(), 0.5*subjlayer->thickness));
  subjlayer->setSupportPolygons(clipp.substract());
}

void Model::MakeSupportPolygons()
{ 
  int count = layers.size();
  m_progress->restart (_("Support"), count*2);
  for (int i=count-1; i>0; i--) 
    {
      m_progress->update(count-i);
      if (layers[i]->LayerNo == 0) continue;
      MakeSupportPolygons(layers[i-1], layers[i]);
    }
  // shrink a bit
  for (int i=0; i<count; i++) 
    {
      //if (layers[i]->LayerNo == 0) continue;
      double distance = 2*settings.Hardware.GetExtrudedMaterialWidth(layers[i]->thickness);
      m_progress->update(i+count);
      vector<Poly> merged = Clipping::getMerged(layers[i]->GetSupportPolygons());
      layers[i]->setSupportPolygons(Clipping::getOffset(merged,-distance));
    }
  //  m_progress->stop (_("Done"));
}

void Model::MakeSkirt()
{
  Clipping clipp;
  uint count = layers.size();
  uint startindex = count;
  // find maximum of all calculated skirts
  clipp.clear();
  for (int i=count-1; i >= 0; i--) 
    {
      if (layers[i]->getZ() > settings.Slicing.SkirtHeight) {
	startindex = i;
	continue;
      }
      Poly sp = layers[i]->GetSkirtPolygon();
      clipp.addPoly(sp,subject);
    }
  vector<Poly> skirts = clipp.unite();
  // set this skirt for all skirted layers 
  for (int i=startindex-1; i >= 0; i--) {
    layers[i]->setSkirtPolygon(skirts[0]);
  }
}

void Model::MakeShells()
{
  int count = (int)layers.size();
  m_progress->restart (_("Shells"), count);
  double matwidth, skirtheight = settings.Slicing.SkirtHeight;
  bool makeskirt=false;

//#pragma omp parallel for schedule(dynamic) ordered
  for (int i=0; i < count; i++) 
    {
      m_progress->update(i);
      matwidth = settings.Hardware.GetExtrudedMaterialWidth(layers[i]->thickness);
      makeskirt = (layers[i]->getZ() <= skirtheight);
      layers[i]->MakeShells(settings.Slicing.ShellCount,
			    matwidth, settings.Slicing.Optimization, 
			    makeskirt, 
			    false); 
    }
  MakeSkirt();
  m_progress->update(count);
  //m_progress->stop (_("Done"));
}


void Model::CalcInfill()
{
  uint LayerCount = layers.size();
    // (uint)ceil((Max.z+settings.Hardware.LayerThickness*0.5)/settings.Hardware.LayerThickness);

  vector<int> altInfillLayers;
  settings.Slicing.GetAltInfillLayers (altInfillLayers, LayerCount);

  // for full polys/layers:
  double fullInfillDistance ;
  // normal fill:
  double infillDistance;
  double altInfillDistance;
  double infilldist;

  Infill::clearPatterns();
  m_progress->start (_("Infill"), layers.size());

  //cerr << "make infill"<< endl;
  int count = (int)layers.size();
  //#pragma omp parallel for schedule(static) ordered
  for (int i=0; i < count ; i++) 
    {
      m_progress->update(i);
      // inFill      

      fullInfillDistance = settings.Hardware.GetExtrudedMaterialWidth(layers[i]->thickness);
      infillDistance = fullInfillDistance *(1+settings.Slicing.InfillDistance);
      altInfillDistance = fullInfillDistance *(1+settings.Slicing.AltInfillDistance);

      if (std::find(altInfillLayers.begin(), altInfillLayers.end(), i) 
      	  != altInfillLayers.end())
      	infilldist = altInfillDistance;
      else
      	infilldist = infillDistance;

      layers[i]->CalcInfill(settings.Slicing.NormalFilltype,
			    settings.Slicing.FullFilltype,
			    settings.Slicing.SupportFilltype,
			    infilldist, fullInfillDistance,
			    settings.Slicing.InfillRotation,
			    settings.Slicing.InfillRotationPrLayer, 
			    settings.Slicing.ShellOnly,
			    settings.Display.DisplayDebuginFill);

    }
  //m_progress->stop (_("Done"));
}

void Model::ConvertToGCode()
{
  string GcodeTxt;
  string GcodeStart = settings.GCode.getStartText();
  string GcodeLayer = settings.GCode.getLayerText();
  string GcodeEnd = settings.GCode.getEndText();


  GCodeState state(gcode);

  gcode.clear();

  Vector3d printOffset = settings.Hardware.PrintMargin;
  double printOffsetZ = settings.Hardware.PrintMargin.z;

  is_calculating=true;

  //m_progress->start (_("Converting"), 9.);
  // m_progress->update(0.);

  if (settings.RaftEnable)
    {
      printOffset += Vector3d (settings.Raft.Size, settings.Raft.Size, 0);
      MakeRaft (state, printOffsetZ);
    }

  // m_progress->set_label (_("Slicing"));
  // m_progress->update(1.);

  // Make Layers
  Slice(printOffsetZ);
  
  // m_progress->set_label (_("Shells"));
  // m_progress->update(2.);
  MakeShells();

  // m_progress->set_label (_("Uncovered"));
  // m_progress->update(3.);
  if (settings.Slicing.SolidTopAndBottom)
    MakeUncoveredPolygons(!settings.Slicing.Support); // not bridging when support

  // m_progress->set_label (_("Support"));
  // m_progress->update(4.);

  if (settings.Slicing.Support)
    MakeSupportPolygons(); // easier before multiplied uncovered bottoms

  // m_progress->set_label (_("Tops and Bottoms"));
  // m_progress->update(5.);
  MakeFullSkins(); // must before multiplied uncovered bottoms

  //  m_progress->update(6.);
  if (settings.Slicing.SolidTopAndBottom)
    MultiplyUncoveredPolygons();

  // m_progress->set_label (_("Infill"));
  // m_progress->update(7.);
  CalcInfill();

  state.ResetLastWhere(Vector3d(0,0,0));
  uint count =  layers.size();
  // m_progress->set_label (_("GCode"));
  // m_progress->update(8.);
  m_progress->start (_("Making GCode"), count+1);
  
  state.AppendCommand(ABSOLUTEPOSITIONING, false, "Absolute Pos");

  for (uint p=0;p<count;p++){
    m_progress->update(p);
    //cerr << "\rGCode layer " << (p+1) << " of " << count  ;
    layers[p]->MakeGcode (state,
			  printOffsetZ,
			  settings.Slicing, settings.Hardware);
  }
  int h = (int)state.timeused/3600;
  int m = ((int)state.timeused%3600)/60;
  int s = ((int)state.timeused-3600*h-60*m);
  std::ostringstream ostr;
  ostr << _("Time Estimation: ") ;
  if (h>0) ostr << h <<_("h") ;
  ostr <<m <<_("m") <<s <<_("s") ;
  statusbar->push(ostr.str());//cout << ostr.str() << endl;

  double AntioozeDistance = settings.Slicing.AntioozeDistance;
  if (!settings.Slicing.EnableAntiooze)
    AntioozeDistance = 0;

  // m_progress->set_label (_("Collecting GCode"));
  // m_progress->update(9.);
  gcode.MakeText (GcodeTxt, GcodeStart, GcodeLayer, GcodeEnd,
		  settings.Slicing.UseIncrementalEcode,
		  settings.Slicing.Use3DGcode,
		  AntioozeDistance,
		  settings.Slicing.AntioozeSpeed);

  m_progress->stop (_("Done"));

  is_calculating=false;

  double time = gcode.GetTimeEstimation();
  h = (int)time/3600;
  m = ((int)time%3600)/60;
  s = ((int)time-3600*h-60*m);
  ostr.clear();
  ostr << " -- GCode Estimation: ";
  if (h>0) ostr << h <<_("h");
  ostr<< m <<_("m") << s <<_("s") ;
  double totlength = gcode.commands.back().e;
  ostr << " - total extruded: "<< totlength << "mm";
  double ccm = totlength*settings.Hardware.FilamentDiameter*settings.Hardware.FilamentDiameter/4.*M_PI/1000 ;
  ostr << " = " << ccm << "cm^3 ";
  ostr << "(ABS~" << ccm*1.08 << "g, PLA~" << ccm*1.25 << "g)"; 
  statusbar->push(ostr.str());
}

