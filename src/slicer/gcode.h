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
#include "config.h"
#pragma once

#include "settings.h"

#include <vmmlib/vmmlib.h>
//#include <polylib/Polygon2d.h>

#include "../gcode.h"

//#include "printlines.h"
struct printline;

using namespace std;
using namespace vmml;

//using namespace PolyLib;

class GCode;
class Command;
struct GCodeStateImpl;

class GCodeState {
  GCodeStateImpl *pImpl;
 public:
  GCodeState(GCode &code);
  ~GCodeState();
  /* void SetZ (double z); */
  void AppendCommand(Command &command, bool incrementalE);
  void AppendCommand(GCodes code, bool incrementalE=false, string comment="");
  void AddLines (vector<printline> lines,
		 double extrusionfactor,
		 double offsetZ, 
		 const Settings::SlicingSettings &slicing,
		 const Settings::HardwareSettings &hardware);
  void AddLines (vector<Vector3d> lines,
		 double extrusionFactor,
		 double maxspeed,
		 double movespeed,
		 double offsetZ, 
		 const Settings::SlicingSettings &slicing,
		 const Settings::HardwareSettings &hardware);
  void MakeAcceleratedGCodeLine (printline line,
				 double extrusionfactor,
				 double offsetZ, 
				 const Settings::SlicingSettings &slicing,
				 const Settings::HardwareSettings &hardware);
  void MakeAcceleratedGCodeLine (Vector3d start, Vector3d end,
				 double extrusionFactor,
				 double maxspeed,
				 double offsetZ, 
				 const Settings::SlicingSettings &slicing,
				 const Settings::HardwareSettings &hardware);
  /* double GetLastLayerZ(double curZ); */
  /* void  SetLastLayerZ(double z); */
  const Vector3d &LastPosition();
  void  SetLastPosition(const Vector3d &v);
  void  ResetLastWhere(Vector3d to);
  double DistanceFromLastTo(Vector3d here);
  double LastCommandF();
  double timeused;
};

