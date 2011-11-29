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
#include "slicer.h"

int findClosestUnused(const std::vector<Vector3f> &lines, Vector3f point,
		      const std::vector<bool> &used)
{
	int closest = -1;
	float closestDist = numeric_limits<float>::max();

	size_t count = lines.size();

	for (uint i = 0; i < count; i++)
	{
		if (used[i])
		  continue;

		float dist = (lines[i]-point).length();
//		cerr << "dist for " << i << " is " << dist << " == " << lines[i] << " - " << point << " vs. " << closestDist << "\n";
		if(dist >= 0.0 && dist < closestDist)
		{
		  closestDist = dist;
		  closest = i;
		}
	}

//	cerr << "findClosestUnused " << closest << " of " << used.size() << " of " << lines.size() << "\n";
	return closest;
}

uint findOtherEnd(uint p)
{
	uint a = p%2;
	if(a == 0)
		return p+1;
	return p-1;
}

struct GCodeStateImpl
{
  GCode &code;
  Vector3f LastPosition;
  Command lastCommand;
  float lastLayerZ;

  GCodeStateImpl(GCode &_code) :
    code(_code),
    LastPosition(0,0,0),
    lastLayerZ(0)
  {}
};

GCodeState::GCodeState(GCode &code)
{
  pImpl = new GCodeStateImpl(code);
}
GCodeState::~GCodeState()
{
  delete pImpl;
}
void GCodeState::SetZ(float z)
{
  pImpl->LastPosition.z = z;
}
const Vector3f &GCodeState::LastPosition()
{
  return pImpl->LastPosition;
}
void GCodeState::SetLastPosition(const Vector3f &v)
{
  // For some reason we get lots of -nan co-ordinates in lines[] - odd ...
  if (!isnan(v.y) && !isnan(v.x)) // >= 0 && v.x >= 0)
    pImpl->LastPosition = v;
}
void GCodeState::AppendCommand(Command &command)
{
  pImpl->lastCommand = command;
  pImpl->code.commands.push_back(command);
}

float GCodeState::GetLastLayerZ(float curZ)
{
  if (pImpl->lastLayerZ <= 0)
    pImpl->lastLayerZ = curZ;
  return pImpl->lastLayerZ;
}

void GCodeState::SetLastLayerZ(float z)
{
  pImpl->lastLayerZ = z;
}

void GCodeState::ResetLastWhere(Vector3f to)
{
  pImpl->lastCommand.where = to;
}

float GCodeState::DistanceFromLastTo(Vector3f here)
{
  return (pImpl->lastCommand.where - here).length();
}

float GCodeState::LastCommandF()
{
  return pImpl->lastCommand.f;
}

