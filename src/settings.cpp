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

#include <cstdlib>
#include <glib.h>
//#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>

#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QTextStream>
#include <QLineEdit>
#include <QTextEdit>

#include "settings.h"
#include "stdafx.h"

#include "slicer/infill.h"
/*
 * How settings are intended to work:
 *
 * Settings is a subclass of QSettings.
 *
 * Qt Designer Widgets are named as <Group>_<Key>, so automatically
 * converted to Settings.  This works for most settings, but
 * there are exceptions...
 *
 * Old Glib .conf KeyFiles files can be imported.
 *
 * All default setting values have to be at least in the default .conf file
 *
 * A redraw is done on every change made by the GUI.
 */


#ifdef WIN32
#  define DEFAULT_COM_PORT "COM0"
#else
#  define DEFAULT_COM_PORT "/dev/ttyUSB0"
#endif

const string serialspeeds[] = { "9600", "19200", "38400", "57600", "115200", "230400", "250000" };


// convert GUI name to group/key
bool splitpoint(const QString &widget_name, QString &group, QString &key) {
    QStringList list = widget_name.split("_");
    if (list.size()<2)
        list = widget_name.split(".");
    if (list.size()<2)
         return false;
  group = list[0];
  key = list[1];
  return true;
}


void set_up_combobox(QComboBox *combo, vector<string> values)
{
  if (combo->model())
    return;
  combo->clear();
  for (uint i=0; i<values.size(); i++) {
    //cerr << " adding " << values[i] << endl;
      combo->addItem(QString(values[i].c_str()), QVariant());
  }
  combo->setCurrentIndex(0);
}

QString combobox_get_active_value(QComboBox *combo){
    return combo->currentText();
}

bool combobox_set_to(QComboBox *combo, QString value)
{
  int n = combo->findText(value);
  if (n >= 0){
      combo->setCurrentIndex(n);
      return true;
  }
  return false;
}



/////////////////////////////////////////////////////////////////



Settings::Settings ()
{
  set_defaults();
  m_user_changed = false;
  inhibit_callback = false;
}

Settings::~Settings()
{
}

// merge into current settings
void Settings::merge (const QSettings &settings)
{
    for (QString group : settings.childGroups()) {
        beginGroup(group);
        for (QString key : settings.allKeys()){
            setValue(key, settings.value(key));
        }
        endGroup();
    }
}


int Settings::mergeGlibKeyfile (const Glib::ustring keyfile)
{
    GKeyFile * gkf = g_key_file_new();
    if (!g_key_file_load_from_file(gkf, keyfile.data(), G_KEY_FILE_NONE, NULL)){
        fprintf (stderr, "Could not read config file %s\n", keyfile.data());
        return EXIT_FAILURE;
    }
    GError *error;
    gsize ngroups;
    gchar ** groups = g_key_file_get_groups(gkf, &ngroups);
    for (uint g = 0; g < ngroups; g++) {
        beginGroup(QString(groups[g]));
        gsize nkeys;
        gchar ** keys = g_key_file_get_keys (gkf, groups[g], &nkeys, &error);
        for (uint k = 0; k < nkeys; k++) {
            gchar * v = g_key_file_get_value (gkf, groups[g], keys[k], &error);
            setValue(QString(keys[k]).replace('.','_'), QString(v));
        }
        endGroup();
    }
    QTextStream(stderr) << info() << endl;
    return ngroups;
}

QString Settings::info(){
    QString str;
    QTextStream s(&str);
    for (QString g : childGroups()){
        beginGroup(g);
        for (QString k : childKeys()){
            s << g << "_" << k << "\t = " << value(k).toString() << endl;
        }
        endGroup();
    }
    return str;
}

// always merge when loading settings
bool Settings::load_from_file (QString filename) {
  merge(QSettings(filename, Format::NativeFormat));
  return true;
}
bool Settings::load_from_data (QString data) {
  merge(QSettings());
  return true;
}



// make "ExtruderN" group, if i<0 (not given), use current selected Extruder number
QString Settings::numberedExtruder(const QString &group, int num) const {
  if (num >= 0 && group == "Extruder") {
      return QString("Extruder").append(num);
  }
  return group;
}

