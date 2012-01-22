/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2011 martin.dieringer@gmx.de

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

#include <gtkmm.h>
#include <time.h>

#include "types.h"
#include "config.h"
#include "settings.h"
#include "gcode.h"
#include "progress.h"


#include <libreprap/comms.h>

using namespace std;


// For libreprap callbacks
#ifdef WIN32
#  define RR_CALL __cdecl
#else
#  define RR_CALL
#endif

typedef struct rr_dev_t *rr_dev;

class Printer
{
	bool printing;

	unsigned long unconfirmed_lines;

	unsigned long lastdonelines; // for live view
	time_t lasttimeshown;

	SerialState serial_state;
	void set_printing (bool printing);
	void update_core_settings();

	double temps[TEMP_LAST];
	sigc::connection temp_timeout;
	bool temp_timeout_cb();

	time_t print_started_time;
	double total_time_to_print ;

	View *m_view;
        //GCode *m_gcode;
	Model *m_model;
 public:
	Printer(View *view);
	~Printer();

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
	void update_temp_poll_interval();

	bool IsPrinting() { return printing; }

        //void setGCode (GCode gcode);
	void setModel(Model *model);

	// Communication
	//void SetGcode(GCode gcode);

	bool IsConnected();
	void SimplePrint();

	void Print();
	void Pause();
	void Stop();
	void Continue();
	void Kick();
	void Restart();
	void Reset();
	
	void draw_current(Vector3d &from);
	double getCurrentPrintingZ();

	void RunExtruder(double extruder_speed, double extruder_length, bool reverse);
	void SendNow(string str);

	void EnableTempReading(bool on);
	void SetLogFileClear(bool on);
	void SwitchPower(bool on);

	void Home(string axis);
	void Move(string axis, double distance);
	void Goto(string axis, double position);

	rr_dev device;
	sigc::connection devconn;

	void PrintButton();
	void StopButton();
	void ContinuePauseButton();
	void ResetButton();

	Glib::RefPtr<Gtk::TextBuffer> commlog;

 private:
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
