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
#include "config.h"
#include <gtkmm.h>

#include "model.h"
#include "progress.h"

//ViewProgress::ViewProgress(Progress *progress, Gtk::Box *box, Gtk::ProgressBar *bar, Gtk::Label *label) :
ViewProgress::ViewProgress(Gtk::Box *box, Gtk::ProgressBar *bar, Gtk::Label *label) :
  m_box (box), m_bar(bar), m_label(label), to_terminal(true)
{
  m_bar_max = 0.0;
  box->hide();
  // progress->m_signal_progress_start.connect  (sigc::mem_fun(*this, &ViewProgress::start));
  // progress->m_signal_progress_update.connect (sigc::mem_fun(*this, &ViewProgress::update));
  // progress->m_signal_progress_stop.connect   (sigc::mem_fun(*this, &ViewProgress::stop));
  // progress->m_signal_progress_label.connect  (sigc::mem_fun(*this, &ViewProgress::set_label));
}

void ViewProgress::start (const char *label, double max)
{
  //  GDK_THREADS_ENTER ();
  m_box->show();
  m_bar_max = max;
  m_label->set_label (label);
  m_bar_cur = 0.0;
  m_bar->set_fraction(0.0);
  //g_main_context_iteration(NULL,false);
  Gtk::Main::iteration(false);
  //GDK_THREADS_LEAVE ();
}
void ViewProgress::restart (const char *label, double max)
{
  //m_box->show();
  //GDK_THREADS_ENTER ();
  if (to_terminal) {
    cerr << m_label->get_label() << " -- " << _(" done.") << "                     " << endl;  
  }
  m_bar_max = max;
  m_label->set_label (label);
  m_bar_cur = 0.0;
  m_bar->set_fraction(0.0);
  //g_main_context_iteration(NULL,false);
  Gtk::Main::iteration(false);
  //GDK_THREADS_LEAVE ();
}

void ViewProgress::stop (const char *label)
{
  //GDK_THREADS_ENTER ();
  if (to_terminal) {
    cerr << m_label->get_label() << " -- " << _(" done.") << "                     " << endl;  
  }
  m_label->set_label (label);
  m_bar_cur = m_bar_max;
  m_bar->set_fraction(1.0);
  m_box->hide();
  //g_main_context_iteration(NULL,false);
  Gtk::Main::iteration(false);
  //GDK_THREADS_LEAVE ();
}

void ViewProgress::update (double value)
{
  //GDK_THREADS_ENTER ();
  m_bar_cur = CLAMP(value, 0, 1.0);
  m_bar->set_fraction(value / m_bar_max);
  ostringstream o; o << value <<"/"<< m_bar_max;
  m_bar->set_text(o.str());
  if (to_terminal) {
    int perc = (int(m_bar->get_fraction()*100));
    cerr << m_label->get_label() << " " << o.str() << " -- " << perc << "%              \r";
  }
  //g_main_context_iteration(NULL,false);
  Gtk::Main::iteration(false);
  //GDK_THREADS_LEAVE ();
}

void ViewProgress::set_label (const std::string label)
{
  //GDK_THREADS_ENTER ();
  std::string old = m_label->get_label();
  if (old != label)
    m_label->set_label (label);
  //g_main_context_iteration(NULL,false);
  Gtk::Main::iteration(false);
  //GDK_THREADS_LEAVE ();
}

void ViewProgress::set_terminal_output (bool terminal)
{
  to_terminal=terminal;
}