Vector4f Settings::get_colour(const QString &group, const QString &name) {
    beginGroup(group);
    int size = beginReadArray(name);
    vector<float> s;
    for (int i=0; i<size; i++){
        setArrayIndex(i);
        s.push_back(value("rgba").toFloat());
    }
    endArray();
    endGroup();
    return Vector4f(float(s[0]),float(s[1]),float(s[2]),float(s[3]));
}

vector<double> Settings::get_double_list(const QString &group, const QString &name)
{
    beginGroup(group);
    int size = beginReadArray(name);
    vector<double> s;
    for (int i=0; i<size; i++){
        setArrayIndex(i);
        s.push_back(value("double").toDouble());
    }
    endArray();
    endGroup();
    return s;
}

/*QStringList Settings::get_string_list(const QString &group, const QString &name)
{
    beginGroup(group);
    int num = beginReadArray(name);
    QStringList list;
    for (int i = 0; i<num; i++){
        list.append(value("string").toString());
    }
    endArray();
    endGroup();
    return list;
}*/

void Settings::set_colour(const QString &group, const QString &name,
                          const Vector4f &value) {
   beginGroup(group);
   beginWriteArray(name);
   for (int i=0; i<4; i++){
       setArrayIndex(i);
       setValue("rgba",value[i]);
   }
   endArray();
   endGroup();
}

void Settings::set_double_list(const QString &group, const QString &name, const vector<double> &values)
{
    beginGroup(group);
    beginWriteArray(name);
    for (int i=0; i<values.size(); i++){
        setArrayIndex(i);
        setValue("double",values[i]);
    }
    endArray();
    endGroup();
}


QVariant Settings::value(const QString &group, const QString &key,
                            const QVariant &defaultValue)
{
    beginGroup(group);
    QVariant v = value(key, defaultValue);
    endGroup();
    return v;
}

QStringList Settings::get_keys(const QString &group)
{
    beginGroup(group);
    QStringList keys = allKeys();
    endGroup();
    return keys;
}


void Settings::set_defaults ()
{
  filename = "";

  beginGroup("Global");
  setValue("SettingsName","Default Settings");
  setValue("SettingsImage","");
  setValue("Version",VERSION);
  endGroup();
  beginGroup("GCode");
  setValue("Start",
         "; This code is sent to the printer at the beginning.\n"
         "; Adjust it to your needs.\n"
         "; \n"
         "; GCode generated by RepSnapper:\n"
         "; http://reprap.org/wiki/RepSnapper_Manual:Introduction\n"
         "G21             ; metric coordinates\n"
         "G90             ; absolute positioning\n"
         "T0              ; select new extruder\n"
         "G28             ; go home\n"
         "G92 E0          ; set extruder home\n"
         "G1 X5 Y5 F500 ; move away 5 mm from 0.0, to use the same reset for each layer\n\n");
  setValue("Layer","");
  setValue("End",
         "; This code is sent to the printer after the print.\n"
         "; Adjust it to your needs.\n"
         "G1 X0 Y0 F2000.0 ; feed for start of next move\n"
         "M104 S0.0        ; heater off\n");
  endGroup();


  // Extruders.clear();
  // Extruders.push_back(Extruder);

  beginGroup("Hardware");
  // The vectors map each to 3 spin boxes, one per dimension
  setValue("Volume.X", 200);
  setValue("Volume.Y", 200);
  setValue("Volume.Z", 140);
  setValue("PrintMargin.X", 10);
  setValue("PrintMargin.Y", 10);
  setValue("PrintMargin.Z", 0);
  endGroup();
  setValue("Misc","SpeedsAreMMperSec",true);
}


// make old single coordinate colours to lists
void Settings::convert_old_colour  (const QString &group, const QString &key) {
  try {
    QTextStream(stderr) << "converting  "<< group << "_" <<key <<endl;
    const Vector4f color(get_double(group, key+"R"),
                         get_double(group, key+"G"),
                         get_double(group, key+"B"),
                         get_double(group, key+"A"));
    set_colour(group, key, color);
    remove(group, key+"R");
    remove(group, key+"G");
    remove(group, key+"B");
    remove(group, key+"A");
    } catch (const Glib::Exception &err) {
        cerr << err.what() << endl;
    }
}

