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

class Progress {
  Gtk::Box *m_box;
  Gtk::ProgressBar *m_bar;
  Gtk::Label *m_label;
  double m_bar_max;
  double m_bar_cur;
 public:
  Progress(Gtk::Box *box, Gtk::ProgressBar *bar, Gtk::Label *label);
  void start (const char *label, double max);
  void stop (const char *label);
  void update (double value);
  void set_label (std::string label);
  double maximum() { return m_bar_max; }
  double value() { return m_bar_cur; }
};

#endif // PROGRESS_H
