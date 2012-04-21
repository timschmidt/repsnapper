/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2011 Michael Meeks

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
#ifndef VIEW_H
#define VIEW_H

#include <vector>
#include <gtkmm.h>

#include "stdafx.h"
//#include "progress.h"
#include "printer.h"

/* class Model; */
/* class Render; */
/* class TreeObject; */
/* class Shape; */
/* class ConnectView; */
//class PrintInhibitor;

class View : public Gtk::Window
{
  class TempRow;
  class AxisRow;
  class TranslationSpinRow;

  friend class PrintInhibitor;
  friend class Render;

  void toggle_fullscreen();
  void load_gcode();
  void save_gcode();
  void move_gcode_to_platform ();
  void convert_to_gcode();
  void load_stl();
  void autoarrange();
  void save_stl();
  void slice_svg();
  void send_gcode();
  void printing_changed();
  void power_toggled();
  void kick_clicked();
  void print_clicked();
  /* void stop_clicked(); */
  void continue_clicked();
  void reset_clicked();
  /* void add_statusbar_msg(const char *name, const char *message); */
  /* void add_statusbar_msg(Gtk::Widget *widget, const char *message); */
  void update_scale_value();
  void scale_object();
  void scale_object_x();
  void scale_object_y();
  void scale_object_z();

  Printer *m_printer;

  Glib::RefPtr<Gtk::Builder> m_builder;
  Model *m_model;
  ViewProgress *m_progress;
  ConnectView *m_cnx_view;
  Gtk::Entry *m_gcode_entry;
  Render *m_renderer;


  void on_gcodebuffer_cursor_set (const Gtk::TextIter &iter, 
				  const Glib::RefPtr <Gtk::TextMark> &refMark);
  Gtk::TextView * m_gcodetextview;

  Gtk::Button *m_print_button;
  /* Gtk::Button *m_stop_button; */
  Gtk::Button *m_continue_button;
  Gtk::Button *m_reset_button;
  Gtk::ToggleButton *m_power_button;

  void connect_button(const char *name, const sigc::slot<void> &slot);
  void connect_action(const char *name, const sigc::slot<void> &slot);
  void connect_toggled(const char *name, const sigc::slot<void, Gtk::ToggleButton *> &slot);
  virtual bool on_delete_event(GdkEventAny* event);

  void hide_on_response(int, Gtk::Dialog *dialog);
  void show_dialog(const char *name);
  void about_dialog();
  void load_settings();
  void save_settings();
  void save_settings_as();

  // interactive bits
  void enable_logging_toggled (Gtk::ToggleButton *button);
  void fan_enabled_toggled (Gtk::ToggleButton *button);
  void run_extruder();
  void clear_logs();
  void home_all();
  Gtk::CheckButton *m_extruder_reverse;
  Gtk::SpinButton *m_extruder_speed;
  Gtk::SpinButton *m_extruder_length;
  Gtk::SpinButton *m_extruder_speed_mult;
  Gtk::SpinButton *m_extruder_length_mult;
  Gtk::SpinButton *m_fan_voltage;
  AxisRow *m_axis_rows[3];

  TempRow *m_temps[TEMP_LAST];
  void temp_changed();

  void edit_custombutton(string name="", string code="", Gtk::ToolButton *button=NULL);
  void new_custombutton() {edit_custombutton();};
  void hide_custombutton_dlg(int code, Gtk::Dialog *dialog);
  void add_custombutton(string name, string gcode);
  void custombutton_pressed(string name, Gtk::ToolButton *button);

  // rfo bits
  Gtk::TreeView *m_treeview;
  TranslationSpinRow *m_translation_row;
  void delete_selected_objects();
  void duplicate_selected_objects();
  void split_selected_objects();
  void divide_selected_objects();
  void auto_rotate();
  void update_settings_gui();
  void handle_ui_settings_changed();
  bool key_pressed_event(GdkEventKey *event);
  bool moveSelected( float x, float y, float z=0);

  void setModel (Model *model);
  void showAllWidgets();

  bool saveWindowSizeAndPosition(Settings &settings) const;

  bool statusBarMessage(Glib::ustring message);
  void stop_progress();
  
 public:
  void setNonPrintingMode(bool noprinting=true, string filename="");
  void PrintToFile();
  string printtofile_name;

  Model *get_model() { return m_model; }
  ViewProgress *get_view_progress() { return m_progress; }
  bool get_selected_objects(vector<TreeObject*> &objects, vector<Shape*> &shapes);

  View(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);
  virtual ~View();
  static View *create (Model *model);

  void progess_bar_start (const char *label, double max);

  void inhibit_print_changed();
  void alert (Gtk::MessageType t, const char *message,
	      const char *secondary);
  void rotate_selection (Vector4d rotate);
  void twist_selection (double angle);
  void invertnormals_selection ();
  void mirror_selection ();
  void placeonplatform_selection ();
  void stl_added (Gtk::TreePath &path);
  void model_changed ();

  /* Matrix4d &SelectedNodeMatrix(guint objectNr = 1); */
  /* void SelectedNodeMatrices(std::vector<Matrix4d *> &result ); */
  /* void newObject(); */

  // view nasties ...
  void Draw (vector<Gtk::TreeModel::Path> &selected);
  void DrawGrid ();
  void showCurrentPrinting(unsigned long fromline, unsigned long toline);
};

#ifdef MODEL_IMPLEMENTATION
#  error "The whole point is to avoid coupling the model to the view - think again"
#endif

#endif // VIEW_H