void Settings::load_settings (QString filename)
{
  inhibit_callback = true;

  // set_defaults();

  if (childGroups().contains("Extruder"))
    remove("Extruder"); // avoid converting old if merging new file

  try {
    if (!load_from_file (filename)) {
      QTextStream(stdout) << tr("Failed to load settings from file '") << filename << "'\n";
      return;
    }
  } catch (const Glib::Exception &err) {
      QTextStream(stdout) << tr("Exception ") << err.what().c_str()
              << tr(" loading settings from file '") << filename << "'\n";
    return;
  }

  QTextStream(stderr) << tr("Parsing config from '") << filename << "'\n";

  // convert old colour handling:
  for (QString g : childGroups()) {
    //cerr << "["<<groups[g] << "] " ;
      beginGroup(g);
      for (QString k : childKeys()) {
          int n = k.length();
          int c = k.indexOf("Colour");
          if (c >= 0 && c < n-6 && k[c+6] == "R")
              convert_old_colour(g, k.left(c+6));
      }
      endGroup();
  }

  // convert old user buttons:
  QStringList CustomButtonLabels;
  QStringList CustomButtonGCodes;
  if (childGroups().contains("UserButtons")) {
    CustomButtonLabels = value("UserButtons","Labels").toStringList();
    CustomButtonGCodes = value("UserButtons","GCodes").toStringList();
  }
  try {
      for (QString k : get_keys("CustomButtons")) {
          bool havekey = false;
          for (int o = 0; o < CustomButtonLabels.size(); o++) {
              if (CustomButtonLabels[o] == k) {
                  CustomButtonGCodes[o] = get_string("CustomButtons",k);
                  havekey = true;
                  break;
              }
          }
          if (!havekey) {
              CustomButtonLabels.push_back(k);
              CustomButtonGCodes.push_back(get_string("CustomButtons",k));
          }
      }
      remove("CustomButtons");
  } catch (Glib::Exception &err) {}
  if (!childGroups().contains("UserButtons")) {
    setValue("UserButtons","Labels",CustomButtonLabels);
    setValue("UserButtons","GCodes",CustomButtonGCodes);
  }

  // convert old extruders, now we count "Extruder0", "Extruder1" ...
  // instead of "Extruder", "Extruder2" ...
  if (childGroups().contains("Extruder")) { // have old Extruders
    int ne = int(getNumExtruders() + 1); // +1 because "Extruder" is not counted
    if (childGroups().contains("Extruder0")) ne--; // already have "new" Extruders
    copyGroup("Extruder","Extruder0");
    if (ne > 1) {
      for (int k = 2; k < ne+1; k++) // copy 2,3,4,... to 1,2,3,...
          copyGroup(numberedExtruder("Extruder",k),
                    numberedExtruder("Extruder",k-1));
      remove(numberedExtruder("Extruder",ne));
    }
    remove("Extruder");
  }
  int ne = int(getNumExtruders());
  for (int k = 0; k < ne; k++) {
      beginGroup(numberedExtruder("Extruder",k));
      if (!childKeys().contains("OffsetX"))
          setValue("OffsetX", 0);
      if (!childKeys().contains("OffsetY"))
          setValue("OffsetY", 0);
      endGroup();
  }
  SelectExtruder(0);

  if (childGroups().contains("Misc")) {
      beginGroup("Misc");
      if(!childKeys().contains("SpeedsAreMMperSec")  ||
              !value("SpeedsAreMMperSec").toBool()) {
          cout << "!!!" << endl << _("\tThe config file has old speed settings (mm/min).\n\t "
                                     "Adjust them to mm/sec\n\t or use RepSnapper from 2.0 to 2.2 "
                                     "to convert them automatically.") << "!!!" <<  endl;
          setValue("SpeedsAreMMperSec",true);
          endGroup();
      }
      endGroup();
  }
  inhibit_callback = false;
  m_user_changed = false;
//  m_signal_visual_settings_changed.emit();
//  m_signal_update_settings_gui.emit();
}

