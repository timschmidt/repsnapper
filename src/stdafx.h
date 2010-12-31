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
// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						

#ifdef WIN32
#  pragma warning( disable : 4311 4312 4244 4267 4800)
#endif
typedef unsigned int        uint;

#define DEBUG_ECHO (1<<0)
#define DEBUG_INFO (1<<1)
#define DEBUG_ERRORS (1<<2)
#ifdef WIN32
	#include <windows.h>											// Header File For Windows
	#include <tchar.h>
typedef unsigned int        uint;
#endif
typedef unsigned int        UINT;
#include "config.h"
#include "platform.h"
#include <stdio.h>
#include <FL/Fl.H>
#include <gtkmm.h>
#include <gtkglmm.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <vmmlib/vmmlib.h>
#ifndef WIN32
// whatever this is it doesn't exist in windows and doesn't appear to be needed
#  include <alloca.h>
#endif
#include "math.h"                                               // Needed for sqrtf

// Unpleasant needs un-winding ...
using namespace std;
using namespace vmml;

// try to avoid compile time explosion by reducing includes
class GUI;
class Poly;
class GCode;
class Command;
class Point2f;
class Printer;
class Triangle;
class Segment2f;
class AsyncSerial;
class RepRapSerial;
class CuttingPlane;
class ProcessController;
class ModelViewController;

class RFO;
class RFO_File;
class RFO_Object;
class RFO_Transform3D;

//utility macros
//assuming IEEE-754(GLfloat), which i believe has max precision of 7 bits
# define Epsilon 1.0e-5

struct InFillHit;

typedef unsigned int uint;

void MakeAcceleratedGCodeLine(Vector3f start, Vector3f end, float DistanceToReachFullSpeed, float extrusionFactor, GCode &code, float z, float minSpeedXY, float maxSpeedXY, float minSpeedZ, float maxSpeedZ, bool UseIncrementalEcode, bool Use3DGcode, float &E, bool EnableAcceleration);
bool IntersectXY(const Vector2f &p1, const Vector2f &p2, const Vector2f &p3, const Vector2f &p4, InFillHit &hit);	// Utilityfunction for CalcInFill
bool InFillHitCompareFunc(const InFillHit& d1, const InFillHit& d2);

extern void HSVtoRGB 	(const float &h, const float &s, const float &v, float &r,float &g,float &b);			
extern void RGBtoHSV 	(const float &r, const float &g, const float &b, float &h, float &s, float &v);
extern void RGBTOHSL	(float red, float green, float blue, float &hue, float &sat, float &lightness);
extern void	RGBTOYUV	(float r, float g, float b, float &y, float &u, float &v);
extern void YUVTORGB	(float y, float u, float v, float &r, float &g, float &b);

#ifdef ENABLE_LUA

extern "C" {
  #include <lua.hpp>
}
#include <luabind/luabind.hpp>
using namespace luabind;

#endif // ENABLE_LUA

// ivconv
#include "ivcon.h"
