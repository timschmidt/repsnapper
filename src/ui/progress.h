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
#include <QFrame>
#include <QLabel>
#include <QProgressBar>
#include <QTime>

class ViewProgress {
  QWidget *m_box;
  QProgressBar *m_bar;
  QLabel *m_label;
  double m_bar_max;
  double m_bar_cur;

  QTime time;

  QString label;

 public:
  void start (const char *label, double max);
  bool restart (const char *label, double max);
  void stop (const char *label = "");
  bool update (const double value, bool take_priority=true);
  ViewProgress(){}
  ViewProgress(QWidget *box, QProgressBar *bar, QLabel *label);
  void set_label (QString label);
  double maximum() { return m_bar_max; }
  double value() { return m_bar_cur; }
  bool to_terminal;
  void set_terminal_output(bool terminal);
  bool do_continue;
  void stop_running(){do_continue = false;}
};

#endif // PROGRESS_H
