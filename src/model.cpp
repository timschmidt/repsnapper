/*
    This file is a part of the RepSnapper project.
    Copyright (C) 2010  Kulitorum

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include "config.h"
#include <vector>
#include <string>

// should move to platform.h with com port fun.
#include <sys/types.h>
#include <dirent.h>

#include <functional>
#include "stdafx.h"
#include "model.h"
#include "rfo.h"
#include "file.h"
#include "render.h"
#include "settings.h"
#include "progress.h"
#include "connectview.h"
#include "reprapserial.h"

// Exception safe guard to stop people printing
// GCode while loading it / converting etc.
struct PrintInhibitor {
  Model *m_mvc;
public:
  PrintInhibitor(Model *mvc) : m_mvc (mvc)
  {
    m_mvc->m_continue_button->set_sensitive (false);
    m_mvc->m_print_button->set_sensitive (false);
  }
  ~PrintInhibitor()
  {
    m_mvc->m_continue_button->set_sensitive (true);
    m_mvc->m_print_button->set_sensitive (true);
  }
};

bool Model::on_delete_event(GdkEventAny* event)
{
  Gtk::Main::quit();
  return false;
}

void Model::connect_button(const char *name, const sigc::slot<void> &slot)
{
  Gtk::Button *button = NULL;
  m_builder->get_widget (name, button);
  if (button)
    button->signal_clicked().connect (slot);
  else {
    std::cerr << "missing button " << name << "\n";
  }
}

void Model::connect_action(const char *name, const sigc::slot<void> &slot)
{
  Glib::RefPtr<Glib::Object> object;
  object = m_builder->get_object (name);
  Glib::RefPtr<Gtk::Action> action = Glib::RefPtr<Gtk::Action>::cast_dynamic(object);
  if (action)
    action->signal_activate().connect (slot);
  else {
    std::cerr << "missing action " << name << "\n";
  }
}

void Model::connect_toggled(const char *name, const sigc::slot<void, Gtk::ToggleButton *> &slot)
{
  Gtk::ToggleButton *button = NULL;
  m_builder->get_widget (name, button);
  if (button)
    button->signal_toggled().connect (sigc::bind(slot, button));
  else {
    std::cerr << "missing toggle button " << name << "\n";
  }
}

void Model::load_gcode ()
{
  FileChooser::ioDialog (this, FileChooser::OPEN, FileChooser::GCODE);
}

void Model::save_gcode ()
{
  FileChooser::ioDialog (this, FileChooser::SAVE, FileChooser::GCODE);
}

void Model::load_stl ()
{
  FileChooser::ioDialog (this, FileChooser::OPEN, FileChooser::STL);
}

void Model::save_stl ()
{
  FileChooser::ioDialog (this, FileChooser::SAVE, FileChooser::STL);
}

void Model::send_gcode ()
{
  SendNow (m_gcode_entry->get_text());
}

Model *Model::create()
{
  Glib::RefPtr<Gtk::Builder> builder;
  try {
    builder = Gtk::Builder::create_from_file("repsnapper.ui");
  }
  catch(const Glib::FileError& ex)
  {
    std::cerr << "FileError: " << ex.what() << std::endl;
    throw ex;
  }
  catch(const Gtk::BuilderError& ex)
  {
    std::cerr << "BuilderError: " << ex.what() << std::endl;
    throw ex;
  }
  Model *mvc = 0;
  builder->get_widget_derived("main_window", mvc);

  return mvc;
}

void Model::printing_changed()
{
  if (serial->isPrinting()) {
    m_print_button->set_label ("Restart");
    m_continue_button->set_label ("Pause");
  } else {
    m_print_button->set_label ("Print");
    m_continue_button->set_label ("Continue");
  }
}

void Model::power_toggled()
{
  if (m_power_button->get_active())
    serial->SendNow("M80");
  else
    serial->SendNow("M81");
}

void Model::enable_logging_toggled (Gtk::ToggleButton *button)
{
  settings.Misc.FileLoggingEnabled = button->get_active();
}

void Model::fan_enabled_toggled (Gtk::ToggleButton *button)
{
  if (!button->get_active())
    serial->SendNow("M107");
  else {
    std::stringstream oss;
    oss << "M106 S" << (int)m_fan_voltage->get_value();
    serial->SendNow (oss.str());
  }
}

void Model::clear_logs()
{
  serial->clear_logs();
}

void Model::hide_on_response(int, Gtk::Dialog *dialog)
{
  dialog->hide();
}

void Model::show_dialog(const char *name)
{
  Gtk::Dialog *dialog;
  m_builder->get_widget (name, dialog);
  if (!dialog) {
    cerr << "no such dialog " << name << "\n";
    return;
  }
  dialog->signal_response().connect (sigc::bind(sigc::mem_fun(*this, &Model::hide_on_response), dialog));
  dialog->show();
}

void Model::about_dialog()
{
  show_dialog ("about_dialog");
}

static const char *axis_names[] = { "X", "Y", "Z" };

enum SpinType { TRANSLATE, ROTATE, SCALE };
class Model::SpinRow {
  void spin_value_changed (int axis)
  {
    RFO_File *file;
    RFO_Object *object;

    if (m_inhibit_update)
      return;
    if (!m_mvc->get_selected_stl(object, file))
      return;
    if (!object && !file)
      return;

    double val = m_xyz[axis]->get_value();
    switch (m_type) {
    default:
    case TRANSLATE: {
      Matrix4f *mat;
      if (!file)
	mat = &object->transform3D.transform;
      else
	mat = &file->transform3D.transform;
      Vector3f trans = mat->getTranslation();
      trans.xyz[axis] = val;
      mat->setTranslation (trans);
      m_mvc->CalcBoundingBoxAndCenter();
      break;
    }
    case ROTATE: {
      Vector4f rot(0.0, 0.0, 0.0, 1.0);
      rot.xyzw[axis] = (M_PI * val) / 180.0;
      m_mvc->RotateObject(rot);
      break;
    }
    case SCALE:
      cerr << "Scale not yet implemented\n";
      break;
    }
  }
public:
  bool m_inhibit_update;
  Model *m_mvc;
  SpinType m_type;
  Gtk::Box *m_box;
  Gtk::SpinButton *m_xyz[3];

  void selection_changed ()
  {
    RFO_File *file;
    RFO_Object *object;
    if (!m_mvc->get_selected_stl(object, file) || (!object && !file))
      return;

    m_inhibit_update = true;
    for (uint i = 0; i < 3; i++) {
      switch (m_type) {
      default:
      case TRANSLATE: {
	Matrix4f *mat;
	if (!object) {
	  for (uint i = 0; i < 3; i++)
	    m_xyz[i]->set_value(0.0);
	  break;
	}
	else if (!file)
	  mat = &object->transform3D.transform;
	else
	  mat = &file->transform3D.transform;
	Vector3f trans = mat->getTranslation();
	for (uint i = 0; i < 3; i++)
	  m_xyz[i]->set_value(trans.xyz[i]);
	break;
      }
      case ROTATE:
	for (uint i = 0; i < 3; i++)
	  m_xyz[i]->set_value(0.0);
	break;
      case SCALE:
	for (uint i = 0; i < 3; i++)
	  m_xyz[i]->set_value(0.0);
	break;
      }
    }
    m_inhibit_update = false;
  }

  SpinRow(Model *mvc, Gtk::TreeView *rfo_tree,
	  const char *box_name, SpinType type) :
    m_inhibit_update(false), m_mvc(mvc), m_type (type)
  {
    mvc->m_builder->get_widget (box_name, m_box);

    for (uint i = 0; i < 3; i++) {
      m_box->add (*new Gtk::Label (axis_names[i]));
      m_xyz[i] = new Gtk::SpinButton();
      m_xyz[i]->set_numeric();
      switch (m_type) {
      default:
      case TRANSLATE:
	m_xyz[i]->set_digits (1);
	m_xyz[i]->set_increments (0.5, 10);
	m_xyz[i]->set_range(-500.0, +500.0);
	break;
      case ROTATE:
	m_xyz[i]->set_digits (0);
	m_xyz[i]->set_increments (45.0, 90.0);
	m_xyz[i]->set_range(-360.0, +360.0);
	break;
      case SCALE:
	m_xyz[i]->set_digits (3);
	m_xyz[i]->set_increments (0.5, 1.0);
	m_xyz[i]->set_range(0.001, +10.0);
	break;
      }
      m_box->add (*m_xyz[i]);
      m_xyz[i]->signal_value_changed().connect
	(sigc::bind(sigc::mem_fun(*this, &SpinRow::spin_value_changed), (int)i));
    }
    selection_changed();
    m_box->show_all();

    rfo_tree->get_selection()->signal_changed().connect
      (sigc::mem_fun(*this, &SpinRow::selection_changed));
  }

  ~SpinRow()
  {
    for (uint i = 0; i < 3; i++)
      delete m_xyz[i];
  }
};

class Model::TempRow : public Gtk::HBox {
public:
  // see: http://reprap.org/wiki/RepRapGCodes for more details
  void heat_toggled (Gtk::ToggleButton *pOn)
  {
    static const char *mdetail[] = { "M104", "M140" };
    std::stringstream oss;
    oss << mdetail[m_type];
    oss << " S";
    if (pOn->get_active())
      oss << m_target->get_value();
    else
      oss << "0";
    m_serial->SendNow (oss.str());
  }

public:
  enum TempType { NOZZLE, BED };

  RepRapSerial *m_serial;
  TempType m_type;
  Gtk::Label *m_temp;
  Gtk::SpinButton *m_target;

  void update_temp (double value)
  {
    m_target->set_value (value);
  }

  TempRow(RepRapSerial *serial, TempType type) :
    m_serial(serial), m_type(type)
  {
    static const char *names[] = { "Nozzle", "Bed" };

    add(*(new Gtk::Label(names[type])));
    Gtk::ToggleButton *pOn = new Gtk::ToggleButton("heat on");
    pOn->signal_toggled().connect
      (sigc::bind (sigc::mem_fun (*this, &TempRow::heat_toggled), pOn));
    add(*pOn);
    add(*(new Gtk::Label("temperature:")));
    m_temp = new Gtk::Label();
    add (*m_temp);
    add(*(new Gtk::Label("target:")));
    m_target = new Gtk::SpinButton();
    m_target->set_increments (1, 5);
    m_target->set_range(25.0, 256.0);
    switch (type) {
    case NOZZLE:
    default:
      m_target->set_value(200.0);
      break;
    case BED:
      m_target->set_value(100.0);
      break;
    }
    add (*m_target);
  }

  ~TempRow()
  {
    delete m_temp;
    delete m_target;
  }
};

class Model::AxisRow : public Gtk::HBox {
public:
  void home_clicked()
  {
    m_mvc->Home(std::string (axis_names[m_axis]));
    m_target->set_value(0.0);
  }
  void spin_value_changed ()
  {
    if (m_inhibit_update)
      return;
    m_mvc->Goto (std::string (axis_names[m_axis]), m_target->get_value());
  }
  void nudge_clicked (double nudge)
  {
    m_inhibit_update = true;
    m_target->set_value (MAX (m_target->get_value () + nudge, 0.0));
    m_mvc->Move (std::string (axis_names[m_axis]), nudge);
    m_inhibit_update = false;
  }
  void add_nudge_button (double nudge)
  {
    std::stringstream label;
    if (nudge > 0)
      label << "+";
    label << nudge;
    Gtk::Button *button = new Gtk::Button(label.str());
    add(*button);
    button->signal_clicked().connect
      (sigc::bind(sigc::mem_fun (*this, &AxisRow::nudge_clicked), nudge));
  }
  bool m_inhibit_update;
  Model *m_mvc;
  Gtk::SpinButton *m_target;
  int m_axis;

public:
  void notify_homed()
  {
    m_inhibit_update = true;
    m_target->set_value(0.0);
    m_inhibit_update = false;
  }
  AxisRow(Model *mvc, int axis) :
    m_inhibit_update(false), m_mvc(mvc), m_axis(axis)
  {
    add(*(new Gtk::Label(axis_names[axis])));
    Gtk::Button *home = new Gtk::Button("Home");
    home->signal_clicked().connect
      (sigc::mem_fun (*this, &AxisRow::home_clicked));
    add (*home);

    add_nudge_button (-10.0);
    add_nudge_button (-1.0);
    add_nudge_button (-0.1);
    m_target = new Gtk::SpinButton();
    m_target->set_digits (1);
    m_target->set_increments (0.1, 1);
    m_target->set_range(-200.0, +200.0);
    m_target->set_value(0.0);
    add (*m_target);
    m_target->signal_value_changed().connect
      (sigc::mem_fun(*this, &AxisRow::spin_value_changed));

    add_nudge_button (+0.1);
    add_nudge_button (+1.0);
    add_nudge_button (+10.0);
  }

  ~AxisRow()
  {
    delete m_target;
  }
};

void Model::home_all()
{
  serial->SendNow("G28");
  for (uint i = 0; i < 3; i++)
    m_axis_rows[i]->notify_homed();
}

void Model::temp_changed()
{
  m_temps[TempRow::NOZZLE]->update_temp (serial->getTempExtruder());
  m_temps[TempRow::BED]->update_temp (serial->getTempBed());
}

void Model::load_settings()
{
  FileChooser::ioDialog (this, FileChooser::OPEN, FileChooser::SETTINGS);
}

void Model::save_settings()
{
  SaveConfig ("repsnapper.conf");
}

void Model::save_settings_as()
{
  FileChooser::ioDialog (this, FileChooser::SAVE, FileChooser::SETTINGS);
}

Model::Model(BaseObjectType* cobject,
					 const Glib::RefPtr<Gtk::Builder>& builder)
  : Gtk::Window(cobject), m_builder(builder)
{
  // Menus
  connect_action ("OpenStl",         sigc::mem_fun(*this, &Model::load_stl) );
  connect_action ("OpenGCode",       sigc::mem_fun(*this, &Model::load_gcode) );
  connect_action ("Quit",            sigc::ptr_fun(&Gtk::Main::quit));
  connect_action ("About",           sigc::mem_fun(*this, &Model::about_dialog) );

  connect_action ("PreferencesDialog", sigc::bind(sigc::mem_fun(*this, &Model::show_dialog),
						  "preferences_dlg"));
  connect_action ("LoadSettings",    sigc::mem_fun(*this, &Model::load_settings));
  connect_action ("SaveSettings",    sigc::mem_fun(*this, &Model::save_settings));
  connect_action ("SaveSettingsAs",  sigc::mem_fun(*this, &Model::save_settings_as));

#if 0
  // Simple tab
  connect_button ("s_load_stl",      sigc::mem_fun(*this, &Model::load_stl) );
  connect_button ("s_convert_gcode", sigc::mem_fun(*this, &Model::ConvertToGCode) );
  connect_button ("s_load_gcode",    sigc::mem_fun(*this, &Model::load_gcode) );
  connect_button ("s_print",         sigc::mem_fun(*this, &Model::SimplePrint) );
#endif

  // Model tab
  connect_button ("m_load_stl",      sigc::mem_fun(*this, &Model::load_stl) );
  connect_button ("m_save_stl",      sigc::mem_fun(*this, &Model::save_stl) );
  connect_button ("m_delete",        sigc::mem_fun(*this, &Model::delete_selected_stl) );
  connect_button ("m_duplicate",     sigc::mem_fun(*this, &Model::duplicate_selected_stl) );
  connect_button ("m_auto_rotate",   sigc::mem_fun(*this, &Model::OptimizeRotation) );
  connect_button ("m_rot_x",         sigc::bind(sigc::mem_fun(*this, &Model::RotateObject), Vector4f(1,0,0, M_PI/4)));
  connect_button ("m_rot_y",         sigc::bind(sigc::mem_fun(*this, &Model::RotateObject), Vector4f(0,1,0, M_PI/4)));
  connect_button ("m_rot_z",         sigc::bind(sigc::mem_fun(*this, &Model::RotateObject), Vector4f(0,0,1, M_PI/4)));
  m_builder->get_widget ("m_rfo_tree", m_rfo_tree);
  m_rfo_tree->set_model (rfo.m_model);
  m_rfo_tree->append_column("Name", rfo.m_cols->m_name);
  static const struct {
    const char *name;
    SpinType type;
  } spin_boxes[] = {
    { "m_box_translate", TRANSLATE},
    { "m_box_rotate", ROTATE },
    { "m_box_scale", SCALE }
  };
  for (uint i = 0; i < 3; i++)
    m_spin_rows[i] = new SpinRow (this, m_rfo_tree, spin_boxes[i].name, spin_boxes[i].type);

  // GCode tab
  Gtk::TextView *textv = NULL;
  m_builder->get_widget ("txt_gcode_result", textv);
  textv->set_buffer (gcode.buffer);

  m_builder->get_widget ("g_gcode", m_gcode_entry);
  m_gcode_entry->set_activates_default();
  m_gcode_entry->signal_activate().connect (sigc::mem_fun(*this, &Model::send_gcode));;

  connect_button ("g_load_gcode",    sigc::mem_fun(*this, &Model::load_gcode) );
  connect_button ("g_convert_gcode", sigc::mem_fun(*this, &Model::ConvertToGCode) );
  connect_button ("g_save_gcode",    sigc::mem_fun(*this, &Model::save_gcode) );
  connect_button ("g_send_gcode",    sigc::mem_fun(*this, &Model::send_gcode) );

  // Print tab
  connect_button ("p_kick",          sigc::mem_fun(*this, &Model::Continue));
  m_builder->get_widget ("p_power", m_power_button);
  m_power_button->signal_toggled().connect    (sigc::mem_fun(*this, &Model::power_toggled));
  m_builder->get_widget ("p_print", m_print_button);
  m_print_button->signal_clicked().connect    (sigc::mem_fun(*this, &Model::PrintButton));
  m_builder->get_widget ("p_pause", m_continue_button);
  m_continue_button->signal_clicked().connect (sigc::mem_fun(*this, &Model::ContinuePauseButton));

  // Interactive tab
  connect_button ("i_home_all",        sigc::mem_fun(*this, &Model::home_all));
  connect_toggled ("i_enable_logging", sigc::mem_fun(*this, &Model::enable_logging_toggled));
  connect_button ("i_clear_logs",      sigc::mem_fun(*this, &Model::clear_logs) );
  m_builder->get_widget ("i_reverse", m_extruder_reverse);
  m_builder->get_widget ("i_ex_speed", m_extruder_speed);
  m_extruder_speed->set_range(100.0, 10000.0);
  m_extruder_speed->set_increments (100, 500);
  m_extruder_speed->set_value (3000.0);
  m_builder->get_widget ("i_ex_length", m_extruder_length);
  m_extruder_length->set_range(0.0, 1000.0);
  m_extruder_length->set_increments (5, 20);
  m_extruder_length->set_value (150.0);
  // FIXME: connect i_update_interval (etc.)
  connect_toggled ("i_fan_enabled", sigc::mem_fun(*this, &Model::fan_enabled_toggled));
  m_builder->get_widget ("i_fan_voltage", m_fan_voltage);
  m_fan_voltage->set_range(0.0, 25.0);
  m_fan_voltage->set_increments (1, 2);
  m_fan_voltage->set_value (5.0);

  // Should these guys really be hardware settings on that page:
  // DownstreamMultiplier etc. ?
  m_builder->get_widget ("i_ex_speed_mult", m_extruder_speed_mult);
  m_extruder_speed_mult->set_range(0.1, 10.0);
  m_extruder_speed_mult->set_increments (0.1, 1);
  m_extruder_speed_mult->set_value (1.0);
  m_builder->get_widget ("i_ex_length_mult", m_extruder_length_mult);
  m_extruder_length_mult->set_range(0.1, 10.0);
  m_extruder_length_mult->set_increments (0.1, 1);
  m_extruder_length_mult->set_value (1.0);
  connect_button ("i_extrude_length", sigc::mem_fun(*this, &Model::RunExtruder) );

  // Main view progress bar
  Gtk::Box *box = NULL;
  Gtk::Label *label = NULL;
  Gtk::ProgressBar *bar = NULL;

  m_builder->get_widget("progress_box", box);
  m_builder->get_widget("progress_bar", bar);
  m_builder->get_widget("progress_label", label);
  m_progress = new Progress (box, bar, label);

  serial = new RepRapSerial(m_progress, &settings);
  serial->signal_printing_changed().connect (sigc::mem_fun(*this, &Model::printing_changed));
  serial->signal_temp_changed().connect (sigc::mem_fun(*this, &Model::temp_changed));

  m_view = new ConnectView(serial, &settings);
  Gtk::Box *connect_box = NULL;
  m_builder->get_widget ("p_connect_button_box", connect_box);
  connect_box->add (*m_view);

  Gtk::Box *temp_box;
  m_builder->get_widget ("i_temp_box", temp_box);
  m_temps[TempRow::NOZZLE] = new TempRow (serial, TempRow::NOZZLE);
  m_temps[TempRow::BED] = new TempRow (serial, TempRow::BED);
  temp_box->add (*m_temps[TempRow::NOZZLE]);
  temp_box->add (*m_temps[TempRow::BED]);

  Gtk::Box *axis_box;
  m_builder->get_widget ("i_axis_controls", axis_box);
  for (uint i = 0; i < 3; i++) {
    m_axis_rows[i] = new AxisRow (this, i);
    axis_box->add (*m_axis_rows[i]);
  }

  Gtk::TextView *log_view;
  m_builder->get_widget ("i_txt_comms", log_view);
  log_view->set_buffer (serial->get_log (RepRapSerial::LOG_COMMS));
  m_builder->get_widget ("i_txt_errs", log_view);
  log_view->set_buffer (serial->get_log (RepRapSerial::LOG_ERRORS));
  m_builder->get_widget ("i_txt_echo", log_view);
  log_view->set_buffer (serial->get_log (RepRapSerial::LOG_ECHO));

  // 3D preview of the bed
  Gtk::Box *pBox = NULL;
  m_builder->get_widget("viewarea", pBox);
  if (!pBox)
    std::cerr << "missing box!";
  else {
    Gtk::Widget *view = new Render (this, m_rfo_tree->get_selection());
    pBox->add (*view);
    Gtk::Window *pWindow = NULL;
    m_builder->get_widget("main_window", pWindow);
    if (pWindow)
      pWindow->show_all();
  }

  // connect settings
  settings.connectToUI (*((Builder *)&m_builder));
}

Model::~Model()
{
  for (uint i = 0; i < 3; i++) {
    delete m_spin_rows[i];
    delete m_axis_rows[i];
  }
  delete m_temps[TempRow::NOZZLE];
  delete m_temps[TempRow::BED];
  delete m_view;
  delete m_progress;
  delete serial;
}

void Model::redraw()
{
   queue_draw();
}

void Model::Init()
{
	DetectComPorts (true);
	LoadConfig();

	Glib::signal_timeout().connect
		(sigc::mem_fun(*this, &Model::timer_function),
		 250 /* ms */);
}