/*
void Settings::save_settings(QString filename)
{
  inhibit_callback = true;
  setValue("Global","Version",VERSION);

  remove("Extruder"); // is only temporary


  Glib::ustring contents = to_data();
  // cerr << contents << endl;
  Glib::file_set_contents(filename.toUtf8().constData(), contents);

  SelectExtruder(selectedExtruder); // reload default extruder

  inhibit_callback = false;
  // all changes safely saved
  m_user_changed = false;
}*/

bool Settings::set_to_gui (QWidget *widget, const QString &group,
                           const QString &key)
{
  inhibit_callback = true;
  QString ks(key);
  QString widget_name = group + "_" + ks.replace('.','_');
  QWidget *w = widget->findChild<QWidget*>(widget_name);
  if (!w) {
//      QTextStream(stderr) << "    " << widget_name << tr(" not defined in GUI!")<< endl;
      return false;
  }
  cerr << "setting to gui: " << widget_name.toStdString()
       << " = " << value(group, key).toString().toStdString() << endl;
  QCheckBox *check = dynamic_cast<QCheckBox *>(w);
  if (check) {
    check->setChecked(get_boolean(group,key));
    return true;
  }
  QSpinBox *spin = dynamic_cast<QSpinBox *>(w);
  if (spin) {
    spin->setValue(get_double(group,key));
    return true;
  }
  QDoubleSpinBox *dspin = dynamic_cast<QDoubleSpinBox *>(w);
  if (dspin) {
    dspin->setValue(get_double(group,key));
    return true;
  }
  QSlider *range = dynamic_cast<QSlider *>(w);
  if (range) {
    range->setValue(get_double(group,key));
    return true;
  }
  QComboBox *combo = dynamic_cast<QComboBox *>(w);
  if (combo) {
    if (widget_name == "Hardware_SerialSpeed") // has real value
      combobox_set_to(combo, get_string(group, key));
    else // has index
      combo->setCurrentIndex(get_integer(group,key));
    return true;
  }
  QLineEdit *entry = dynamic_cast<QLineEdit *>(w);
  if (entry) {
    entry->setText(get_string(group,key));
    return true;
  }
  /*Gtk::Expander *exp = dynamic_cast<Gtk::Expander *>(w);
  if (exp) {
    exp->set_expanded (get_boolean(group,key));
    return;
  }*/
  ColorButton *col = dynamic_cast<ColorButton *>(w);
  if(col) {
    vector<double> c = get_double_list(group,key);
    col->set_color(c[0],c[1],c[2],c[3]);
    return true;
  }
  QTextEdit *tv = dynamic_cast<QTextEdit *>(w);
  if (tv) {
    tv->setText(get_string(group,key));
    return true;
  }

  QTextStream(stderr) << tr("set_to_gui of ") << widget_name << " not done!" << endl;
  return false;
}


void Settings::get_colour_from_gui (ColorButton * colorButton,
                                    const QString &group, const QString &key)
{
  QColor c = colorButton->get_color();

  // FIXME: detect 'changed' etc.
  vector<double> d(4);
  d[0] = c.red()/255.;
  d[1] = c.green()/255.;
  d[2] = c.blue()/255.;
  d[3] = c.alpha() / 255.0;

  set_double_list(group, key, d);

//  m_signal_visual_settings_changed.emit();
}


// whole group or all groups
void Settings::set_to_gui (QWidget *widget, const string filter)
{
  inhibit_callback = true;
  for (QString g : childGroups()) {
      beginGroup(g);
      for (QString k: get_keys(g)) {
          set_to_gui(widget, g, k);
      }
      endGroup();
  }

  //set_filltypes_to_gui (QUiLoader);

  if (filter == "" || filter == "Misc") {
      beginGroup("Misc");
      int w = value("WindowWidth").toInt();
      int h = value("WindowHeight").toInt();
      if (widget && w > 0 && h > 0) widget->resize(w,h);
      int x = value("WindowPosX").toInt();
      int y = value("WindowPosY").toInt();
      if (widget && x > 0 && y > 0) widget->move(x,y);
      endGroup();
  }

  // Set serial speed. Find the row that holds this value
  if (filter == "" || filter == "Hardware") {
    QComboBox *portspeed = widget->findChild<QComboBox*>("Hardware_SerialSpeed");
    if (portspeed) {
        QString s;
        QTextStream ostr(&s);
        ostr << get_integer("Hardware","SerialSpeed");
        //cerr << "portspeed " << get_integer("Hardware","SerialSpeed") << endl;
        combobox_set_to(portspeed, s);
    }
  }
  inhibit_callback = false;
}