void GCodeState::MakeAcceleratedGCodeLine (Vector3f start, Vector3f end,
					   float extrusionFactor,
					   float &E, float z,
					   const Settings::SlicingSettings &slicing,
					   const Settings::HardwareSettings &hardware)
{
	if ((end-start).length() < 0.05)	// ignore micro moves
		return;

	Command command;

	if (slicing.EnableAcceleration)
	{
		if(end != start) //If we are going to somewhere else
		{
			float len;
			ResetLastWhere (start);

			Vector3f direction = end-start;
			len = direction.length();	// total distance
			direction.normalize();

			// Set start feedrage
			command.Code = SETSPEED;
			command.where = start;
			if (slicing.UseIncrementalEcode)
				command.e = E;		// move or extrude?
			else
				command.e = 0.0f;		// move or extrude?
			command.f = hardware.MinPrintSpeedXY;

			AppendCommand(command);

			if(len < hardware.DistanceToReachFullSpeed*2)
			{
				// TODO: First point of acceleration is the middle of the line segment
				command.Code = COORDINATEDMOTION;
				command.where = (start+end)*0.5f;
				float extrudedMaterial = DistanceFromLastTo(command.where)*extrusionFactor;
				if(slicing.UseIncrementalEcode)
					E += extrudedMaterial;
				else
					E = extrudedMaterial;
				command.e = E;		// move or extrude?
				float lengthFactor = (len/2.0f)/hardware.DistanceToReachFullSpeed;
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
				float extrudedMaterial = DistanceFromLastTo(command.where)*extrusionFactor;
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
		}// if we are going somewhere
	} // Firmware acceleration
	else	// No accleration
	{
		// Make a accelerated line from lastCommand.where to lines[thisPoint]
		if(end != start) //If we are going to somewhere else
		{
			ResetLastWhere (start);
			if (slicing.Use3DGcode)
			{
				{
				command.Code = EXTRUDEROFF;
				AppendCommand(command);
				float speed = hardware.MinPrintSpeedXY;
				command.Code = COORDINATEDMOTION3D;
				command.where = start;
				command.e = E;		// move or extrude?
				command.f = speed;
				AppendCommand(command);
				}	// we need to move before extruding

				command.Code = EXTRUDERON;
				AppendCommand(command);

				float speed = hardware.MinPrintSpeedXY;
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
				Vector3f direction = end-start;
				direction.normalize();

				// set start speed to max
				if(LastCommandF() != hardware.MaxPrintSpeedXY)
				{
					command.Code = SETSPEED;
					command.where = Vector3f(start.x, start.y, z);
					command.f=hardware.MaxPrintSpeedXY;
					command.e = E;
					AppendCommand(command);
				}

				// store endPoint
				command.Code = COORDINATEDMOTION;
				command.where = end;
				float extrudedMaterial = DistanceFromLastTo(command.where)*extrusionFactor;
				if(slicing.UseIncrementalEcode)
					E += extrudedMaterial;
				else
					E = extrudedMaterial;
				command.e = E;		// move or extrude?
				command.f = hardware.MaxPrintSpeedXY;
				AppendCommand(command);
			}	// 5D gcode
		}// If we are going to somewhere else*/
	}// If using firmware acceleration
}

// Convert Cuttingplane to GCode
void CuttingPlane::MakeGcode(GCodeState &state,
			     const std::vector<Vector2f> *infill,
			     float &E, float z,
			     const Settings::SlicingSettings &slicing,
			     const Settings::HardwareSettings &hardware)
{
	// Make an array with all lines, then link'em

	Command command;

	float lastLayerZ = state.GetLastLayerZ(z);

	// Set speed for next move
	command.Code = SETSPEED;
	command.where = Vector3f(0,0,lastLayerZ);
	command.e = E;					// move
	command.f = hardware.MinPrintSpeedZ;		// Use Min Z speed
	state.AppendCommand(command);
	command.comment = "";
	// Move Z axis
	command.Code = ZMOVE;
	command.where = Vector3f(0,0,z);
	command.e = E;					// move
	command.f = hardware.MinPrintSpeedZ;		// Use Min Z speed
	state.AppendCommand(command);
	command.comment = "";

	std::vector<Vector3f> lines;

	if (infill != NULL)
		for (size_t i = 0; i < infill->size(); i++)
			lines.push_back (Vector3f ((*infill)[i].x, (*infill)[i].y, z));

	if( optimizers.size() != 0 )
	{
		// new method
		for( uint i = 1; i < optimizers.size()-1; i++)
		{
			optimizers[i]->RetrieveLines(lines);
		}
	}
	else
	{
		// old method
		// Copy polygons
		if(offsetPolygons.size() != 0)
		{
			for(size_t p=0;p<offsetPolygons.size();p++)
			{
				for(size_t i=0;i<offsetPolygons[p].points.size();i++)
				{
					Vector2f P3 = offsetVertices[offsetPolygons[p].points[i]];
					Vector2f P4 = offsetVertices[offsetPolygons[p].points[(i+1)%offsetPolygons[p].points.size()]];
					lines.push_back(Vector3f(P3.x, P3.y, z));
					lines.push_back(Vector3f(P4.x, P4.y, z));
				}
			}
		}
	}
//	cerr << "lines at z %g = " << z << " count " << lines.size() << "\n";

	// Find closest point to last point

	std::vector<bool> used;
	used.resize(lines.size());
	for(size_t i=0;i<used.size();i++)
		used[i] = false;

//	cerr << "last position " << state.LastPosition() << "\n";
	int thisPoint = findClosestUnused (lines, state.LastPosition(), used);
	if (thisPoint == -1)	// No lines = no gcode
	{
#if CUTTING_PLANE_DEBUG // this happens often for the last slice ...
		cerr << "find closest, and hence slicing failed at z" << z << "\n";
#endif
		return;
	}
	used[thisPoint] = true;

	while(thisPoint != -1)
	{
//		float len;
		// Make a MOVE accelerated line from LastPosition to lines[thisPoint]
		if(state.LastPosition() != lines[thisPoint]) //If we are going to somewhere else
		{
		  state.MakeAcceleratedGCodeLine (state.LastPosition(), lines[thisPoint],
						  0.0f, E, z, slicing, hardware);

		  state.SetLastPosition (lines[thisPoint]);
		} // If we are going to somewhere else

		used[thisPoint] = true;
		// Find other end of line
		thisPoint = findOtherEnd(thisPoint);
		used[thisPoint] = true;
		// store thisPoint

		// Make a PLOT accelerated line from LastPosition to lines[thisPoint]
		state.MakeAcceleratedGCodeLine (state.LastPosition(), lines[thisPoint],
						hardware.GetExtrudeFactor(),
						E, z, slicing, hardware);
		state.SetLastPosition(lines[thisPoint]);
		thisPoint = findClosestUnused (lines, state.LastPosition(), used);
		if(thisPoint != -1)
			used[thisPoint] = true;
		}
	state.SetLastLayerZ(z);
}
