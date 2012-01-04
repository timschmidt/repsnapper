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

#include "stdafx.h"
#include "model.h"
#include "rfo.h"
#include "file.h"
#include "settings.h"
#include "connectview.h"

#include "slicer/cuttingplane.h"
#include "slicer/infill.h"

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

  double E = 0.0;
  double rot;
  uint LayerNr = 0;
  uint layerCount = settings.Raft.Phase[0].LayerCount +
		    settings.Raft.Phase[1].LayerCount;
  Settings::RaftSettings::PhasePropertiesType *props = &settings.Raft.Phase[0];

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
					  props->MaterialDistanceRatio,
					  E, z, 
					  settings.Slicing, settings.Hardware);
	  reverseLines = !reverseLines;
	}
      // Set startspeed for Z-move
      Command g;
      g.Code = SETSPEED;
      g.where = Vector3d(P2.x, P2.y, z);
      g.f=settings.Hardware.MinPrintSpeedZ;
      g.comment = "Move Z";
      g.e = E;
      gcode.commands.push_back(g);
      z += props->Thickness * settings.Hardware.LayerThickness;

      // Move Z
      g.Code = ZMOVE;
      g.where = Vector3d(P2.x, P2.y, z);
      g.f = settings.Hardware.MinPrintSpeedZ;
      g.comment = "Move Z";
      g.e = E;
      gcode.commands.push_back(g);

      LayerNr++;
    }

  // restore the E state
  Command gotoE;
  gotoE.Code = GOTO;
  gotoE.e = 0;
  gotoE.comment = _("Reset E for the remaining print");
  gcode.commands.push_back(gotoE);
}

void Model::Slice(GCodeState &state, double printOffsetZ)
{
  int LayerNr = 0;
  // Offset it a bit in Z, z = 0 gives a empty slice because no triangle crosses this Z value
  // start at z=0, cut off everything below
  double z = settings.Hardware.LayerThickness*0.5 ;   // + Min.z;
  double optimization = settings.Slicing.Optimization;

  m_progress.start (_("Slicing"), Max.z);

  for (vector<CuttingPlane *>::iterator pIt = cuttingplanes.begin();
       pIt != cuttingplanes. end(); pIt++)
    delete *pIt;
  cuttingplanes.clear();
  double thickness;
  double hackedZ;
  double hacked_layerthickness = optimization;
  bool polys_ok;
  while(z < Max.z)
    {
      thickness = settings.Hardware.LayerThickness;
      // one plane per layer, with all objects
      CuttingPlane * plane = new CuttingPlane(LayerNr, thickness); 
      m_progress.update(z);
      g_main_context_iteration(NULL,false);
      hackedZ = z;
      polys_ok = false;
      // try to slice all objects until polygons can be made, otherwise hack z
      while (!polys_ok){
	plane->setZ(hackedZ);
	for(uint o=0;o<rfo.Objects.size();o++)
	  {
	    for(uint f=0;f<rfo.Objects[o].files.size();f++)
	      {
		// Get a pointer to the object:
		Slicer* slicer = &rfo.Objects[o].files[f].stl;
		Matrix4d T = rfo.GetSTLTransformationMatrix(o,f);
		Vector3d t = T.getTranslation();
		t+= Vector3d(settings.Hardware.PrintMargin.x
			     +settings.Raft.Size*settings.RaftEnable, 
			     settings.Hardware.PrintMargin.y
			     +settings.Raft.Size*settings.RaftEnable, 0);
		T.setTranslation(t);
		// adding points and lines from object to plane:
		slicer->CalcCuttingPlane(T, optimization, plane);
	      }
	  }
	polys_ok = plane->MakePolygons(optimization);
	hackedZ += hacked_layerthickness;
      }
      
      plane->setZ(z+printOffsetZ); // set back to real z
      cuttingplanes.push_back(plane);
      z += thickness;
      LayerNr++;
    }
}


void Model::MakeUncoveredPolygons()
{
  CuttingPlane emptyplane(cuttingplanes.size(),settings.Hardware.LayerThickness);
  emptyplane.Clear();

  uint count = cuttingplanes.size();
  m_progress.start (_("Find Uncovered"), 2*count+2);
  // bottom to top:
  for (uint i=0; i <count-1; i++) 
    {
      m_progress.update(i);
      g_main_context_iteration(NULL,false);
      MakeUncoveredPolygons(cuttingplanes[i],cuttingplanes[i+1]);
    }  

  // top to bottom:
  for (uint i=count-1; i>1; i--) 
    {
      m_progress.update(count + count - i);
      MakeUncoveredPolygons(cuttingplanes[i],cuttingplanes[i-1]);
    }
  m_progress.update(2*count+1);
  MakeUncoveredPolygons(cuttingplanes.front(),&emptyplane);
  m_progress.update(2*count+2);
  MakeUncoveredPolygons(cuttingplanes.back(),&emptyplane);
  m_progress.stop (_("Done"));
}

