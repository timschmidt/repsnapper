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

void Model::MakeRaft(GCodeState &state, double &z)
{
  vector<InFillHit> HitsBuffer;

  uint LayerNr = 0;
  double size = settings.Raft.Size;

  Vector2d raftMin =  Vector2d(Min.x - size + printOffset.x, Min.y - size + printOffset.y);
  Vector2d raftMax =  Vector2d(Max.x + size + printOffset.x, Max.y + size + printOffset.y);

  Vector2d Center = (Vector2d(Max.x + size, Max.y + size)-Vector2d(Min.x + size, Min.y + size))/2+Vector2d(printOffset.x, printOffset.y);

  double Length = sqrt(2)*(   ((raftMax.x)>(raftMax.y)? (raftMax.x):(raftMax.y))  -  ((raftMin.x)<(raftMin.y)? (raftMin.x):(raftMin.y))  )/2.0;	// bbox of object

  double E = 0.0;
  double rot;

  while(LayerNr < settings.Raft.Phase[0].LayerCount + settings.Raft.Phase[1].LayerCount)
    {
      Settings::RaftSettings::PhasePropertiesType *props;
      props = LayerNr < settings.Raft.Phase[0].LayerCount ?
	&settings.Raft.Phase[0] : &settings.Raft.Phase[1];
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
  double z = Min.z + settings.Hardware.LayerThickness*0.5;
  double optimization = settings.Slicing.Optimization;


  //vector<CuttingPlane> planes;
  cuttingplanes.clear();

  double hackedZ;
  double hacked_layerthickness = optimization;
  bool polys_ok;
  while(z<Max.z)
    {
      CuttingPlane* plane = new CuttingPlane(LayerNr); // one plane per layer, with all objects
      m_progress.update(z/2.);
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
		slicer->CalcCuttingPlane(T, optimization, *plane);
	      }
	  }
	polys_ok = plane->MakePolygons(optimization);
	//cerr << layerno << " -- " << hackedZ << " - " << polys_ok << endl;
	hackedZ += hacked_layerthickness;
      }
      
      //      cerr << "model.Slice plane.Z="<< plane->getZ() << " vert:"<<plane->GetVertices().size() <<endl; 
      plane->setZ(z+printOffsetZ); // set back to real z
      cuttingplanes.push_back(*plane);
      z += settings.Hardware.LayerThickness;
      LayerNr++;
    }
  
  // for (uint i=0; i<planes.size();i++){
  //   cerr << i << ". ";
  //   planes[i].printinfo();
  //   vector<Poly> polys = planes[i].GetPolygons();
  //   for (uint j=0; j <polys.size(); j++){
  //     cout << " -- poly " << j << ": ";
  //     polys[j].printinfo();
  //   }
  // }
  //return planes;
}


void Model::CalcInfill(GCodeState &state)
{

  uint LayerCount = cuttingplanes.size();
    // (uint)ceil((Max.z+settings.Hardware.LayerThickness*0.5)/settings.Hardware.LayerThickness);

  vector<int> altInfillLayers;
  settings.Slicing.GetAltInfillLayers (altInfillLayers, LayerCount);

  double z;
  double printOffsetZ = settings.Hardware.PrintMargin.z;
  double E = 0.0;

  
  for (uint i=0; i <cuttingplanes.size(); i++) 
    {
      CuttingPlane plane = cuttingplanes[i];

      z = plane.getZ();
      m_progress.update(Max.z/2. + z/2.);
      // inFill
      vector<Vector2d> *infill = NULL;
      cerr << "calcinfill z="<<z<< endl;
      for (guint shell = 1; shell <= settings.Slicing.ShellCount; shell++)
	{
	  plane.ClearShrink();

	  plane.Shrink(settings.Slicing.ShrinkQuality,
		       settings.Hardware.ExtrudedMaterialWidth,
		       settings.Slicing.Optimization,
		       settings.Display.DisplayCuttingPlane,
		       false, shell);

	  if (shell < settings.Slicing.ShellCount )
	    { // no infill - just a shell ...
	      plane.MakeGcode (state, NULL /* infill */, E, z + printOffsetZ,
			       settings.Slicing, settings.Hardware);
	    }
	  else if (settings.Slicing.ShellOnly == false)
	    { // last shell => calculate infill
	      // check if this if a layer we should use the alternate infill distance on
	      double infillDistance;
	      if (settings.Slicing.SolidTopAndBottom &&
		  ( i < settings.Slicing.ShellCount ||
		    i > LayerCount-settings.Slicing.ShellCount-2 ))
		{
		  infillDistance = settings.Hardware.ExtrudedMaterialWidth*settings.Hardware.ExtrusionFactor;  // full fill for first layers (shell thickness)
		}
	      else {
			infillDistance = settings.Slicing.InfillDistance;
	      }
	      
	      if (std::find(altInfillLayers.begin(), altInfillLayers.end(), i) 
		  != altInfillLayers.end())
		infillDistance = settings.Slicing.AltInfillDistance;
	      infill = plane.CalcInFill (infillDistance,
					 settings.Slicing.InfillRotation,
					 settings.Slicing.InfillRotationPrLayer, 
					 settings.Display.DisplayDebuginFill);
	    }
	}
      // Make the last shell GCode from the plane and the infill
      plane.MakeGcode (state, infill, E, z + printOffsetZ,
		       settings.Slicing, settings.Hardware);
      delete infill;
    }
}

void Model::ConvertToGCode()
{
  string GcodeTxt;
  string GcodeStart = settings.GCode.getStartText();
  string GcodeLayer = settings.GCode.getLayerText();
  string GcodeEnd = settings.GCode.getEndText();


  m_progress.start (_("Converting"), Max.z);

  GCodeState state(gcode);

  gcode.clear();

  printOffset = settings.Hardware.PrintMargin;
  double printOffsetZ = settings.Hardware.PrintMargin.z;

  if (settings.RaftEnable)
    {
      printOffset += Vector3d (settings.Raft.Size, settings.Raft.Size, 0);
      MakeRaft (state, printOffsetZ);
    }

  // Make Layers
  Slice(state, printOffsetZ);

  // cout << "##### AFTER Slice 2nd plane polygons: ";  
  // vector<Poly> polys = cuttingplanes[1].GetPolygons();
  // for (uint j=0; j <polys.size(); j++){
  //   cout << " -- poly " << j << ": ";
  //   polys[j].printinfo();
  // }
  // cout << "###########################" << endl;


  // cout <<"vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv" << endl;
  // for (uint i=0; i <cuttingplanes.size(); i++) 
  //   {
  //     CuttingPlane plane = cuttingplanes[i];
  //     cout << i << ": ";
  //     plane.printinfo();
  //     vector <Poly> polys = plane.GetPolygons();
  //     for (uint j=0; j <polys.size(); j++){
  // 	cout << " -- poly " << j << ": ";
  // 	polys[j].printinfo();
  //     }
  //   }
  // cout <<"^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^" << endl;
  CalcInfill(state);


  double AntioozeDistance = settings.Slicing.AntioozeDistance;
  if (!settings.Slicing.EnableAntiooze)
    AntioozeDistance = 0;

  gcode.MakeText (GcodeTxt, GcodeStart, GcodeLayer, GcodeEnd,
		  settings.Slicing.UseIncrementalEcode,
		  settings.Slicing.Use3DGcode,
		  AntioozeDistance,
		  settings.Slicing.AntioozeSpeed);
  m_progress.stop (_("Done"));
}



