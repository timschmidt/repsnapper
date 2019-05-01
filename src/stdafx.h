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

#ifndef STDAFX_H
#define STDAFX_H

#include "config.h"

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif

#ifdef _MSC_VER // Visual C++ compiler
#  pragma warning( disable : 4311 4312 4244 4267 4800)
#endif

typedef unsigned int        uint;

#define DEBUG_ECHO (1<<0)
#define DEBUG_INFO (1<<1)
#define DEBUG_ERRORS (1<<2)
#ifdef WIN32
        #include <windows.h>   // Header File For Windows
        #include <tchar.h>
        #undef interface // Undo braindead define from Windows that conflicts with glibmm DBUS binding
typedef unsigned int        guint;
#define random   rand
#define srandom  srand
#endif
#include "platform.h"   // OpenGL, glu, glut in cross-platform way
#include <stdio.h>
#include <glib/gi18n.h>
//#include <gtkmm.h>
#include "math.h" // Needed for sqrtf
#include "types.h"


#define VMMLIB_BASIC_ONLY
#include <vmmlib/vmmlib.hpp>


// Unpleasant needs un-winding ...
using namespace std;
//using namespace vmml;

typedef vmml::vec2d Vector2d;
typedef vmml::vec2f Vector2f;
typedef vmml::vec3d Vector3d;
typedef vmml::vec3f Vector3f;
typedef vmml::vec4d Vector4d;
typedef vmml::vec4f Vector4f;
typedef vmml::mat4d Matrix4d;
typedef vmml::mat4f Matrix4f;
typedef vmml::mat3d Matrix3d;


float const GREEN[] = {0.1, 1, 0.1};
float const GREEN2[] = {0.3, 0.8, 0.3};
float const BLUEGREEN[] = {0.1, 0.9, 0.7};
float const BLUE2[] = {0.5,0.5,1.0};
float const RED[] = {1, 0, 0};
float const RED2[] = {0.8,0.5,0.5};
float const RED3[] = {0.8,0.3,0.1};
float const ORANGE[] = {1, 0.5, 0};
float const YELLOW[] = {1, 1, 0};
float const YELLOW2[] = {1, 1, 0.2};
float const WHITE[] = {1, 1, 1};
float const GREY[] = {0.5,0.5,0.5};
float const VIOLET[] = {0.8,0.0,0.8};

//#define USECAIRO

//utility macros
//assuming IEEE-754(GLfloat), which i believe has max precision of 7 bits
# define Epsilon 1.0e-5

#ifdef __GNUC__
#define UNUSED __attribute__ ((unused))
#else
#define UNUSED
#endif

const double UNUSED INFTY = numeric_limits<double>::infinity();

#endif // STDAFX_H
