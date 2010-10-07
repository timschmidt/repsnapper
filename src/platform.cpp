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

