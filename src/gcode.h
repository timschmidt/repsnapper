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
#pragma once
#include <gtkmm.h>

#include <vector>
#include "platform.h"
#include "math.h"                                               // Needed for sqrtf

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include <vmmlib/vmmlib.h>

#include "command.h"

using namespace std;
using namespace vmml;


class GCodeImpl;
class RepRapSerial;

class GCodeIter
{
  Glib::RefPtr<Gtk::TextBuffer> m_buffer;
  Gtk::TextBuffer::iterator m_it;
 public:
  unsigned long m_line_count, m_cur_line;
  GCodeIter (Glib::RefPtr<Gtk::TextBuffer> buffer);
  std::string next_line ();
  bool finished();
  double time_used;
  time_t time_started;
  double time_estimation;
  Command getCurrentCommand(Vector3d defaultwhere);
};

class GCode
{
public:
  GCode();

  void Read  (Model *model, ViewProgress *progress, string filename);
  //void Write (Model *model, string filename);
  void draw  (const Settings &settings, int layer=-1, bool liveprinting=false,
	      int linewidth=3);
  void drawCommands(const Settings &settings, uint start, uint end,
		    bool liveprinting, int linewidth, bool arrows);
  void MakeText(string &GcodeTxt, const string &GcodeStart,
		const string &GcodeLayer, const string &GcodeEnd,
		bool RelativeEcode,
		double AntioozeDistance, double AntioozeAmount,
		double AntioozeSpeed, bool AntioozeRepushAfter, 
		ViewProgress * progress);
  
  bool append_text (const std::string &line);
  std::string get_text() const;
  void clear();

  std::vector<Command> commands;
  Vector3d Min, Max, Center;

  Glib::RefPtr<Gtk::TextBuffer> buffer;
  GCodeIter *get_iter ();

  double GetTotalExtruded(bool relativeEcode) const;
  double GetTimeEstimation() const;

  vector<unsigned long> layerchanges;
  int getLayerNo(const double z) const;
  int getLayerNo(const unsigned long commandno) const;
  
private:
  unsigned long unconfirmed_blocks;
};
