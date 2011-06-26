/*
    Provide some simple platform abstraction, and
    the right headers for GL stuff depending on OS

    This file is a part of the RepSnapper project.
    Copyright (C) 2010  Michael Meeks

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef PLATFORM_H
#define PLATFORM_H

#include <vector>
#include <string>

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

class Platform {
  public:
	static unsigned long getTickCount();
	static void setBinaryPath(const char *apparg);
	static std::vector<std::string> getConfigPaths();
};

// helper instance for easy locking
class ToolkitLock {
  bool m_locked;
 public:
  ToolkitLock(bool force = false);
  ~ToolkitLock();
};

#endif // PLATFORM_H
