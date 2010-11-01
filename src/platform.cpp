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
#include "stdafx.h"
#include "platform.h"
#ifndef WIN32
#  include <sys/time.h>
#endif

unsigned long Platform::getTickCount()
{
#ifdef WIN32
	return GetTickCount();
#else
	struct timeval now;
	gettimeofday (&now, NULL);
	return now.tv_sec * 1000 + now.tv_usec / 1000;
#endif
}