void Model::SaveConfig(string filename)
{
  settings.save_settings (filename);
}

void Model::LoadConfig(string filename)
{
  settings.load_settings (filename);
}

void Model::SimpleAdvancedToggle()
{
   cout << "not yet implemented\n";
}

/* Called every 250ms (0.25 of a second) */
bool Model::timer_function()
{
  ToolkitLock guard;
#warning FIXME: busy polling for com ports is a disaster [!]
#warning FIXME: we should auto-select one at 'connect' time / combo drop-down instead
  if (!serial->isConnected()) {
    static uint count = 0;
    if ((count++ % 4) == 0) /* every second */
      DetectComPorts();
  }
  return true;
}

void Model::DetectComPorts(bool init)
{
	bool bDirty = init;
	vector<std::string> currentComports;

#ifdef WIN32
	int highestCom = 0;
	for(int i = 1; i <=9 ; i++ )
	{
	        TCHAR strPort[32] = {0};
	        _stprintf(strPort, _T("COM%d"), i);

	        DWORD dwSize = 0;
	        LPCOMMCONFIG lpCC = (LPCOMMCONFIG) new BYTE[1];
	        GetDefaultCommConfig(strPort, lpCC, &dwSize);
		int r = GetLastError();
	        delete [] lpCC;

	        lpCC = (LPCOMMCONFIG) new BYTE[dwSize];
	        GetDefaultCommConfig(strPort, lpCC, &dwSize);
	        delete [] lpCC;

		if( r != 87 )
		{
			ToolkitLock guard;
			highestCom = i;
		}
		else
		{
			ToolkitLock guard;
		}
	}
	currentComports.push_back(string("COM"+highestCom));

#elif defined(__APPLE__)
	const char *ttyPattern = "tty.";

#else // Linux
	const char *ttyPattern = "ttyUSB";
#endif

#ifndef WIN32
	DIR *d = opendir ("/dev");
	if (d) { // failed
		struct	dirent *e;
		while ((e = readdir (d))) {
			//fprintf (stderr, "name '%s'\n", e->d_name);
			if (strstr(e->d_name,ttyPattern)) {
				string device = string("/dev/") + e->d_name;
				currentComports.push_back(device);
			}
		}
		closedir(d);

		if ( currentComports.size() != this->comports.size() )
			bDirty = true;
	}

	if ( bDirty ) {
		ToolkitLock guard;

		bool bWasEmpty = !comports.size();

		comports.clear();

#warning we need to propagate the available com ports to the UI ...
#if 0
		for (size_t indx = 0; indx < currentComports.size(); ++indx) {
			string menuLabel = string(currentComports[indx]);
			comports.push_back(currentComports[indx]);
		}
#endif

		// auto-select a new com-port
		if (bWasEmpty && comports.size())
		  settings.Hardware.PortName = ValidateComPort(comports[0]);
	}
#endif
}

