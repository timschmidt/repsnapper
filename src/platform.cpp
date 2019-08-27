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

static char *binary_path = NULL;

#ifdef USE_GLIB
void Platform::setBinaryPath (const char *apparg)
{
  const char *p;

  if (!(p = strrchr (apparg, G_DIR_SEPARATOR)))
    return;

  binary_path = g_strndup (apparg, p - apparg);
}
#endif
bool Platform::has_extension(const std::string &fname, const char *extn)
{
  if (fname.find_last_of(".") == std::string::npos)
    return false;
  std::string this_extn = fname.substr(fname.find_last_of(".") + 1);
  return this_extn == extn;
}

#ifdef USE_GLIB
std::vector<std::string> Platform::getConfigPaths()
{
  const gchar * const *datadirs = g_get_system_data_dirs();
  std::vector<std::string> dirs;

  /* Always prefer config files in the current directory */
  dirs.push_back(std::string("src") + G_DIR_SEPARATOR);

  /* Otherwise prefer the app's current directory */
  if (binary_path) {
    dirs.push_back(std::string(binary_path) + G_DIR_SEPARATOR);
    /* Finally prefer an etc/xdg path in app's current directory */
#ifdef WIN32
    dirs.push_back(std::string(binary_path) + G_DIR_SEPARATOR + ".." + G_DIR_SEPARATOR +
       "etc" + G_DIR_SEPARATOR + "xdg" + G_DIR_SEPARATOR + "repsnapper");
#endif
  }

  dirs.push_back(std::string(G_STRINGIFY(RSDATADIR)) + G_DIR_SEPARATOR);
  dirs.push_back(std::string(G_STRINGIFY(SYSCONFDIR)) + G_DIR_SEPARATOR);
  for(gsize i = 0; datadirs[i] != NULL; ++i)
    dirs.push_back(std::string(datadirs[i]) + G_DIR_SEPARATOR + "repsnapper" + G_DIR_SEPARATOR);

  return dirs;
}
#endif


std::string str(double r, int prec) {
  ostringstream o;
  if (prec>=0) o.precision(prec);
  o<<r;
  return o.str();
}
