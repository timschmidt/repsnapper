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
#include <gtkmm.h>
#include "settings.h"

#include <stdafx.h>

#include "infill.h"
/*
 * How settings are intended to work:
 *
 * Settings is a subclass of Glib::KeyFile.
 *
 * Glade Builder Widgets are named as <Group>.<Key>, so automatically
 * converted to KeyFile settings.  This works for most settings, but
 * there are exceptions...
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
bool splitpoint(const string &glade_name, string &group, string &key) {
  int pos = glade_name.find(".");
  if (pos==(int)std::string::npos) return false;
  group = glade_name.substr(0,pos);
  key = glade_name.substr(pos+1);
  return true;
}


void set_up_combobox(Gtk::ComboBox *combo, vector<string> values)
{
  if (combo->get_model())
    return;
  Gtk::TreeModelColumn<Glib::ustring> column;
  Gtk::TreeModelColumnRecord record;
  record.add(column);
  Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(record);
  combo->pack_start (column);
  combo->set_model(store);
  for (uint i=0; i<values.size(); i++) {
    //cerr << " adding " << values[i] << endl;
    store->append()->set_value(0, Glib::ustring(values[i].c_str()));
  }
#if GTK_VERSION_GE(2, 24)
  if (!combo->get_has_entry())
#endif
    combo->set_active(0);
  //cerr << "ok" << endl;
}

string combobox_get_active_value(Gtk::ComboBox *combo){
#if GTK_VERSION_GE(2, 24)
  if (combo->get_has_entry())
    {
      Gtk::Entry *entry = combo->get_entry();
      if (entry)
	return string(entry->get_text());
    } else
#endif
    {
      uint c = combo->get_active_row_number();
      Glib::ustring rval;
      combo->get_model()->children()[c].get_value(0,rval);
      return string(rval);
    }
  cerr << "could not get combobox active value" << endl;
  return "";
}

bool combobox_set_to(Gtk::ComboBox *combo, string value)
{
  Glib::ustring val(value);
  Glib::RefPtr<Gtk::TreeModel> model = combo->get_model();
  uint nch = model->children().size();
  Glib::ustring rval;
  Glib::ustring gvalue(value.c_str());
#if GTK_VERSION_GE(2, 24)
  if (combo->get_has_entry())
    {
      Gtk::Entry *entry = combo->get_entry();
      if (entry) {
	entry->set_text(value);
	return true;
      }
    }
  else
#endif
    {
      for (uint c=0; c < nch; c++) {
	Gtk::TreeRow row = model->children()[c];
	row.get_value(0,rval);
	if (rval== gvalue) {
	  combo->set_active(c);
	  return true;
	}
      }
    }
  cerr << "value " << value << " not found in combobox" << endl;
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
void Settings::merge (const Glib::KeyFile &keyfile)
{
  vector< Glib::ustring > groups = keyfile.get_groups();
  for (uint g = 0; g < groups.size(); g++) {
    vector< Glib::ustring > keys = keyfile.get_keys(groups[g]);
    for (uint k = 0; k < keys.size(); k++) {
      set_value(groups[g], keys[k], keyfile.get_value(groups[g], keys[k]));
    }
  }
}
// always merge when loading settings
bool Settings::load_from_file (string file) {
  Glib::KeyFile k;
  if (!k.load_from_file(file)) return false;
  merge(k);
  return true;
}
bool Settings::load_from_data (string data) {
  Glib::KeyFile k;
  if (!k.load_from_file(data)) return false;
  merge(k);
  return true;
}



// make "ExtruderN" group, if i<0 (not given), use current selected Extruder number
string Settings::numberedExtruder(const string &group, int num) const {
  if (group == "Extruder") {
    ostringstream oss; oss << "Extruder" << num;
    //cerr << "call for " << oss.str() << endl;
    return oss.str();
  }
  return group;
}

Vector4f Settings::get_colour(const string &group, const string &name) const {
  vector<double> s = get_double_list(group, name);
  return Vector4f(s[0],s[1],s[2],s[3]);
}

void Settings::set_colour  (const string &group, const string &name,
			    const Vector4f &value) {
  Glib::KeyFile::set_double_list(group, name, value);
}


void
Settings::assign_from(Settings *pSettings)
{
  this->load_from_data(pSettings->to_data());
  m_user_changed = false;
  m_signal_visual_settings_changed.emit();
  m_signal_update_settings_gui.emit();
}

void Settings::set_defaults ()
{

  filename = "";

  set_string("Global","SettingsName","Default Settings");
  set_string("Global","SettingsImage","");

  set_string("Global","Version",VERSION);

  set_string("GCode","Start",
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
  set_string("GCode","Layer","");
  set_string("GCode","End",
	     "; This code is sent to the printer after the print.\n"
	     "; Adjust it to your needs.\n"
	     "G1 X0 Y0 F2000.0 ; feed for start of next move\n"
	     "M104 S0.0        ; heater off\n");



  // Extruders.clear();
  // Extruders.push_back(Extruder);

  // The vectors map each to 3 spin boxes, one per dimension
  set_double("Hardware","Volume.X", 200);
  set_double("Hardware","Volume.Y", 200);
  set_double("Hardware","Volume.Z", 140);
  set_double("Hardware","PrintMargin.X", 10);
  set_double("Hardware","PrintMargin.Y", 10);
  set_double("Hardware","PrintMargin.Z", 0);

  set_boolean("Misc","SpeedsAreMMperSec",true);
}


// make old single coordinate colours to lists
void Settings::convert_old_colour  (const string &group, const string &key) {
  try {
    cerr << "converting  "<< group << "." <<key <<endl;
    const double c[5] = { get_double(group, key+"R"),
			  get_double(group, key+"G"),
			  get_double(group, key+"B"),
			  get_double(group, key+"A"),0};
    set_double_list(group,key,c);
    remove_key(group, key+"R");
    remove_key(group, key+"G");
    remove_key(group, key+"B");
    remove_key(group, key+"A");
  } catch (const Glib::KeyFileError &err) {
  }
}

void Settings::load_settings (Glib::RefPtr<Gio::File> file)
{
  inhibit_callback = true;
  filename = file->get_path();

  // set_defaults();

  if (has_group("Extruder"))
    remove_group("Extruder"); // avoid converting old if merging new file

  try {
    if (!load_from_file (filename)) {
      std::cout << _("Failed to load settings from file '") << filename << "\n";
      return;
    }
  } catch (const Glib::KeyFileError &err) {
    std::cout << _("Exception ") << err.what() << _(" loading settings from file '") << filename << "\n";
    return;
  }

  std::cerr << _("Parsing config from '") << filename << "\n";

  // convert old colour handling:
  vector< Glib::ustring > groups = get_groups();
  for (uint g = 0; g < groups.size(); g++) {
    //cerr << "["<<groups[g] << "] " ;
    vector< Glib::ustring > keys = get_keys(groups[g]);
    for (uint k = 0; k < keys.size(); k++) {
      int n = keys[k].length();
      int c = keys[k].find("Colour");
      if (c >= 0 && c < n-6 && keys[k].substr(c+6,1) == "R")
	convert_old_colour(groups[g],keys[k].substr(0,c+6));
    }
  }

  // convert old user buttons:
  std::vector<std::string> CustomButtonLabels;
  std::vector<std::string> CustomButtonGCodes;
  if (has_group("UserButtons")) {
    CustomButtonLabels = get_string_list("UserButtons","Labels");
    CustomButtonGCodes = get_string_list("UserButtons","GCodes");
  }
  try {
    vector< Glib::ustring > keys = get_keys("CustomButtons");
    for (uint k = 0; k < keys.size(); k++) {
      bool havekey = false;
      for (uint o = 0; o < CustomButtonLabels.size(); o++) {
	if (CustomButtonLabels[o] == keys[k]) {
	  CustomButtonGCodes[o] = get_string("CustomButtons",keys[k]);
	  havekey = true;
	  break;
	}
      }
      if (!havekey) {
	CustomButtonLabels.push_back(keys[k]);
	CustomButtonGCodes.push_back(get_string("CustomButtons",keys[k]));
      }
    }
    remove_group("CustomButtons");
  } catch (Glib::KeyFileError &err) {}
  if (!has_group("UserButtons")) {
    set_string_list("UserButtons","Labels",CustomButtonLabels);
    set_string_list("UserButtons","GCodes",CustomButtonGCodes);
  }

  // convert old extruders, now we count "Extruder0", "Extruder1" ...
  // instead of "Extruder", "Extruder2" ...
  if (has_group("Extruder")) { // have old Extruders
    uint ne = getNumExtruders() + 1; // +1 because "Extruder" is not counted
    if (has_group("Extruder0")) ne--; // already have "new" Extruders
    copyGroup("Extruder","Extruder0");
    if (ne > 1) {
      for (uint k = 2; k < ne+1; k++) // copy 2,3,4,... to 1,2,3,...
	copyGroup(numberedExtruder("Extruder",k),
		  numberedExtruder("Extruder",k-1));
      remove_group(numberedExtruder("Extruder",ne));
    }
    remove_group("Extruder");
  }
  uint ne = getNumExtruders();
  for (uint k = 0; k < ne; k++) {
    if (!has_key(numberedExtruder("Extruder",k), "OffsetX"))
      set_double(numberedExtruder("Extruder",k), "OffsetX", 0);
    if (!has_key(numberedExtruder("Extruder",k), "OffsetY"))
      set_double(numberedExtruder("Extruder",k), "OffsetY", 0);
  }
  SelectExtruder(0);

  if ( ( has_group("Misc") &&
	 !has_key("Misc","SpeedsAreMMperSec") ) ||
       !get_boolean("Misc","SpeedsAreMMperSec") )
    cout << "!!!" << endl <<  _("\tThe config file has old speed settings (mm/min).\n\t Adjust them to mm/sec\n\t or use RepSnapper from 2.0 to 2.2 to convert them automatically.") << "!!!" <<  endl;
  set_boolean("Misc","SpeedsAreMMperSec",true);

  inhibit_callback = false;
  m_user_changed = false;
  m_signal_visual_settings_changed.emit();
  m_signal_update_settings_gui.emit();
}


void Settings::save_settings(Glib::RefPtr<Gio::File> file)
{
  inhibit_callback = true;
  set_string("Global","Version",VERSION);

  remove_group("Extruder"); // is only temporary

  Glib::ustring contents = to_data();
  // cerr << contents << endl;
  Glib::file_set_contents (file->get_path(), contents);

  SelectExtruder(selectedExtruder); // reload default extruder

  inhibit_callback = true;
  // all changes safely saved
  m_user_changed = false;
}

void Settings::set_to_gui (Builder &builder,
			   const string &group, const string &key)
{
  inhibit_callback = true;
  Glib::ustring glade_name = group + "." + key;
  // inhibit warning for settings not defined in glade UI:
  if (!builder->get_object (glade_name)) {
    //cerr << glade_name << _(" not defined in GUI!")<< endl;
    return;
  }

  Gtk::Widget *w = NULL;
  builder->get_widget (glade_name, w);
  if (!w) {
    std::cerr << _("Missing user interface item ") << glade_name << "\n";
    return;
  }
  Gtk::CheckButton *check = dynamic_cast<Gtk::CheckButton *>(w);
  if (check) {
    check->set_active (get_boolean(group,key));
    return;
  }
  Gtk::SpinButton *spin = dynamic_cast<Gtk::SpinButton *>(w);
  if (spin) {
    spin->set_value (get_double(group,key));
    return;
  }
  Gtk::Range *range = dynamic_cast<Gtk::Range *>(w);
  if (range) {
    range->set_value (get_double(group,key));
    return;
  }
  Gtk::ComboBox *combo = dynamic_cast<Gtk::ComboBox *>(w);
  if (combo) {
    if (glade_name == "Hardware.SerialSpeed") // has real value
      combobox_set_to(combo, get_string(group,key));
    else // has index
      combo->set_active(get_integer(group,key));
    return;
  }
  Gtk::Entry *entry = dynamic_cast<Gtk::Entry *>(w);
  if (entry) {
    entry->set_text (get_string(group,key));
    return;
  }
  Gtk::Expander *exp = dynamic_cast<Gtk::Expander *>(w);
  if (exp) {
    exp->set_expanded (get_boolean(group,key));
    return;
  }
  Gtk::ColorButton *col = dynamic_cast<Gtk::ColorButton *>(w);
  if(col) {
    vector<double> c = get_double_list(group,key);
    Gdk::Color co; co.set_rgb_p(c[0],c[1],c[2]);
    col->set_use_alpha(true);
    col->set_color(co);
    col->set_alpha(c[3] * 65535.0);
    return;
  }
  Gtk::TextView *tv = dynamic_cast<Gtk::TextView *>(w);
  if (tv) {
    tv->get_buffer()->set_text(get_string(group,key));
    return;
  }

  cerr << "set_to_gui of "<< glade_name << " not done!" << endl;
}


void Settings::get_from_gui (Builder &builder, const string &glade_name)
{
  if (inhibit_callback) return;
  if (!builder->get_object (glade_name)) {
    cerr << "no such object " << glade_name << endl;
    return;
  }
  Gtk::Widget *w = NULL;
  builder->get_widget (glade_name, w);
  while (w) { // for using break ...
    string group, key;
    if (!splitpoint(glade_name, group, key)) return;
    //cerr << "get " << group  << "." << key << " from gui"<< endl;
    m_user_changed = true; // is_changed;
    Gtk::CheckButton *check = dynamic_cast<Gtk::CheckButton *>(w);
    if (check) {
      set_boolean(group, key, check->get_active());
      break;
    }
    Gtk::SpinButton *spin = dynamic_cast<Gtk::SpinButton *>(w);
    if (spin) {
      set_double(group, key, spin->get_value());
      break;
    }
    Gtk::Range *range = dynamic_cast<Gtk::Range *>(w);
    if (range) {
      set_double(group, key, range->get_value());
      break;
    }
    Gtk::ComboBox *combo = dynamic_cast<Gtk::ComboBox *>(w);
    if (combo) {
      if (glade_name == "Hardware.SerialSpeed") // has real value
	set_string(group,key,combobox_get_active_value(combo));
      else
	set_integer(group,key,combo->get_active_row_number ());
      break;
    }
    Gtk::Entry *e = dynamic_cast<Gtk::Entry *>(w);
    if (e) {
      set_string(group,key,e->get_text());
      break;
    }
    Gtk::Expander *exp = dynamic_cast<Gtk::Expander *>(w);
    if (exp) {
      set_boolean(group,key,exp->get_expanded());
      break;
    }
    Gtk::ColorButton *cb = dynamic_cast<Gtk::ColorButton *>(w);
    if (cb) {
      get_colour_from_gui(builder, glade_name);
      break;
    }
    Gtk::TextView *tv = dynamic_cast<Gtk::TextView *>(w);
    if (tv) {
      set_string(group,key,tv->get_buffer()->get_text());
      break;
    }
    cerr << _("Did not get setting from  ") << glade_name << endl;
    m_user_changed = false;
    break;
  }
  if (m_user_changed) {
    // update currently edited extruder
    if (glade_name.substr(0,8) == "Extruder") {
      copyGroup("Extruder",numberedExtruder("Extruder", selectedExtruder));
    }
    m_signal_visual_settings_changed.emit();
  }
}



void Settings::get_colour_from_gui (Builder &builder, const string &glade_name)
{
  string group,key;
  if (!splitpoint(glade_name, group,key)) return;
  Gdk::Color c;
  Gtk::ColorButton *w = NULL;
  builder->get_widget (glade_name, w);
  if (!w) return;

  c = w->get_color();

  // FIXME: detect 'changed' etc.
  vector<double> d(4);
  d[0] = c.get_red_p();
  d[1] = c.get_green_p();
  d[2] = c.get_blue_p();
  d[3] = (float) (w->get_alpha()) / 65535.0;

  set_double_list(group, key, d);

  m_signal_visual_settings_changed.emit();
}


// whole group or all groups
void Settings::set_to_gui (Builder &builder, const string filter)
{
  inhibit_callback = true;
  vector< Glib::ustring > groups = get_groups();
  for (uint g = 0; g < groups.size(); g++) {
    vector< Glib::ustring > keys = get_keys(groups[g]);
    for (uint k = 0; k < keys.size(); k++) {
      set_to_gui(builder, groups[g], keys[k]);
    }
  }

  //set_filltypes_to_gui (builder);

  if (filter == "" || filter == "Misc") {
    Gtk::Window *pWindow = NULL;
    builder->get_widget("main_window", pWindow);
    try {
      int w = get_integer("Misc","WindowWidth");
      int h = get_integer("Misc","WindowHeight");
      if (pWindow && w > 0 && h > 0) pWindow->resize(w,h);
      int x = get_integer("Misc","WindowPosX");
      int y = get_integer("Misc","WindowPosY");
      if (pWindow && x > 0 && y > 0) pWindow->move(x,y);
    } catch (const Glib::KeyFileError &err) {
      std::cout << _("Exception ") << err.what() << _(" loading setting\n");
    }
  }

  // Set serial speed. Find the row that holds this value
  if (filter == "" || filter == "Hardware") {
    Gtk::ComboBox *portspeed = NULL;
    builder->get_widget ("Hardware.SerialSpeed", portspeed);
    if (portspeed) {
      std::ostringstream ostr;
      ostr << get_integer("Hardware","SerialSpeed");
      //cerr << "portspeed " << get_integer("Hardware","SerialSpeed") << endl;
      combobox_set_to(portspeed, ostr.str());
    }
  }
  inhibit_callback = false;
}


void Settings::connect_to_ui (Builder &builder)
{
  if (has_group("Ranges")) {
    vector<string> ranges = get_keys("Ranges");
    for (uint i = 0; i < ranges.size(); i++) {
      // get min, max, increment, page-incr.
      vector<double> vals = get_double_list("Ranges", ranges[i]);
      Gtk::Widget *w = NULL;
      try {
	builder->get_widget (ranges[i], w);
	if (!w) {
	  std::cerr << "Missing user interface item " << ranges[i] << "\n";
	  continue;
	}
	Gtk::SpinButton *spin = dynamic_cast<Gtk::SpinButton *>(w);
	if (spin) {
	  spin->set_range (vals[0],vals[1]);
	  spin->set_increments (vals[2],vals[3]);
	  continue;
	}
	Gtk::Range *range = dynamic_cast<Gtk::Range *>(w); // sliders etc.
	if (range) {
	  range->set_range (vals[0],vals[1]);
	  range->set_increments (vals[2],vals[3]);
	  continue;
	}
      } catch (Glib::Exception &ex) {
      }
    }
  }

  // add signal callbacks to GUI elements
  vector< Glib::ustring > groups = get_groups();
  for (uint g = 0; g < groups.size(); g++) {
    if (groups[g] == "Ranges") continue; // done that above
    vector< Glib::ustring > keys = get_keys(groups[g]);
    for (uint k = 0; k < keys.size(); k++) {
      string glade_name = groups[g] + "." + keys[k];
      if (!builder->get_object (glade_name))
	continue;
      Gtk::Widget *w = NULL;
      try {
	builder->get_widget (glade_name, w);
	if (!w) {
	  std::cerr << "Missing user interface item " << glade_name << "\n";
	  continue;
	}
	Gtk::CheckButton *check = dynamic_cast<Gtk::CheckButton *>(w);
	if (check) {
	  check->signal_toggled().connect
	    (sigc::bind(sigc::bind<string>(sigc::mem_fun(*this, &Settings::get_from_gui), glade_name), builder));
	  continue;
	}
	Gtk::SpinButton *spin = dynamic_cast<Gtk::SpinButton *>(w);
	if (spin) {
	  spin->signal_value_changed().connect
	    (sigc::bind(sigc::bind<string>(sigc::mem_fun(*this, &Settings::get_from_gui), glade_name), builder)) ;
	  continue;
	}
	Gtk::Range *range = dynamic_cast<Gtk::Range *>(w);
	if (range) {
	  range->signal_value_changed().connect
	    (sigc::bind(sigc::bind<string>(sigc::mem_fun(*this, &Settings::get_from_gui), glade_name), builder));
	  continue;
	}
	Gtk::ComboBox *combo = dynamic_cast<Gtk::ComboBox *>(w);
	if (combo) {
	  if (glade_name == "Hardware.SerialSpeed") { // Serial port speed
	    vector<string> speeds(serialspeeds,
				  serialspeeds+sizeof(serialspeeds)/sizeof(string));
	    set_up_combobox(combo, speeds);
	  } else if (glade_name.find("Filltype")!=std::string::npos) { // Infill types
	    uint nfills = sizeof(InfillNames)/sizeof(string);
	    vector<string> infills(InfillNames,InfillNames+nfills);
	    set_up_combobox(combo,infills);
	  }
	  combo->signal_changed().connect
	    (sigc::bind(sigc::bind<string>(sigc::mem_fun(*this, &Settings::get_from_gui), glade_name), builder));
	  continue;
	}
	Gtk::Entry *e = dynamic_cast<Gtk::Entry *>(w);
	if (e) {
	  e->signal_changed().connect
	    (sigc::bind(sigc::bind<string>(sigc::mem_fun(*this, &Settings::get_from_gui), glade_name), builder));
	  continue;
	}
	Gtk::Expander *exp = dynamic_cast<Gtk::Expander *>(w);
	if (exp) {
	  exp->property_expanded().signal_changed().connect
	    (sigc::bind(sigc::bind<string>(sigc::mem_fun(*this, &Settings::get_from_gui), glade_name), builder));
	  continue;
	}
	Gtk::ColorButton *cb = dynamic_cast<Gtk::ColorButton *>(w);
	if (cb) {
	  cb->signal_color_set().connect
	    (sigc::bind(sigc::bind<string>(sigc::mem_fun(*this, &Settings::get_from_gui), glade_name), builder));
	  continue;
	}
	Gtk::TextView *tv = dynamic_cast<Gtk::TextView *>(w);
	if (tv) {
	  tv->get_buffer()->signal_changed().connect
	    (sigc::bind(sigc::bind<string>(sigc::mem_fun(*this, &Settings::get_from_gui), glade_name), builder));
	  continue;
	}
      } catch (Glib::Exception &ex) {
      }
    }
  }


  /* Update UI with defaults */
  m_signal_update_settings_gui.emit();
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

