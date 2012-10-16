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

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "command.h"

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
  std::string next_line_stripped();
  bool finished();
  double time_used;
  time_t time_started;
  double time_estimation;
  Command getCurrentCommand(Vector3d defaultwhere,
			    const vector<char> &E_letters);
  void set_to_lineno(long lineno);
};

class GCode
{

    int gl_List;

public:
  GCode();

  void Read  (Model *model, const vector<char> E_letters,
	      ViewProgress *progress, string filename);
  //void Write (Model *model, string filename);
  void draw  (const Settings &settings,
	      int layer=-1, bool liveprinting=false,
	      int linewidth=3);
  void drawCommands(const Settings &settings, uint start, uint end,
		    bool liveprinting, int linewidth, bool arrows, bool boundary=false);
  void MakeText(string &GcodeTxt, const Settings &settings,
		ViewProgress * progress);

  //bool append_text (const std::string &line);
  std::string get_text() const;
  void clear();

  std::vector<Command> commands;
  uint size() { return commands.size(); };

  Vector3d Min, Max, Center;

  void translate(Vector3d trans);

  Glib::RefPtr<Gtk::TextBuffer> buffer;
  GCodeIter *get_iter ();

  double GetTotalExtruded(bool relativeEcode) const;
  double GetTimeEstimation() const;

  void updateWhereAtCursor(const vector<char> &E_letters);
  Vector3d currentCursorWhere;
  Vector3d currentCursorFrom;
  Command currentCursorCommand;
  vector<uint> buffer_zpos_lines; // line numbers where a z position is set


  vector<unsigned long> layerchanges;
  int getLayerNo(const double z) const;
  int getLayerNo(const unsigned long commandno) const;
  unsigned long getLayerStart(const uint layerno) const;
  unsigned long getLayerEnd(const uint layerno) const;

private:
  unsigned long unconfirmed_blocks;
};
