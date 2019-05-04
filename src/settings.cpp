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

#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QTextStream>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QFileInfo>
#include <QDir>
#include <QPlainTextEdit>

#include <src/ui/prefs_dlg.h>

#include "../version.h"
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

static QString numbered(const QString &qstring, int num){
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
//  set_defaults(); // done in GUI
  m_user_changed = false;
  inhibit_callback = false;
  bool haveSettings = !allKeys().empty();
  if (!haveSettings) set_defaults();
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

int Settings::mergeGlibKeyfile (const QString keyfile)
{
    GKeyFile * gkf = g_key_file_new();
    if (!g_key_file_load_from_file(gkf, keyfile.toStdString().c_str(), G_KEY_FILE_NONE, NULL)){
        fprintf (stderr, "Could not read config file %s\n", keyfile.data());
        return EXIT_FAILURE;
    }
    GError *error;
    gsize ngroups;
    gchar ** groups = g_key_file_get_groups(gkf, &ngroups);
    bool isRange = false;
    for (uint g = 0; g < ngroups; g++) {
        const QString group(groups[g]);
        if (group == "Extruder") continue;
        if (group == "Ranges") {
            isRange == true;
        }
        if (!isRange)
            beginGroup(group);
        gsize nkeys;
        gchar ** keys = g_key_file_get_keys (gkf, groups[g], &nkeys, &error);
        for (uint k = 0; k < nkeys; k++) {
            QString key = grouped(keys[k]);
            QString v(g_key_file_get_value (gkf, groups[g], keys[k], &error));
            if (isRange)
                key = key+"/Range";
            if (isRange || key.endsWith("Colour")) {
                QStringList vsplit = v.split(";");
                set_array(key, toFloats(vsplit));
            } else {
                setValue(key, v.replace("\\n","\n"));
            }
        }
        if (!isRange)
            endGroup();
    }
    QTextStream(stderr) << info() << endl;
    return ngroups;
}

QString Settings::info(){
    QString str;
    QTextStream s(&str);
    for (QString g : childGroups()){
        s << "I " << g << endl;
        beginGroup(g);
        for (QString k : allKeys()){
            s << "I \t" << k << "\t = " << value(k).toString() << endl;
        }
        endGroup();
    }
    return str;
}

bool Settings::load_from_file (QString filename) {
    // always merge when loading settings
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
    vector<float> ranges = get_array(name+"/Range");
    while (ranges.size()<4)
        ranges.push_back(0.f);
    return ranges;
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
    set_array(name+"/Range", values);
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

/**
 * @brief Settings::set_defaults
 */
void Settings::set_defaults ()
{
  filename = "";

  beginGroup("Global");
  setValue("SettingsName","Default Settings");
  setValue("SettingsImage","");
  setValue("Version",VERSION);
  endGroup();

  // Extruders.clear();
  // Extruders.push_back(Extruder);

  beginGroup("Display");
  set_array("PolygonColour", vector<float>{0.7f,1.f,1.f,0.5f});
  set_array("WireframeColour", vector<float>{1,0.5,0,0.5});
  set_array("NormalsColour", vector<float>{0.6,1,0,1});
  set_array("GCodeMoveColour", vector<float>{1,0,1,1});
  set_array("GCodePrintingColour", vector<float>{1,1,1,1});
  endGroup();
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

  if (!load_from_file (filename)) {
      QTextStream(stdout) << tr("Failed to load settings from file '") << filename << "'\n";
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


  QString contents = to_data();
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
      real_widget_name.chop(5);
  QWidget *w = parentwidget->findChild<QWidget*>(real_widget_name);
  if (widget_name.startsWith("Extruder")){ // only check for number of Extruders in list
      if (widget_name[8] != '_'){
          QListView *exList = parentwidget->findChild<QListView*>("extruder_listview");
          if (exList) {
              int extrNum = widget_name.mid(8,1).toInt();
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
      QAbstractSlider *slider = dynamic_cast<QAbstractSlider *>(w);
      if (slider) {
          if (real_widget_name.endsWith("_Range")){
              vector<float> vals = get_array(real_widget_name);
              slider->setRange(vals[0],vals[1]);
              slider->setSingleStep(vals[2]);
              slider->setPageStep(vals[3]);
          } else
              slider->setValue(get_double(real_widget_name));
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
      QPlainTextEdit *tv = dynamic_cast<QPlainTextEdit *>(w);
      if (tv) {
          tv->setPlainText(get_string(real_widget_name));
          break;
      }
      QTextStream(stderr) << tr("set_to_gui of ") << real_widget_name << " not done!" << endl;
  }
  inhibit_callback = false;
  return false;
}


// whole group or all groups
void Settings::set_all_to_gui (QWidget *parent_widget, const string filter)
{
    if (inhibit_callback) return;
  inhibit_callback = true;
  for (QString k : allKeys()) {
      if (filter.empty() || k.startsWith(QString::fromStdString(filter))) {
//          cerr << "setting to gui (filter " << filter << "): "
//               << k.toStdString() << endl;
          QString widget_name(k.replace('/','_'));
          set_to_gui(parent_widget, widget_name);
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
    // and fetch all defaults from GUI
    inhibit_callback = false;
  QList<QWidget*> widgets_with_setting =
          widget->findChildren<QWidget*>(QRegularExpression(".+_.+"));
  QWidget *w;
  for (int i=0; i < widgets_with_setting.size(); i++){
      w = (widgets_with_setting[i]);
      const QString widget_name = w->objectName();
      if (widget_name.startsWith("qt_") || widget_name.startsWith("widget_")
              || widget_name.startsWith("tab")
              || widget_name.startsWith("label_")
              || widget_name.startsWith("i_txt_"))
          continue;
      bool readFromGUI = !contains(grouped(widget_name)); // don't have setting yet
      if (dynamic_cast<QPushButton *>(w)) continue;
      QCheckBox *check = dynamic_cast<QCheckBox *>(w);
      if (check) {
          widget->connect(check, SIGNAL(stateChanged(int)), this, SLOT(get_int_from_gui(int)));
          if (readFromGUI) emit check->stateChanged(check->isChecked());
          continue;
      }
      QSpinBox *spin = dynamic_cast<QSpinBox *>(w);
      if (spin) {
          widget->connect(spin, SIGNAL(valueChanged(int)), this, SLOT(get_int_from_gui(int)));
          if (readFromGUI) emit spin->valueChanged(spin->value());
          continue;
      }
      QDoubleSpinBox *dspin = dynamic_cast<QDoubleSpinBox *>(w);
      if (dspin) {
          widget->connect(dspin, SIGNAL(valueChanged(double)), this, SLOT(get_double_from_gui(double)));
          if (readFromGUI) emit dspin->valueChanged(dspin->value());
          continue;
      }
      QAbstractSlider *range = dynamic_cast<QAbstractSlider *>(w); // sliders etc.
      if (range) {
//          cerr << "connecting " << range->objectName().toStdString() << endl;
          widget->connect(range, SIGNAL(rangeChanged(int, int)), this, SLOT(get_range_from_gui(int,int)));
          widget->connect(range, SIGNAL(valueChanged(int)), this, SLOT(get_int_from_gui(int)));
          if (readFromGUI) {
              emit range->rangeChanged(range->minimum(), range->maximum());
              emit range->valueChanged(range->value());
          }
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
          if (readFromGUI) emit e->editingFinished();
          continue;
      }
      ColorButton *cb = dynamic_cast<ColorButton *>(w);
      if (cb) {
          widget->connect(cb, SIGNAL(clicked()), this, SLOT(get_from_gui()));
          continue;
      }
      QPlainTextEdit *tv = dynamic_cast<QPlainTextEdit*>(w);
      if (tv) {
          widget->connect(tv, SIGNAL(textChanged()), this, SLOT(get_from_gui()));
 //         widget->connect(tv->document(), SIGNAL(contentsChanged()), this, SLOT(get_from_gui()));
          if (readFromGUI) emit tv->document()->contentsChanged();
          continue;
      }
  }
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

vector<ExtruderSettings> Settings::getExtruderSettings()
{
    vector<ExtruderSettings> settings;
    for (int i=0; i< getNumExtruders(); i++){
        ExtruderSettings extruder;
        extruder.name = numbered("Extruder",i);
        extruder.offset = get_extruder_offset(i);
        extruder.maxLineSpeed = get_double(extruder.name+"/MaxLineSpeed");
        extruder.displayColor = get_Vector4f(extruder.name+"/DisplayColour");
        settings.push_back(extruder);
    }
    return settings;
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
  int total = getNumExtruders();
  copyGroup(numbered("Extruder", selectedExtruder),
            numbered("Extruder", total));
  return total;
}

void Settings::RemoveExtruder()
{
    int total = getNumExtruders();
    if (total < 2) return;
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
      //set_all_to_gui(prefsDlg, "Extruder");
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
    offx = abs(get_double(ext+"/OffsetX"));
    offy = abs(get_double(ext+"/OffsetY"));
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
QString Settings::get_image_path()
{
    return QFileInfo(QFile(filename)).absoluteDir().absolutePath() + "/"+ get_string("Global/Image");
}

void Settings::setValue(const QString &group, const QString &key, const QVariant &value)
{
    beginGroup(group);
//    if (group.startsWith("Extrud"))
//        QTextStream(stderr) << "setting " << group << " -- " << key << "\t = " << value.toString() << endl;
    QSettings::setValue(key, value);
    endGroup();
    if (!inhibit_callback)
        emit settings_changed(group);
}

void Settings::remove(const QString &group, const QString &key)
{
    beginGroup(group);
    QSettings::remove(key);
    endGroup();
}

void Settings::setMaxLayers(QWidget *parent, const int numLayers)
{
    if (inhibit_callback) return;
    inhibit_callback=true;
    QSlider* slider = parent->findChild<QSlider*>("Display_LayerValue");
    if (slider) {
        float fraction = (1.f*slider->value())/slider->maximum();
        slider->setMaximum(numLayers);
        slider->setValue(int(fraction * numLayers + 0.5));
    }
    slider = parent->findChild<QSlider*>("Display_GCodeDrawStart");
    if (slider) {
        float fraction = 1.f*slider->value()/slider->maximum();
        slider->setMaximum(numLayers);
        slider->setValue(int(fraction * numLayers + 0.5));
    }
    slider = parent->findChild<QSlider*>("Display_GCodeDrawEnd");
    if (slider) {
        float fraction = 1.f*slider->value()/slider->maximum();
        slider->setMaximum(numLayers);
        slider->setValue(int(fraction * numLayers + 0.5));
    }
    inhibit_callback=false;
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
    QObject *w = dynamic_cast<QObject*>(sender());
    //    cerr << "get " << w->objectName().toStdString() << endl;
    QString name = w!=nullptr ? grouped(w->objectName()) : "";
    while (w) { // for using break ...
        m_user_changed = true; // is_changed;
        QLineEdit *e = dynamic_cast<QLineEdit *>(w);
        if (e) {
            setValue(name,e->text());
            break;
        }
        QPlainTextEdit *tv = dynamic_cast<QPlainTextEdit *>(w);
        if (tv) {
            setValue(name, tv->document()->toPlainText());
            break;
        }
        ColorButton *cb = dynamic_cast<ColorButton *>(w);
        if (cb) {
            get_colour_from_gui(cb, name);
            break;
        }
        QTextStream(stderr) << tr("Did not get setting from  ")
                            << name << endl;
        m_user_changed = false;
        break;
    }
    if (m_user_changed) {
        // update currently edited extruder
        if (name.startsWith("Extruder/")) {
            copyGroup("Extruder",numbered("Extruder",selectedExtruder));
            // if selected for support, disable support for other extruders
            if (name.endsWith("UseForSupport") && get_boolean(name) ) {
                for (uint i = 0; i < getNumExtruders(); i++) {
                    if (i != selectedExtruder) {
                        setValue(numbered("Extruder",i)+"/UseForSupport", false);
                    }
                }
            }
            emit settings_changed("Extruder");
        }
        //    m_signal_visual_settings_changed.emit();
    }
}

void Settings::get_int_from_gui(int value)
{
    if (inhibit_callback) return;
    QWidget *w = (QWidget*)sender();
    //    cerr << "get " << w->objectName().toStdString() << " int " << value << endl;    splitpoint(w->objectName(),group,key);
    while (w) { // for using break ...
        QString name = grouped(w->objectName());
        QCheckBox *check = dynamic_cast<QCheckBox *>(w);
        if (check) {
          setValue(name, check->checkState());
          break;
        }
        QSpinBox *spin = dynamic_cast<QSpinBox *>(w);
        if (spin) {
          setValue(name, value);
          break;
        }
        QAbstractSlider *slider = dynamic_cast<QAbstractSlider *>(w);
        if (slider) {
          setValue(name, value);
          break;
        }
        QComboBox *combo = dynamic_cast<QComboBox *>(w);
        if (combo) {
          if (name == "Hardware/SerialSpeed") // has real value
              setValue(name,combo->currentText().toUtf8().constData());
          else
              setValue(name,combo->currentIndex());
          break;
        }
    }
    if (w && !inhibit_callback) emit settings_changed(w->objectName());
}
void Settings::get_double_from_gui(double value)
{
    if (inhibit_callback) return;
    QWidget *w = (QWidget*)sender();
//    cerr << "get " << w->objectName().toStdString() << " double " << value <<endl;
    while (w) {
        QString name = grouped(w->objectName());
        QDoubleSpinBox *dspin = dynamic_cast<QDoubleSpinBox *>(w);
        if (dspin) {
            setValue(name, value);
            break;
        }
    }
}

void Settings::get_range_from_gui(int min, int max)
{
    if (inhibit_callback) return;
    QWidget * w = (QWidget*) sender();
    while(w) {
        QAbstractSlider *slider = dynamic_cast<QAbstractSlider *>(w);
        if (slider) {
            const QString key = grouped(w->objectName());
            const vector<float> ranges = {float(min), float(max),
                                          float(slider->singleStep()),
                                          float(slider->pageStep())};
            set_ranges(key, ranges);
          break;
        }
    }
}

void Settings::get_colour_from_gui (ColorButton * colorButton, const QString &key)
{
  if (inhibit_callback) return;
  QColor c = colorButton->get_color();
  if (c.value() == 0){
      c.setRgbF(0.5,0.5,0.5,0.5);
  }
  set_array(grouped(key), toFloats(c));
}


/////////////////////////////////////////////////////////////////

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