template<typename Base, typename T>
 bool instanceof(const T*) {
    return std::is_base_of<Base, T>::value;
}


void Settings::connect_to_ui (QWidget *widget)
{
  if (childGroups().contains("Ranges")) {
    QStringList ranges = get_keys("Ranges");
    for (uint i = 0; i < ranges.size(); i++) {
      // get min, max, increment, page-incr.
      vector<double> vals = get_double_list("Ranges", ranges[i]);
      if (vals.size() >= 2) {
          QWidget *w = widget->findChild<QWidget*>(ranges[i]);
          if (!w) {
              QTextStream(stderr) << "Missing user interface item " << ranges[i] << "\n";
              continue;
          }
          QSpinBox *spin = dynamic_cast<QSpinBox *>(w);
          if (spin) {
              spin->setRange(vals[0],vals[1]);
              //TODO   spin->set_increments (vals[2],vals[3]);
              continue;
          }
          QDoubleSpinBox *dspin = dynamic_cast<QDoubleSpinBox *>(w);
          if (dspin) {
              dspin->setRange(vals[0],vals[1]);
              //TODO   spin->set_increments (vals[2],vals[3]);
              continue;
          }
          QAbstractSlider *range = dynamic_cast<QAbstractSlider *>(w); // sliders etc.
          if (range) {
              range->setRange(vals[0],vals[1]);
              //TODO range->set_increments (vals[2],vals[3]);
              continue;
          }
      }
    }
  }

  // add signal callbacks to all GUI elements with "_"
  QList<QWidget*> widgets_with_setting =
          widget->findChildren<QWidget*>(QRegularExpression(".+_.+"));
  for (int i=0; i< widgets_with_setting.size(); i++){
      QWidget *w = (widgets_with_setting[i]);
      QString group,key;
      splitpoint(w->objectName(), group, key);
      QCheckBox *check = dynamic_cast<QCheckBox *>(w);
      if (check) {
          widget->connect(check, SIGNAL(stateChanged(int)), this, SLOT(get_int_from_gui(int)));
          continue;
      }
      QSpinBox *spin = dynamic_cast<QSpinBox *>(w);
      if (spin) {
          widget->connect(spin, SIGNAL(valueChanged(int)), this, SLOT(get_int_from_gui(int)));
          continue;
      }
      QDoubleSpinBox *dspin = dynamic_cast<QDoubleSpinBox *>(w);
      if (dspin) {
          widget->connect(dspin, SIGNAL(valueChanged(double)), this, SLOT(get_double_from_gui(double)));
          continue;
      }
      QAbstractSlider *range = dynamic_cast<QAbstractSlider *>(w); // sliders etc.
      if (range) {
          widget->connect(range, SIGNAL(valueChanged(int)), this, SLOT(get_int_from_gui(int)));
          continue;
      }
      QComboBox *combo = dynamic_cast<QComboBox *>(w);
      if (combo) {
          if (w->objectName() == "Hardware.SerialSpeed") { // Serial port speed
              vector<string> speeds(serialspeeds,
                                    serialspeeds+sizeof(serialspeeds)/sizeof(string));
              set_up_combobox(combo, speeds);
          } else if (w->objectName().contains("Filltype")) { // Infill types
              uint nfills = sizeof(InfillNames)/sizeof(string);
              vector<string> infills(InfillNames,InfillNames+nfills);
              set_up_combobox(combo,infills);
          }
          widget->connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(get_int_from_gui(int)));
          continue;
      }
      QLineEdit *e = dynamic_cast<QLineEdit *>(w);
      if (e) {
          widget->connect(e, SIGNAL(editingFinished()), this, SLOT(get_from_gui()));
          continue;
      }
      /* Gtk::Expander *exp = dynamic_cast<Gtk::Expander *>(w);
      if (exp) {
        exp->property_expanded().signal_changed().connect
          (sigc::bind(sigc::bind<string>(sigc::mem_fun(*this, &Settings::get_from_gui), widget_name), QUiLoader));
        continue;
      }*/
      ColorButton *cb = dynamic_cast<ColorButton *>(w);
      if (cb) {
          widget->connect(cb, SIGNAL(clicked()), this, SLOT(get_from_gui()));
          continue;
      }
      QTextEdit *tv = dynamic_cast<QTextEdit *>(w);
      if (tv) {
          widget->connect(tv, SIGNAL(textChanged()), this, SLOT(get_from_gui()));
          continue;
      }
  }

  /* Update UI with defaults */
