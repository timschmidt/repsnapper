/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010  Kulitorum

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

#include "shape.h"

#include "gcode.h"
#include "printlines.h"


struct GCodeStateImpl
{
  GCode &code;
  Vector3d LastPosition;
  Command lastCommand;
  //double lastLayerZ;

  GCodeStateImpl(GCode &_code) :
    code(_code),
    LastPosition(0,0,0)
    //lastLayerZ(0)
  {}
};

GCodeState::GCodeState(GCode &code)
{
  pImpl = new GCodeStateImpl(code);
  timeused = 0;
}
GCodeState::~GCodeState()
{
  delete pImpl;
}
// void GCodeState::SetZ(double z)
// {
//   if (!isnan(z))
//     pImpl->LastPosition.z = z;
// }
const Vector3d &GCodeState::LastPosition()
{
  return pImpl->LastPosition;
}
void GCodeState::SetLastPosition(const Vector3d &v)
{
  // For some reason we get lots of -nan co-ordinates in lines[] - odd ...
  if (!isnan(v.y) && !isnan(v.x) && !isnan(v.z))
    pImpl->LastPosition = v;
}
void GCodeState::AppendCommand(Command &command, bool relativeE)
{
  if (!command.is_value) {
    Vector3d lastwhere = Vector3d(pImpl->lastCommand.where);
    if (!relativeE) 
      command.e += pImpl->lastCommand.e;
    if (command.f!=0)
      timeused += (command.where - lastwhere).length()/command.f*60;
    pImpl->lastCommand = command;
  }
  pImpl->code.commands.push_back(command);
}
void GCodeState::AppendCommand(GCodes code, bool relativeE, string comment)
{
  Command comm(code);
  comm.comment = comment;
  AppendCommand(comm, relativeE);
}
// double GCodeState::GetLastLayerZ(double curZ)
// {
//   if (pImpl->lastLayerZ <= 0)
//     pImpl->lastLayerZ = curZ;
//   return pImpl->lastLayerZ;
// }

// void GCodeState::SetLastLayerZ(double z)
// {
//   if (!isnan(z))
//     pImpl->lastLayerZ = z;
// }

void GCodeState::ResetLastWhere(Vector3d to)
{
  pImpl->lastCommand.where = to;
}
double GCodeState::DistanceFromLastTo(Vector3d here)
{
  return (pImpl->lastCommand.where - here).length();
}

double GCodeState::LastCommandF()
{
  return pImpl->lastCommand.f;
}

void GCodeState::AddLines (vector<printline> plines,
			   double extrusionfactor,
			   double offsetZ, 
			   const Settings::SlicingSettings &slicing,
			   const Settings::HardwareSettings &hardware)
{
  for (uint i=0; i < plines.size(); i++)
    MakeGCodeLine (plines[i], extrusionfactor, offsetZ, slicing, hardware);
}

void GCodeState::AddLines (vector<Vector3d> linespoints,
			   double extrusionFactor,
			   double maxspeed,
			   double maxmovespeed,
			   double offsetZ, 
			   const Settings::SlicingSettings &slicing,
			   const Settings::HardwareSettings &hardware)
{
  for (uint i=0; i < linespoints.size(); i+=2)
    {
      // MOVE to start of next line
      if(LastPosition() != linespoints[i]) 
	{
	  MakeGCodeLine (LastPosition(), linespoints[i],
			 Vector3d(0,0,0),0, 0, 0, maxmovespeed,
			 offsetZ, slicing, hardware);
	  SetLastPosition (linespoints[i]);
	} 
      // PLOT to endpoint of line 
      MakeGCodeLine (LastPosition(), linespoints[i+1], 
		     Vector3d(0,0,0),0, extrusionFactor, 0, maxspeed, 
		     offsetZ, slicing, hardware);
    SetLastPosition(linespoints[i+1]);
    }
  //SetLastLayerZ(z);
}


void GCodeState::MakeGCodeLine (printline pline,
				double extrusionfactor,
				double offsetZ, 
				const Settings::SlicingSettings &slicing,
				const Settings::HardwareSettings &hardware)
{
  if(LastPosition() != pline.from) { // then first move to pline.from
    MakeGCodeLine(LastPosition(), pline.from, 
		 Vector3d(0,0,0),0,
		 0, 0, hardware.MoveSpeed,
		 offsetZ, slicing, hardware);
    SetLastPosition(pline.from);
  }
  MakeGCodeLine(pline.from, pline.to, pline.arcIJK, pline.arc,
	       pline.extrusionfactor * extrusionfactor,
	       pline.absolute_extrusion,
	       pline.speed, 
	       offsetZ, slicing, hardware);
  SetLastPosition(pline.to);
}

void GCodeState::MakeGCodeLine (Vector3d start, Vector3d end,
				Vector3d arcIJK, short arc,
				double extrusionFactor, 
				double absolute_extrusion,
				double maxspeed,
				double offsetZ, 
				const Settings::SlicingSettings &slicing,
				const Settings::HardwareSettings &hardware)
{
   // if ((end-start).length() < 0.05)	// ignore micro moves
   //  return;

  Command command;
  command.is_value = false;

  bool relativeE = slicing.RelativeEcode;

  double minspeed = hardware.MinPrintSpeedXY;
  maxspeed = max(minspeed,maxspeed); // in case maxspeed is too low
  ResetLastWhere (start);
  command.where = end;
  double extrudedMaterial;
  if (start==end)  { // pure extrusions
    extrudedMaterial = 0;
    command.comment = _("Extrusion only ");
  } else 
    extrudedMaterial = DistanceFromLastTo(command.where)*extrusionFactor;

  if (absolute_extrusion!=0) {
    command.comment += _("Absolute Extrusion");
  }
  extrudedMaterial += absolute_extrusion;
  command.e = extrudedMaterial;
  command.f = maxspeed;
  if (arc == 0) { // make line
    command.Code = COORDINATEDMOTION;
  } else { // make arc
    if (arc==1) {
      command.Code = ARC_CW;
      command.comment = "cw arc";
    }
    else if (arc==-1) {
      command.Code = ARC_CCW;
      command.comment = "ccw arc";
    }
    else cerr << "Undefined arc direction! "<< arc << endl;
    command.arcIJK = arcIJK;
  }
  AppendCommand(command,relativeE);
}

