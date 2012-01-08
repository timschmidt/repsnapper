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
  if (!isnan(v.y) && !isnan(v.x)) // >= 0 && v.x >= 0)
    pImpl->LastPosition = v;
}
void GCodeState::AppendCommand(Command &command, bool incrementalE)
{
  Vector3d lastwhere = Vector3d(pImpl->lastCommand.where);
  if (incrementalE)
    command.e += pImpl->lastCommand.e;
  pImpl->lastCommand = command;
  pImpl->code.commands.push_back(command);
  if (command.f!=0)
	{
	  timeused += (command.where - lastwhere).length()/command.f*60;
	  // cerr << "last   at "<< lastwhere << " feedrate: " << command.f<< endl;
	  // cerr << "   now at "<<command.where << " time used: " << timeused << endl;
	}
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

void GCodeState::AddLines (vector<printline> lines,
			   double extrusionfactor,
			   double offsetZ, 
			   const Settings::SlicingSettings &slicing,
			   const Settings::HardwareSettings &hardware)
{
  for (uint i=0; i < lines.size(); i++)
    MakeAcceleratedGCodeLine (lines[i], extrusionfactor, offsetZ, slicing, hardware);
}

void GCodeState::AddLines (vector<Vector3d> lines,
			   double extrusionFactor,
			   double offsetZ, 
			   const Settings::SlicingSettings &slicing,
			   const Settings::HardwareSettings &hardware)
{
  for (uint i=0; i < lines.size(); i+=2)
    {
      // MOVE to start of next line
      if(LastPosition() != lines[i]) 
	{
	  MakeAcceleratedGCodeLine (LastPosition(), lines[i], 0.0, 
				    offsetZ, slicing, hardware);
	  SetLastPosition (lines[i]);
	} 
      // PLOT to endpoint of line 
      MakeAcceleratedGCodeLine (LastPosition(), lines[i+1], extrusionFactor, 
				offsetZ, slicing, hardware);
    SetLastPosition(lines[i+1]);
    }
  //SetLastLayerZ(z);
}


void GCodeState::MakeAcceleratedGCodeLine (printline pline,
					   double extrusionfactor,
					   double offsetZ, 
					   const Settings::SlicingSettings &slicing,
					   const Settings::HardwareSettings &hardware)
{
  if(LastPosition() != pline.from) {
    MakeAcceleratedGCodeLine(LastPosition(), pline.from, 0, 
			     offsetZ, slicing, hardware);
  }
  MakeAcceleratedGCodeLine(pline.from, pline.to, extrusionfactor*pline.extrusionfactor, 
			   offsetZ, slicing, hardware);
  SetLastPosition(pline.to);
}

void GCodeState::MakeAcceleratedGCodeLine (Vector3d start, Vector3d end,
					   double extrusionFactor, 
					   double offsetZ, 
					   const Settings::SlicingSettings &slicing,
					   const Settings::HardwareSettings &hardware)
{
   // if ((end-start).length() < 0.05)	// ignore micro moves
   //  return;

  Command command;

  bool incrementalE = slicing.UseIncrementalEcode;

  if (slicing.EnableAcceleration)
    {
      double len;
      ResetLastWhere (start);
	  
      Vector3d direction = end-start;
      len = direction.length();	// total distance
      direction.normalize();
	  
      // Set start
      command.Code = SETSPEED;
      command.where = start;
      command.e = 0.0;	       
      command.f = hardware.MinPrintSpeedXY;
	  
      AppendCommand(command,incrementalE);
	  
      if(len < hardware.DistanceToReachFullSpeed*2)
	{
	  // TODO: First point of acceleration is the middle of the line segment
	  command.Code = COORDINATEDMOTION;
	  command.where = (start+end)*0.5;
	  double extrudedMaterial = DistanceFromLastTo(command.where)*extrusionFactor;
	  command.e = extrudedMaterial;  
	  double lengthFactor = (len/2.0)/hardware.DistanceToReachFullSpeed;
	  command.f = (hardware.MaxPrintSpeedXY-hardware.MinPrintSpeedXY)*lengthFactor+hardware.MinPrintSpeedXY;
	  AppendCommand(command,incrementalE);
	  // And then decelerate
	  command.Code = COORDINATEDMOTION;
	  command.where = end;
	  command.e = extrudedMaterial;
	  command.f = hardware.MinPrintSpeedXY;
	  AppendCommand(command,incrementalE);
		
	}// if we will never reach full speed
      else
	{
	  // Move to max speed
	  command.Code = COORDINATEDMOTION;
	  command.where = start+(direction*hardware.DistanceToReachFullSpeed);
	  double extrudedMaterial = DistanceFromLastTo(command.where)*extrusionFactor;
	  command.e = extrudedMaterial;
	  command.f = hardware.MaxPrintSpeedXY;
	  AppendCommand(command,incrementalE);
		
	  // Sustian speed till deacceleration starts
	  command.Code = COORDINATEDMOTION;
	  command.where = end-(direction*hardware.DistanceToReachFullSpeed);
	  extrudedMaterial = DistanceFromLastTo(command.where)*extrusionFactor;
	  command.e = extrudedMaterial;
	  command.f = hardware.MaxPrintSpeedXY;
	  AppendCommand(command,incrementalE);
		
	  // deacceleration untill end
	  command.Code = COORDINATEDMOTION;
	  command.where = end;
	  extrudedMaterial = DistanceFromLastTo(command.where)*extrusionFactor;
	  command.e = extrudedMaterial;
	  command.f = hardware.MinPrintSpeedXY;
	  AppendCommand(command,incrementalE);
	} // if we will reach full speed
    }
  else	// No accleration
    {
      // Make a accelerated line from lastCommand.where to lines[thisPoint]
      ResetLastWhere (start);
      if (slicing.Use3DGcode)
	{
	  command.Code = EXTRUDEROFF;
	  AppendCommand(command,incrementalE);
	  double speed = hardware.MinPrintSpeedXY;
	  command.Code = COORDINATEDMOTION3D;
	  command.where = start;
	  command.e = 0;	    
	  command.f = speed;
	  AppendCommand(command,incrementalE);
		
	  command.Code = EXTRUDERON;
	  AppendCommand(command,incrementalE);
		
	  speed = hardware.MinPrintSpeedXY;
	  command.Code = COORDINATEDMOTION3D;
	  command.where = end;
	  command.e = 0;    
	  command.f = speed;
	  AppendCommand(command,incrementalE);
	  // Done, switch extruder off
	  command.Code = EXTRUDEROFF;
	  AppendCommand(command,incrementalE);
	}
      else	// 5d gcode, no acceleration
	{
	  // set start speed to max
	  if(LastCommandF() != hardware.MaxPrintSpeedXY)
	    {
	      command.Code = SETSPEED;
	      command.where = Vector3d(start.x, start.y, start.z);
	      command.f=hardware.MaxPrintSpeedXY;
	      command.e = 0;
	      AppendCommand(command,incrementalE);
	    }
	  // store endPoint
	  command.Code = COORDINATEDMOTION;
	  command.where = end;
	  double extrudedMaterial = DistanceFromLastTo(command.where)*extrusionFactor;
	  command.e = extrudedMaterial;
	  command.f = hardware.MaxPrintSpeedXY;
	  AppendCommand(command,incrementalE);
	}	// 5D gcode
    }// If using firmware acceleration
}

