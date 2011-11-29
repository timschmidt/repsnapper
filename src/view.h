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
#include <vmmlib/vmmlib.h>
#include "types.h"
#include "progress.h"

class Model;
class Render;
class Progress;
class RFO_Object;
class RFO_File;
class ConnectView;
class PrintInhibitor;

class View : public Gtk::Window
{
  class TempRow;
  class AxisRow;
  class TranslationSpinRow;

  friend class PrintInhibitor;

  void load_gcode();
  void save_gcode();
  void convert_to_gcode();
  void load_stl();
  void save_stl();
  void send_gcode();
  void printing_changed();
  void power_toggled();
  void kick_clicked();
  void print_clicked();
  void continue_clicked();
  void add_statusbar_msg(const char *name, const char *message);
  void add_statusbar_msg(Gtk::Widget *widget, const char *message);
  void update_scale_slider();
  void scale_object();

  Glib::RefPtr<Gtk::Builder> m_builder;
  Model *m_model;
  ViewProgress *m_progress;
  ConnectView *m_cnx_view;
  Gtk::Entry *m_gcode_entry;
  Render *m_renderer;

  Gtk::Button *m_print_button;
  Gtk::Button *m_continue_button;
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

  // rfo bits
  Gtk::TreeView *m_rfo_tree;
  TranslationSpinRow *m_translation_row;
  void delete_selected_stl();
  void duplicate_selected_stl();
  void auto_rotate();
  void update_settings_gui();
  bool key_pressed_event(GdkEventKey *event);
  bool moveSelected( float x, float y );

  void setModel (Model *model);
  void showAllWidgets();

  bool updateStatusBar(GdkEventCrossing *event, Glib::ustring message);

 public:
  Model *get_model() { return m_model; }
  bool get_selected_stl(RFO_Object *&object, RFO_File *&file);

  View(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);
  ~View();
  static View *create (Model *model);

  void progess_bar_start (const char *label, double max);

  void inhibit_print_changed();
  void alert (Gtk::MessageType t, const char *message,
	      const char *secondary);
  void rotate_selection (vmml::Vector4f rotate);
  void stl_added (Gtk::TreePath &path);

  vmml::Matrix4f &SelectedNodeMatrix(guint objectNr = 1);
  void SelectedNodeMatrices(std::vector<vmml::Matrix4f *> &result );
  void newObject();

  // view nasties ...
  void Draw (Gtk::TreeModel::iterator &selected);
  void DrawGrid ();
};

#ifdef MODEL_IMPLEMENTATION
#  error "The whole point is to avoid coupling the model to the view - think again"
#endif

#endif // VIEW_H