double Settings::GetExtrudedMaterialWidth(double layerheight) const
{
  // ExtrudedMaterialWidthRatio is preset by user
  return min(max(get_double("Extruder","MinimumLineWidth"),
		 get_double("Extruder","ExtrudedMaterialWidthRatio") * layerheight),
	     get_double("Extruder","MaximumLineWidth"));
}

// TODO This depends whether lines are packed or not - ellipsis/rectangle

// how much mm filament material per extruded line length mm -> E gcode
double Settings::GetExtrusionPerMM(double layerheight) const
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
double Settings::GetInfillDistance(double layerthickness, float percent) const
{
  double fullInfillDistance = GetExtrudedMaterialWidth(layerthickness);
  if (percent == 0) return 10000000;
  return fullInfillDistance * (100./percent);
}

uint Settings::getNumExtruders() const
{
  vector< Glib::ustring > groups = get_groups();
  uint num=0;
  for (uint g = 0; g < groups.size(); g++)
    if (groups[g].substr(0,8) == "Extruder"
	&& groups[g].length() > 8 ) // count only numbered
      num++;
  return num;
}

std::vector<char> Settings::get_extruder_letters() const
{
  uint num = getNumExtruders();
  std::vector<char> letters(num);
  for (uint i = 0; i < num; i++)
    letters[i] = get_string(numberedExtruder("Extruder",i),"GCLetter")[0];
  return letters;
}

