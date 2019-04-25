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
//#include <glibmm/keyfile.h>
#include <glib.h>
#include <glibmm/ustring.h>


#include "stdafx.h"

#include <QSettings>
#include <QCoreApplication>
#include <QString>
#include <QFile>
#include <QtUiTools/QUiLoader>
#include <QMainWindow>
#include <QPushButton>

class ColorButton : public QPushButton {
    QColor color;
public:
    void set_color (QColor c) {
        color = c;
        QPalette pal;
        pal.setColor(QPalette::Background, color);
        setPalette(pal);
    }
    void set_color (float r, float g, float b, float alpha = 1.) {
        set_color(QColor(255*r,255*g,255*b,255*alpha));
    }
    QColor get_color(){return color;}
};

class Settings : public QSettings {
    Q_OBJECT

  QString filename; // where it's loaded from

  bool m_user_changed;
  bool inhibit_callback; // don't update settings from gui while setting to gui

 public:

  void copyGroup(const QString &from, const QString &to);

  // overwrite to get the chance to make multiple extruder manipulation

  int      get_integer (const QString &group, const QString &name);
  double   get_double  (const QString &group, const QString &name);
  bool     get_boolean (const QString &group, const QString &name);
  QString  get_string  (const QString &group, const QString &name);
  Vector4f get_colour  (const QString &group, const QString &name);
  //QStringList get_string_list(const QString &group, const QString &name);
  vector<double>   get_double_list  (const QString &group, const QString &name);

  using QSettings::value;
  QVariant value(const QString &group, const QString &key,
                 const QVariant &defaultValue = QVariant());
  QStringList get_keys(const QString &group);

  /* void set_integer (const string &group, const string &name, const int value); */
  /* void set_double  (const string &group, const string &name, const double value); */
  /* void set_boolean (const string &group, const string &name, const bool value); */
  /* void set_string  (const string &group, const string &name, const string &value); */
  void set_colour  (const QString &group, const QString &name, const Vector4f &value);
  void set_double_list  (const QString &group, const QString &name, const vector<double> &values);
  void set_to_gui              (QWidget *widget, const string filter="");

  QString numberedExtruder(const QString &group, int num=-1) const;


  vmml::vec3d getPrintVolume();
  vmml::vec3d getPrintMargin();


  static double RoundedLinewidthCorrection(double extr_width,
                       double layerheight);
  double GetExtrudedMaterialWidth(const double layerheight);
  double GetExtrusionPerMM(double layerheight);
  std::vector<QChar> get_extruder_letters();
  Vector3d get_extruder_offset(uint num);
  uint GetSupportExtruder();
  void CopyExtruder(uint num);
  void RemoveExtruder(uint num);
  void SelectExtruder(uint num, QWidget *widget = nullptr);
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

  bool set_to_gui              (QWidget *widget, const QString &group, const QString &key);
  void get_colour_from_gui     (ColorButton * colorButton,
                                const QString &group, const QString &key);
  void convert_old_colour      (const QString &group, const QString &key);
  void set_defaults ();
  QWidget* get_widget_and_setting(QWidget *widget, const QObject* qobject, QString &group, QString &key);

 public:
  Settings();
  ~Settings();

  bool has_user_changed() const { return m_user_changed; }

  bool set_user_button(const QString &name, const QString &gcode);
  bool del_user_button(const QString &name);
  QString get_user_gcode(const QString &name);

  Matrix4d getBasicTransformation(Matrix4d T);

  // return real mm depending on hardware extrusion width setting
  double GetInfillDistance(double layerthickness, float percent);


  // connect settings to relevant GUI widgets
  void connect_to_ui (QWidget *widget);


  void merge (const QSettings &settings);
  int mergeGlibKeyfile (const Glib::ustring keyfile);
  bool load_from_file (QString filename);
  bool load_from_data (QString data);

  void load_settings (QString filename);
  void load_settings_as (const Glib::ustring onlygroup = "",
             const QString as_group = "");
//  void save_settings (QString filename);
  void save_settings_as (const Glib::ustring onlygroup = "",
             const Glib::ustring as_group = "");

  std::string get_image_path();

  using QSettings::setValue;
  void setValue(const QString &group, const QString &key,
                const QVariant &value);
  using QSettings::remove;
  void remove(const QString &group, const QString &key);
  //sigc::signal< void > m_signal_visual_settings_changed;
//  sigc::signal< void > m_signal_update_settings_gui;
//  sigc::signal< void > m_signal_core_settings_changed;


  QString info();

public slots:
  void get_from_gui();
  void get_int_from_gui(int value);
  void get_double_from_gui(double value);

};

