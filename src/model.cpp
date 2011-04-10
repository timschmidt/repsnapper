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
#include <cerrno>
#include <functional>

// should move to platform.h with com port fun.
#include <sys/types.h>
#include <dirent.h>

#include <glib/gutils.h>

#include "stdafx.h"
#include "model.h"
#include "rfo.h"
#include "file.h"
#include "render.h"
#include "settings.h"
#include "progress.h"
#include "connectview.h"

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
  SendNow(m_gcode_entry->get_text());
  m_gcode_entry->set_text("");
}

Model *Model::create()
{
  std::vector<std::string> dirs = Platform::getConfigPaths();
  Glib::ustring ui;
  for (std::vector<std::string>::const_iterator i = dirs.begin();
       i != dirs.end(); ++i) {
    Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(*i + "repsnapper.ui");
    try {
      char *ptr;
      gsize length;
      file->load_contents(ptr, length);
      ui = Glib::ustring(ptr);
      break;
    } catch(Gio::Error e) {
      switch(e.code()) {
      case Gio::Error::NOT_FOUND:
        continue;
        
      default:
        Gtk::MessageDialog dialog("Error reading UI description!!", false,
                                  Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE);
        dialog.set_secondary_text(e.what());
        dialog.run();
        return NULL;
      }
    }
  }

  if(ui.empty()) {
    Gtk::MessageDialog dialog("Couldn't find UI description!", false,
                              Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE);
    dialog.set_secondary_text("Check that repsnapper has been correctly installed.");
    dialog.run();
    return NULL;
  }

  Glib::RefPtr<Gtk::Builder> builder;
  try {
    builder = Gtk::Builder::create_from_string(ui);
  }
  catch(const Gtk::BuilderError& ex)
  {
    Gtk::MessageDialog dialog("Error loading UI!", false,
                              Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE);
    dialog.set_secondary_text(ex.what());
    dialog.run();
    throw ex;
  }
  Model *mvc = 0;
  builder->get_widget_derived("main_window", mvc);

  return mvc;
}

void Model::printing_changed()
{
  if (m_printing) {
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
    SendNow("M80");
  else
    SendNow("M81");
}

void Model::enable_logging_toggled (Gtk::ToggleButton *button)
{
  settings.Misc.FileLoggingEnabled = button->get_active();
}

void Model::fan_enabled_toggled (Gtk::ToggleButton *button)
{
  if (!button->get_active())
    SendNow("M107");
  else {
    std::stringstream oss;
    oss << "M106 S" << (int)m_fan_voltage->get_value();
    SendNow(oss.str());
  }
}

void Model::clear_logs()
{
  commlog->set_text("");
  errlog->set_text("");
  echolog->set_text("");
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
  dialog->set_transient_for (*this);
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
    rr_enqueue(m_device, RR_PRIO_HIGH, NULL, oss.str().data(), oss.str().size());
  }

public:
  enum TempType { NOZZLE, BED };

  rr_dev m_device;
  TempType m_type;
  Gtk::Label *m_temp;
  Gtk::SpinButton *m_target;

  void update_temp (double value)
  {
    m_target->set_value (value);
  }

  TempRow(rr_dev device, TempType type) :
    m_device(device), m_type(type)
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
  SendNow("G28");
  for (uint i = 0; i < 3; i++)
    m_axis_rows[i]->notify_homed();
}

void Model::handle_reply(rr_dev device, void *data, rr_reply reply, float value)
{
  Model *instance = (Model*) data;
  switch(reply) {
  case RR_NOZZLE_TEMP:
    instance->m_temps[TempRow::NOZZLE]->update_temp(value);
    break;

  case RR_BED_TEMP:
    instance->m_temps[TempRow::BED]->update_temp(value);
    break;

  case RR_OK:
    // TODO: Only enqueue a certain number of blocks at a time
    break;

  default:
    break;
  }
}

void Model::handle_send(rr_dev device, void *cbdata, void *blkdata, const char *block, size_t len) {
  Model *instance = static_cast<Model*>(cbdata);
  if(instance->m_printing) {
    instance->m_progress->update((unsigned long)(blkdata));
  }
  instance->commlog->insert(instance->commlog->end(), string(block, len));
}

void Model::handle_recv(rr_dev device, void *data, const char *reply, size_t len) {
  Model *instance = static_cast<Model*>(data);
  instance->commlog->insert(instance->commlog->end(), string(reply, len) + "\n");
}