//  m_signal_update_settings_gui.emit();
}


// extrusion ratio for round-edge lines
double Settings::RoundedLinewidthCorrection(double extr_width,
                        double layerheight)
{
  double factor = 1 + (M_PI/4.-1) * layerheight/extr_width;
  // assume 2 half circles at edges
  //    /-------------------\     //
  //   |                     |    //
  //    \-------------------/     //
  //cerr << "round factor " << factor << endl;
  return factor;
}

double Settings::GetExtrudedMaterialWidth(double layerheight)
{
  // ExtrudedMaterialWidthRatio is preset by user
  return min(max(get_double("Extruder","MinimumLineWidth"),
         get_double("Extruder","ExtrudedMaterialWidthRatio") * layerheight),
         get_double("Extruder","MaximumLineWidth"));
}

// TODO This depends whether lines are packed or not - ellipsis/rectangle

// how much mm filament material per extruded line length mm -> E gcode
double Settings::GetExtrusionPerMM(double layerheight)
{
  double f = get_double("Extruder","ExtrusionFactor"); // overall factor
  if (get_boolean("Extruder","CalibrateInput")) {  // means we use input filament diameter
    const double matWidth = GetExtrudedMaterialWidth(layerheight); // this is the goal
    // otherwise we just work back from the extruded diameter for now.
    const double filamentdiameter = get_double("Extruder","FilamentDiameter");
    f *= (matWidth * matWidth) / (filamentdiameter * filamentdiameter);
  } // else: we work in terms of output anyway;

  return f;
}

// return infill distance in mm
double Settings::GetInfillDistance(double layerthickness, float percent)
{
  double fullInfillDistance = GetExtrudedMaterialWidth(layerthickness);
  if (percent == 0) return 10000000;
  return fullInfillDistance * (100./percent);
}

uint Settings::getNumExtruders() const
{
  uint num=0;
  for (QString g : childGroups())
    if (g.startsWith("Extruder")
            && g.length() > 8 ) // count only numbered
      num++;
  return num;
}

std::vector<QChar> Settings::get_extruder_letters()
{
  uint num = getNumExtruders();
  std::vector<QChar> letters(num);
  for (uint i = 0; i < num; i++)
    letters[i] = get_string(numberedExtruder("Extruder",i),"GCLetter")[0];
  return letters;
}

uint Settings::GetSupportExtruder()
{
  uint num = getNumExtruders();
  for (uint i = 0; i < num; i++)
    if (get_boolean(numberedExtruder("Extruder",i),"UseForSupport"))
      return i;
  return 0;
}

Vector3d Settings::get_extruder_offset(uint num)
{
  QString ext = numberedExtruder("Extruder",num);
  return Vector3d(get_double(ext, "OffsetX"),
          get_double(ext, "OffsetY"), 0.);
}


void Settings::copyGroup(const QString &from, const QString &to)
{
  QStringList keys = get_keys(from);
  for (uint i = 0; i < keys.size(); i++)
      setValue(to, keys[i], value(from, keys[i]));
}

int Settings::get_integer(const QString &group, const QString &name)
{
    return value(group, name).Int;
}

double Settings::get_double(const QString &group, const QString &name)
{
    return value(group, name).Double;
}

bool Settings::get_boolean(const QString &group, const QString &name)
{
    return value(group, name).Bool;
}

QString Settings::get_string(const QString &group, const QString &name)
{
    return value(group, name).toString();
}


