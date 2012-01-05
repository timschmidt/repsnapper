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
#include "cuttingplane.h"

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
void GCodeState::AppendCommand(Command &command)
{
  Vector3d lastwhere = Vector3d(pImpl->lastCommand.where);
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

void GCodeState::AddLines (vector<Vector3d> lines,
			   double extrusionFactor,
			   double offsetZ, double &E, 
			   const Settings::SlicingSettings &slicing,
			   const Settings::HardwareSettings &hardware)
{
  for (uint i=0; i < lines.size(); i+=2)
    {
      // MOVE to start of next line
      if(LastPosition() != lines[i]) 
	{
	  MakeAcceleratedGCodeLine (LastPosition(), lines[i], 0.0, 
				    offsetZ, E, slicing, hardware);
	  SetLastPosition (lines[i]);
	} 
      // PLOT to endpoint of line 
      MakeAcceleratedGCodeLine (LastPosition(), lines[i+1], extrusionFactor, 
				offsetZ, E, slicing, hardware);
    SetLastPosition(lines[i+1]);
    }
  //SetLastLayerZ(z);
}


void GCodeState::MakeAcceleratedGCodeLine (Vector3d start, Vector3d end,
					   double extrusionFactor, 
					   double offsetZ, double &E, 
					   const Settings::SlicingSettings &slicing,
					   const Settings::HardwareSettings &hardware)
{
  if ((end-start).length() < 0.05)	// ignore micro moves
    return;

  Command command;
  Vector3d wherenow;

  if (slicing.EnableAcceleration)
    {
      double len;
      ResetLastWhere (start);
	  
      Vector3d direction = end-start;
      len = direction.length();	// total distance
      direction.normalize();
	  
      // Set start feedrage
      command.Code = SETSPEED;
      command.where = start;
      if (slicing.UseIncrementalEcode && !isnan(E))
	command.e = E;		// move or extrude?
      else
	command.e = 0.0;		// move or extrude?
      command.f = hardware.MinPrintSpeedXY;
	  
      AppendCommand(command);
	  
      if(len < hardware.DistanceToReachFullSpeed*2)
	{
	  // TODO: First point of acceleration is the middle of the line segment
	  command.Code = COORDINATEDMOTION;
	  command.where = (start+end)*0.5;
	  double extrudedMaterial = DistanceFromLastTo(command.where)*extrusionFactor;
	  if (!isnan(extrudedMaterial)) {
	    if(slicing.UseIncrementalEcode)
	      E += extrudedMaterial;
	    else
	      E = extrudedMaterial;
	    command.e = E;		// move or extrude?
	  }
	  double lengthFactor = (len/2.0)/hardware.DistanceToReachFullSpeed;
	  command.f = (hardware.MaxPrintSpeedXY-hardware.MinPrintSpeedXY)*lengthFactor+hardware.MinPrintSpeedXY;
	  AppendCommand(command);
	      
	  // And then decelerate
	  command.Code = COORDINATEDMOTION;
	  command.where = end;
	  if(slicing.UseIncrementalEcode)
	    E += extrudedMaterial;
	  else
	    E = extrudedMaterial;
	  command.e = E;		// move or extrude?
	  command.f = hardware.MinPrintSpeedXY;
	  AppendCommand(command);
		
	}// if we will never reach full speed
      else
	{
	  // Move to max speed
	  command.Code = COORDINATEDMOTION;
	  command.where = start+(direction*hardware.DistanceToReachFullSpeed);
	  double extrudedMaterial = DistanceFromLastTo(command.where)*extrusionFactor;
	  if(slicing.UseIncrementalEcode)
	    E += extrudedMaterial;
	  else
	    E = extrudedMaterial;
	  command.e = E;		// move or extrude?
	  command.f = hardware.MaxPrintSpeedXY;
	  AppendCommand(command);
		
	  // Sustian speed till deacceleration starts
	  command.Code = COORDINATEDMOTION;
	  command.where = end-(direction*hardware.DistanceToReachFullSpeed);
	  extrudedMaterial = DistanceFromLastTo(command.where)*extrusionFactor;
	  if(slicing.UseIncrementalEcode)
	    E += extrudedMaterial;
	  else
	    E = extrudedMaterial;
	  command.e = E;		// move or extrude?
	  command.f = hardware.MaxPrintSpeedXY;
	  AppendCommand(command);
		
	  // deacceleration untill end
	  command.Code = COORDINATEDMOTION;
	  command.where = end;
	  extrudedMaterial = DistanceFromLastTo(command.where)*extrusionFactor;
	  if(slicing.UseIncrementalEcode)
	    E += extrudedMaterial;
	  else
	    E = extrudedMaterial;
	  command.e = E;		// move or extrude?
	  command.f = hardware.MinPrintSpeedXY;
	  AppendCommand(command);
	} // if we will reach full speed
    } // Firmware acceleration
  else	// No accleration
    {
      // Make a accelerated line from lastCommand.where to lines[thisPoint]
      ResetLastWhere (start);
      if (slicing.Use3DGcode)
	{
	  {
	    command.Code = EXTRUDEROFF;
	    AppendCommand(command);
	    double speed = hardware.MinPrintSpeedXY;
	    command.Code = COORDINATEDMOTION3D;
	    command.where = start;
	    command.e = E;		// move or extrude?
	    command.f = speed;
	    AppendCommand(command);
	  }	// we need to move before extruding
		
	  command.Code = EXTRUDERON;
	  AppendCommand(command);
		
	  double speed = hardware.MinPrintSpeedXY;
	  command.Code = COORDINATEDMOTION3D;
	  command.where = end;
	  command.e = E;		// move or extrude?
	  command.f = speed;
	  AppendCommand(command);
	  // Done, switch extruder off
	  command.Code = EXTRUDEROFF;
	  AppendCommand(command);
	}
      else	// 5d gcode, no acceleration
	{
	  Vector3d direction = end-start;
	  direction.normalize();
		
	  // set start speed to max
	  if(LastCommandF() != hardware.MaxPrintSpeedXY)
		  {
		    command.Code = SETSPEED;
		    command.where = Vector3d(start.x, start.y, start.z);
		    command.f=hardware.MaxPrintSpeedXY;
		    command.e = E;
		    AppendCommand(command);
		}
	      
	      // store endPoint
	      command.Code = COORDINATEDMOTION;
	      command.where = end;
	      double extrudedMaterial = DistanceFromLastTo(command.where)*extrusionFactor;
	      if(slicing.UseIncrementalEcode)
		E += extrudedMaterial;
	      else
		E = extrudedMaterial;
	      command.e = E;		// move or extrude?
	      command.f = hardware.MaxPrintSpeedXY;
	      AppendCommand(command);
	    }	// 5D gcode
	}// If using firmware acceleration
}

