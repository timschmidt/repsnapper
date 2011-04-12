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
#ifndef MODEL_VIEW_CONTROLLER_H
#define MODEL_VIEW_CONTROLLER_H

#include <math.h>

#include <giomm/file.h>

#include <reprap/comms.h>

#include "stl.h"
#include "rfo.h"
#include "gcode.h"
#include "settings.h"

enum FileType { TYPE_STL, TYPE_RFO, TYPE_GCODE, TYPE_AUTO };
#ifdef WIN32
#  pragma warning( disable : 4244 4267)
#endif

class Render;
class ConnectView;
class PrintInhibitor;

class Model : public Gtk::Window
{
	class TranslationSpinRow;
	class TempRow;
	class AxisRow;

	Glib::RefPtr<Gtk::Builder> m_builder;
	Progress *m_progress;
	ConnectView *m_view;
	Gtk::Entry *m_gcode_entry;
  bool m_printing;
  unsigned long m_unconfirmed_lines;

	friend class PrintInhibitor;
	Gtk::Button *m_print_button;
	Gtk::Button *m_continue_button;
	Gtk::ToggleButton *m_power_button;

	void connect_button(const char *name, const sigc::slot<void> &slot);
	void connect_action(const char *name, const sigc::slot<void> &slot);
	void connect_toggled(const char *name, const sigc::slot<void, Gtk::ToggleButton *> &slot);
	virtual bool on_delete_event(GdkEventAny* event);

	void load_gcode();
	void save_gcode();
	void load_stl();
	void save_stl();
	void send_gcode();
	void printing_changed();
	void power_toggled();
	void hide_on_response(int, Gtk::Dialog *dialog);
	void show_dialog(const char *name);
	void about_dialog();
	void load_settings();
	void save_settings();
	void save_settings_as();

  // Callbacks
  static void handle_reply(rr_dev device, void *data, rr_reply reply, float value);
  static void handle_send(rr_dev device, void *cbdata, void *blkdata, const char *block, size_t len);
  static void handle_recv(rr_dev device, void *data, const char *reply, size_t len);
  bool handle_dev_fd(Glib::IOCondition cond);
  static void handle_want_writable(rr_dev device, void *data, char state);

	// interactive bits
	void home_all();
	void enable_logging_toggled (Gtk::ToggleButton *button);
	void fan_enabled_toggled (Gtk::ToggleButton *button);
	void clear_logs();
	Gtk::CheckButton *m_extruder_reverse;
	Gtk::SpinButton *m_extruder_speed;
	Gtk::SpinButton *m_extruder_length;
	Gtk::SpinButton *m_extruder_speed_mult;
	Gtk::SpinButton *m_extruder_length_mult;
	Gtk::SpinButton *m_fan_voltage;
	AxisRow *m_axis_rows[3];

	TempRow *m_temps[2];
	void temp_changed();

  Glib::RefPtr<Gtk::TextBuffer> commlog, errlog, echolog;

	// rfo bits
	Gtk::TreeView *m_rfo_tree;
    TranslationSpinRow *translation_row;
	void delete_selected_stl();
	void duplicate_selected_stl();
	bool get_selected_stl(RFO_Object *&object, RFO_File *&file);

	sigc::signal< void > m_signal_rfo_changed;

public:
	// Something in the rfo changed
	sigc::signal< void > signal_rfo_changed() { return m_signal_rfo_changed; }

	Model(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);
	void progess_bar_start (const char *label, double max);
	void alert (const char *message);
	static void alert (Gtk::Window *toplevel, const char *message);

	static Model *create();
	~Model();
	void SimpleAdvancedToggle();

	void SaveConfig(Glib::RefPtr<Gio::File> file);
	void LoadConfig() { LoadConfig(Gio::File::create_for_path("repsnapper.conf")); }
	void LoadConfig(Glib::RefPtr<Gio::File> file);

	// RFO Functions
	void ReadRFO(std::string filename);

	// STL Functions
	void ReadStl(Glib::RefPtr<Gio::File> file);
	RFO_File *AddStl(STL stl, string filename);
	void OptimizeRotation();
	void RotateObject(Vector4f rotate);
    void ScaleObject();
    void UpdateScaleSlider();

	void setObjectname(string name);
	void setFileMaterial(string material);
	void setFileType(string type);
	void setFileLocation(string location);

	// GCode Functions
	void init();
	void ReadGCode(Glib::RefPtr<Gio::File> file);
	void ConvertToGCode();
	void MakeRaft(float &z);
	void WriteGCode(Glib::RefPtr<Gio::File> file);

	void draw() { queue_draw(); }


	// Communication
	bool IsConnected();
	void SimplePrint();

	void Print();
	void Continue();
	void Restart();

	void RunExtruder();
	void SendNow(string str);
	void setPort(string s);
	void setSerialSpeed(int s );
	void SetValidateConnection(bool validate);

	void EnableTempReading(bool on);
	void SetLogFileClear(bool on);
	void SwitchPower(bool on);

	void Home(string axis);
	void Move(string axis, float distance);
	void Goto(string axis, float position);
	void STOP();

	Matrix4f &SelectedNodeMatrix(uint objectNr = 1);
	void SelectedNodeMatrices(vector<Matrix4f *> &result );
	void newObject();

	rr_dev device;
  sigc::connection devconn;

	/*- Custom button interface -*/
	void SendCustomButton(int nr);
	void SaveCustomButton();
	void TestCustomButton();
	void GetCustomButtonText(int nr);
	void RefreshCustomButtonLabels();

	void PrintButton();
	void ContinuePauseButton();

	Settings settings;

	// Model derived: Bounding box info
	Vector3f Center;
	Vector3f Min;
	Vector3f Max;
	void CalcBoundingBoxAndCenter();

	// Truly the model
	RFO rfo;
	GCode gcode;

	// view nasties ...
	vmml::Vector3f printOffset; // margin + raft
	void Draw (Gtk::TreeModel::iterator &selected);
	void DrawGrid ();

};

#endif // MODEL_VIEW_CONTROLLER_H
