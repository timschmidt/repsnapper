/*
    This is file part of the RepSnapper project.

    Provide some simple platform abstraction, and
    the right headers for GL stuff depending on OS.

    Copyright (C) 2010 Logick

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

#ifndef PLATFORM_H
#define PLATFORM_H
/**
 * OSX uses a different setup for opengl, we compile with carbon
 */
#ifdef __APPLE__
	#include <OpenGL/gl.h>
	#include <OpenGL/glu.h>
	#include <GLUT/glut.h>
#else
	#include <GL/gl.h>		// Header File For The OpenGL32 Library
	#include <GL/glu.h>		// Header File For The GLu32 Library
#ifndef WIN32
	#include <GL/glut.h>	// Header GLUT Library
#endif
#endif

#endif // PLATFORM_H