// find polys in subjplane that are not covered by any filled polygons of clipplane
// -> split polys of subjplane in fully filled and normally filled
void Model::MakeUncoveredPolygons(CuttingPlane * subjplane,
				  const CuttingPlane * clipplane)
{
  ClipperLib::Polygons emptypolys;emptypolys.clear();
  ClipperLib::Clipper clpr;
  ClipperLib::Polygons subjOffsetCP =
    subjplane->getClipperPolygons(subjplane->GetOffsetPolygons()); // outer shell
  clpr.AddPolygons(subjOffsetCP,
		   ClipperLib::ptSubject);
  //clpr.AddPolygons(subjplane->getClipperPolygons(subjplane->GetFullFillPolygons()),
  //		   ClipperLib::ptSubject);
  clpr.AddPolygons(clipplane->getClipperPolygons(clipplane->GetOffsetPolygons()),
		   ClipperLib::ptClip);
  clpr.AddPolygons(clipplane->getClipperPolygons(clipplane->GetFullFillPolygons()),
   		   ClipperLib::ptClip);
  ClipperLib::Polygons uncovered;
  ClipperLib::PolyFillType filltype = ClipperLib::pftEvenOdd;
  clpr.Execute(ClipperLib::ctDifference, uncovered, filltype, ClipperLib::pftEvenOdd);
  subjplane->addFullFillPolygons(uncovered); // maybe already have some 
  subjplane->mergeFullPolygons();
  // substract full fill from normal fill:
  clpr.Clear();
  clpr.AddPolygons(subjOffsetCP, ClipperLib::ptSubject);
  clpr.AddPolygons(uncovered, ClipperLib::ptClip);
  ClipperLib::Polygons normal;
  clpr.Execute(ClipperLib::ctDifference, normal, filltype, ClipperLib::pftEvenOdd);
  subjplane->setNormalFillPolygons(normal);
}				 
				 
void Model::MultiplyUncoveredPolygons()
{
  uint shells = settings.Slicing.ShellCount;
  uint count = cuttingplanes.size();
  m_progress.start (_("Uncovered Shells"), count*3);
  // bottom-up
  for (uint i=0; i < count; i++) 
    {
      m_progress.update(i);
      ClipperLib::Polygons fullpolys = 
	cuttingplanes[i]->getClipperPolygons(cuttingplanes[i]->GetFullFillPolygons());
      for (uint s=1; s < shells; s++) 
	if (int(i-s) > 1)
	    cuttingplanes[i-s]->addFullPolygons(fullpolys);
    }    
  // top-down
  for (int i=count-1; i>=0; i--) 
    {
      m_progress.update(count + count -i);
      ClipperLib::Polygons fullpolys = 
	cuttingplanes[i]->getClipperPolygons(cuttingplanes[i]->GetFullFillPolygons());
      for (uint s=1; s < shells; s++) 
	if (i+s < count)
	    cuttingplanes[i+s]->addFullPolygons(fullpolys);
    }    
  // merge results
  for (uint i=0; i < count; i++) 
    {
      m_progress.update(count + count +i);
      cuttingplanes[i]->mergeFullPolygons();
    }
  m_progress.stop (_("Done"));
}


void Model::MakeSupportPolygons(CuttingPlane * subjplane, // lower -> will change
				const CuttingPlane * clipplane) // upper 
{
  ClipperLib::Clipper clpr;
  vector<Poly>  polysc = clipplane->GetPolygons();
  ClipperLib::Polygons clipcpolys =
    clipplane->getClipperPolygons(polysc); // outer polygons
  clpr.AddPolygons(clipcpolys, ClipperLib::ptSubject);
  clpr.AddPolygons(clipplane->getClipperPolygons(clipplane->GetSupportPolygons()),
		   ClipperLib::ptSubject);

  vector<Poly> polyss = subjplane->GetPolygons();
  clpr.AddPolygons(subjplane->getClipperPolygons(polyss),
				   ClipperLib::ptClip);

  ClipperLib::Polygons support;
  clpr.Execute(ClipperLib::ctDifference, support, 
	       ClipperLib::pftEvenOdd, ClipperLib::pftEvenOdd);
  subjplane->setSupportPolygons(support);
}

void Model::MakeSupportPolygons()
{ 
  int count = cuttingplanes.size();
  m_progress.start (_("Support"), count-1);
  for (int i=count-1; i>0; i--) 
    {
      m_progress.update(count-i);
      g_main_context_iteration(NULL,false);
      MakeSupportPolygons(cuttingplanes[i-1], cuttingplanes[i]);
    }
  m_progress.stop (_("Done"));
}

