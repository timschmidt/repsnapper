/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010 Michael Meeks

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#ifndef PROGRESS_H
#define PROGRESS_H

#include <string>
#include <gtkmm.h>

class Progress {
 public:
  // Progress reporting
  sigc::signal< void, const char *, double > m_signal_progress_start;
  sigc::signal< void, double >               m_signal_progress_update;
  sigc::signal< void, const char * >         m_signal_progress_stop;

  // helpers
  void start (const char *label, double max)
  {
    m_signal_progress_start.emit (label, max);
  }
  void stop (const char *label)
  {
    m_signal_progress_stop.emit (label);
  }
  void update (double value)
  {
    m_signal_progress_update.emit (value);
  }
};


class ViewProgress {
  Gtk::Box *m_box;
  Gtk::ProgressBar *m_bar;
  Gtk::Label *m_label;
  double m_bar_max;
  double m_bar_cur;
  void start (const char *label, double max);
  void stop (const char *label);
  void update (double value);
 public:
  ViewProgress(Progress *model, Gtk::Box *box, Gtk::ProgressBar *bar, Gtk::Label *label);
  void set_label (std::string label);
  double maximum() { return m_bar_max; }
  double value() { return m_bar_cur; }
};

#endif // PROGRESS_H
