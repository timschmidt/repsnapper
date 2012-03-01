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
  if (!isnan(v.y) && !isnan(v.x)) // >= 0 && v.x >= 0)
    pImpl->LastPosition = v;
}
void GCodeState::AppendCommand(Command &command, bool incrementalE)
{
  Vector3d lastwhere = Vector3d(pImpl->lastCommand.where);
  if (incrementalE)
    command.e += pImpl->lastCommand.e;
  if (command.f!=0)
    timeused += (command.where - lastwhere).length()/command.f*60;
  pImpl->lastCommand = command;
  pImpl->code.commands.push_back(command);
}
void GCodeState::AppendCommand(GCodes code, bool incrementalE, string comment)
{
  Command comm(code);
  comm.comment = comment;
  AppendCommand(comm, incrementalE);
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
    MakeAcceleratedGCodeLine (plines[i], extrusionfactor, 
			      offsetZ, slicing, hardware);
}

void GCodeState::AddLines (vector<Vector3d> lines,
			   double extrusionFactor,
			   double maxspeed,
			   double maxmovespeed,
			   double offsetZ, 
			   const Settings::SlicingSettings &slicing,
			   const Settings::HardwareSettings &hardware)
{
  for (uint i=0; i < lines.size(); i+=2)
    {
      // MOVE to start of next line
      if(LastPosition() != lines[i]) 
	{
	  MakeAcceleratedGCodeLine (LastPosition(), lines[i],
				    Vector3d(0,0,0),0,
				    0, maxmovespeed,
				    offsetZ, slicing, hardware);
	  SetLastPosition (lines[i]);
	} 
      // PLOT to endpoint of line 
      MakeAcceleratedGCodeLine (LastPosition(), lines[i+1], 
				Vector3d(0,0,0),0,
				extrusionFactor, maxspeed, 
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
  if(LastPosition() != pline.from) { // then first move to pline.from
    MakeAcceleratedGCodeLine(LastPosition(), pline.from, 
			     Vector3d(0,0,0),0,
			     0, hardware.MoveSpeed,
			     offsetZ, slicing, hardware);
  }
  MakeAcceleratedGCodeLine(pline.from, pline.to, pline.arcIJK, pline.arc,
			   extrusionfactor * pline.extrusionfactor, 
			   pline.speed, 
			   offsetZ, slicing, hardware);
  SetLastPosition(pline.to);
}

void GCodeState::MakeAcceleratedGCodeLine (Vector3d start, Vector3d end,
					   Vector3d arcIJK, short arc,
					   double extrusionFactor, 
					   double maxspeed,
					   double offsetZ, 
					   const Settings::SlicingSettings &slicing,
					   const Settings::HardwareSettings &hardware)
{
   // if ((end-start).length() < 0.05)	// ignore micro moves
   //  return;

  Command command;

  bool incrementalE = slicing.UseIncrementalEcode;

  double minspeed = hardware.MinPrintSpeedXY;
  maxspeed = max(minspeed,maxspeed); // in case maxspeed is too low
  // if (slicing.EnableAcceleration)
  //   {
  //     double len;
  //     ResetLastWhere (start);
	  
  //     Vector3d direction = end-start;
  //     len = direction.length();	// total distance
  //     direction.normalize();
	  
  //     // Set start
  //     command.Code = SETSPEED;
  //     command.where = start;
  //     command.e = 0.0;	       
  //     command.f = minspeed;
	  
  //     AppendCommand(command,incrementalE);
	  
  //     if(len < hardware.DistanceToReachFullSpeed*2)
  // 	{
  // 	  // TODO: First point of acceleration is the middle of the line segment
  // 	  command.Code = COORDINATEDMOTION;
  // 	  command.where = (start+end)*0.5;
  // 	  double extrudedMaterial = DistanceFromLastTo(command.where)*extrusionFactor;
  // 	  command.e = extrudedMaterial;  
  // 	  double lengthFactor = (len/2.0)/hardware.DistanceToReachFullSpeed;
  // 	  command.f = (maxspeed-minspeed)*lengthFactor+minspeed;
  // 	  AppendCommand(command,incrementalE);
  // 	  // And then decelerate
  // 	  command.Code = COORDINATEDMOTION;
  // 	  command.where = end;
  // 	  command.e = extrudedMaterial;
  // 	  command.f = minspeed;
  // 	  AppendCommand(command,incrementalE);
		
  // 	}// if we will never reach full speed
  //     else
  // 	{
  // 	  // Move to max speed
  // 	  command.Code = COORDINATEDMOTION;
  // 	  command.where = start+(direction*hardware.DistanceToReachFullSpeed);
  // 	  double extrudedMaterial = DistanceFromLastTo(command.where)*extrusionFactor;
  // 	  command.e = extrudedMaterial;
  // 	  command.f = maxspeed;
  // 	  AppendCommand(command,incrementalE);
		
  // 	  // Sustian speed till deacceleration starts
  // 	  command.Code = COORDINATEDMOTION;
  // 	  command.where = end-(direction*hardware.DistanceToReachFullSpeed);
  // 	  extrudedMaterial = DistanceFromLastTo(command.where)*extrusionFactor;
  // 	  command.e = extrudedMaterial;
  // 	  command.f = maxspeed;
  // 	  AppendCommand(command,incrementalE);
		
  // 	  // deacceleration untill end
  // 	  command.Code = COORDINATEDMOTION;
  // 	  command.where = end;
  // 	  extrudedMaterial = DistanceFromLastTo(command.where)*extrusionFactor;
  // 	  command.e = extrudedMaterial;
  // 	  command.f = minspeed;
  // 	  AppendCommand(command,incrementalE);
  // 	} // if we will reach full speed
  //   }
  // else	// No accleration 
    {
      ResetLastWhere (start);
      if (slicing.Use3DGcode)
	{
	  command.Code = EXTRUDEROFF;
	  AppendCommand(command,incrementalE);
	  command.Code = COORDINATEDMOTION3D;
	  command.where = start;
	  command.e = 0;	    
	  command.f = maxspeed;
	  AppendCommand(command,incrementalE);
		
	  command.Code = EXTRUDERON;
	  AppendCommand(command,incrementalE);
		
	  command.Code = COORDINATEDMOTION3D;
	  command.where = end;
	  command.e = 0;
	  command.f = maxspeed;
	  AppendCommand(command,incrementalE);
	  // Done, switch extruder off
	  command.Code = EXTRUDEROFF;
	  AppendCommand(command,incrementalE);
	}
      else	// 5d gcode, no acceleration
	{
	  // // set start speed to max
	  // if(LastCommandF() != maxspeed)
	  //   {
	  //     command.Code = SETSPEED;
	  //     command.where = Vector3d(start.x, start.y, start.z);
	  //     command.f=maxspeed;
	  //     command.e = 0;
	  //     AppendCommand(command,incrementalE);
	  //   }
	  // store endPoint
	  command.where = end;
	  double extrudedMaterial = DistanceFromLastTo(command.where)*extrusionFactor;
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
	  AppendCommand(command,incrementalE);
	}	// 5D gcode
    }// If using firmware acceleration
}

