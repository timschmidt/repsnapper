/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2011-12 martin.dieringer@gmx.de

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


#pragma once

#include "stdafx.h"


#include "printer.h"

//#if IOCHANNEL

using namespace Glib;

enum RR_response { SEND_OK, SEND_ERROR, SEND_REPEAT };

class RRSerial
{

  Printer *printer;
  int device_fd;
  Glib::RefPtr<IOChannel> iochannel; 

  sigc::connection out_connection;
  sigc::connection connect_signal_io (const sigc::slot< bool, IOCondition >& slot,
				      IOCondition condition, int priority);
  void connect_out_signal(bool connect);
  void connect_in(bool connect);
  Glib::Mutex io_mutex;

  Glib::RefPtr<Glib::MainContext> serial_context;
  Glib::Thread * serial_thread;

  bool runthread;
  Glib::Mutex print_mutex;
  void thread_run ();

  string line_for_printer(string str, long lineno);

  vector<unsigned long> resend_gcodelines; // save gcode line numbers for resend
  string last_line_sent;
  RR_response last_response;

  void log (string s, RR_logtype type = LOG_COMM) const;

 public:

  RRSerial(Printer *printer);
  ~RRSerial();

  int wait_for_oks;

  bool connect (const char * serialname);
  bool disconnect ();
  bool connected() const;

  RR_response send(string str, long printer_lineno);
  unsigned long set_resend(unsigned long printer_lineno);
  long printer_lineno;

  bool iochannel_in(IOCondition cond);
  bool iochannel_out(IOCondition cond);
  bool iochannel_err(IOCondition cond);
  
  void start_printing(bool from_begin = false, unsigned long gcodelines = 0);
  void stop_printing();

  void reset_printer() const;

  static bool test_port(const string serialname);

};


//#endif // IOCHANNEL
