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

#include "../shape.h"

#include "gcodestate.h"
#include "../slicer/printlines.h"


struct GCodeStateImpl
{
  GCode &code;
  Vector3d LastPosition;
  Command lastCommand;
  double lastLength;

  GCodeStateImpl(GCode &_code) :
    code(_code),
    LastPosition(0,0,0)
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
  pImpl->LastPosition = v;
}
void GCodeState::AppendCommand(Command &command, bool relativeE,
                               double minLength)
{
    bool merged = false;
    if (!command.is_value) {
        if (!relativeE)
            command.e += pImpl->lastCommand.e;
        if (command.where) {
            if (pImpl->lastCommand.where && command.f!=0.) {
                const double length =
                        (*command.where - *pImpl->lastCommand.where).length();
                timeused += length/command.f*60;
                const vector<Command> &prevCommands = pImpl->code.commands;
                if (prevCommands.size()>2){
                    if (prevCommands[prevCommands.size()-2].where
                            && (prevCommands[prevCommands.size()-2].where->
                                distance(*command.where) < minLength)) {
                        Command &last = pImpl->code.commands.back();
                        if (abs(last.f/command.f-1)<0.01){ // similar f
                            merged = last.append(command);
                        }
                    }
                }
                if (merged)
                    pImpl->lastLength += length;
                else
                    pImpl->lastLength = length;
            } else {
                pImpl->lastLength = 0.;
            }
            pImpl->LastPosition = *command.where;
            if (merged)
                pImpl->lastCommand = pImpl->code.commands.back();
            else
                pImpl->lastCommand = command;
        }
    }
    if (!merged)
        pImpl->code.commands.push_back(command);
    if (command.where && command.where->z() > pImpl->code.Max.z())
        pImpl->code.Max.z() = command.where->z();
}
void GCodeState::AppendCommand(GCodes code, bool relativeE, string comment,
                               double minLength)
{
  Command comm(code);
  comm.comment = comment;
  AppendCommand(comm, relativeE, minLength);
}
void GCodeState::AppendCommands(vector<Command> commands, bool relativeE,
                                double minLength)
{
  for (uint i = 0; i < commands.size(); i++) {
    AppendCommand(commands[i], relativeE, minLength);
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

void GCodeState::ResetLastWhere(const Vector3d &to)
{
    if (pImpl->lastCommand.where)
        delete pImpl->lastCommand.where;
  pImpl->lastCommand.where = new Vector3d(to);
}
double GCodeState::DistanceFromLastTo(Vector3d *here)
{
  return (*pImpl->lastCommand.where - *here).length();
}

double GCodeState::LastCommandF()
{
  return pImpl->lastCommand.f;
}

// // dont use -- commands are generated in PLine3 printlines.cpp
// void GCodeState::AddLines (vector<PLine3> plines,
// 			   double extrusionfactor,
// 			   double offsetZ,
// 			   const Settings::SlicingSettings &slicing,
// 			   const Settings::HardwareSettings &hardware)
// {
//   for (uint i=0; i < plines.size(); i++)
//     MakeGCodeLine (plines[i], extrusionfactor, offsetZ, slicing, hardware);
// }

/*
void GCodeState::AddLines (vector<Vector3d> linespoints,
                           double extrusionFactor,
                           double maxspeed,
                           double maxmovespeed,
                           double offsetZ,
                           Settings &settings)
{
  bool relEcode = settings.get_boolean("Slicing/RelativeEcode");
  double minmovespeed = settings.get_double("Hardware/MinMoveSpeedXY") * 60;

  for (uint i=0; i < linespoints.size(); i+=2)
    {
      // MOVE to start of next line
      if(LastPosition() != linespoints[i])
    {
      MakeGCodeLine (LastPosition(), linespoints[i],
             Vector3d(0,0,0),0, 0, 0, minmovespeed, maxmovespeed,
             offsetZ, relEcode);
      SetLastPosition (linespoints[i]);
    }
      // PLOT to endpoint of line
      MakeGCodeLine (LastPosition(), linespoints[i+1],
             Vector3d(0,0,0),0, extrusionFactor, 0, minmovespeed, maxspeed,
             offsetZ, relEcode);
    SetLastPosition(linespoints[i+1]);
    }
  //SetLastLayerZ(z);
}
*/

// // dont use -- commands are generated in PLine3 printlines.cpp
// // (speeds are in mm/min which is obsolete)
// void GCodeState::MakeGCodeLine (PLine3 pline,
// 				double extrusionfactor,
// 				double offsetZ,
// 				const Settings::SlicingSettings &slicing,
// 				const Settings::HardwareSettings &hardware)
// {
//   bool relativeE = slicing.RelativeEcode;
//   double minspeed = hardware.MinPrintSpeedXY;
//   double maxspeed = hardware.MaxPrintSpeedXY;
//   cerr << "dont use GCodeState::MakeGCodeLine (PLine3 pline..." << endl;

//   if(LastPosition() != pline.from) { // then first move to pline.from
//     maxspeed = max(minspeed, (double)hardware.MoveSpeed); // in case maxspeed is too low
//     Command command(COORDINATEDMOTION, pline.from, 0, maxspeed);
//     AppendCommand(command,relativeE);
//     //SetLastPosition(pline.from);
//   }

//   if (pline.arc == 0) { // make line
//       maxspeed = max(minspeed, pline.speed); // in case maxspeed is too low
//     double extrudedMaterial = DistanceFromLastTo(pline.to) * extrusionfactor;
//     Command command(COORDINATEDMOTION, pline.to, extrudedMaterial, maxspeed);
//     if (pline.from==pline.to) command.comment = _("Extrusion only ");
//   } else { // make arc
//     cerr << "no arc in GCodeState::MakeGCodeLine (PLine3 pline..."
// 	 << "   dont use this function" << endl;

//   }
//   MakeGCodeLine(pline.from, pline.to, pline.arcIJK, pline.arc,
// 	       pline.extrusionfactor * extrusionfactor,
// 	       pline.absolute_extrusion,
// 	       pline.speed,
// 	       offsetZ, slicing, hardware);
//   //SetLastPosition(pline.to);
// }

/*
void GCodeState::MakeGCodeLine (Vector3d start, Vector3d end,
                Vector3d arcIJK, short arc,
                double extrusionFactor,
                double absolute_extrusion,
                double minspeed,
                double maxspeed,
                double offsetZ,
                bool relativeE)
{
   // if ((end-start).length() < 0.05)	// ignore micro moves
   //  return;

  Command command;
  command.is_value = false;

  maxspeed = max(minspeed,maxspeed); // in case maxspeed is too low
  ResetLastWhere (start);
  command.where = end;
  if (start==end)  { // pure extrusions
    command.comment = _("Extrusion only ");
  }
  double extrudedMaterial = DistanceFromLastTo(command.where)*extrusionFactor;

  if (absolute_extrusion!=0.) {
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
*/
