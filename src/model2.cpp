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

void Model::MakeRaft(GCodeState &state, float &z)
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
	  if(IntersectXY(P1,P2,P3,P4,hit))	//Intersect edges of bbox
	    HitsBuffer.push_back(hit);
	  P3 = Vector2d(raftMax.x,raftMax.y);
	  //			glVertex2fv(&P3.x);
	  //			glVertex2fv(&P4.x);
	  if(IntersectXY(P1,P2,P3,P4,hit))
	    HitsBuffer.push_back(hit);
	  P4 = Vector2d(raftMax.x,raftMin.y);
	  //			glVertex2fv(&P3.x);
	  //			glVertex2fv(&P4.x);
	  if(IntersectXY(P1,P2,P3,P4,hit))
	    HitsBuffer.push_back(hit);
	  P3 = Vector2d(raftMin.x,raftMin.y);
	  //			glVertex2fv(&P3.x);
	  //			glVertex2fv(&P4.x);
	  if(IntersectXY(P1,P2,P3,P4,hit))
	    HitsBuffer.push_back(hit);
	  //			glEnd();

	  if(HitsBuffer.size() == 0)	// it can only be 2 or zero
	    continue;
	  if(HitsBuffer.size() != 2)
	    continue;

	  std::sort(HitsBuffer.begin(), HitsBuffer.end(), InFillHitCompareFunc);

	  P1 = HitsBuffer[0].p;
	  P2 = HitsBuffer[1].p;

	  state.MakeAcceleratedGCodeLine (Vector3d(P1.x,P1.y,z), Vector3d(P2.x,P2.y,z),
					  props->MaterialDistanceRatio,
					  E, z, settings.Slicing, settings.Hardware);
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

void Model::ConvertToGCode()
{
  string GcodeTxt;
  string GcodeStart = settings.GCode.getStartText();
  string GcodeLayer = settings.GCode.getLayerText();
  string GcodeEnd = settings.GCode.getEndText();

  PrintInhibitor inhibitPrint(this);
  m_progress.start (_("Converting"), Max.z);
  if (m_printing)
    {
      error (_("Complete print before converting"),
	     _("Converting to GCode while printing will abort the print"));
      return;
    }

  // Make Layers
  uint LayerNr = 0;
  uint LayerCount = (uint)ceil((Max.z+settings.Hardware.LayerThickness*0.5)/settings.Hardware.LayerThickness);

  vector<int> altInfillLayers;
  settings.Slicing.GetAltInfillLayers (altInfillLayers, LayerCount);

  printOffset = settings.Hardware.PrintMargin;

  // Offset it a bit in Z, z = 0 gives a empty slice because no triangle crosses this Z value
  float z = Min.z + settings.Hardware.LayerThickness*0.5;

  gcode.clear();
  double E = 0.0;
  GCodeState state(gcode);

  float printOffsetZ = settings.Hardware.PrintMargin.z;

  if (settings.RaftEnable)
    {
      printOffset += Vector3d (settings.Raft.Size, settings.Raft.Size, 0);
      MakeRaft (state, printOffsetZ);
    }
  while(z<Max.z)
    {
      m_progress.update(z);
      for(uint o=0;o<rfo.Objects.size();o++)
	{
	  for(uint f=0;f<rfo.Objects[o].files.size();f++)
	    {
	      Slicer* stl = &rfo.Objects[o].files[f].stl;	// Get a pointer to the object
	      Matrix4d T = rfo.GetSTLTransformationMatrix(o,f);
	      Vector3d t = T.getTranslation();
	      t+= Vector3d(settings.Hardware.PrintMargin.x+settings.Raft.Size*settings.RaftEnable, settings.Hardware.PrintMargin.y+settings.Raft.Size*settings.RaftEnable, 0);
	      T.setTranslation(t);
	      CuttingPlane plane;
	      stl->CalcCuttingPlane(z, plane, T);	// output is alot of un-connected line segments with individual vertices, describing the outline

	      float hackedZ = z;
	      while (plane.LinkSegments (hackedZ, settings.Slicing.Optimization) == false)	// If segment linking fails, re-calc a new layer close to this one, and use that.
		{ // This happens when there's triangles missing in the input STL
		  hackedZ+= 0.2 * settings.Hardware.LayerThickness;
		  stl->CalcCuttingPlane (hackedZ, plane, T);	// output is alot of un-connected line segments with individual vertices
		}

	      // inFill
	      vector<Vector2d> *infill = NULL;

	      for (guint shell = 1; shell <= settings.Slicing.ShellCount; shell++)
		{
		  plane.ClearShrink();
		  plane.SetZ (z + printOffsetZ);
		  switch( settings.Slicing.ShrinkQuality )
		    {
		    case SHRINK_FAST:
		      plane.ShrinkFast   (settings.Hardware.ExtrudedMaterialWidth, settings.Slicing.Optimization,
					  settings.Display.DisplayCuttingPlane, false, shell);
		      break;
		    case SHRINK_LOGICK:
		      plane.ShrinkLogick (settings.Hardware.ExtrudedMaterialWidth, settings.Slicing.Optimization,
					  settings.Display.DisplayCuttingPlane, shell);
		      break;
		    default:
		      g_error (_("unknown shrinking algorithm"));
		      break;
		    }

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
			  ( LayerNr < settings.Slicing.ShellCount ||
				      LayerNr > LayerCount-settings.Slicing.ShellCount-2 ))
			{
			  infillDistance = settings.Hardware.ExtrudedMaterialWidth*settings.Hardware.ExtrusionFactor;  // full fill for first layers (shell thickness)
			}
		      else {
			infillDistance = settings.Slicing.InfillDistance;
		      }

		      if (std::find(altInfillLayers.begin(), altInfillLayers.end(), LayerNr) != altInfillLayers.end())
			infillDistance = settings.Slicing.AltInfillDistance;
		      infill = plane.CalcInFill (LayerNr, infillDistance, settings.Slicing.InfillRotation,
						 settings.Slicing.InfillRotationPrLayer, settings.Display.DisplayDebuginFill);
		    }
		}
	      // Make the last shell GCode from the plane and the infill
	      plane.MakeGcode (state, infill, E, z + printOffsetZ,
			       settings.Slicing, settings.Hardware);
	      delete infill;
	    }
	}
      LayerNr++;
      z += settings.Hardware.LayerThickness;
    }

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

