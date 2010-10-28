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
#pragma once

#include "stdafx.h"

class Printer
{
public:
	Printer(){
		ExtrudeRatio = 1.0f;			// 1:1
		ExtrusionBroadWidth = 2.0f;		// 2.0mm
		ExtrusionDelayForLayer = 0.0f;	// 1000 ms
		ExtrusionDelayForPolygon = 0.0f;// 200 ms
		ExtrusionFoundationWidth = 2.0f;// 2 mm
		ExtrusionHeight = 0.4f;			// 0.4mm layerthickness
		ExtrusionInfillWidth = 0.5f;	// 0.5mm fill of solid layers
		ExtrusionLastFoundationWidth = 1.0f;// 1mm fill of top foundation layer
		ExtrusionOverRun = -1.0f;		// Stop extruder 1mm before it reaches its target
		ExtrusionSize = 0.7f;			// width of extruded material
		ExtrusionSpeed = 1.0f;			// ??
		ExtrusionTemp = 190;			// Temperature to extrude at
		FastXYFeedrate = 8000;			// Max speed the extruder can keep up with
		UpperFineLayers = 2;			// # top layers with fine infill pattern
		}
	
	void Draw(const ProcessController &PC);
	
	float ExtrudeRatio;	//If you are using the 4D firmware in the Arduino, this decides how much extrudate actually to lay down for a given movement. Thus if you set this to 0.7 and the head moves 100mm, then 70mm of extrudate will be deposited in that move. You can use this to control how fat or thin the extrudate trail is, though the machine should be set up so that this factor is 1.0.
	float ExtrusionBroadWidth;//The gap between the infill zig-zag pattern used to fill the interior of an object when coarse infill is being used. Set this negative to suppress coarse infill.
	float ExtrusionDelayForLayer;//For the first use of the extruder in a layer the time delay between turning on the extruder motor and starting to move the extruder to lay down material. See also ValveDelayForLayer (below).
	float ExtrusionDelayForPolygon;//For the second and all subsequent use of the extruder in a layer the time delay between turning on the extruder motor and starting to move the extruder to lay down material. See also ValveDelayForPolygon (below).
	float ExtrusionFoundationWidth;//The gap between the infill zig-zag pattern used to fill the interior of the foundations (if any).
	float ExtrusionHeight;//The depth of each layer. This value should be the same for all extruders in the machine at any one time.
	float ExtrusionInfillWidth;//The gap between the infill zig-zag pattern used to fill the interior of an object when fine infill is being used.
	float ExtrusionLastFoundationWidth;//The gap between the infill zig-zag pattern used to fill the interior of the last layer of the foundations (if any).
	float ExtrusionOverRun;//The distance before the end of a sequence of infill or outline depositions to turn off the extruder motor. See also ValveOverRun (below). Set this negative to keep running to the end.
	float ExtrusionSize;//The width of the filament laid down by the extruder.
	float ExtrusionSpeed;// On the latest extruders (see here) this sets the rate at which the extrudate exits the nozzle in millimeters per minute when the extruder is running in thin air (i.e. not building, or running the delay at the start of a layer or polygon - see above).
						 // Legacy. On earlier extruders it sets the PWM signal to send the extruder motor, as a fraction of 255. See also Extruder0_t0 (below).
						 // In either case, if there is no motor in use, set this negative.
	float ExtrusionTemp; //The temperature to run the extruder at.
	float FastXYFeedrate;//The fastest that this extruder can plot. This is the speed that the system will accelerate writing up to when accelerations are enabled. The system takes the minimum of this vale and the equivalent value for the machine overall when extruding.
	float Reverse;		 // The time to reverse the extruder motor when it is turned off, drawing the extrudate back into it and reducing dribble. 
	float UpperFineLayers;//Give this many layers at the top of the object fine infill. 
};
