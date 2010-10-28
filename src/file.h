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
/*
 * Simple file & file-chooser abstraction to allow
 * native gtk+ file selection under Unix.
 */
#ifndef FILE_H
#define FILE_H

#include "modelviewcontroller.h"

namespace FileChooser {
  enum Type {
    STL,
    RFO,
    GCODE,
  };
  enum Op {
    OPEN,
    SAVE
  };
  void ioDialog (ModelViewController *mvc, Op o, Type t, bool dropRFO = false);
};

#endif