string Model::ValidateComPort (const string &port)
{
	DetectComPorts();

	// is it a valid port ?
	for (uint i = 0; i < comports.size(); i++) {
		if (port == comports[i])
			return port;
	}
	       
	if (comports.size())
		return comports[0];
	else
		return "No ports found";
}

void Model::ReadGCode(string filename)
{
  PrintInhibitor inhibitPrint(this);
  m_progress->start ("Converting", 100.0);
  gcode.Read (this, m_progress, filename);
  m_progress->stop ("Done");
}

void Model::ConvertToGCode()
{
	string GcodeTxt;

	string GcodeStart = settings.GCode.getStartText();
	string GcodeLayer = settings.GCode.getLayerText();
	string GcodeEnd = settings.GCode.getEndText();

	PrintInhibitor inhibitPrint(this);
	m_progress->start ("Converting", Max.z);

	// Make Layers
	uint LayerNr = 0;
	uint LayerCount = (uint)ceil((Max.z+settings.Hardware.LayerThickness*0.5f)/settings.Hardware.LayerThickness);

	vector<int> altInfillLayers;
	settings.Slicing.GetAltInfillLayers (altInfillLayers, LayerCount);

	printOffset = settings.Hardware.PrintMargin;

	float z = Min.z + settings.Hardware.LayerThickness*0.5f;				// Offset it a bit in Z, z=0 gives a empty slice because no triangles crosses this Z value

	gcode.clear();

	float printOffsetZ = settings.Hardware.PrintMargin.z;

	if(settings.RaftEnable)
	{
		printOffset += Vector3f(settings.Raft.Size, settings.Raft.Size, 0);
		MakeRaft(printOffsetZ);
	}
	float E=0.0f;
	while(z<Max.z)
	{
	  m_progress->update(z);
		for(uint o=0;o<rfo.Objects.size();o++)
		{
			for(uint f=0;f<rfo.Objects[o].files.size();f++)
			{
				STL* stl = &rfo.Objects[o].files[f].stl;	// Get a pointer to the object
				Matrix4f T = rfo.GetSTLTransformationMatrix(o,f);
				Vector3f t = T.getTranslation();
				t+= Vector3f(settings.Hardware.PrintMargin.x+settings.Raft.Size*settings.RaftEnable, settings.Hardware.PrintMargin.y+settings.Raft.Size*settings.RaftEnable, 0);
				T.setTranslation(t);
				CuttingPlane plane;
				stl->CalcCuttingPlane(z, plane, T);	// output is alot of un-connected line segments with individual vertices, describing the outline

				float hackedZ = z;
				while (plane.LinkSegments (hackedZ, settings.Slicing.Optimization) == false)	// If segment linking fails, re-calc a new layer close to this one, and use that.
				{ // This happens when there's triangles missing in the input STL
					hackedZ+= 0.1f;
					stl->CalcCuttingPlane (hackedZ, plane, T);	// output is alot of un-connected line segments with individual vertices
				}
				plane.SetZ (z + printOffsetZ);

				// inFill
				vector<Vector2f> infill;

//				CuttingPlane infillCuttingPlane;
//				plane.MakeContainedPlane(infillCuttingPlane);
				if(settings.Slicing.ShellOnly == false)
				{
					switch( settings.Slicing.ShrinkQuality )
					{
					case SHRINK_FAST:
						plane.ShrinkFast(settings.Hardware.ExtrudedMaterialWidth*0.5f, settings.Slicing.Optimization, settings.Display.DisplayCuttingPlane, false, settings.Slicing.ShellCount);
						break;
					case SHRINK_LOGICK:
						plane.ShrinkLogick(settings.Hardware.ExtrudedMaterialWidth, settings.Slicing.Optimization, settings.Display.DisplayCuttingPlane, settings.Slicing.ShellCount);
						break;
					}

					// check if this if a layer we should use the alternate infill distance on
					float infillDistance = settings.Slicing.InfillDistance;
					if (std::find(altInfillLayers.begin(), altInfillLayers.end(), LayerNr) != altInfillLayers.end())
					{
						infillDistance = settings.Slicing.AltInfillDistance;
					}

					plane.CalcInFill(infill, LayerNr, infillDistance, settings.Slicing.InfillRotation, settings.Slicing.InfillRotationPrLayer, settings.Display.DisplayDebuginFill);
				}
				// Make the GCode from the plane and the infill
				plane.MakeGcode(infill, gcode, E, z+printOffsetZ, settings.Hardware.MinPrintSpeedXY,
						settings.Hardware.MaxPrintSpeedXY, settings.Hardware.MinPrintSpeedZ,
						settings.Hardware.MaxPrintSpeedZ, settings.Hardware.DistanceToReachFullSpeed,
						settings.Hardware.ExtrusionFactor,
						settings.Slicing.UseIncrementalEcode,
						settings.Slicing.Use3DGcode,
						settings.Slicing.EnableAcceleration);
			}
		}
		LayerNr++;
		z+=settings.Hardware.LayerThickness;
	}

	float AntioozeDistance = settings.Slicing.AntioozeDistance;
	if (!settings.Slicing.EnableAntiooze)
	  AntioozeDistance = 0;

	gcode.MakeText (GcodeTxt, GcodeStart, GcodeLayer, GcodeEnd,
			settings.Slicing.UseIncrementalEcode,
			settings.Slicing.Use3DGcode,
			AntioozeDistance,
			settings.Slicing.AntioozeSpeed);
	m_progress->stop("Done");
}