uint Settings::GetSupportExtruder() const
{
  uint num = getNumExtruders();
  for (uint i = 0; i < num; i++)
    if (get_boolean(numberedExtruder("Extruder",i),"UseForSupport"))
      return i;
  return 0;
}

Vector3d Settings::get_extruder_offset(uint num) const
{
  string ext = numberedExtruder("Extruder",num);
  return Vector3d(get_double(ext, "OffsetX"),
		  get_double(ext, "OffsetY"), 0.);
}


void Settings::copyGroup(const string &from, const string &to)
{
  vector<string> keys = get_keys(from);
  for (uint i = 0; i < keys.size(); i++)
    set_value(to, keys[i], get_value(from, keys[i]));
}

// create new
void Settings::CopyExtruder(uint num)
{
  uint total = getNumExtruders();
  string from = numberedExtruder("Extruder",num);
  string to   = numberedExtruder("Extruder",total);
  copyGroup(from, to);
}
void Settings::RemoveExtruder(uint num)
{
  ostringstream oss; oss << "Extruder"<<num;
  remove_group(oss.str());
}
void Settings::SelectExtruder(uint num, Builder *builder)
{
  if (num >= getNumExtruders()) return;
  selectedExtruder = num;
  copyGroup(numberedExtruder("Extruder",num),"Extruder");
  // show Extruder settings on gui
  if (builder) {
    set_to_gui(*builder, "Extruder");
  }
}

