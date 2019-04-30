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
#include <QStringLiteral>

#include <src/ui/prefs_dlg.h>

#include "settings.h"
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
static bool group_key_split(const QString &widget_name, QString &group, QString &key) {
    int index = widget_name.indexOf('_');
    if (index < 0)
        index = widget_name.indexOf('.');
    if (index < 0)
         return false;
  group = widget_name.left(index);
  key = widget_name.mid(index+1);
  return true;
}

static vector<float> toFloats(QColor c)
{
    vector<float> d(4);
    d[0] = c.red()/255.f;
    d[1] = c.green()/255.f;
    d[2] = c.blue()/255.f;
    d[3] = c.alpha()/255.f;
    return d;
}

static vector<float> toFloats(QStringList &slist)
{
    vector<float> f;
    for (QString &s : slist)
        if (s.length() > 0)
            f.push_back(s.toFloat());
    return f;
}

QString Settings::numbered(const QString &qstring, int num){
    return qstring + QString::number(num);
}

void set_up_combobox(QComboBox *combo, vector<string> values)
{
//    cerr << "setup combo " + combo->objectName().toStdString() << " " ;
//    for (auto &v:values) cerr << v << " ";
//    cerr << endl;
  combo->clear();
  for (uint i=0; i<values.size(); i++) {
      combo->addItem(QString(values[i].c_str()), QVariant());
  }
  combo->setCurrentIndex(0);
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
void Settings::merge (QSettings &settings)
{
    for (QString group : settings.childGroups()) {
        beginGroup(group);
        settings.beginGroup(group);
        for (QString key : settings.childKeys()){
            setValue(key, settings.value(key));
        }
        settings.endGroup();
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
        const QString group(groups[g]);
        if (group == "Extruder") continue;
        beginGroup(group);
        gsize nkeys;
        gchar ** keys = g_key_file_get_keys (gkf, groups[g], &nkeys, &error);
        for (uint k = 0; k < nkeys; k++) {
            QString key = grouped(keys[k]);
            //key.replace('.','_');
            QString v(g_key_file_get_value (gkf, groups[g], keys[k], &error));
            if (group == "Ranges" || key.endsWith("Colour")) {
                QStringList vsplit = v.split(";");
                set_array(key, toFloats(vsplit));
            } else {
                setValue(key, v.replace("\\n","\n"));
            }
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
        for (QString k : allKeys()){
            s << "Info: " << g << "--" << k << "\t = " << value(k).toString() << endl;
        }
        endGroup();
    }
    return str;
}

// always merge when loading settings
bool Settings::load_from_file (QString filename) {
    QSettings filesettings(filename, Format::NativeFormat);
    merge(filesettings);
    return true;
}
bool Settings::load_from_data (QString data) {
 //TODO merge(QSettings());
  return true;
}

Vector4f Settings::get_Vector4f(const QString &key) {
    vector<float> s = get_array(key);
    while (s.size()<4)
        s.push_back(0.f);
    return Vector4f(s[0],s[1],s[2],s[3]);
}

vector<float> Settings::get_array(const QString &name)
{
    int size = beginReadArray(name);
    vector<float> s;
    for (int i=0; i<size; ++i){
        setArrayIndex(i);
        s.push_back(value("float").toFloat());
    }
    endArray();
    return s;
}
vector<float> Settings::get_ranges(const QString &name)
{
    return get_array("Ranges/"+name);
}

double Settings::get_slider_fraction(const QString &name)
{
    vector<float> ranges = get_ranges(name);
    return double(ranges[0]) + double(get_integer(name) / (ranges[1]-ranges[0]));
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

void Settings::set_array(const QString &key, const QColor &qcolor)
{
    set_array(key, toFloats(qcolor));
}

void Settings::set_array(const QString &name,
                         const Vector4f &value) {
   beginWriteArray(name);
   for (int i=0; i<4; ++i){
       setArrayIndex(i);
       setValue("float",value[i]);
   }
   endArray();
}

void Settings::set_array(const QString &name,
                         const vector<float> &values)
{
    beginWriteArray(name, values.size());
    for (int i=0; i<values.size(); ++i){
        setArrayIndex(i);
        setValue("float",values[i]);
    }
    endArray();
}
void Settings::set_ranges(const QString &name, const vector<float> &values)
{
    set_array("Ranges/"+name, values);
}

QStringList Settings::get_keys(const QString &group)
{
    beginGroup(group);
    QStringList keys = childKeys();
    endGroup();
    return keys;
}


QStringList Settings::get_keys_and_values(const QString &group, QList<QVariant> &values)
{
    beginGroup(group);
    QStringList keys = childKeys();
    for (QString k : keys){
        values.append(value(k));
    }
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
  setValue("Volume_X", 200);
  setValue("Volume_Y", 200);
  setValue("Volume_Z", 140);
  setValue("PrintMargin_X", 10);
  setValue("PrintMargin_Y", 10);
  setValue("PrintMargin_Z", 0);
  endGroup();
  setValue("Misc/SpeedsAreMMperSec",true);

  selectedExtruder = 99;
  SelectExtruder(0); // force copy from "Extruder0" to "Extruder"
}


// make old single coordinate colours to lists
void Settings::convert_old_colour  (const QString &group, const QString &key)
{
    QTextStream(stderr) << "converting old "<< group << "_" << key <<endl;
    beginGroup(group);
    const Vector4f color(get_double(key+"R"),
                         get_double(key+"G"),
                         get_double(key+"B"),
                         get_double(key+"A"));
    set_array(key, color);
    remove(key+"R");
    remove(key+"G");
    remove(key+"B");
    remove(key+"A");
    endGroup();
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
    CustomButtonLabels = value("UserButtons/Labels").toStringList();
    CustomButtonGCodes = value("UserButtons/GCodes").toStringList();
  }
  try {
      for (QString k : get_keys("CustomButtons")) {
          bool havekey = false;
          for (int o = 0; o < CustomButtonLabels.size(); o++) {
              if (CustomButtonLabels[o] == k) {
                  CustomButtonGCodes[o] = get_string("CustomButtons/"+k);
                  havekey = true;
                  break;
              }
          }
          if (!havekey) {
              CustomButtonLabels.push_back(k);
              CustomButtonGCodes.push_back(get_string("CustomButtons/"+k));
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
          copyGroup(numbered("Extruder",k),
                    numbered("Extruder",k-1));
    }
    remove("Extruder");
  }
  int ne = int(getNumExtruders());
  for (int k = 0; k < ne; k++) {
      beginGroup(numbered("Extruder",k));
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

bool Settings::set_to_gui (QWidget *parentwidget, const QString &widget_name)
{
  QString real_widget_name(widget_name);
  if (widget_name.endsWith("_size"))
      real_widget_name = real_widget_name.chopped(5);
  QWidget *w = parentwidget->findChild<QWidget*>(real_widget_name);
  if (widget_name.startsWith("Extruder")){ // only check for number of Extruders in list
      if (widget_name[8]!='_'){
          int extrNum = widget_name.mid(8,1).toInt();
          QListView *exList = parentwidget->findChild<QListView*>("extruder_listview");
          if (exList) {
              int haveEx = exList->model()->rowCount();
              if (getNumExtruders() < haveEx){
                  exList->model()->removeRow(haveEx-1);
                  exList->update();
                  QModelIndex index = exList->model()->index(haveEx-2,0);
                  exList->clicked(index);
              } else if (haveEx < extrNum+1) {
                  cerr << "add Extruder "<< extrNum << endl;
                  exList->model()->insertRow(haveEx);
                  QModelIndex index = exList->model()->index(haveEx,0);
                  exList->model()->setData(index, numbered("Extruder ",extrNum+1));
                  exList->clicked(index);
              }
          }
          return true; // only "Extruder" settings without number are set to gui
      }
  }
  inhibit_callback = true;
  while (w) {
//      QTextStream(stderr) << "    " << widget_name << tr(" not defined in GUI!")<< endl;
      cerr << "setting to gui in " << parentwidget->objectName().toStdString() << ": " << real_widget_name.toStdString()
           << " = " << value(grouped(real_widget_name)).toString().toStdString() << endl;
      QCheckBox *check = dynamic_cast<QCheckBox *>(w);
      if (check) {
          check->setChecked(get_boolean(real_widget_name));
           break;
      }
      QSpinBox *spin = dynamic_cast<QSpinBox *>(w);
      if (spin) {
          spin->setValue(get_double(real_widget_name));
          break;
      }
      QDoubleSpinBox *dspin = dynamic_cast<QDoubleSpinBox *>(w);
      if (dspin) {
          dspin->setValue(get_double(real_widget_name));
          break;
      }
      QSlider *range = dynamic_cast<QSlider *>(w);
      if (range) {
          range->setValue(get_double(real_widget_name));
          break;
      }
      QComboBox *combo = dynamic_cast<QComboBox *>(w);
      if (combo) {
          if (widget_name == "Hardware_SerialSpeed") // has real value
              combobox_set_to(combo, get_string(real_widget_name));
          else // has index
              combo->setCurrentIndex(get_integer(real_widget_name));
          break;
      }
      QLineEdit *entry = dynamic_cast<QLineEdit *>(w);
      if (entry) {
          entry->setText(get_string(real_widget_name));
          break;
      }
      ColorButton *col = dynamic_cast<ColorButton *>(w);
      if(col) {
          vector<float> c = get_array(real_widget_name);
          col->set_color(c[0],c[1],c[2],c[3]);
          break;
      }
      QTextEdit *tv = dynamic_cast<QTextEdit *>(w);
      if (tv) {
          tv->setText(get_string(real_widget_name));
          break;
      }
      QTextStream(stderr) << tr("set_to_gui of ") << real_widget_name << " not done!" << endl;
  }
  inhibit_callback = false;
  return false;
}

void Settings::get_colour_from_gui (ColorButton * colorButton, const QString &key)
{
  QColor c = colorButton->get_color();
  set_array(grouped(key), toFloats(c));
}


// whole group or all groups
void Settings::set_all_to_gui (QWidget *parent_widget, const string filter)
{
  inhibit_callback = true;
  for (QString k : allKeys()) {
      if (filter.empty() || k.startsWith(QString::fromStdString(filter))) {
//          cerr << "setting to gui (filter " << filter << "): "
//               << k.toStdString() << endl;
          QString widget_name(k.replace('/','_'));
          if (k.startsWith("GCode")) {
              beginGroup("GCode");
              set_to_gui(parent_widget,"Start");
              set_to_gui(parent_widget,"Layer");
              set_to_gui(parent_widget,"End");
              endGroup();
          } else if (k.startsWith("Ranges")){
              vector<float> vals = get_array(k);
              if (vals.size() >= 4) {
                  QWidget *w = parent_widget->findChild<QWidget*>
                          (widget_name.replace("Ranges/",""));
                  if (!w) {
                      QTextStream(stderr) << "Missing user interface item " << widget_name << "_" << k << "\n";
                      continue;
                  }
                  QSpinBox *spin = dynamic_cast<QSpinBox *>(w);
                  if (spin) {
                      spin->setRange(vals[0],vals[1]);
                      spin->setSingleStep(vals[2]);
                          // TODO page incr?    spin->set(vals[3]);
                      continue;
                  }
                  QDoubleSpinBox *dspin = dynamic_cast<QDoubleSpinBox *>(w);
                  if (dspin) {
                      dspin->setRange(vals[0],vals[1]);
                      dspin->setSingleStep(vals[2]);
                          // TODO page incr?    dspin->set(vals[3]);
                      continue;
                  }
                  QAbstractSlider *range = dynamic_cast<QAbstractSlider *>(w); // sliders etc.
                  if (range) {
                      range->setRange(vals[0],vals[1]);
                      range->setSingleStep(vals[2]);
                      range->setPageStep(vals[3]);
                      continue;
                  }
              }
          } else { // not "Ranges"
              set_to_gui(parent_widget, widget_name);
          }
      }
  }

  if (filter.empty() || filter == "Misc") {
    beginGroup("Misc");
    int w = value("WindowWidth").toInt();
    int h = value("WindowHeight").toInt();
    if (parent_widget && w > 0 && h > 0) parent_widget->resize(w,h);
    int x = value("WindowPosX").toInt();
    int y = value("WindowPosY").toInt();
    if (parent_widget && x > 0 && y > 0) parent_widget->move(x,y);
    endGroup();
  }
  inhibit_callback = false;
}

template<typename Base, typename T>
 bool instanceof(const T*) {
    return std::is_base_of<Base, T>::value;
}


void Settings::connect_to_gui (QWidget *widget)
{
  // add signal callbacks to all GUI elements with "_"
  QList<QWidget*> widgets_with_setting =
          widget->findChildren<QWidget*>(QRegularExpression(".+_.+"));
  for (int i=0; i< widgets_with_setting.size(); i++){
      QWidget *w = (widgets_with_setting[i]);
      QString group,key;
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
//          cerr << "connecting " << range->objectName().toStdString() << endl;
          widget->connect(range, SIGNAL(valueChanged(int)), this, SLOT(get_int_from_gui(int)));
          widget->connect(range, SIGNAL(rangeChanged(int,int)), this, SLOT(get_range_from_gui(int,int)));
          continue;
      }
      QComboBox *combo = dynamic_cast<QComboBox *>(w);
      if (combo) {
          if (w->objectName() == "Hardware_SerialSpeed") { // Serial port speed
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
  return min(max(get_double("Extruder/MinimumLineWidth"),
         get_double("Extruder/ExtrudedMaterialWidthRatio") * layerheight),
         get_double("Extruder/MaximumLineWidth"));
}

// TODO This depends whether lines are packed or not - ellipsis/rectangle

// how much mm filament material per extruded line length mm -> E gcode
double Settings::GetExtrusionPerMM(double layerheight)
{
  double f = get_double("Extruder/ExtrusionFactor"); // overall factor
  if (get_boolean("Extruder/CalibrateInput")) {  // means we use input filament diameter
    const double matWidth = GetExtrudedMaterialWidth(layerheight); // this is the goal
    // otherwise we just work back from the extruded diameter for now.
    const double filamentdiameter = get_double("Extruder/FilamentDiameter");
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

int Settings::getNumExtruders() const
{
  uint num=0;
  for (QString g : childGroups())
    if (g.startsWith("Extruder")
            && g.length() == 9 ) // count only numbered
      num++;
  return num;
}

std::vector<QChar> Settings::get_extruder_letters()
{
  uint num = getNumExtruders();
  std::vector<QChar> letters(num);
  for (uint i = 0; i < num; i++)
    letters[i] = get_string(numbered("Extruder",i)+"/GCLetter")[0];
  return letters;
}

uint Settings::GetSupportExtruder()
{
  uint num = getNumExtruders();
  for (uint i = 0; i < num; i++)
      if (get_boolean(numbered("Extruder",i)+"/UseForSupport"))
          return i;
  return 0;
}

Vector3d Settings::get_extruder_offset(uint num)
{
  QString ext = numbered("Extruder",num);
  return Vector3d(get_double(ext+"/OffsetX"),
                  get_double(ext+"/OffsetY"), 0.);
}


void Settings::copyGroup(const QString &from, const QString &to)
{
    QList<QVariant> values;
    QStringList keys = get_keys_and_values(from, values);
    beginGroup(to);
    for (int i = 0; i < keys.size(); i++){
        setValue(keys[i], values[i]);
//        cerr << "copy: "<< keys[i].toStdString() << endl;
    }
    endGroup();
}

QString Settings::grouped(const QString &name)
{
    return QString(name).replace('.','/').replace('_','/');
}

QVariant Settings::groupedValue(const QString &name, const QVariant &deflt)
{
    return value(grouped(name), deflt);
}

int Settings::get_integer(const QString &name)
{
    return groupedValue(name, 0).toInt();
}

double Settings::get_double(const QString &name)
{
    return groupedValue(name, 0.).toDouble();
}

bool Settings::get_boolean(const QString &name)
{
    return groupedValue(name, false).toBool();
}

QString Settings::get_string(const QString &name)
{
    return groupedValue(name, "").toString();
}

// create new
uint Settings::CopyExtruder()
{
  uint total = getNumExtruders();
  copyGroup(numbered("Extruder", selectedExtruder),
            numbered("Extruder", total));
  return total;
}

void Settings::RemoveExtruder()
{
    int total = getNumExtruders();
    for (int n = selectedExtruder + 1; n < total; n++){
        copyGroup(numbered("Extruder",n), numbered("Extruder",n-1));
    }
    remove(numbered("Extruder", total-1));
}

void Settings::SelectExtruder(uint num, QWidget *widget)
{
    int have = getNumExtruders();
  if (num >= have) return;
  if (num != selectedExtruder) {
      // save current settings to previous selected
      if (selectedExtruder < have)
          copyGroup("Extruder", numbered("Extruder",selectedExtruder));
      selectedExtruder = num;
      copyGroup(numbered("Extruder",num),"Extruder");
  }
  // if given widget show Extruder settings on gui
  PrefsDlg * prefsDlg = (PrefsDlg*) widget;
  if (prefsDlg) {
      prefsDlg->selectExtruder(num);
      set_all_to_gui(prefsDlg, "Extruder");
  }
}

Matrix4d Settings::getBasicTransformation(Matrix4d T)
{
  Vector3d t;
  T.get_translation(t);
  const Vector3d margin = getPrintMargin();
  double rsize = get_double("Raft/Size") * (get_boolean("Raft/Enable")?1:0);
  t+= Vector3d(margin.x() + rsize, margin.y() + rsize, 0);
  T.set_translation(t);
  return T;
}


Vector3d Settings::getPrintVolume()
{
  return Vector3d (get_double("Hardware/Volume.X"),
                   get_double("Hardware/Volume.Y"),
                   get_double("Hardware/Volume.Z"));

}
Vector3d Settings::getPrintMargin()
{
  Vector3d margin(get_double("Hardware/PrintMargin.X"),
          get_double("Hardware/PrintMargin.Y"),
          get_double("Hardware/PrintMargin.Z"));
  Vector3d maxoff = Vector3d::ZERO;
  uint num = getNumExtruders();
  for (uint i = 0; i < num ; i++) {
    QString ext = numbered("Extruder",i);
    double offx = 0, offy = 0;
    try {
      offx = abs(get_double(ext+"/OffsetX"));
      offy = abs(get_double(ext+"/OffsetY"));
    } catch (const Glib::Exception &err) {
    }
    if (offx > abs(maxoff.x())) maxoff.x() = offx;
    if (offy > abs(maxoff.y())) maxoff.y() = offy;
  }
  if (get_boolean("Slicing/Skirt")) {
    double distance = get_double("Slicing/SkirtDistance");
    maxoff += Vector3d(distance, distance, 0);
  }
  return margin + maxoff;
}


// Locate it in relation to ourselves ...
std::string Settings::get_image_path()
{
  std::string basename = Glib::path_get_dirname(filename.toUtf8().constData());
  return Glib::build_filename (basename, get_string("Global/Image").toUtf8().data());
}

void Settings::setValue(const QString &group, const QString &key, const QVariant &value)
{
    beginGroup(group);
//    if (group.startsWith("Extrud"))
//        QTextStream(stderr) << "setting " << group << " -- " << key << "\t = " << value.toString() << endl;
    QSettings::setValue(key, value);
    endGroup();
    if (!inhibit_callback)
        emit settings_changed();
}

void Settings::remove(const QString &group, const QString &key)
{
    beginGroup(group);
    QSettings::remove(key);
    endGroup();
}

void Settings::setMaxLayers(QWidget *parent, const int numLayers)
{
    QSlider* slider = parent->findChild<QSlider*>("Display_LayerValue");
    if (slider)
        slider->setMaximum(numLayers);
    slider = parent->findChild<QSlider*>("Display_GCodeDrawStart");
    if (slider)
        slider->setMaximum(numLayers);
    slider = parent->findChild<QSlider*>("Display_GCodeDrawEnd");
    if (slider)
        slider->setMaximum(numLayers);
}


bool Settings::set_user_button(const QString &name, const QString &gcode) {
    beginGroup("UserButtons");
    QStringList buttonlabels = value("Labels").toStringList();
    QStringList buttongcodes = value("GCodes").toStringList();
    for (int i = 0; i < buttonlabels.size(); i++){
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
    QStringList buttonlabels = value("UserButtons/Labels").toStringList();
    QStringList buttongcodes = value("UserButtons/GCodes").toStringList();
    for (int i = 0; i < buttonlabels.size(); i++){
        if (buttonlabels[i] == name)
            return buttongcodes[i];
    }
    return "";
}

bool Settings::del_user_button(const QString &name) {
    QStringList buttonlabels = value("UserButtons/Labels").toStringList();
    QStringList buttongcodes = value("UserButtons/GCodes").toStringList();
    for (int i = 0; i < buttonlabels.size(); i++){
        if (buttonlabels[i] == name) {
            buttonlabels.erase(buttonlabels.begin()+i);
            buttongcodes.erase(buttongcodes.begin()+i);
            return true;
        }
    }
  return false;
}


void Settings::get_from_gui () // no params
{
    if (inhibit_callback) return;
    QString group, key;
    QWidget *w = dynamic_cast<QWidget*>(sender());
//    cerr << "get " << w->objectName().toStdString() << endl;
    group_key_split(w->objectName(),group,key);
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
            get_colour_from_gui(cb, group+"/"+key);
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
            copyGroup("Extruder",numbered("Extruder",selectedExtruder));
            // if selected for support, disable support for other extruders
            if (key == "UseForSupport" && get_boolean(group+"/"+key) ) {
                for (uint i = 0; i < getNumExtruders(); i++) {
                    if (i != selectedExtruder) {
                        setValue(numbered("Extruder",i), key, false);
                    }
                }
            }
        }
//    m_signal_visual_settings_changed.emit();
    }
}

void Settings::get_int_from_gui(int value)
{
    if (inhibit_callback) return;
    QString group, key;
    QWidget *w = (QWidget*)sender();
//    cerr << "get " << w->objectName().toStdString() << " int " << value << endl;    splitpoint(w->objectName(),group,key);
    group_key_split(w->objectName(),group,key);
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
              setValue(group,key,combo->currentText().toUtf8().constData());
          else
              setValue(group,key,combo->currentIndex());
          break;
        }
    }
}
void Settings::get_double_from_gui(double value)
{
    if (inhibit_callback) return;
    QString group, key;
    QWidget *w = (QWidget*)sender();
//    cerr << "get " << w->objectName().toStdString() << " double " << value <<endl;
    while (w) {
        group_key_split(w->objectName(),group,key);
        QDoubleSpinBox *dspin = dynamic_cast<QDoubleSpinBox *>(w);
        if (dspin) {
          setValue(group, key, value);
          break;
        }
    }
}

void Settings::get_range_from_gui(int min, int max)
{
    if (inhibit_callback) return;
    QString group, key;
    QWidget *w = (QWidget*)sender();
    while(w) {
        QAbstractSlider *slider = dynamic_cast<QAbstractSlider *>(w);
        if (slider) {
            QString key=w->objectName().replace("_","/");
            vector<float> ranges = get_ranges(key);
            ranges[0] = float(min);
            ranges[1] = float(max);
            set_ranges(key, ranges);
          break;
        }
    }
}

ColorButton::ColorButton(QWidget *widget) : QPushButton(widget){}

void ColorButton::set_color(QColor c) {
    color = c;
    QPalette pal(c);
//    pal.setColor(QPalette::Background, c);
    setPalette(pal);
    setAutoFillBackground(true);
    repaint();
}

void ColorButton::set_color(float r, float g, float b, float alpha) {
    set_color(QColor(255*r,255*g,255*b,255*alpha));
}
