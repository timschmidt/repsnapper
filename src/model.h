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
#ifndef MODEL_H
#define MODEL_H

#include <math.h>

#include <giomm/file.h>

#include "slicer.h"
#include "rfo.h"
#include "types.h"
#include "gcode.h"
#include "settings.h"

#ifdef WIN32
#  pragma warning( disable : 4244 4267)
#endif

// For libreprap callbacks
#ifdef WIN32
#  define RR_CALL __cdecl
#else
#  define RR_CALL
#endif

typedef struct rr_dev_t *rr_dev;

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

class Model
{
	// Exception safe guard to stop people printing
	// GCode while loading it / converting etc.
	struct PrintInhibitor {
	  Model *m_model;
	public:
	  PrintInhibitor(Model *model) : m_model (model)
	  {
	    m_model->m_inhibit_print = true;
	    m_model->m_signal_inhibit_changed.emit();
	  }
	  ~PrintInhibitor()
	  {
	    m_model->m_inhibit_print = false;
	    m_model->m_signal_inhibit_changed.emit();
	  }
	};

	bool m_printing;
	unsigned long m_unconfirmed_lines;

	sigc::signal< void > m_signal_rfo_changed;

	bool m_inhibit_print;
	sigc::signal< void > m_signal_inhibit_changed;

	double m_temps[TEMP_LAST];
	sigc::connection m_temp_timeout;
	bool temp_timeout_cb();

	SerialState m_serial_state;
	void set_printing (bool printing);

public:
	SerialState get_serial_state () { return m_serial_state; }
	void serial_try_connect (bool connect);
	sigc::signal< void, SerialState > m_signal_serial_state_changed;

	double get_temp (TempType t) { return m_temps[(int)t]; }
	sigc::signal< void > m_signal_temp_changed;

	sigc::signal< void > m_printing_changed;

	sigc::signal< void, Gtk::MessageType, const char *, const char * > m_signal_alert;
	void alert (const char *message);
	void error (const char *message, const char *secondary);

	Progress m_progress;

	// Something in the rfo changed
	sigc::signal< void > signal_rfo_changed() { return m_signal_rfo_changed; }

	sigc::signal< void > signal_inhibit_changed() { return m_signal_inhibit_changed; }
	bool get_inhibit_print() { return m_inhibit_print; }
	void update_temp_poll_interval();

	Model();
	~Model();
	void progess_bar_start (const char *label, double max);

	bool IsPrinting() { return m_printing; }
	void SimpleAdvancedToggle();
	void SaveConfig(Glib::RefPtr<Gio::File> file);
	void LoadConfig() { LoadConfig(Gio::File::create_for_path("repsnapper.conf")); }
	void LoadConfig(Glib::RefPtr<Gio::File> file);

	// STL Functions
	void ReadStl(Glib::RefPtr<Gio::File> file);
	RFO_File *AddStl(RFO_Object *parent, Slicer stl, string filename);
	sigc::signal< void, Gtk::TreePath & > m_signal_stl_added;

	void Read(Glib::RefPtr<Gio::File> file);

	void DeleteRFO(Gtk::TreeModel::iterator &iter);

	void OptimizeRotation(RFO_File *file, RFO_Object *object);
	void ScaleObject(RFO_File *file, RFO_Object *object, double scale);
	void RotateObject(RFO_File *file, RFO_Object *object, Vector4f rotate);
	bool updateStatusBar(GdkEventCrossing *event, Glib::ustring = "");

	// GCode Functions
	void init();
	void ReadGCode(Glib::RefPtr<Gio::File> file);
	void ConvertToGCode();
	void MakeRaft(GCodeState &state, float &z);
	void WriteGCode(Glib::RefPtr<Gio::File> file);
	void ClearGCode();
	Glib::RefPtr<Gtk::TextBuffer> GetGCodeBuffer();
	void GlDrawGCode(); // should be in the view

	// Communication
	bool IsConnected();
	void SimplePrint();

	void Print();
	void Pause();
	void Continue();
	void Kick();
	void Restart();

	void RunExtruder(double extruder_speed, double extruder_length, bool reverse);
	void SendNow(string str);

	void EnableTempReading(bool on);
	void SetLogFileClear(bool on);
	void SwitchPower(bool on);

	void Home(string axis);
	void Move(string axis, double distance);
	void Goto(string axis, double position);
	void STOP();

	Matrix4f &SelectedNodeMatrix(guint objectNr = 1);
	void SelectedNodeMatrices(vector<Matrix4f *> &result );
	void newObject();

	rr_dev m_device;
	sigc::connection m_devconn;

	void PrintButton();
	void ContinuePauseButton();
	void ClearLogs();

	Settings settings;

	// Model derived: Bounding box info
	Vector3f Center;
	Vector3f Min;
	Vector3f Max;
	vmml::Vector3f printOffset; // margin + raft

	void CalcBoundingBoxAndCenter();
        bool FindEmptyLocation(Vector3f &result, Slicer *stl);

	sigc::signal< void > m_model_changed;
	void ModelChanged();

	// Truly the model
	RFO rfo;
	Glib::RefPtr<Gtk::TextBuffer> commlog, errlog, echolog;

 private:
	GCode gcode;
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

	GCodeIter *m_iter;
};

#endif // MODEL_H