bool Model::handle_dev_fd(Glib::IOCondition cond) {
  int result;
  if(cond & Glib::IO_IN) {
    result = rr_handle_readable(device);
    if(result < 0) {
      Gtk::MessageDialog dialog(*this, "Error reading from device!", false,
                                Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE);
      dialog.set_secondary_text(strerror(errno));
      dialog.run();
    }
  }
  if(cond & Glib::IO_OUT) {
    result = rr_handle_writable(device);
    if(result < 0) {
      Gtk::MessageDialog dialog(*this, "Error writing to device!", false,
                                Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE);
      dialog.set_secondary_text(strerror(errno));
      dialog.run();
    }
  }
  return true;
}

void Model::handle_want_writable(rr_dev device, void *data, char state) {
  Model *instance = static_cast<Model*>(data);
  // TODO: Is there a way to merely change the flags to listen on?
  instance->devconn.disconnect();
  if(state) {
    instance->devconn = Glib::signal_io().connect(sigc::mem_fun(*instance, &Model::handle_dev_fd), rr_dev_fd(device), Glib::IO_IN | Glib::IO_OUT);
  } else {
    instance->devconn = Glib::signal_io().connect(sigc::mem_fun(*instance, &Model::handle_dev_fd), rr_dev_fd(device), Glib::IO_IN);
  }
}

void Model::load_settings()
{
  FileChooser::ioDialog (this, FileChooser::OPEN, FileChooser::SETTINGS);
}

void Model::save_settings()
{
  SaveConfig (Gio::File::create_for_path(Glib::get_user_config_dir() + "/repsnapper/repsnapper.conf"));
}

void Model::save_settings_as()
{
  FileChooser::ioDialog (this, FileChooser::SAVE, FileChooser::SETTINGS);
}

Model::Model(BaseObjectType* cobject,
					 const Glib::RefPtr<Gtk::Builder>& builder)
  : Gtk::Window(cobject), m_builder(builder), m_printing(false),
    commlog(Gtk::TextBuffer::create()), errlog(Gtk::TextBuffer::create()),
    echolog(Gtk::TextBuffer::create())
{
  // Variable defaults
  Center.x = Center.y = 100.0f;
  Center.z = 0.0f;
  
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
  connect_toggled ("Misc.FileLoggingEnabled", sigc::mem_fun(*this, &Model::enable_logging_toggled));
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

  connect_button ("i_extrude_length", sigc::mem_fun(*this, &Model::RunExtruder) );

  // Main view progress bar
  Gtk::Box *box = NULL;
  Gtk::Label *label = NULL;
  Gtk::ProgressBar *bar = NULL;

  m_builder->get_widget("progress_box", box);
  m_builder->get_widget("progress_bar", bar);
  m_builder->get_widget("progress_label", label);
  m_progress = new Progress (box, bar, label);

  // TODO: Configurable protocol, cache size
  device = rr_create(RR_PROTO_FIVED,
                     &handle_send, static_cast<void*>(this),
                     &handle_recv, static_cast<void*>(this),
                     &handle_reply, static_cast<void*>(this),
                     NULL, NULL,
                     &handle_want_writable, static_cast<void*>(this),
                     64);

  m_view = new ConnectView(device, &settings);
  Gtk::Box *connect_box = NULL;
  m_builder->get_widget ("p_connect_button_box", connect_box);
  connect_box->add (*m_view);
  settings.m_signal_visual_settings_changed.connect (sigc::mem_fun(*this, &Model::draw));

  Gtk::Box *temp_box;
  m_builder->get_widget ("i_temp_box", temp_box);
  m_temps[TempRow::NOZZLE] = new TempRow(device, TempRow::NOZZLE);
  m_temps[TempRow::BED] = new TempRow(device, TempRow::BED);
  temp_box->add (*m_temps[TempRow::NOZZLE]);
  temp_box->add (*m_temps[TempRow::BED]);

  Gtk::Box *axis_box;
  m_builder->get_widget ("i_axis_controls", axis_box);
  for (uint i = 0; i < 3; i++) {
    m_axis_rows[i] = new AxisRow (this, i);
    axis_box->add (*m_axis_rows[i]);
  }

  Gtk::TextView *log_view;
  m_builder->get_widget("i_txt_comms", log_view);
  log_view->set_buffer(commlog);
  m_builder->get_widget("i_txt_errs", log_view);
  log_view->set_buffer(errlog);
  m_builder->get_widget("i_txt_echo", log_view);
  log_view->set_buffer(echolog);

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
  save_settings();
  if(rr_dev_fd(device) >= 0) {
    rr_close(device);
  }
  rr_free(device);
  for (uint i = 0; i < 3; i++) {
    delete m_spin_rows[i];
    delete m_axis_rows[i];
  }
  delete m_temps[TempRow::NOZZLE];
  delete m_temps[TempRow::BED];
  delete m_view;
  delete m_progress;
}

void Model::SaveConfig(Glib::RefPtr<Gio::File> file)
{
  settings.save_settings(file);
}