void Model::MakeSkirt()
{
  ClipperLib::Clipper clpr;
  uint count = cuttingplanes.size();
  uint startindex = count;
  // find maximum of all calculated skirts
  for (int i=count-1; i >= 0; i--) 
    {
      if (cuttingplanes[i]->getZ() > settings.Slicing.SkirtHeight) {
	startindex = i;
	continue;
      }
      clpr.AddPolygon(cuttingplanes[i]->skirtPolygon.getClipperPolygon(),
		      ClipperLib::ptSubject);
    }
  ClipperLib::Polygons emptypolys;emptypolys.clear();
  clpr.AddPolygons(emptypolys, ClipperLib::ptClip);

  ClipperLib::Polygons skirts;
  clpr.Execute(ClipperLib::ctDifference, skirts, 
	       ClipperLib::pftPositive, ClipperLib::pftPositive);
  // set this skirt for all skirted layers 
  for (int i=startindex-1; i >= 0; i--) {
    cuttingplanes[i]->setSkirtPolygon(skirts);
  }
}

void Model::MakeShells()
{
  uint count = cuttingplanes.size();
  m_progress.start (_("Shells"), count);
  double matwidth, skirtheight = settings.Slicing.SkirtHeight;
  bool makeskirt=false;
  for (uint i=0; i < count; i++) 
    {
      m_progress.update(i);
      g_main_context_iteration(NULL,false);
      //cerr << "shrink plane " << i << endl;
      matwidth = settings.Hardware.GetExtrudedMaterialWidth(cuttingplanes[i]->thickness);
      
      makeskirt = (cuttingplanes[i]->getZ() <= skirtheight);
      cuttingplanes[i]->MakeShells(settings.Slicing.ShellCount,
				   matwidth, settings.Slicing.Optimization, 
				   makeskirt, false);
    }
  MakeSkirt();
  m_progress.stop (_("Done"));
}


void Model::CalcInfill(GCodeState &state)
{

  uint LayerCount = cuttingplanes.size();
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
  m_progress.start (_("Infill"), cuttingplanes.size());

  //cerr << "make infill"<< endl;
  for (uint i=0; i <cuttingplanes.size(); i++) 
    {
      CuttingPlane * plane = cuttingplanes[i];
      m_progress.update(i);
      g_main_context_iteration(NULL,false);
      // inFill      

      fullInfillDistance = settings.Hardware.GetExtrudedMaterialWidth(plane->thickness);
      infillDistance = fullInfillDistance *(1+settings.Slicing.InfillDistance);
      altInfillDistance = fullInfillDistance *(1+settings.Slicing.AltInfillDistance);

      if (std::find(altInfillLayers.begin(), altInfillLayers.end(), i) 
      	  != altInfillLayers.end())
      	infilldist = altInfillDistance;
      else
      	infilldist = infillDistance;

      plane->CalcInfill(infilldist, fullInfillDistance,
			settings.Slicing.InfillRotation,
			settings.Slicing.InfillRotationPrLayer, 
			settings.Slicing.ShellOnly,
			settings.Display.DisplayDebuginFill);
    }
  m_progress.stop (_("Done"));
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

  if (settings.RaftEnable)
    {
      printOffset += Vector3d (settings.Raft.Size, settings.Raft.Size, 0);
      MakeRaft (state, printOffsetZ);
    }

  // Make Layers
  Slice(state, printOffsetZ);

  MakeShells();

  if (settings.Slicing.SolidTopAndBottom)
    MakeUncoveredPolygons();

  if (settings.Slicing.Support)
    MakeSupportPolygons(); // easier before multiplied uncovered bottoms

  if (settings.Slicing.SolidTopAndBottom)
    MultiplyUncoveredPolygons();

  CalcInfill(state);

  uint count =  cuttingplanes.size();
  m_progress.start (_("Making GCode"), count+1);
  double E = 0.0;
  for (uint p=0;p<count;p++){
    m_progress.update(p);
    g_main_context_iteration(NULL,false);
    //cerr << "\rGCode layer " << (p+1) << " of " << count  ;
    cuttingplanes[p]->MakeGcode (state, E,
		cuttingplanes[p]->getZ() + printOffsetZ,
		settings.Slicing, settings.Hardware);
  }
  int h = (int)state.timeused/3600;
  int m = ((int)state.timeused%3600)/60;
  int s = ((int)state.timeused-3600*h-60*m);
  std::ostringstream ostr;
  ostr << " Time to Print Estimation: " << h <<"h "<<m <<"m " <<s <<"s" ;
  cout << ostr.str() << endl;
  // ??? add this to statusbar or where else?

  double AntioozeDistance = settings.Slicing.AntioozeDistance;
  if (!settings.Slicing.EnableAntiooze)
    AntioozeDistance = 0;

  gcode.MakeText (GcodeTxt, GcodeStart, GcodeLayer, GcodeEnd,
		  settings.Slicing.UseIncrementalEcode,
		  settings.Slicing.Use3DGcode,
		  AntioozeDistance,
		  settings.Slicing.AntioozeSpeed);
  m_progress.stop (_("Done"));

  double time = gcode.GetTimeEstimation();
  int hr = (int)time/3600;
  int min = ((int)time%3600)/60;
  int sec = ((int)time-3600*hr-60*min);
  cout << "GCode Time Estimation "<< hr <<"h "<<min <<"m " <<sec <<"s" <<endl; 
  //??? to statusbar or where else?

}

