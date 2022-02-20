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


#include "../model.h"
#include "progress.h"
#include <omp.h>

//ViewProgress::ViewProgress(Progress *progress, Gtk::Box *box, Gtk::ProgressBar *bar, Gtk::Label *label) :
ViewProgress::ViewProgress(QWidget *box, QProgressBar *bar, QLabel *label) :
  m_box (box), m_bar(bar), m_label(label), to_terminal(true),
  estimatedDuration(0.), estimatedTotal(0.)
{
  m_bar_max = 0.0;
  m_box->hide();
  m_label->hide();
  time = QElapsedTimer();
  lastUpdated = 0;
  // progress->m_signal_progress_start.connect  (sigc::mem_fun(*this, &ViewProgress::start));
  // progress->m_signal_progress_update.connect (sigc::mem_fun(*this, &ViewProgress::update));
  // progress->m_signal_progress_stop.connect   (sigc::mem_fun(*this, &ViewProgress::stop));
  // progress->m_signal_progress_label.connect  (sigc::mem_fun(*this, &ViewProgress::set_label));
}

void ViewProgress::start (const char *label, double max, double totalTime)
{
  do_continue = true;
  estimatedTotal = totalTime;
  m_box->show();
  m_bar_max = max;
  this->label = label;
  m_label->setText(label);
  m_bar_cur = 0.0;
  m_bar->setRange(0, int(max));
  m_bar->setValue(0);
  lastUpdated = 0;
  time.start();
  connect(this, SIGNAL(update_signal(double)), this, SLOT(update(double)));
}

// only start if not cancelled
bool ViewProgress::restart (const char *label, double max)
{
  if (!do_continue) return false;
  m_box->show();
  if (to_terminal) {
      const int time_used =  int(time.elapsed()/1000);
   QTextStream(stderr) << m_bar->text() << " -- " << _(" done in ") << time_used << _(" seconds") << "       " << endl;
  }
  m_bar_max = max;
  this->label = label;
  m_label->setText(label);
  m_bar_cur = 0.0;
  m_bar->setRange(0, int(max));
  m_bar->setValue(0);
  lastUpdated = 0;
  time.restart();
  //g_main_context_iteration(NULL,false);
//  Gtk::Main::iteration(false);
  return true;
}

void ViewProgress::stop (const char *label)
{
  if (to_terminal) {
        const int time_used = int(time.elapsed()/1000); // seconds
    QTextStream(stderr)  << m_bar->text() << " -- " << _(" done in ") << time_used << _(" seconds") << "       " << endl;
  }
  this->label = label;
  m_label->setText(label);
  m_bar_cur = m_bar_max;
  m_bar->setValue(0);
  m_box->hide();
//  Gtk::Main::iteration(false);
}

QString timeleft_str(long seconds) {
  QString s;
  QTextStream ostr(&s);
  int hrs = int(seconds/3600);
  if (hrs>0) {
    if (hrs>1) ostr << hrs << _("h ");
    else ostr << hrs << _("h ");
    seconds -= 3600*hrs;
  }
  if (seconds > 60)
    ostr << seconds/60 << _("m");
  if (hrs == 0 && seconds<300)
    ostr << " " << seconds%60 << _("s");
  return s;
}


bool ViewProgress::update (const double value)
{
  // Don't allow progress to go backward
//  if (value < m_bar_cur)
//    return do_continue;
  QCoreApplication::processEvents();
  if (time.elapsed() - lastUpdated < 200)
      return true;
  m_bar_cur = min(max(value, 0.), 1.0);
  QString s;
  QTextStream o(&s);
  const double elapsed = double(time.elapsed())/1000.; // seconds
  double totalTime;
  if (estimatedDuration > 0 && estimatedTotal > 0) {
      const double realfactor = elapsed / estimatedDuration;
      totalTime = estimatedTotal * realfactor;
      m_bar->setRange(0, int(totalTime));
      m_bar->setValue(int(elapsed));
  } else {
      m_bar->setRange(0, int(m_bar_max));
      m_bar->setValue(int(value));
      totalTime  = elapsed * m_bar_max  / value;
  }
  const long left = long(totalTime-elapsed);
  o << label << " (" << timeleft_str(left)
    << " of " << timeleft_str(long(totalTime)) << " left) : "
    << int(value) <<"/"<< int(m_bar_max);
  m_bar->setFormat(s);
  if (to_terminal) {
    int perc = (int(100.*m_bar->value()/m_bar->maximum()));
    QTextStream(stderr) << s << " -- "
                        << perc << "%              \r";
  }
  lastUpdated = time.elapsed();
  return do_continue;
}

void ViewProgress::set_label (const QString label)
{
  QString old = m_label->text();
  this->label = label;
  if (old != label)
    m_label->setText (label);
  m_label->adjustSize();
//  Gtk::Main::iteration(false);
}

void ViewProgress::set_terminal_output (bool terminal)
{
  to_terminal=terminal;
}