void Model::init()
{
#ifdef ENABLE_LUA
	buffer = gui->LuaScriptEditor->buffer();
	buffer->text("--Clear existing gcode\nbase:ClearGcode()\n-- Set start speed for acceleration firmware\nbase:AddText(\"G1 F2600\\n\")\n\n	 z=0.0\n	 e=0;\n	oldx = 0;\n	oldy=0;\n	while(z<47) do\n	angle=0\n		while (angle < math.pi*2) do\n	x=(50*math.cos(z/30)*math.sin(angle))+70\n		y=(50*math.cos(z/30)*math.cos(angle))+70\n\n		--how much to extrude\n\n		dx=oldx-x\n		dy=oldy-y\n		if not (angle==0) then\n			e = e+math.sqrt(dx*dx+dy*dy)\n			end\n\n			-- Make gcode string\n\n			s=string.format(\"G1 X%f Y%f Z%f F2600 E%f\\n\", x,y,z,e)\n			if(angle == 0) then\n				s=string.format(\"G1 X%f Y%f Z%f F2600 E%f\\n\", x,y,z,e)\n				end\n				-- Add gcode to gcode result\nbase:AddText(s)\n	 angle=angle+0.2\n	 oldx=x\n	 oldy=y\n	 end\n	 z=z+0.4\n	 end\n	 ");
#endif
}

