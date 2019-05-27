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
//#include <glibmm/ustring.h>


#include "stdafx.h"

#include <QSettings>
#include <QCoreApplication>
#include <QString>
#include <QFile>
#include <QtUiTools/QUiLoader>
#include <QMainWindow>
#include <QPushButton>

class ColorButton : public QPushButton{
    Q_OBJECT
    QColor color;
public:
    ColorButton(QWidget* widget);
    void set_color (QColor c);
    void set_color (float r, float g, float b, float alpha = 1.);
    QColor get_color(){return color;}
};

typedef struct {
    QString name;
    Vector3d offset;
    float maxLineSpeed;
    Vector4f displayColor;
} ExtruderSettings;


class Settings : public QSettings {
    Q_OBJECT

  QString filename; // where it's loaded from

  bool m_user_changed;
  bool inhibit_callback; // don't update settings from gui while setting to gui

 public:

  void copyGroup(const QString &from, const QString &to);

  QString grouped(const QString &name);
  QVariant groupedValue(const QString &name, const QVariant &deflt = QVariant());

  int      get_integer (const QString &name);
  double   get_double  (const QString &name, double deflt = 0.);
  bool     get_boolean (const QString &name, bool deflt = false);
  QString  get_string  (const QString &name);
  Vector4f get_Vector4f  (const QString &name);
  vector<float> get_array(const QString &name);

  using QSettings::value;
  QStringList get_keys(const QString &group);
  QStringList get_keys_and_values(const QString &group, QList<QVariant> &values);

  /* void set_integer (const string &group, const string &name, const int value); */
  /* void set_double  (const string &group, const string &name, const double value); */
  /* void set_boolean (const string &group, const string &name, const bool value); */
  /* void set_string  (const string &group, const string &name, const string &value); */
  void set_array  (const QString &key, const QColor &qcolor, bool overwrite=true);
  void set_array  (const QString &name, const Vector4f &value, bool overwrite=true);
  void set_array  (const QString &name, const vector<float> &values, bool overwrite=true);
  void set_ranges(const QString &name, const vector<float> &values);
  void set_all_to_gui (QWidget *widget, const string filter="");

  vmml::vec3d getPrintVolume();
  vmml::vec3d getPrintMargin();

  Vector3d getRotation();
  Vector3d getTranslation();
  Vector4d getScaleValues();

  void setRotation(const Vector3d &rot);
  void setTranslation(const Vector3d &trans);
  void setScaleValues(const Vector4d &scale);

  static double RoundedLinewidthCorrection(double extr_width,
                       double layerheight);
  double GetExtrudedMaterialWidth(const double layerheight, uint extruder);
  double GetExtrusionPerMM(double layerheight, uint extruder);
  std::vector<QChar> get_extruder_letters();
  Vector3d get_extruder_offset(uint num);
  uint GetSupportExtruder();
  uint CopyExtruder(uint num);
  void RemoveExtruder(uint num);
  uint getNumExtruders() const;

  double getLayerHeight() { return get_double("Slicing/LayerThickness");}

  vector<ExtruderSettings> getExtruderSettings();


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
  QString STLPath;
  QString RFOPath;
  QString GCodePath;
  QString SettingsPath;


 private:

  bool set_to_gui              (QWidget *widget, const QString &key);
  void get_colour_from_gui     (ColorButton * colorButton, const QString &key);
  void convert_old_colour      (const QString &group, const QString &key);
  void set_defaults ();
  void cleanup();

public:
  Settings();
  ~Settings();

  bool has_user_changed() const { return m_user_changed; }

  bool set_user_button(const QString &name, const QString &gcode);
  bool del_user_button(const QString &name);
  QString get_user_gcode(const QString &name);

  Matrix4d getBasicTransformation(Matrix4d T);

  // return real mm depending on hardware extrusion width setting
  double GetInfillDistance(double layerthickness, double percent, uint extruderNo);


  // connect settings to relevant GUI widgets
  void connect_to_gui (QWidget *widget);


  void merge (QSettings &settings);
  bool mergeGlibKeyfile (const QString &keyfile);
  bool load_from_file (const QString &filename);
  bool load_from_data (const QString &data);

//  void load_settings (QString filename);
//  void load_settings_as (const QString onlygroup = "",
//             const QString as_group = "");
//  void save_settings (QString filename);
  void save_settings_as (const QString onlygroup = "",
             const QString as_group = "");

  QString get_image_path();

  using QSettings::setValue;
  void setValue(const QString &group, const QString &key, const QVariant &value);
  using QSettings::remove;
  void remove(const QString &group, const QString &key);
  //sigc::signal< void > m_signal_visual_settings_changed;
//  sigc::signal< void > m_signal_update_settings_gui;
//  sigc::signal< void > m_signal_core_settings_changed;

  void setMaxHeight(QWidget *parent, const double h);

  QString info();

  static QString numbered(const QString &qstring, uint num);
  uint currentExtruder; // the extruder currently on GUI

signals:
  void settings_changed(const QString &name);
public slots:
  void get_from_gui();
  void get_int_from_gui(int value);
  void get_string_from_gui(QString &string);
  void get_double_from_gui(double value);

};