// create new
void Settings::CopyExtruder(uint num)
{
  uint total = getNumExtruders();
  QString from = numberedExtruder("Extruder",num);
  QString to   = numberedExtruder("Extruder",total);
  copyGroup(from, to);
}

void Settings::RemoveExtruder(uint num)
{
  remove(QString("Extruder").append(num));
}

void Settings::SelectExtruder(uint num, QWidget *widget)
{
  if (num >= getNumExtruders()) return;
  selectedExtruder = num;
  copyGroup(numberedExtruder("Extruder",num),"Extruder");
  // show Extruder settings on gui
  if (widget) {
    set_to_gui(widget, "Extruder" "");
  }
}

Matrix4d Settings::getBasicTransformation(Matrix4d T)
{
  Vector3d t;
  T.get_translation(t);
  const Vector3d margin = getPrintMargin();
  double rsize = get_double("Raft","Size") * (get_boolean("Raft","Enable")?1:0);
  t+= Vector3d(margin.x() + rsize, margin.y() + rsize, 0);
  T.set_translation(t);
  return T;
}


Vector3d Settings::getPrintVolume()
{
  return Vector3d (get_double("Hardware","Volume.X"),
                   get_double("Hardware","Volume.Y"),
                   get_double("Hardware","Volume.Z"));

}
Vector3d Settings::getPrintMargin()
{
  Vector3d margin(get_double("Hardware","PrintMargin.X"),
          get_double("Hardware","PrintMargin.Y"),
          get_double("Hardware","PrintMargin.Z"));
  Vector3d maxoff = Vector3d::ZERO;
  uint num = getNumExtruders();
  for (uint i = 0; i < num ; i++) {
    QString ext = numberedExtruder("Extruder",i);
    double offx = 0, offy = 0;
    try {
      offx = abs(get_double(ext, "OffsetX"));
      offy = abs(get_double(ext, "OffsetY"));
    } catch (const Glib::Exception &err) {
    }
    if (offx > abs(maxoff.x())) maxoff.x() = offx;
    if (offy > abs(maxoff.y())) maxoff.y() = offy;
  }
  if (get_boolean("Slicing","Skirt")) {
    double distance = get_double("Slicing","SkirtDistance");
    maxoff += Vector3d(distance, distance, 0);
  }
  return margin + maxoff;
}


// Locate it in relation to ourselves ...
std::string Settings::get_image_path()
{
  std::string basename = Glib::path_get_dirname(filename.toUtf8().constData());
  return Glib::build_filename (basename, get_string("Global","Image").toUtf8().data());
}

void Settings::setValue(const QString &group, const QString &key, const QVariant &value)
{
    beginGroup(group);
    QTextStream(stderr) << "setting " << group << " -- " << key << "\t = " << value.toString() << endl;
    QSettings::setValue(key, value);
    endGroup();
}

void Settings::remove(const QString &group, const QString &key)
{
    beginGroup(group);
    QSettings::remove(key);
    endGroup();
}


bool Settings::set_user_button(const QString &name, const QString &gcode) {
    beginGroup("UserButtons");
    QStringList buttonlabels = value("Labels").toStringList();
    QStringList buttongcodes = value("GCodes").toStringList();
    for (uint i = 0; i < buttonlabels.size(); i++){
        if (buttonlabels[i] == name) {
            // change button
            buttongcodes[i] = gcode;
            setValue("GCodes",buttongcodes);
        } else {
            // add button
            buttonlabels.push_back(name);
            buttongcodes.push_back(gcode);
            setValue("Labels",buttonlabels);
            setValue("GCodes",buttongcodes);
        }
    }
    endGroup();
    return true;
}

QString Settings::get_user_gcode(const QString &name) {
    QStringList buttonlabels = value("UserButtons","Labels").toStringList();
    QStringList buttongcodes = value("UserButtons","GCodes").toStringList();
    for (uint i = 0; i < buttonlabels.size(); i++){
        if (buttonlabels[i] == name)
            return buttongcodes[i];
    }
    return "";
}