void Model::WriteGCode (string filename)
{
  cerr << "Unimplemented\n";
}

void Model::Restart()
{
  serial->Clear();	// resets line nr and clears buffer
  Print();
}

void Model::ContinuePauseButton()
{
  if (serial->isPrinting())
    serial->pausePrint();
  else
    Continue();
}

void Model::Continue()
{
  serial->continuePrint();
}

void Model::PrintButton()
{
  if (serial->isPrinting()) {
    Restart();
  } else {
    Print();
  }
}

void Model::ConnectToPrinter(char on)
{
  // FIXME: remove me !
}

bool Model::IsConnected()
{
	return serial->isConnected();
}

void Model::SimplePrint()
{
	if( serial->isPrinting() )
		alert ("Already printing.\nCannot start printing");

	if( !serial->isConnected() )
	{
		ConnectToPrinter(true);
		WaitForConnection(5.0);
	}

	Print();
}

void Model::WaitForConnection(float seconds)
{
	serial->WaitForConnection(seconds*1000);
}
void Model::Print()
{
	if( !serial->isConnected() ) {
		alert ("Not connected to printer.\nCannot start printing");
		return;
	}

	serial->Clear();
	serial->SetDebugMask();
	serial->clear_logs();
	gcode.queue_to_serial (serial);
	m_progress->start ("Printing", serial->Length());
	serial->StartPrint();
}

