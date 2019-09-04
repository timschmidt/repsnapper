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
#include <string>

#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QTextStream>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QFileInfo>
#include <QDir>
#include <QPlainTextEdit>
#include <QSerialPortInfo>
#include <QRadioButton>
#include <QSplitter>

#include <src/ui/prefs_dlg.h>

#include <src/printer/printer.h>

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

//const string serialspeeds[] = { "9600", "19200", "38400", "57600", "115200", "230400", "250000" };


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

QString Settings::numbered(const QString &qstring, uint num){
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
  currentExtruder = 0;
  set_defaults();
  cleanup();
}

Settings::~Settings()
{
}

void Settings::cleanup(){
    for (QString k : allKeys()) {
        if (k.contains("Range/") || k.startsWith("Ranges/")
                || k.startsWith("Extruder/"))
            remove(k);
    }
}

// merge into current settings
void Settings::merge (QSettings &settings)
{
    for (QString group : settings.childGroups()) {
        beginGroup(group);
        settings.beginGroup(group);
        for (QString key : settings.childKeys()){
            if (key.isEmpty()) continue;
            setValue(key, settings.value(key));
        }
        settings.endGroup();
        endGroup();
    }
}

#ifdef USE_GLIB
bool Settings::mergeGlibKeyfile (const QString &keyfile)
{
    GKeyFile * gkf = g_key_file_new();
    if (!g_key_file_load_from_file(gkf, keyfile.toStdString().c_str(), G_KEY_FILE_NONE, NULL)){
        cerr <<  "Could not read config file " << keyfile.toStdString() << endl;
        return false;
    }
    GError *error;
    gsize ngroups;
    gchar ** groups = g_key_file_get_groups(gkf, &ngroups);
    for (uint g = 0; g < ngroups; g++) {
        QString group = groups[g];
//        cerr << "GROUP "<< groups[g] << endl;
        if (group == "Ranges")
            continue;
        beginGroup(group);
        gsize nkeys;
        gchar ** keys = g_key_file_get_keys (gkf, groups[g], &nkeys, &error);
        for (uint k = 0; k < nkeys; k++) {
            QString key = grouped(keys[k]);
            if (key.isEmpty()) continue;
            QString v(g_key_file_get_value (gkf, groups[g], keys[k], &error));
            if (key.endsWith("Colour")) {
                QStringList vsplit = v.split(";");
                set_array(key, toFloats(vsplit));
            } else {
                setValue(key, v.replace("\\n","\n"));
            }
        }
        endGroup();
    }
    cleanup();
    QTextStream(stderr) << info() << endl;
    return true;
}
#endif

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

bool Settings::load_from_file (const QString &filename) {
    // always merge when loading settings
    QSettings filesettings(filename, Format::NativeFormat);
    merge(filesettings);
    return true;
}