Matrix4d Settings::getBasicTransformation(Matrix4d T) const
{
  Vector3d t;
  T.get_translation(t);
  const Vector3d margin = getPrintMargin();
  double rsize = get_double("Raft","Size") * (get_boolean("Raft","Enable")?1:0);
  t+= Vector3d(margin.x() + rsize, margin.y() + rsize, 0);
  T.set_translation(t);
  return T;
}


Vector3d Settings::getPrintVolume() const
{
  return Vector3d (get_double("Hardware","Volume.X"),
		   get_double("Hardware","Volume.Y"),
		   get_double("Hardware","Volume.Z"));

}
Vector3d Settings::getPrintMargin() const
{
  Vector3d margin(get_double("Hardware","PrintMargin.X"),
		  get_double("Hardware","PrintMargin.Y"),
		  get_double("Hardware","PrintMargin.Z"));
  Vector3d maxoff = Vector3d::ZERO;
  uint num = getNumExtruders();
  for (uint i = 0; i < num ; i++) {
    string ext = numberedExtruder("Extruder",i);
    double offx = 0, offy = 0;
    try {
      offx = abs(get_double(ext, "OffsetX"));
      offy = abs(get_double(ext, "OffsetY"));
    } catch (const Glib::KeyFileError &err) {
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
  std::string basename = Glib::path_get_dirname(filename);
  return Glib::build_filename (basename, get_string("Global","Image"));
}



bool Settings::set_user_button(const string &name, const string &gcode) {
  try {
    vector<string> buttonlabels = get_string_list("UserButtons","Labels");
    vector<string> buttongcodes = get_string_list("UserButtons","GCodes");
    for (uint i = 0; i < buttonlabels.size(); i++){
      if (buttonlabels[i] == name) {
	// change button
	buttongcodes[i] = gcode;
	set_string_list("UserButtons","GCodes",buttongcodes);
      } else {
	// add button
	buttonlabels.push_back(name);
	buttongcodes.push_back(gcode);
	set_string_list("UserButtons","Labels",buttonlabels);
	set_string_list("UserButtons","GCodes",buttongcodes);
      }
    }
  } catch (const Glib::KeyFileError &err) {
  }
  return true;
}

string Settings::get_user_gcode(const string &name) {
  try {
    vector<string> buttonlabels = get_string_list("UserButtons","Labels");
    vector<string> buttongcodes = get_string_list("UserButtons","GCodes");
    for (uint i = 0; i < buttonlabels.size(); i++){
      if (buttonlabels[i] == name)
	return buttongcodes[i];
    }
  } catch (const Glib::KeyFileError &err) {
  }
  return "";
}

bool Settings::del_user_button(const string &name) {
  try {
    vector<string> buttonlabels = get_string_list("UserButtons","Labels");
    vector<string> buttongcodes = get_string_list("UserButtons","GCodes");
    for (uint i = 0; i < buttonlabels.size(); i++){
      if (buttonlabels[i] == name) {
	buttonlabels.erase(buttonlabels.begin()+i);
	buttongcodes.erase(buttongcodes.begin()+i);
	return true;
      }
    }
  } catch (const Glib::KeyFileError &err) {
  }
  return false;
}