void Model::Home(string axis)
{
  if(m_printing)
    {
      alert(_("Can't go home while printing"));
      return;
    }
  if(axis == "X" || axis == "Y" || axis == "Z")
    {
      string buffer="G1 F";
      std::stringstream oss;
      if(axis == "Z")
	oss << settings.Hardware.MaxPrintSpeedZ;
      else
	oss << settings.Hardware.MaxPrintSpeedXY;
      buffer+= oss.str();
      SendNow(buffer);
      buffer="G1 ";
      buffer += axis;
      buffer+="-250 F";
      buffer+= oss.str();
      SendNow(buffer);
      buffer="G92 ";
      buffer += axis;
      buffer+="0";
      SendNow(buffer);	// Set this as home
      oss.str("");
      buffer="G1 ";
      buffer += axis;
      buffer+="1 F";
      buffer+= oss.str();
      SendNow(buffer);
      if(axis == "Z")
	oss << settings.Hardware.MinPrintSpeedZ;
      else
	oss << settings.Hardware.MinPrintSpeedXY;
      buffer="G1 ";
      buffer+="F";
      buffer+= oss.str();
      SendNow(buffer);	// set slow speed
      buffer="G1 ";
      buffer += axis;
      buffer+="-10 F";
      buffer+= oss.str();
      SendNow(buffer);
      buffer="G92 ";
      buffer += axis;
      buffer+="0";
      SendNow(buffer);	// Set this as home
    }
  else if(axis == "ALL")
    {
      SendNow("G28");
    }
  else
    alert(_("Home called with unknown axis"));
}

void Model::Move(string axis, double distance)
{
  if (m_printing)
    {
      alert(_("Can't move manually while printing"));
      return;
    }
  if (axis == "X" || axis == "Y" || axis == "Z")
    {
      SendNow("G91");	// relative positioning
      string buffer="G1 F";
      std::stringstream oss;
      if(axis == "Z")
	oss << settings.Hardware.MaxPrintSpeedZ;
      else
	oss << settings.Hardware.MaxPrintSpeedXY;
      buffer+= oss.str();
      SendNow(buffer);
      buffer="G1 ";
      buffer += axis;
      oss.str("");
      oss << distance;
      buffer+= oss.str();
      oss.str("");
      if(axis == "Z")
	oss << settings.Hardware.MaxPrintSpeedZ;
      else
	oss << settings.Hardware.MaxPrintSpeedXY;
      buffer+=" F"+oss.str();
      SendNow(buffer);
      SendNow("G90");	// absolute positioning
    }
  else
    alert (_("Move called with unknown axis"));
}

void Model::Goto(string axis, double position)
{
  if (m_printing)
    {
      alert (_("Can't move manually while printing"));
      return;
    }
  if(axis == "X" || axis == "Y" || axis == "Z")
    {
      string buffer="G1 F";
      std::stringstream oss;
      oss << settings.Hardware.MaxPrintSpeedXY;
      buffer+= oss.str();
      SendNow(buffer);
      buffer="G1 ";
      buffer += axis;
      oss.str("");
      oss << position;
      buffer+= oss.str();
      oss.str("");
      oss << settings.Hardware.MaxPrintSpeedXY;
      buffer+=" F"+oss.str();
      SendNow(buffer);
    }
  else
    alert (_("Goto called with unknown axis"));
}
