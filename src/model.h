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
#include "stl.h"
#include "rfo.h"
#include "gcode.h"
#include "settings.h"

enum FileType { TYPE_STL, TYPE_RFO, TYPE_GCODE, TYPE_AUTO };
#ifdef WIN32
#  pragma warning( disable : 4244 4267)
#endif

class View;
class ConnectView;
class RepRapSerial;
class PrintInhibitor;

class Model : public Gtk::Window
{
	class SpinRow;
	class TempRow;
	class AxisRow;

	Glib::RefPtr<Gtk::Builder> m_builder;
	Progress *m_progress;
	ConnectView *m_view;
	Gtk::Entry *m_gcode_entry;

	friend class PrintInhibitor;
	Gtk::Button *m_print_button;
	Gtk::Button *m_continue_button;
	Gtk::ToggleButton *m_power_button;

	void connect_button(const char *name, const sigc::slot<void> &slot);
	void connect_action(const char *name, const sigc::slot<void> &slot);
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

	// interactive bits
	void home_all();
	void enable_logging_toggled(Gtk::ToggleButton *button);
	void clear_logs();
	Gtk::CheckButton *m_extruder_reverse;
	Gtk::SpinButton *m_extruder_speed;
	Gtk::SpinButton *m_extruder_length;
	AxisRow *m_axis_rows[3];

	TempRow *m_temps[2];
	void temp_changed();

	// rfo bits
	Gtk::TreeView *m_rfo_tree;
	SpinRow *m_spin_rows[3];
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
	void Init();
	void SimpleAdvancedToggle();

	void SaveConfig(string filename);
	void LoadConfig() { LoadConfig ("repsnapper.conf"); }
	void LoadConfig(string filename);

	// RFO Functions
	void ReadRFO(string filename);

	// STL Functions
	void ReadStl(string filename);
	RFO_File *AddStl(STL stl, string filename);
	void OptimizeRotation();
	void RotateObject(Vector4f rotate);

	void setObjectname(string name);
	void setFileMaterial(string material);
	void setFileType(string type);
	void setFileLocation(string location);

	// GCode Functions
	void init();
	void ReadGCode(string filename);
	void ConvertToGCode();
	void MakeRaft(float &z);
	void WriteGCode(string filename);

	void draw() { queue_draw(); }

	// Callback functions
	vector<string> comports; // list of available usb serial ports
	bool timer_function();
	void DetectComPorts(bool init = false);
	string ValidateComPort (const string &port);

	// LUA
	void RunLua(char* buffer);

	// Communication
	void ConnectToPrinter(char on);
	bool IsConnected();
	void SimplePrint();
	void WaitForConnection(float seconds);

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
	void SetFan(int val);

	// LUA stuff
	void AddText(string line);
	void ClearGcode();
	string GetText();

	void Home(string axis);
	void Move(string axis, float distance);
	void Goto(string axis, float position);
	void STOP();

	Matrix4f &SelectedNodeMatrix(uint objectNr = 1);
	void SelectedNodeMatrices(vector<Matrix4f *> &result );
	void newObject();

	RepRapSerial *serial;

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

	// fltk compat foo ...
        void redraw();
	int w() { return get_width(); }
	int h() { return get_height(); }

	// Truly the model
	RFO rfo;
	GCode gcode;

	// view nasties ...
	vmml::Vector3f printOffset; // margin + raft
	void Draw (Gtk::TreeModel::iterator &selected);
	void DrawGrid ();

};

#endif // MODEL_VIEW_CONTROLLER_H