bool Settings::del_user_button(const QString &name) {
    QStringList buttonlabels = value("UserButtons","Labels").toStringList();
    QStringList buttongcodes = value("UserButtons","GCodes").toStringList();
    for (uint i = 0; i < buttonlabels.size(); i++){
        if (buttonlabels[i] == name) {
            buttonlabels.erase(buttonlabels.begin()+i);
            buttongcodes.erase(buttongcodes.begin()+i);
            return true;
        }
    }
  return false;
}


QWidget* Settings::get_widget_and_setting(QWidget *widget, const QObject* qobject,
                                          QString &group, QString &key){
    if (inhibit_callback) return nullptr;
    QString widget_name = qobject->objectName();
    QWidget *w = widget->findChild<QWidget*>(widget_name);
    if (!w) {
        QTextStream(stderr) << "    " << widget_name << tr(" not defined in GUI!")<< endl;
        return nullptr;
    }
    return splitpoint(widget_name, group, key) ? w : nullptr;
}

void Settings::get_from_gui () // no params
{
    QString group, key;
    QWidget *w = (QWidget*)sender();
//    cerr << "get " << w->objectName().toStdString() << endl;
    splitpoint(w->objectName(),group,key);
    while (w) { // for using break ...
        //cerr << "get " << group  << "." << key << " from gui"<< endl;
        m_user_changed = true; // is_changed;
        QLineEdit *e = dynamic_cast<QLineEdit *>(w);
        if (e) {
            setValue(group,key,e->text());
            break;
        }
        /*Gtk::Expander *exp = dynamic_cast<Gtk::Expander *>(w);
    if (exp) {
      set_boolean(group,key,exp->get_expanded());
      break;
    }*/
    ColorButton *cb = dynamic_cast<ColorButton *>(w);
    if (cb) {
      get_colour_from_gui(cb, group, key);
      break;
    }
    QTextEdit *tv = dynamic_cast<QTextEdit *>(w);
    if (tv) {
      setValue(group, key, tv->document()->toPlainText());
      break;
    }
    QTextStream(stderr) << tr("Did not get setting from  ")
                        << QString(group + "_" + key) << endl;
    m_user_changed = false;
    break;
  }
  if (m_user_changed) {
    // update currently edited extruder
    if (group == "Extruder") {
      copyGroup("Extruder",numberedExtruder("Extruder", selectedExtruder));
      // if selected for support, disable support for other extruders
      if (key == "UseForSupport" && get_boolean(group,key) ) {
        for (uint i = 0; i < getNumExtruders(); i++) {
          if (i != selectedExtruder) {
            setValue(numberedExtruder("Extruder", i), key, false);
          }
        }
      }
    }
//    m_signal_visual_settings_changed.emit();
  }
}

void Settings::get_int_from_gui(int value)
{
    QString group, key;
    QWidget *w = (QWidget*)sender();
//    cerr << "get " << w->objectName().toStdString() << " int " << value << endl;    splitpoint(w->objectName(),group,key);
    splitpoint(w->objectName(),group,key);
    while (w) { // for using break ...
        QCheckBox *check = dynamic_cast<QCheckBox *>(w);
        if (check) {
          setValue(group, key, check->checkState());
          break;
        }
        QSpinBox *spin = dynamic_cast<QSpinBox *>(w);
        if (spin) {
          setValue(group, key, value);
          break;
        }
        QAbstractSlider *slider = dynamic_cast<QAbstractSlider *>(w);
        if (slider) {
          setValue(group, key, value);
          break;
        }
        QComboBox *combo = dynamic_cast<QComboBox *>(w);
        if (combo) {
          if (group == "Hardware" && key == "SerialSpeed") // has real value
              setValue(group,key,combobox_get_active_value(combo).toUtf8().constData());
          else
              setValue(group,key,combo->currentIndex());
          break;
        }
    }
}
void Settings::get_double_from_gui(double value)
{
    QString group, key;
    QWidget *w = (QWidget*)sender();
//    cerr << "get " << w->objectName().toStdString() << " double " << value <<endl;
    splitpoint(w->objectName(),group,key);
    while (w) {
        QDoubleSpinBox *dspin = dynamic_cast<QDoubleSpinBox *>(w);
        if (dspin) {
          setValue(group, key, value);
          break;
        }
    }
}
