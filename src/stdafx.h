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
#include "platform.h"
#include <stdio.h>
#include <glib/gi18n.h>
#include <gtkmm.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <vmmlib/vmmlib.h>
#include "math.h" // Needed for sqrtf
#include "types.h"

// Unpleasant needs un-winding ...
using namespace std;
using namespace vmml;

//utility macros
//assuming IEEE-754(GLfloat), which i believe has max precision of 7 bits
# define Epsilon 1.0e-5

typedef unsigned int uint;

#endif // STDAFX_H
