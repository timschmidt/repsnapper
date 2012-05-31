/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010  Kulitorum
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

// everything taken from model.h


#pragma once

#include "stdafx.h" 


enum RR_logtype  { LOG_COMM, LOG_ERROR, LOG_ECHO };
enum TempType { TEMP_NOZZLE, TEMP_BED, TEMP_LAST };


// use Glib::IOChannel instead of libreprap
#define IOCHANNEL 0

// use IOCHANNEL on WIN32
#ifdef WIN32
#undef IOCHANNEL
#define IOCHANNEL 1
#endif

#if IOCHANNEL
//#include "reprap_serial.h"
class RRSerial;
#else

# include <libreprap/comms.h>

// For libreprap callbacks
# ifdef WIN32
#  define RR_CALL __cdecl
# else
#  define RR_CALL
# endif

typedef struct rr_dev_t *rr_dev;

#endif

class Printer
{
	bool printing;
	void run_print ();

	unsigned long unconfirmed_lines;

	unsigned long lastdonelines; // for live view
	time_t lasttimeshown;

	bool debug_output;

	SerialState serial_state;
	void set_printing (bool printing);
	void update_core_settings();

	double temps[TEMP_LAST];
	sigc::connection temp_timeout;
	bool temp_timeout_cb();

  Glib::TimeVal print_started_time;
  Glib::TimeVal total_time_to_print ;

	View *m_view;
	Model *m_model;


#if IOCHANNEL
  sigc::connection watchprint_timeout;
  bool watchprint_timeout_cb();

  sigc::connection log_timeout;
  bool log_timeout_cb();

  RRSerial *rr_serial;
#endif

 public:
	Printer(View *view);
	~Printer();


	vector<string> find_ports() const;


  sigc::signal< void, unsigned long > signal_now_printing;

	bool inhibit_print;
	sigc::signal< void > signal_inhibit_changed;

	sigc::signal< void, Gtk::MessageType, const char *, const char * > signal_alert;
	void alert (const char *message);
	void error (const char *message, const char *secondary);

	SerialState get_serial_state () { return serial_state; }
	void serial_try_connect (bool connect);
	sigc::signal< void, SerialState > signal_serial_state_changed;

	double get_temp (TempType t) { return temps[(int)t]; }
	sigc::signal< void > signal_temp_changed;

	sigc::signal< void > printing_changed;

	sigc::signal< void > get_signal_inhibit_changed() { return signal_inhibit_changed; }
	bool get_inhibit_print() { return inhibit_print; }

	bool IsPrinting() { return printing; }

	void setModel(Model *model);

	bool IsConnected();
	void SimplePrint();

	void Print();
	void Pause();
	void Stop();
	void Continue();
	void Kick();
	void Restart();
	void Reset();
	
  bool SetTemp(TempType type, float value, int extruder_no=-1);

  bool SelectExtruder(int extruder_no=-1);

	double getCurrentPrintingZ();

	bool RunExtruder(double extruder_speed, double extruder_length, bool reverse,
			 int extruder_no=-1);
  bool SendNow(string str, long lineno = -1);

  void parse_response (string line);

	void EnableTempReading(bool on);
	void SetLogFileClear(bool on);
	bool SwitchPower(bool on);
	void UpdateTemperatureMonitor();

	bool Home(string axis);
	bool Move(string axis, double distance);
	bool Goto(string axis, double position);

  sigc::signal< void, string, RR_logtype > signal_logmessage;

  string commlog_buffer, error_buffer, echo_buffer;

#if IOCHANNEL
  void log(string s, RR_logtype type);
#else
	rr_dev device;
	sigc::connection devconn;

	int device_fd;

  void comm_log(string s);
  void err_log(string s);
  void echo_log(string s);
#endif

	void PrintButton();
	void StopButton();
	void ContinuePauseButton();
	void ResetButton();


  long get_next_line(string &line);

  const Model * getModel() const {return m_model; };

 private:
#if !IOCHANNEL
	bool handle_dev_fd (Glib::IOCondition cond);

	// libreprap integration
	static void RR_CALL rr_reply_fn   (rr_dev dev, int type, float value,
					   void *expansion, void *closure);
	static void RR_CALL rr_more_fn    (rr_dev dev, void *closure);
	static void RR_CALL rr_error_fn   (rr_dev dev, int error_code,
					   const char *msg, size_t len, void *closure);
	static void RR_CALL rr_wait_wr_fn (rr_dev dev, int wait, void *closure);
	static void RR_CALL rr_log_fn     (rr_dev dev, int type, const char *buffer,
					   size_t len, void *closure);

        void handle_rr_reply(rr_dev dev, int type, float value,
            void *expansion);
	void handle_rr_more (rr_dev dev);
	void handle_rr_error (rr_dev dev, int error_code,
	    const char *msg, size_t len);
	void handle_rr_wait_wr (rr_dev dev, int wait);
	void handle_rr_log (rr_dev dev, int type, const char *buffer,
            size_t len);
#endif
	GCodeIter *gcode_iter;
};


// Exception safe guard to stop people printing
// GCode while loading it / converting etc.
struct PrintInhibitor 
{
  Printer *printer;
public:
PrintInhibitor(Printer *p) : printer (p)
  {
    printer->inhibit_print = true;
    printer->signal_inhibit_changed.emit();
  }
  ~PrintInhibitor()
  {
    printer->inhibit_print = false;
    printer->signal_inhibit_changed.emit();
  }
};
