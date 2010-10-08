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