void Model::RunExtruder()
{
	static bool extruderIsRunning = false;
	if (settings.Slicing.Use3DGcode) {
		if (extruderIsRunning)
			serial->SendNow("M103");
		else
			serial->SendNow("M101");
		extruderIsRunning = !extruderIsRunning;
		return;
	}

	std::stringstream oss;
	string command("G1 F");
	oss << m_extruder_speed->get_value();
	command += oss.str();
	serial->SendNow(command);
	oss.str("");

	serial->SendNow("G92 E0");	// set extruder zero
	oss << m_extruder_length->get_value();
	string command2("G1 E");

	if (m_extruder_reverse->get_active())
		command2+="-";
	command2+=oss.str();
	serial->SendNow(command2);
	serial->SendNow("G1 F1500.0");

	serial->SendNow("G92 E0");	// set extruder zero
}

void Model::SendNow(string str)
{
	serial->SendNow(str);
}

void Model::Home(string axis)
{
	if(serial->isPrinting())
	{
		alert("Can't go home while printing");
		return;
	}
	if(axis == "X" || axis == "Y" || axis == "Z")
	{
		string buffer="G1 F";
		std::stringstream oss;
		if(axis == "Z")
			oss << settings.Hardware.MaxPrintSpeedZ;
		else
			oss << settings.Hardware.MaxPrintSpeedXY;
		buffer+= oss.str();
		SendNow(buffer);
		buffer="G1 ";
		buffer += axis;
		buffer+="-250 F";
		buffer+= oss.str();
		SendNow(buffer);
		buffer="G92 ";
		buffer += axis;
		buffer+="0";
		SendNow(buffer);	// Set this as home
		oss.str("");
		buffer="G1 ";
		buffer += axis;
		buffer+="1 F";
		buffer+= oss.str();
		SendNow(buffer);
		if(axis == "Z")
			oss << settings.Hardware.MinPrintSpeedZ;
		else
			oss << settings.Hardware.MinPrintSpeedXY;
		buffer="G1 ";
		buffer+="F";
		buffer+= oss.str();
		SendNow(buffer);	// set slow speed
		buffer="G1 ";
		buffer += axis;
		buffer+="-10 F";
		buffer+= oss.str();
		SendNow(buffer);
		buffer="G92 ";
		buffer += axis;
		buffer+="0";
		SendNow(buffer);	// Set this as home
	}
	else if(axis == "ALL")
	{
		SendNow("G28");
	}
	else
		alert("Home called with unknown axis");
}

void Model::Move(string axis, float distance)
{
	if(serial->isPrinting())
	{
		alert("Can't move manually while printing");
		return;
	}
	if(axis == "X" || axis == "Y" || axis == "Z")
	{
		SendNow("G91");	// relative positioning
		string buffer="G1 F";
		std::stringstream oss;
		if(axis == "Z")
			oss << settings.Hardware.MaxPrintSpeedZ;
		else
			oss << settings.Hardware.MaxPrintSpeedXY;
		buffer+= oss.str();
		SendNow(buffer);
		buffer="G1 ";
		buffer += axis;
		oss.str("");
		oss << distance;
		buffer+= oss.str();
		oss.str("");
		if(axis == "Z")
			oss << settings.Hardware.MaxPrintSpeedZ;
		else
			oss << settings.Hardware.MaxPrintSpeedXY;
		buffer+=" F"+oss.str();
		SendNow(buffer);
		SendNow("G90");	// absolute positioning
	}
	else
		alert("Move called with unknown axis");
}
void Model::Goto(string axis, float position)
{
	if(serial->isPrinting())
	{
		alert("Can't move manually while printing");
		return;
	}
	if(axis == "X" || axis == "Y" || axis == "Z")
	{
		string buffer="G1 F";
		std::stringstream oss;
		oss << settings.Hardware.MaxPrintSpeedXY;
		buffer+= oss.str();
		SendNow(buffer);
		buffer="G1 ";
		buffer += axis;
		oss.str("");
		oss << position;
		buffer+= oss.str();
		oss.str("");
		oss << settings.Hardware.MaxPrintSpeedXY;
		buffer+=" F"+oss.str();
		SendNow(buffer);
	}
	else
		alert("Goto called with unknown axis");
}
void Model::STOP()
{
	SendNow("M112");
	serial->Clear(); // reset buffer
}

#ifdef ENABLE_LUA

void print_hello(int number)
{
	cout << "hello world " << number << endl;
}

void refreshGraphicsView(Model *MVC)
{
	// FIXME: wow ! Hack: save and load the gcode, to force redraw
	gcode.Write (model, "./tmp.gcode");
	gcode.Read (model,"./tmp.gcode");
}

void ReportErrors(Model *mvc, lua_State * L)
{
	std::stringstream oss;
	oss << "Error: " << lua_tostring(L,-1);
	mvc->alert(oss.str().c_str());
	lua_pop(L, 1);
}
#endif // ENABLE_LUA

void Model::RunLua(char* script)
{
#ifdef ENABLE_LUA
	try{

		lua_State *myLuaState = lua_open();				// Create a new lua state

		luabind::open(myLuaState);						// Connect LuaBind to this lua state
		luaL_openlibs(myLuaState);

		// Add our function to the state's global scope
		luabind::module(myLuaState) [
			luabind::def("print_hello", print_hello)
		];


		luabind::module(myLuaState)
			[
			luabind::class_<Model>("Model")
			.def("ReadStl", &Model::ReadStl)			// To use: base:ReadStl("c:/Vertex.stl")
			.def("ConvertToGCode", &Model::ConvertToGCode)			// To use: base:ConvertToGCode()
			.def("ClearGCode", &Model::ClearGcode)	// To use: base:ClearGcode()
			.def("AddText", &Model::AddText)			// To use: base:AddText("text\n")
			.def("Print", &Model::SimplePrint)			// To use: base:Print()
			];

		luabind::globals(myLuaState)["base"] = this;

		// Now call our function in a lua script, such as "print_hello(5)"
		if luaL_dostring( myLuaState, script)
			ReportErrors(this, myLuaState);

		lua_close(myLuaState);


	}// try
	catch(const std::exception &TheError)
	{
		cerr << TheError.what() << endl;
	}
	refreshGraphicsView (this);
#endif // ENABLE_LUA
}