bool Settings::load_from_data (const QString &data) {
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

void Settings::set_array(const QString &key, const QColor &qcolor, bool overwrite)
{
    set_array(key, toFloats(qcolor), overwrite);
}
void Settings::set_array(const QString &name,
                         const Vector4f &value, bool overwrite) {
   if (!overwrite && contains(name))
       return;
   beginWriteArray(name);
   for (int i=0; i<4; ++i){
       setArrayIndex(i);
       setValue("float",value[i]);
   }
   endArray();
}
void Settings::set_array(const QString &name,
                         const vector<float> &values, bool overwrite)
{
    if (!overwrite && contains(name))
        return;
    beginWriteArray(name, int(values.size()));
    for (uint i=0; i<values.size(); ++i){
        setArrayIndex(int(i));
        setValue("float",values[i]);
    }
    endArray();
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
  beginGroup("Global");
  setValue("SettingsName","Default Settings", false);
  setValue("SettingsImage","", false);
  setValue("Version",VERSION, false);
  endGroup();

  // Extruders.clear();
  // Extruders.push_back(Extruder);
}

// make old single coordinate colours to lists
void Settings::convert_old_colour  (const QString &group, const QString &key)
{
    QTextStream(stderr) << "converting old "<< group << "_" << key <<endl;
    beginGroup(group);
    const Vector4d color(get_double(key+"R"),
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

/*
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
*/

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
  if (widget_name.startsWith("Extruder")){ // only check for number of Extruders in list
      if (widget_name[8] != '_') {
          PrefsDlg *prefs_dlg = dynamic_cast<PrefsDlg*>(parentwidget);
          uint extrNum = widget_name.mid(8,1).toUInt();
          if (prefs_dlg) {
              prefs_dlg->checkForExtruders(extrNum+1);
          }
          if (extrNum != currentExtruder) {
              return true;
          } else {
              real_widget_name.replace(0,9,"Extruder");
          }
      }
  }
  inhibit_callback = true;
  QWidget *w = parentwidget->findChild<QWidget*>(real_widget_name);
  while (w) {
//      qDebug() << "setting to gui in " << parentwidget->objectName() << ": " << real_widget_name
//               << " = " << value(grouped(real_widget_name)) << endl;
      QString name = grouped(real_widget_name);
      QCheckBox *check = dynamic_cast<QCheckBox *>(w);
      if (check) {
          check->setChecked(get_boolean(name));
           break;
      }
      QSpinBox *spin = dynamic_cast<QSpinBox *>(w);
      if (spin) {
          spin->setValue(get_integer(name));
          break;
      }
      QDoubleSpinBox *dspin = dynamic_cast<QDoubleSpinBox *>(w);
      if (dspin) {
          dspin->setValue(get_double(name));
          break;
      }
      QAbstractSlider *slider = dynamic_cast<QAbstractSlider *>(w);
      if (slider) {
          slider->setValue(get_integer(name));
          break;
      }
      QComboBox *combo = dynamic_cast<QComboBox *>(w);
      if (combo) {
          if (widget_name == "Hardware_SerialSpeed") // has real value
              combobox_set_to(combo, get_string(name));
          else // has index
              combo->setCurrentIndex(get_integer(name));
          break;
      }
      QLineEdit *entry = dynamic_cast<QLineEdit *>(w);
      if (entry) {
          entry->setText(get_string(name));
          break;
      }
      ColorButton *col = dynamic_cast<ColorButton *>(w);
      if (col) {
          vector<float> c = get_array(name);
//          cerr << "set color to gui " <<  name.toStdString()  << endl;
          if (c.size() == 4)
              col->set_color(c[0],c[1],c[2],c[3]);
          break;
      }
      QPlainTextEdit *tv = dynamic_cast<QPlainTextEdit *>(w);
      if (tv) {
          tv->setPlainText(get_string(name));
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
  inhibit_callback = false;
}

Vector3d Settings::getRotation()
{
    return Vector3d(get_double("rot/x",0.)*M_PI/180,
                    get_double("rot/y",0.)*M_PI/180,
                    get_double("rot/z",0.)*M_PI/180);
}

Vector3d Settings::getTranslation()
{
    return Vector3d(get_double("translate/x",0.),get_double("translate/y",0.),
                    get_double("translate/z",0.));
}

Vector4d Settings::getScaleValues()
{
    return Vector4d(get_double("scale/x",1.),get_double("scale/y",1.),
                    get_double("scale/z",1.),get_double("scale/value",1.));
}

void Settings::setRotation(const Vector3d &rot)
{
    inhibit_callback = true;
    setValue("rot/x", rot[0]*180/M_PI);
    setValue("rot/y", rot[1]*180/M_PI);
    setValue("rot/z", rot[2]*180/M_PI);
    inhibit_callback = false;
}

void Settings::setTranslation(const Vector3d &trans)
{
    inhibit_callback = true;
    setValue("translate/x", trans[0]);
    setValue("translate/y", trans[1]);
    setValue("translate/z", trans[2]);
    inhibit_callback = false;
}

void Settings::setScaleValues(const Vector4d &scale)
{
    inhibit_callback = true;
    setValue("scale/x", scale[0]);
    setValue("scale/y", scale[1]);
    setValue("scale/z", scale[2]);
    setValue("scale/value", scale[3]);
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
      QString widget_name = w->objectName();
      if (widget_name.startsWith("qt_") || widget_name.startsWith("widget_")
              || widget_name.startsWith("tab")
              || widget_name.startsWith("label_")
              || widget_name.startsWith("i_txt_"))
          continue;
      if (widget_name.startsWith("Extruder_"))
          widget_name.replace("Extruder", numbered("Extruder", currentExtruder));
      bool readFromGUI = !contains(grouped(widget_name)); // don't have setting yet
//      if (widget_name.endsWith("Colour")|| widget_name.startsWith("Extruder"))
//          cerr << widget_name.toStdString()<< endl;
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
          widget->connect(range, SIGNAL(valueChanged(int)), this, SLOT(get_int_from_gui(int)));
          if (readFromGUI) emit range->valueChanged(range->value());
          continue;
      }
      QComboBox *combo = dynamic_cast<QComboBox *>(w);
      if (combo) {
          if (w->objectName() == "Hardware_SerialSpeed") {
              // Serial port speeds
              combo->clear();
              int settingsSpeed = get_integer("Hardware/SerialSpeed");
              for (qint32 speed : QSerialPortInfo::standardBaudRates()) {
                  if (speed >= 9600 &&  speed <= 1000000) {
                      combo->addItem(QString::number(speed), speed);
                  }
              }
              int settingsItem = combo->findText(QString::number(settingsSpeed));
              if (settingsItem < 0) {
                  combo->insertItem(0, QString::number(settingsSpeed), qint32(settingsSpeed));
                  settingsItem = 0;
              }
              combo->setCurrentIndex(settingsItem);
          } if (w->objectName() == "Hardware_Portname") {
              //combo->clear();
              QString settingsPortname = get_string("Hardware/Portname");
              vector<QSerialPortInfo> ports = Printer::findPrinterPorts();
              for (uint i = 0; i < ports.size(); i++) {
                  if (combo->findText(ports[i].portName()) < 0)
                      combo->addItem(
                                  ports[i].portName()+": "+ports[i].description(),
                                  ports[i].portName());
              }
              int settingsItem = combo->findText(settingsPortname);
              if (settingsItem < 0){
                  combo->insertItem(0, "User Port: "+settingsPortname,
                                    settingsPortname);
                  settingsItem = 0;
              }
              combo->setCurrentIndex(settingsItem);
          } else if (w->objectName().contains("Filltype")) { // Infill types
              uint nfills = sizeof(InfillNames)/sizeof(string);
              vector<string> infills(InfillNames,InfillNames+nfills);
              set_up_combobox(combo,infills);
          }
//          if (combo->isEditable()){
//              combo->connect(combo, SIGNAL(editTextChanged(QString &)),
//                             this, SLOT(get_string_from_gui(QString)));
//          }
          widget->connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(get_int_from_gui(int)));
          if (readFromGUI) emit combo->currentIndexChanged(combo->currentIndex());
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
          if (readFromGUI)
                set_array(grouped(widget_name), cb->get_color());
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

double Settings::GetExtrudedMaterialWidth(double layerheight, uint extruderNo)
{
  // ExtrudedMaterialWidthRatio is preset by user
  const QString extruder = numbered("Extruder", extruderNo);
  const double minLineWidth = get_double(extruder+"/MinimumLineWidth", 0.4);
  const double maxLineWidth =  get_double(extruder+"/MaximumLineWidth", 0.4);
  const double desired =
          get_double(extruder+"/ExtrudedMaterialWidthRatio", 1.3)
          * layerheight;
  return min(maxLineWidth, max(minLineWidth, desired));
}

// TODO This depends whether lines are packed or not - ellipsis/rectangle

// how much mm filament material per extruded line length mm -> E gcode
double Settings::GetExtrusionPerMM(double layerheight, uint extruderNo)
{
  const QString extruder = numbered("Extruder", extruderNo);
  double f = get_double(extruder+"/ExtrusionFactor"); // overall factor
  if (get_boolean(extruder+"/CalibrateInput")) {  // means we use input filament diameter
    const double matWidth = GetExtrudedMaterialWidth(layerheight, extruderNo); // this is the goal
    // otherwise we just work back from the extruded diameter for now.
    const double filamentdiameter = get_double(extruder+"/FilamentDiameter");
    f *= (matWidth * matWidth) / (filamentdiameter * filamentdiameter);
  } // else: we work in terms of output anyway;

  return f;
}

// return infill distance in mm
double Settings::GetInfillDistance(double layerthickness, double percent, uint extruderNo)
{
  double fullInfillDistance = GetExtrudedMaterialWidth(layerthickness, extruderNo);
  if (fullInfillDistance == 0.) throw new exception();
  if (percent == 0.) return 10000000;
  return fullInfillDistance * (100./percent);
}

uint Settings::getNumExtruders() const
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
    for (uint i=0; i< getNumExtruders(); i++){
        ExtruderSettings extruder;
        extruder.name = numbered("Extruder",i);
        extruder.offset = get_extruder_offset(i);
        extruder.maxLineSpeed = float(get_double(extruder.name+"/MaxLineSpeed"));
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
  if (letters.size()==0) letters.push_back('E');
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
        if (!keys[i].isEmpty())
            setValue(keys[i], values[i]);
//        cerr << "copy: "<< keys[i].toStdString() << endl;
    }
    endGroup();
}

QString Settings::grouped(const QString &name)
{
    QString g = QString(name).replace('.','/').replace('_','/');
    if (g.startsWith("Extruder/"))
        g.replace("Extruder", numbered("Extruder",currentExtruder));
    return g;
}

QVariant Settings::groupedValue(const QString &name, const QVariant &deflt)
{
    return value(grouped(name), deflt);
}

int Settings::get_integer(const QString &name)
{
    return groupedValue(name, 0).toInt();
}

double Settings::get_double(const QString &name, double deflt)
{
    return groupedValue(name, deflt).toDouble();
}

bool Settings::get_boolean(const QString &name, bool deflt)
{
    return groupedValue(name, deflt).toBool();
}

QString Settings::get_string(const QString &name)
{
    return groupedValue(name, "").toString();
}

// create new
uint Settings::CopyExtruder(uint num)
{
  uint total = getNumExtruders();
  copyGroup(numbered("Extruder", num),
            numbered("Extruder", total));
  return total;
}

void Settings::RemoveExtruder(uint num)
{
    uint total = getNumExtruders();
    if (total < 2) return;
    for (uint n = num + 1; n < total; n++){
        copyGroup(numbered("Extruder",n), numbered("Extruder",n-1));
    }
    remove(numbered("Extruder", total-1));
}

/*
void Settings::SelectExtruder(uint num, QWidget *widget, bool force)
{
  int have = getNumExtruders();
  if (num >= have) return;
  if (force || num != PrefsDlg) {
      // save current settings to previous selected
      if (num < have)
          copyGroup("Extruder", numbered("Extruder",selectedExtruder));
      selectedExtruder = num;
  }
  // if given widget show Extruder settings on gui
  PrefsDlg * prefsDlg = (PrefsDlg*) widget;
  if (prefsDlg) {
      prefsDlg->selectExtruder(num);
      //set_all_to_gui(prefsDlg, "Extruder");
  }
}
*/
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
  Vector3d margin(get_double("Hardware/PrintMargin.X", 1.),
          get_double("Hardware/PrintMargin.Y", 1.),
          get_double("Hardware/PrintMargin.Z", 0.));
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
    if (key.isEmpty()) return;
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

void Settings::setMaxHeight(QWidget *parent, const double h)
{
    if (inhibit_callback) return;
    inhibit_callback=true;
    QSlider* slider = parent->findChild<QSlider*>("Display_LayerValue");
    if (slider) {
        const double prev = slider->maximum() == 0
                ? 0 : 1.0 * slider->value()/slider->maximum();
        slider->setRange(0, int(1000*h));
        slider->setPageStep(int(h*100));
        if (prev >= 0 && prev <= 1)
            slider->setValue(prev * slider->maximum());
        else
            slider->setValue(0);
    }
    slider = parent->findChild<QSlider*>("Display_GCodeDrawStart");
    if (slider) {
        const double prev = slider->maximum() == 0
                ? 0 : 1.0 * slider->value()/slider->maximum();
        slider->setRange(0, int(1000*h));
        slider->setPageStep(int(h*100));
        if (prev >= 0 && prev <= 1)
            slider->setValue(prev * slider->maximum());
        else
            slider->setValue(0);
    }
    slider = parent->findChild<QSlider*>("Display_GCodeDrawEnd");
    if (slider) {
        const double prev = slider->maximum() == 0
                ? 0 : 1.0 * slider->value()/slider->maximum();
        slider->setRange(0, int(1000*h));
        slider->setPageStep(int(h*100));
        if (prev >= 0 && prev <= 1)
            slider->setValue(prev * slider->maximum());
        else
            slider->setValue(0);
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
    if (name.isEmpty()) return;
    if (name.startsWith("Extruder/")){ // from gui
        name.replace("Extruder", numbered("Extruder", currentExtruder)); // to current
//        cerr << "get f g " << name.toStdString() << endl;
    }
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
        if (name.startsWith("Extruder"))  {
            // if selected for support, disable support for other extruders
            if (name.endsWith("UseForSupport") && get_boolean(name) ) {
                for (uint i = 0; i < getNumExtruders(); i++) {
                    if (i != currentExtruder) {
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
//    cerr << "get " << w->objectName().toStdString() << " int " << value << endl;
    while (w) { // for using break ...
        QString name = grouped(w->objectName());
        if (name.isEmpty()) continue;
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
          if (name == "Hardware/SerialSpeed"
                   || name == "Hardware/Portname") {// have real value
             QVariant value =combo->currentData();
             if (!value.isValid())
                 value = combo->currentText();
             setValue(name,value);
          } else
              setValue(name,max(0,combo->currentIndex()));
          break;
        }
    }
    if (w && !inhibit_callback) emit settings_changed(w->objectName());
}

void Settings::get_string_from_gui(QString &string)
{
    if (inhibit_callback) return;
    QWidget *w = (QWidget*)sender();
    while (w) {
        QComboBox *combo = dynamic_cast<QComboBox *>(w);
        if (combo) {
            if (combo->isEditable()){
                qDebug() << "text " << string << " - " << combo->currentText();
            }
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
        if (name.isEmpty()) continue;
        QDoubleSpinBox *dspin = dynamic_cast<QDoubleSpinBox *>(w);
        if (dspin) {
            setValue(name, value);
            break;
        }
    }
    if (w && !inhibit_callback) emit settings_changed(w->objectName());
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
    set_color(QColor(int(255*r),int(255*g),int(255*b),int(255*alpha)));
}
