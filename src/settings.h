/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010 Michael Meeks
    Copyright (C) 2013  martin.dieringer@gmx.de

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

#include <string>
#include <giomm/file.h>
#include <glibmm/keyfile.h>

#include "stdafx.h"


// Allow passing as a pointer to something to
// avoid including glibmm in every header.
typedef Glib::RefPtr<Gtk::Builder> Builder;


class Settings : public Glib::KeyFile {

  Glib::ustring filename; // where it's loaded from

  bool m_user_changed;
  bool inhibit_callback; // don't update settings from gui while setting to gui

 public:

  void copyGroup(const string &from, const string &to);

  // overwrite to get the chance to make multiple extruder manipulations

  /* int      get_integer (const string &group, const string &name) const; */
  /* double   get_double  (const string &group, const string &name) const; */
  /* bool     get_boolean (const string &group, const string &name) const; */
  /* string   get_string  (const string &group, const string &name) const; */
  Vector4f get_colour  (const string &group, const string &name) const;

  /* void set_integer (const string &group, const string &name, const int value); */
  /* void set_double  (const string &group, const string &name, const double value); */
  /* void set_boolean (const string &group, const string &name, const bool value); */
  /* void set_string  (const string &group, const string &name, const string &value); */
  void set_colour  (const string &group, const string &name, const Vector4f &value);

  string numberedExtruder(const string &group, int num=-1) const;


  vmml::vec3d getPrintVolume() const;
  vmml::vec3d getPrintMargin() const;


  static double RoundedLinewidthCorrection(double extr_width,
					   double layerheight);
  double GetExtrudedMaterialWidth(const double layerheight) const;
  double GetExtrusionPerMM(double layerheight) const;
  std::vector<char> get_extruder_letters() const;
  Vector3d get_extruder_offset(uint num) const;
  uint GetSupportExtruder() const;
  void CopyExtruder(uint num);
  void RemoveExtruder(uint num);
  void SelectExtruder(uint num, Builder *builder=NULL);
  uint selectedExtruder;
  uint getNumExtruders() const;


  /* class GCodeImpl; */
  /* enum GCodeTextType { */
  /*   GCODE_TEXT_START, */
  /*   GCODE_TEXT_LAYER, */
  /*   GCODE_TEXT_END, */
  /*   GCODE_TEXT_TYPE_COUNT */
  /* }; */
  /* struct GCodeType { */
  /*   GCodeImpl *m_impl; */
  /*   std::string getText(GCodeTextType t) const ; */
  /*   std::string getStartText() const { return getText (GCODE_TEXT_START); } */
  /*   std::string getLayerText() const { return getText (GCODE_TEXT_LAYER); } */
  /*   std::string getEndText()   const { return getText (GCODE_TEXT_END);   } */
  /* }; */
  /* GCodeType GCode; */


  // Paths we loaded / saved things to last time
  std::string STLPath;
  std::string RFOPath;
  std::string GCodePath;
  std::string SettingsPath;


 private:
  void set_to_gui              (Builder &builder, int i);
  void set_to_gui              (Builder &builder,
				const string &group, const string &key);
  void get_from_gui_old        (Builder &builder, int i);
  void get_from_gui            (Builder &builder, const string &glade_name);
  bool get_group_and_key       (int i, Glib::ustring &group, Glib::ustring &key);
  void get_colour_from_gui     (Builder &builder, const string &glade_name);
  void convert_old_colour      (const string &group, const string &key);
  void set_defaults ();

 public:

  Settings();
  ~Settings();

  bool has_user_changed() const { return m_user_changed; }
  void assign_from(Settings *pSettings);

  bool set_user_button(const string &name, const string &gcode);
  bool del_user_button(const string &name);
  string get_user_gcode(const string &name);

  Matrix4d getBasicTransformation(Matrix4d T) const;

  // return real mm depending on hardware extrusion width setting
  double GetInfillDistance(double layerthickness, float percent) const;

  // sync changed settings with the GUI eg. used post load
  void set_to_gui (Builder &builder, const string filter="");

  // connect settings to relevant GUI widgets
  void connect_to_ui (Builder &builder);


  void merge (const Glib::KeyFile &keyfile);
  bool load_from_file (string file);
  bool load_from_data (string data);

  void load_settings (Glib::RefPtr<Gio::File> file);
  void load_settings_as (const Glib::ustring onlygroup = "",
			 const Glib::ustring as_group = "");
  void save_settings (Glib::RefPtr<Gio::File> file);
  void save_settings_as (const Glib::ustring onlygroup = "",
			 const Glib::ustring as_group = "");

  std::string get_image_path();

  sigc::signal< void > m_signal_visual_settings_changed;
  sigc::signal< void > m_signal_update_settings_gui;
  sigc::signal< void > m_signal_core_settings_changed;
};

