/**
 * This is part of the RepSnapper project
 *
 * Provide some simple platform abstraction, and
 * the right headers for GL stuff depending on OS
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