void Model::LoadConfig(Glib::RefPtr<Gio::File> file)
{
  settings.load_settings(*((Builder *)&m_builder), file);
}

void Model::SimpleAdvancedToggle()
{
   cout << "not yet implemented\n";
}

void Model::ReadGCode(Glib::RefPtr<Gio::File> file)
{
  PrintInhibitor inhibitPrint(this);
  m_progress->start ("Converting", 100.0);
  gcode.Read (this, m_progress, file->get_path());
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
					default:
						g_warning ("unknown shrinking algorithm");
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
				plane.MakeGcode (infill, gcode, E, z + printOffsetZ,
						 settings.Slicing, settings.Hardware);
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

void Model::init() {}

void Model::WriteGCode(Glib::RefPtr<Gio::File> file)
{
  Glib::ustring contents = gcode.get_text();
  Glib::file_set_contents (file->get_path(), contents);
}

void Model::Restart()
{
  Print();
}

void Model::ContinuePauseButton()
{
  if (m_printing) {
    // TODO: Actually pause (stop enqueueing)
    m_printing = false;
  } else {
    Continue();
  }
}

void Model::Continue()
{
  m_printing = true;
  // TODO: Actually unpause (resume enqueueing)
}

void Model::PrintButton()
{
  if (m_printing) {
    Restart();
  } else {
    Print();
  }
}

bool Model::IsConnected()
{
	return rr_dev_fd(device) >= 0;
}

void Model::SimplePrint()
{
	if( m_printing )
		alert ("Already printing.\nCannot start printing");

	if(rr_dev_fd(device) < 0)
	{
		m_view->try_set_state(true);
	}

	Print();
}

void Model::Print()
{
	if(rr_dev_fd(device) < 0) {
		alert ("Not connected to printer.\nCannot start printing");
		return;
	}

	rr_reset(device);
	gcode.queue_to_serial(device);
	m_progress->start ("Printing", gcode.commands.size());
	m_printing = true;
}

void Model::RunExtruder()
{
	static bool extruderIsRunning = false;
	if (settings.Slicing.Use3DGcode) {
		if (extruderIsRunning)
      SendNow("M103");
		else
      SendNow("M101");
		extruderIsRunning = !extruderIsRunning;
		return;
	}

	std::stringstream oss;
	string command("G1 F");
	oss << m_extruder_speed->get_value();
	command += oss.str();
  SendNow(command);
	oss.str("");

  // set extruder zero
	SendNow("G92 E0");
	oss << m_extruder_length->get_value();
	string command2("G1 E");

	if (m_extruder_reverse->get_active())
		command2+="-";
	command2+=oss.str();
  SendNow(command2);
	SendNow("G1 F1500.0");
  SendNow("G92 E0");	// set extruder zero
 }

void Model::SendNow(string str)
{
  if(rr_dev_fd(device) > 0) {
    rr_enqueue(device, RR_PRIO_HIGH, NULL, str.data(), str.size());
  } else {
    Gtk::MessageDialog dialog("Can't send command", false,
                              Gtk::MESSAGE_INFO, Gtk::BUTTONS_CLOSE);
    dialog.set_secondary_text("You must first connect to a device!");
    dialog.run();
  }
}

void Model::Home(string axis)
{
	if(m_printing)
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
	if(m_printing)
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
	if(m_printing)
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
  rr_reset(device);
}

void Model::ReadStl(Glib::RefPtr<Gio::File> file)
{
  STL stl;
  if (stl.loadFile (file->get_path()) == 0)
    AddStl(stl, file->get_path());
}

RFO_File* Model::AddStl(STL stl, string filename)
{
  RFO_File *file, *lastfile;
  RFO_Object *parent;
  get_selected_stl (parent, file);

  if (!parent) {
    if (rfo.Objects.size() <= 0)
      rfo.newObject();
    parent = &rfo.Objects.back();
  }
  g_assert (parent != NULL);

  size_t found = filename.find_last_of("/\\");
  lastfile = NULL;
  if (parent->files.size() > 0)
    lastfile = &parent->files.back();

  Gtk::TreePath path = rfo.createFile (parent, stl, filename.substr(found+1));
  m_rfo_tree->get_selection()->unselect_all();
  m_rfo_tree->get_selection()->select (path);
  m_rfo_tree->expand_all();

  file = &parent->files.back();
  if (lastfile) {
    Vector3f p = lastfile->transform3D.transform.getTranslation();
    Vector3f size = lastfile->stl.Max - lastfile->stl.Min;
    p.x += size.x + 5.0f; // 5mm space
    file->transform3D.transform.setTranslation(p);
  }

  CalcBoundingBoxAndCenter();

  return file;
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
						  props->MaterialDistanceRatio, gcode,
						  E, z, settings.Slicing, settings.Hardware);
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