void Model::ReadStl(string filename)
{
  STL stl;
  if (stl.Read (filename))
    AddStl(stl, filename);
}

RFO_File* Model::AddStl(STL stl, string filename)
{
  RFO_File *file;
  RFO_Object *parent;
  get_selected_stl (parent, file);

  if (!parent) {
    if (rfo.Objects.size() <= 0)
      rfo.newObject();
    parent = &rfo.Objects.back();
  }
  g_assert (parent != NULL);

  size_t found = filename.find_last_of("/\\");
  Gtk::TreePath path = rfo.createFile (parent, stl, filename.substr(found+1));
  m_rfo_tree->get_selection()->unselect_all();
  m_rfo_tree->get_selection()->select (path);
  m_rfo_tree->expand_all();

  CalcBoundingBoxAndCenter();

  return &parent->files.back();
}

void Model::newObject()
{
  rfo.newObject();
}

void Model::setObjectname(string name)
{
  RFO_File *file;
  RFO_Object *object;
  get_selected_stl (object, file);
  if (object)
    object->name = name;
}

void Model::setFileMaterial(string material)
{
  RFO_File *file;
  RFO_Object *object;
  get_selected_stl (object, file);
  if (file)
    file->material = material;
}

void Model::setFileType(string type)
{
  RFO_File *file;
  RFO_Object *object;
  get_selected_stl (object, file);
  if (file)
    file->filetype = type;
}

void Model::setFileLocation(string location)
{
  RFO_File *file;
  RFO_Object *object;
  get_selected_stl (object, file);
  if (file)
    file->location = location;
}

void Model::alert (Gtk::Window *toplevel, const char *message)
{
  Gtk::MessageDialog aDlg (*toplevel, message, false /* markup */,
			   Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true);
  aDlg.run();
}

void Model::alert (const char *message)
{
  alert (this, message);
}

// LUA functions
void Model::ClearGcode()
{
  gcode.clear();
}
void Model::AddText(string line)
{
  gcode.append_text (line);
}
std::string Model::GetText()
{
  return gcode.get_text();
}

void Model::RotateObject(Vector4f rotate)
{
  Vector3f rot(rotate.x, rotate.y, rotate.z);

  RFO_File *file;
  RFO_Object *object;
  get_selected_stl (object, file);

  if (!file)
    return; // FIXME: rotate entire Objects ...

  file->stl.RotateObject(rot, rotate.w);
  CalcBoundingBoxAndCenter();
  queue_draw();
}

void Model::OptimizeRotation()
{
//	stl.OptimizeRotation();
  CalcBoundingBoxAndCenter();
}

void Model::delete_selected_stl()
{
  if (m_rfo_tree->get_selection()->count_selected_rows() <= 0)
    return;

  Gtk::TreeModel::iterator iter = m_rfo_tree->get_selection()->get_selected();
  rfo.DeleteSelected (iter);
  m_rfo_tree->expand_all();
  queue_draw();
}

bool Model::get_selected_stl(RFO_Object *&object, RFO_File *&file)
{
  Gtk::TreeModel::iterator iter = m_rfo_tree->get_selection()->get_selected();
  rfo.get_selected_stl (iter, object, file);
  return object != NULL || file != NULL;
}

void Model::duplicate_selected_stl()
{
  RFO_Object *object;
  RFO_File *file;
  if (!get_selected_stl (object, file) || !file)
    return;

  // duplicate
  RFO_File* obj = AddStl (file->stl, file->location);

  // translate
  Vector3f p = file->transform3D.transform.getTranslation();
  Vector3f size = file->stl.Max - file->stl.Min;
  p.x += size.x + 5.0f;	// 5mm space
  obj->transform3D.transform.setTranslation (p);
  CalcBoundingBoxAndCenter();
  queue_draw();
}

void Model::DrawGrid()
{
	glColor3f (0.5f, 0.5f, 0.5f);
	glBegin(GL_LINES);
	for (uint x = 0; x < settings.Hardware.Volume.x; x += 10) {
		glVertex3f (x, 0.0f, 0.0f);
		glVertex3f (x, settings.Hardware.Volume.y, 0.0f);
	}
	glVertex3f (settings.Hardware.Volume.x, 0.0f, 0.0f);
	glVertex3f (settings.Hardware.Volume.x, settings.Hardware.Volume.y, 0.0f);

	for (uint y = 0;y < settings.Hardware.Volume.y; y += 10) {
		glVertex3f (0.0f, y, 0.0f);
		glVertex3f (settings.Hardware.Volume.x, y, 0.0f);
	}
	glVertex3f (0.0f, settings.Hardware.Volume.y, 0.0f);
	glVertex3f (settings.Hardware.Volume.x, settings.Hardware.Volume.y, 0.0f);

	glEnd();
}

void Model::Draw (Gtk::TreeModel::iterator &selected)
{
	printOffset = settings.Hardware.PrintMargin;
	if(settings.RaftEnable)
		printOffset += Vector3f(settings.Raft.Size, settings.Raft.Size, 0);

	Vector3f translation = rfo.transform3D.transform.getTranslation();
	DrawGrid();

	// Move objects
	glTranslatef(translation.x+printOffset.x, translation.y+printOffset.y, translation.z+settings.Hardware.PrintMargin.z);
	glPolygonOffset (0.5f, 0.5f);
	rfo.draw (settings, 1.0, selected);

	if (settings.Display.DisplayGCode)
	{
		glTranslatef(-(translation.x+printOffset.x), -(translation.y+printOffset.y), -(translation.z+settings.Hardware.PrintMargin.z));
		gcode.draw(this->settings);
		glTranslatef(translation.x+printOffset.x, translation.y+printOffset.y, translation.z+settings.Hardware.PrintMargin.z);
	}
	glPolygonOffset (-0.5f, -0.5f);
	Gtk::TreeModel::iterator iter;
	rfo.draw (this->settings, settings.Display.PolygonOpacity, iter);

	if(settings.Display.DisplayBBox)
	{
		// Draw bbox
		glColor3f(1,0,0);
		glBegin(GL_LINE_LOOP);
		glVertex3f(Min.x, Min.y, Min.z);
		glVertex3f(Min.x, Max.y, Min.z);
		glVertex3f(Max.x, Max.y, Min.z);
		glVertex3f(Max.x, Min.y, Min.z);
		glEnd();
		glBegin(GL_LINE_LOOP);
		glVertex3f(Min.x, Min.y, Max.z);
		glVertex3f(Min.x, Max.y, Max.z);
		glVertex3f(Max.x, Max.y, Max.z);
		glVertex3f(Max.x, Min.y, Max.z);
		glEnd();
		glBegin(GL_LINES);
		glVertex3f(Min.x, Min.y, Min.z);
		glVertex3f(Min.x, Min.y, Max.z);
		glVertex3f(Min.x, Max.y, Min.z);
		glVertex3f(Min.x, Max.y, Max.z);
		glVertex3f(Max.x, Max.y, Min.z);
		glVertex3f(Max.x, Max.y, Max.z);
		glVertex3f(Max.x, Min.y, Min.z);
		glVertex3f(Max.x, Min.y, Max.z);
		glEnd();
	}
}

void Model::CalcBoundingBoxAndCenter()
{
  Max = Vector3f(G_MINFLOAT, G_MINFLOAT, G_MINFLOAT);
  Min = Vector3f(G_MAXFLOAT, G_MAXFLOAT, G_MAXFLOAT);

  for (uint i = 0 ; i < rfo.Objects.size(); i++) {
    for (uint j = 0; j < rfo.Objects[i].files.size(); j++) {
      Matrix4f M = rfo.GetSTLTransformationMatrix (i, j);
      Vector3f stlMin = M * rfo.Objects[i].files[j].stl.Min;
      Vector3f stlMax = M * rfo.Objects[i].files[j].stl.Max;
      for (uint k = 0; k < 3; k++) {
	Min.xyz[k] = MIN(stlMin.xyz[k], Min.xyz[k]);
	Max.xyz[k] = MAX(stlMax.xyz[k], Max.xyz[k]);
      }
    }
  }

  Center = (Max - Min) / 2.0;

  m_signal_rfo_changed.emit();
}

void Model::MakeRaft(float &z)
{
	vector<InFillHit> HitsBuffer;

	uint LayerNr = 0;
	float size = settings.Raft.Size;

	Vector2f raftMin =  Vector2f(Min.x - size + printOffset.x, Min.y - size + printOffset.y);
	Vector2f raftMax =  Vector2f(Max.x + size + printOffset.x, Max.y + size + printOffset.y);

	Vector2f Center = (Vector2f(Max.x + size, Max.y + size)-Vector2f(Min.x + size, Min.y + size))/2+Vector2f(printOffset.x, printOffset.y);

	float Length = sqrtf(2)*(   ((raftMax.x)>(raftMax.y)? (raftMax.x):(raftMax.y))  -  ((raftMin.x)<(raftMin.y)? (raftMin.x):(raftMin.y))  )/2.0f;	// bbox of object

	float E = 0.0f;
	float rot;

	while(LayerNr < settings.Raft.Phase[0].LayerCount + settings.Raft.Phase[1].LayerCount)
	{
	  Settings::RaftSettings::PhasePropertiesType *props;
	  props = LayerNr < settings.Raft.Phase[0].LayerCount ?
	    &settings.Raft.Phase[0] : &settings.Raft.Phase[1];
	  rot = (props->Rotation+(float)LayerNr * props->RotationPrLayer)/180.0f*M_PI;
		Vector2f InfillDirX(cosf(rot), sinf(rot));
		Vector2f InfillDirY(-InfillDirX.y, InfillDirX.x);

		Vector3f LastPosition;
		bool reverseLines = false;

		Vector2f P1, P2;
		for(float x = -Length ; x < Length ; x+=props->Distance)
		{
			P1 = (InfillDirX *  Length)+(InfillDirY*x) + Center;
			P2 = (InfillDirX * -Length)+(InfillDirY*x) + Center;

			if(reverseLines)
			{
				Vector2f tmp = P1;
				P1 = P2;
				P2 = tmp;
			}

//			glBegin(GL_LINES);
//			glVertex2fv(&P1.x);
//			glVertex2fv(&P2.x);

			// Crop lines to bbox*size
			Vector3f point;
			InFillHit hit;
			HitsBuffer.clear();
			Vector2f P3(raftMin.x, raftMin.y);
			Vector2f P4(raftMin.x, raftMax.y);
//			glVertex2fv(&P3.x);
//			glVertex2fv(&P4.x);
			if(IntersectXY(P1,P2,P3,P4,hit))	//Intersect edges of bbox
				HitsBuffer.push_back(hit);
			P3 = Vector2f(raftMax.x,raftMax.y);
//			glVertex2fv(&P3.x);
//			glVertex2fv(&P4.x);
			if(IntersectXY(P1,P2,P3,P4,hit))
				HitsBuffer.push_back(hit);
			P4 = Vector2f(raftMax.x,raftMin.y);
//			glVertex2fv(&P3.x);
//			glVertex2fv(&P4.x);
			if(IntersectXY(P1,P2,P3,P4,hit))
				HitsBuffer.push_back(hit);
			P3 = Vector2f(raftMin.x,raftMin.y);
//			glVertex2fv(&P3.x);
//			glVertex2fv(&P4.x);
			if(IntersectXY(P1,P2,P3,P4,hit))
				HitsBuffer.push_back(hit);
//			glEnd();

			if(HitsBuffer.size() == 0)	// it can only be 2 or zero
				continue;
			if(HitsBuffer.size() != 2)
				continue;

			std::sort(HitsBuffer.begin(), HitsBuffer.end(), InFillHitCompareFunc);

			P1 = HitsBuffer[0].p;
			P2 = HitsBuffer[1].p;

			MakeAcceleratedGCodeLine (Vector3f(P1.x,P1.y,z), Vector3f(P2.x,P2.y,z),
						  settings.Hardware.DistanceToReachFullSpeed,
						  props->MaterialDistanceRatio, gcode, z,
						  settings.Hardware.MinPrintSpeedXY,
						  settings.Hardware.MaxPrintSpeedXY,
						  settings.Hardware.MinPrintSpeedZ,
						  settings.Hardware.MaxPrintSpeedZ,
						  settings.Slicing.UseIncrementalEcode,
						  settings.Slicing.Use3DGcode,
						  E, settings.Slicing.EnableAcceleration);
			reverseLines = !reverseLines;
		}
		// Set startspeed for Z-move
		Command g;
		g.Code = SETSPEED;
		g.where = Vector3f(P2.x, P2.y, z);
		g.f=settings.Hardware.MinPrintSpeedZ;
		g.comment = "Move Z";
		g.e = E;
		gcode.commands.push_back(g);
		z += props->Thickness * settings.Hardware.LayerThickness;

		// Move Z
		g.Code = ZMOVE;
		g.where = Vector3f(P2.x, P2.y, z);
		g.f = settings.Hardware.MinPrintSpeedZ;
		g.comment = "Move Z";
		g.e = E;
		gcode.commands.push_back(g);

		LayerNr++;
	}

	// restore the E state
	Command gotoE;
	gotoE.Code = GOTO;
	gotoE.e = 0;
	gotoE.comment = "Reset E for the remaining print";
	gcode.commands.push_back(gotoE);
}
